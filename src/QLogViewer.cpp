#include <iostream>
#include <QApplication>
#include <rock_widget_collection/Timeline.h>
#include "QLogViewer.hpp"

using namespace pocolog_cpp;

namespace rock_replay_cpp
{

RegisterQLogViewer::RegisterQLogViewer(
    const std::string &type, create_logviewer_fcn_t fcn)
{
  QLogViewer::addWidget(type, fcn);
}

void ExportStreamWorker::exportInterval()
{
  std::cout << "Exporting " << filename_.toStdString() << std::endl;
  std::cout << " Stream " << stream_name_.toStdString() << std::endl;

  start_time_ = base::Time::now();

  reader_->exportStream(
      filename_.toStdString(),
      stream_name_.toStdString(),
      start_index_,
      end_index_,
      exportStreamCallback,
      this);

  emit finished();
}

void ExportStreamWorker::cancel()
{
  canceled_ = true;
}

bool ExportStreamWorker::exportStreamCallback(int sampleNr, void* data)
{
  ExportStreamWorker* thiz = reinterpret_cast<ExportStreamWorker*>(data);
  thiz->updateProgress(sampleNr);
  return !thiz->canceled_;
}

void ExportStreamWorker::updateProgress(int sampleNr)
{
  base::Time diff = base::Time::now() - start_time_;

  if (diff.toSeconds() >= 0.1 || sampleNr >= (end_index_-1))
  {
    emit valueChanged(sampleNr);
    start_time_ = base::Time::now();
  }
}


bool QStreamSelector::getStreamName(
    LogReader *reader,
    QString &stream_name,
    QString &qtype_name)
{
  std::vector<StreamDescription> descriptions = reader->getDescriptions();

  stream_map_t stream_map;
  for (std::vector<StreamDescription>::const_iterator it = descriptions.begin(); it != descriptions.end(); it++)
  {
    std::string stream_name = it->getName();
    size_t found = stream_name.find(".");

    if (found != std::string::npos)
    {
      QString task_name = QString::fromStdString(stream_name.substr(0, found));
      QString port_name = QString::fromStdString(stream_name.substr(found + 1));
      QString qtype_name = QString::fromStdString(it->getTypeName());
      stream_map[task_name].push_back(std::make_pair(port_name, qtype_name));
    }
  }

  QStreamSelector stream_selector(stream_map);
  stream_selector.setWindowTitle("Stream Selector");
  stream_selector.exec();

  if (stream_selector.result() != QDialog::Accepted)
    return false;

  stream_name = stream_selector.stream_name_;
  qtype_name = stream_selector.type_name_;

  return true;
}

QStreamSelector::QStreamSelector(
    stream_map_t stream_map,
    QWidget *parent)
{
  treewidget_ = new QTreeWidget();
  treewidget_->setMinimumWidth(520);
  treewidget_->setMinimumHeight(250);
  treewidget_->setHeaderItem(createItem("Stream Name", "Type"));

  QList<QTreeWidgetItem *> items;

  for (stream_map_t::const_iterator it = stream_map.begin();
       it != stream_map.end(); it++)
  {
    QTreeWidgetItem *item = createItem(it->first, "");
    for (std::vector<std::pair<QString, QString> >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); it2++)
    {
      QTreeWidgetItem *child = createItem(it2->first, it2->second);
      QString data = it->first + "." + it2->first;
      child->setData(0, Qt::UserRole, data);
      item->addChild(child);
    }
    items.append(item);
  }

  treewidget_->setColumnCount(2);
  treewidget_->insertTopLevelItems(0, items);
  treewidget_->setColumnWidth(0, 250);
  treewidget_->expandAll();

  connect(treewidget_,
          SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
          this, SLOT(itemDoubleClicked(QTreeWidgetItem *, int)));

  QVBoxLayout *main_layout = new QVBoxLayout();
  main_layout->addWidget(treewidget_);
  setLayout(main_layout);
  adjustSize();
}

QTreeWidgetItem *QStreamSelector::createItem(
    const QString &name,
    const QString &value,
    QTreeWidgetItem *parent)
{
  QStringList strings;
  strings << name << value;
  return new QTreeWidgetItem(parent, strings);
}

void QStreamSelector::itemDoubleClicked(QTreeWidgetItem *item, int column)
{
  if (treewidget_->indexOfTopLevelItem(item))
  {
    stream_name_ = item->data(0, Qt::UserRole).toString();
    type_name_ = item->text(1);

    done(QDialog::Accepted);
  }
}

QLogViewer *QLogViewer::create(const QString &filepath, int rate)
{
  LogReader *reader = new LogReader(filepath.toStdString());

  QString qstream_name, qtype_name;
  if (!QStreamSelector::getStreamName(reader, qstream_name, qtype_name))
    return NULL;

  std::string type_name = qtype_name.toStdString();

  if (widgetMap().find(type_name) == widgetMap().end())
  {
    std::cout << "Log type not defined not defined " << type_name << std::endl;
    throw std::runtime_error("Not implemented");
  }

  QLogViewer *v = widgetMap()[type_name]();
  v->filename_ = filepath;
  v->construct(reader, qstream_name, rate);
  return v;
}

QLogViewer::QLogViewer()
    : QWidget(NULL)
    , reader_(NULL)
    , widget_(NULL)
    , current_index_box_(NULL)
    , total_samples_label_(NULL)
    , start_box_(NULL)
    , end_box_(NULL)
    , step_box_(NULL)
    , step_(0)
    , running_(false)
{
  setWindowTitle("LogViewer");

  QPushButton *back_button = createControlButton(":/back");
  QPushButton *play_button = createControlButton(":/play");
  QPushButton *next_button = createControlButton(":/next");
  QPushButton *stop_button = createControlButton(":/stop");

  connect(back_button, SIGNAL(clicked(bool)), this, SLOT(backButtonClicked(bool)));
  connect(play_button, SIGNAL(clicked(bool)), this, SLOT(playButtonClicked(bool)));
  connect(next_button, SIGNAL(clicked(bool)), this, SLOT(nextButtonClicked(bool)));
  connect(stop_button, SIGNAL(clicked(bool)), this, SLOT(stopButtonClicked(bool)));

  QGridLayout *control_grid_layout = new QGridLayout();

  current_index_box_ = new QSpinBox();
  total_samples_label_ = new QLabel();
  total_samples_label_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  total_samples_label_->setMaximumHeight(30);

  QLabel *label = NULL;
  step_box_ = new QSpinBox();
  connect(step_box_, SIGNAL(valueChanged(int)), this, SLOT(setStepValue(int)));

  label = new QLabel("Velocity:");
  label->setStyleSheet("font-weight: bold");

  control_grid_layout->addWidget(label, 0, 0);
  control_grid_layout->addWidget(step_box_, 1, 0);

  label = new QLabel("Index:");
  label->setStyleSheet("font-weight: bold");

  control_grid_layout->addWidget(label, 0, 1);
  control_grid_layout->addWidget(current_index_box_, 1, 1);

  label = new QLabel("Total:");
  label->setStyleSheet("font-weight: bold");
  control_grid_layout->addWidget(label, 0, 2);
  control_grid_layout->addWidget(total_samples_label_, 1, 2);

  label = new QLabel("Save interval:");
  label->setStyleSheet("font-weight: bold");

  control_grid_layout->addWidget(label, 0, 3);

  QFrame *frame = new QFrame();
  frame->setFrameStyle(QFrame::Panel | QFrame::Raised);

  QHBoxLayout *save_interval_layout = new QHBoxLayout();
  frame->setLayout(save_interval_layout);

  label = new QLabel("Start:");
  label->setStyleSheet("font-weight: bold");
  start_box_ = new QSpinBox();
  QPushButton *copy_start_button = new QPushButton();
  copy_start_button->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));

  save_interval_layout->addWidget(label);
  save_interval_layout->addWidget(start_box_);
  save_interval_layout->addWidget(copy_start_button);

  label = new QLabel("End:");
  label->setStyleSheet("font-weight: bold");
  end_box_ = new QSpinBox();
  QPushButton *copy_end_button = new QPushButton();
  copy_end_button->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));

  QPushButton *save_interval_button = new QPushButton();
  save_interval_button->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

  connect(copy_start_button, SIGNAL(clicked(bool)), this, SLOT(copyStartButtonClicked(bool)));
  connect(copy_end_button, SIGNAL(clicked(bool)), this, SLOT(copyEndButtonClicked(bool)));
  connect(save_interval_button, SIGNAL(clicked(bool)), this, SLOT(saveIntervalButtonClicked(bool)));

  save_interval_layout->addWidget(label);
  save_interval_layout->addWidget(end_box_);
  save_interval_layout->addWidget(copy_end_button);
  save_interval_layout->addWidget(save_interval_button);

  control_grid_layout->addWidget(frame, 1, 3);

  QHBoxLayout *control_layout = new QHBoxLayout();
  control_layout->addWidget(back_button);
  control_layout->addWidget(play_button);
  control_layout->addWidget(next_button);
  control_layout->addWidget(stop_button);
  control_layout->setAlignment(Qt::AlignBottom);

  QHBoxLayout *all_layout = new QHBoxLayout();
  all_layout->addLayout(control_layout);
  all_layout->addLayout(control_grid_layout);
  all_layout->setAlignment(Qt::AlignTop);

  timeline_ = new Timeline(this);
  timeline_->setBackgroundColor(QColor(240, 235, 226));
  timeline_->setMarginTopBot(0);
  timeline_->setMarginLR(5);
  timeline_->setMaximumHeight(26);
  timeline_->setFrameShape(QFrame::NoFrame);
  timeline_->setFrameShadow(QFrame::Plain);
  timeline_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  connect(timeline_, SIGNAL(indexSliderMoved(int)), this, SLOT(sliderMoved(int)));
  connect(timeline_, SIGNAL(indexSliderReleased(int)), this, SLOT(sliderReleased(int)));

  connect(timeline_, SIGNAL(startMarkerMoved(int)), start_box_, SLOT(setValue(int)));
  connect(timeline_, SIGNAL(endMarkerMoved(int)), end_box_, SLOT(setValue(int)));

  connect(start_box_, SIGNAL(valueChanged(int)), timeline_, SLOT(setStartMarkerIndex(int)));
  connect(end_box_, SIGNAL(valueChanged(int)), timeline_, SLOT(setEndMarkerIndex(int)));
  connect(current_index_box_, SIGNAL(editingFinished()), this, SLOT(currentIndexEditingFinished()));

  QVBoxLayout *main_layout = new QVBoxLayout();
  main_layout->addLayout(all_layout);
  main_layout->addWidget(timeline_);

  setLayout(main_layout);
}

void QLogViewer::closeEvent(QCloseEvent *evt)
{
  QWidget::closeEvent(evt);
  QApplication::closeAllWindows();
}

QLogViewer::~QLogViewer()
{
  delete reader_;
  delete widget_;
}

QWidget *QLogViewer::createWidget()
{
  throw std::runtime_error("createWidget() not defined");
}

void QLogViewer::timeout()
{
  updateSample();
}

void QLogViewer::update()
{
  throw std::runtime_error("update() not implemented");
}

void QLogViewer::updateSample()
{
  timeline_->setSliderIndex(stream_.current_sample_index());

  if (stream_.current_sample_index() < stream_.total_samples() &&
      stream_.current_sample_index() < (size_t)timeline_->getEndMarkerIndex())
  {
    stream_.set_current_sample_index(stream_.current_sample_index() + step_);
    update();
  }
}

void QLogViewer::backButtonClicked(bool checked)
{
  if (step_ > 0)
    step_box_->setValue(--step_);
}

void QLogViewer::playButtonClicked(bool checked)
{
  if (stream_.current_sample_index() < (size_t)timeline_->getStartMarkerIndex())
    stream_.set_current_sample_index(timeline_->getStartMarkerIndex());
  timer_.start(rate_);
  running_  = true;
}

void QLogViewer::nextButtonClicked(bool checked)
{
  if (step_ < MAXIMUM_STEP)
    step_box_->setValue(++step_);
}

void QLogViewer::stopButtonClicked(bool checked)
{
  running_ = false;
  timer_.stop();
}

void QLogViewer::copyStartButtonClicked(bool checked)
{
  start_box_->setValue(timeline_->getSliderIndex());
}

void QLogViewer::copyEndButtonClicked(bool checked)
{
  end_box_->setValue(timeline_->getSliderIndex());
}

void QLogViewer::saveIntervalButtonClicked(bool checked)
{
  timer_.stop();
  QFileInfo info(filename_);

  QString export_filename = QString("%1%2%3-%4-%5.log")
      .arg(info.absolutePath())
      .arg(QDir::separator())
      .arg(info.completeBaseName())
      .arg(start_box_->value(), 6, 10, QChar('0'))
      .arg(end_box_->value(), 6, 10, QChar('0'));

  QFileDialog::Options options;
  QString filter;
  export_filename = QFileDialog::getSaveFileName(NULL,
                                                 "Export interval",
                                                 export_filename,
                                                 "All Files (*);;Log Files (*.log)",
                                                 &filter,
                                                 options);

  if (export_filename.isEmpty())
    return;

  QThread thread;
  ExportStreamWorker worker(reader_,
                            export_filename,
                            stream_name_,
                            start_box_->value(),
                            end_box_->value());

  worker.moveToThread(&thread);

  connect(&thread, SIGNAL(started()), &worker, SLOT(exportInterval()));
  connect(&worker, SIGNAL(finished()), &thread, SLOT(quit()), Qt::DirectConnection);
  thread.start();

  QString message = QString("Exporting interval from %1 to %2")
                      .arg(start_box_->value())
                      .arg(end_box_->value());

  QProgressDialog progress_dialog(
      message,
      "Abort",
      start_box_->value(),
      end_box_->value()-1,
      this);

  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.setValue(start_box_->value());

  connect(&worker, SIGNAL(valueChanged(int)), &progress_dialog, SLOT(setValue(int)));

  progress_dialog.exec();
  if (progress_dialog.wasCanceled()){
    worker.cancel();
    thread.quit();
  }
  thread.wait();

  if (!progress_dialog.wasCanceled())
    std::cout << "Saved log interval: " << export_filename.toStdString() << std::endl;

}

void QLogViewer::sliderReleased(int index)
{
  update();
}

void QLogViewer::sliderMoved(int index)
{
  current_index_box_->setValue(index);
  stream_.set_current_sample_index(index);
}

void QLogViewer::setStepValue(int step)
{
  step_ = step;
}

void QLogViewer::currentIndexEditingFinished()
{
  timeline_->setSliderIndex(current_index_box_->value());
  updateSample();
}

void QLogViewer::construct(
    const QString &filepath,
    const QString &stream_name,
    int rate)
{
  filename_ = filepath;
  construct(new LogReader(filepath.toStdString()), stream_name, rate);
}

void QLogViewer::construct(
    LogReader *reader,
    const QString &stream_name,
    int rate)
{
  reader_ = reader;
  stream_name_ = stream_name;
  stream_ = reader_->stream(stream_name_.toStdString());

  current_index_box_->setMaximum(stream_.total_samples() - 1);
  start_box_->setMaximum(stream_.total_samples() - 2);
  end_box_->setMaximum(stream_.total_samples() - 1);

  total_samples_label_->setText(QString("%1").arg(stream_.total_samples()));

  timeline_->setSteps(stream_.total_samples() - 1);
  timeline_->setStepSize(1);
  timeline_->setSliderIndex(0);

  step_box_->setMinimum(0);
  step_box_->setMaximum(MAXIMUM_STEP);

  rate_ = rate;
  widget_ = createWidget();

  connect(&timer_, SIGNAL(timeout()), this, SLOT(timeout()));
}

QPushButton* QLogViewer::createControlButton(const QString &icon_path)
{
  QPixmap pix(icon_path);
  QIcon icon(pix);

  QPushButton *button = new QPushButton();

  button->setIcon(icon);
  button->setIconSize(QSize(40, 40));
  button->setMinimumSize(60, 50);
  button->setMaximumSize(70, 70);
  button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  return button;
}

} // namespace rock_replay_cpp
