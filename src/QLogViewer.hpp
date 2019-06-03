#ifndef __QLOGVIEWER_HPP__
#define __QLOGVIEWER_HPP__

#include <QtGui>
#include <QTimer>
#include <iostream>
#include <rock_widget_collection/Timeline.h>
#include "LogReader.hpp"

#define REGISTER_LOGVIEWER(S, T, C)                                            \
class Register##C : RegisterQLogViewer {                                       \
public:                                                                        \
  Register##C()                                                                \
    : RegisterQLogViewer((S), createLogViewer) {}                              \
  static QLogViewer* createLogViewer() { return new C(); }                     \
  static Register##C register##C;                                              \
};                                                                             \
template <> QLogViewer* QLogViewer::createLogViewer<T>() { return new C(); }   \
Register##C Register##C::register##C = Register##C();


namespace rock_replay_cpp
{

class QLogViewer;

typedef QLogViewer* (*create_logviewer_fcn_t)(void);
typedef std::map<QString, std::vector<std::pair<QString, QString> > > stream_map_t;

class ExportStreamWorker : public QObject
{
  Q_OBJECT
public:

  ExportStreamWorker(LogReader* reader,
                     const QString& filename,
                     const QString& stream_name,
                     int start_index,
                     int end_index)
  {
    reader_ = reader;

    filename_ = filename;
    stream_name_ = stream_name;

    start_index_ = start_index;
    end_index_ = end_index;
    canceled_ = false;
  }

  void cancel();

signals:
    void finished();
    void valueChanged(int);
    void start();

public slots:
    void exportInterval();

private:

  static bool exportStreamCallback(int sampleNr, void *data);

  void updateProgress(int sampleNr);

  LogReader *reader_;

  QString filename_;
  QString stream_name_;

  base::Time start_time_;

  int start_index_;
  int end_index_;
  bool canceled_;
};

class RegisterQLogViewer
{
public:
  RegisterQLogViewer(const std::string& type, create_logviewer_fcn_t fcn);
};

class QStreamSelector : public QDialog
{
  Q_OBJECT
public:
  static bool getStreamName(
      LogReader *reader,
      QString& stream_name,
      QString &type_name);

private:
  QStreamSelector(stream_map_t stream_map, QWidget *parent = 0);

  QTreeWidgetItem* createItem(
      const QString& name,
      const QString& value,
      QTreeWidgetItem* parent = NULL);

protected slots:
  void itemDoubleClicked(
      QTreeWidgetItem *item,
      int column);
private:
  QTreeWidget *treewidget_;
  QString stream_name_;
  QString type_name_;
};

class QLogViewer : public QWidget
{
  Q_OBJECT

public:
  template <typename T>
  static QLogViewer *create(const QString &filepath,
                            const QString &stream_name,
                            int rate = 100)
  {
    QLogViewer *v = createLogViewer<T>();
    v->construct(filepath, stream_name, rate);
    return v;
  }

  static QLogViewer *create(const QString& filepath, int rate = 100);

  void closeEvent(QCloseEvent *evt);

  virtual ~QLogViewer();

protected:
  template <typename T>
  static QLogViewer *createLogViewer()
  {
    std::cout << "Log type not defined not defined " << typeid(T).name() << std::endl;
    throw std::runtime_error("Not implemented");
  }

  template <typename T>
  bool nextSample(T &sample)
  {
    return stream_.next<T>(sample);
  }


protected slots:

  void timeout();

  void backButtonClicked(bool checked = false);

  void playButtonClicked(bool checked = false);

  void nextButtonClicked(bool checked = false);

  void stopButtonClicked(bool checked = false);

  void copyStartButtonClicked(bool checked = false);

  void copyEndButtonClicked(bool checked = false);

  void saveIntervalButtonClicked(bool checked = false);

  void sliderReleased(int index);

  void sliderMoved(int index);

  void setStepValue(int step);

  void currentIndexEditingFinished();

protected:
  virtual void update();

  QLogViewer();

  QWidget *widget() const
  {  typedef QLogViewer* (*create_logviewer_fcn_t)();
    return widget_;
  }

  virtual QWidget *createWidget() = 0;

  virtual void construct(const QString &filepath,
                         const QString &stream_name,
                         int rate = 100);

  virtual void construct(LogReader *reader,
                         const QString &stream_name,
                         int rate = 100);

  void updateSample();


private:
  static
  std::map<std::string, create_logviewer_fcn_t>&
  widgetMap()
  {
    static std::map<std::string, create_logviewer_fcn_t> map;
    return map;
  }

  static
  void
  addWidget(const std::string& type, create_logviewer_fcn_t fcn)
  {
    widgetMap().insert(std::make_pair(type, fcn));
  }

  QPushButton *createControlButton(const QString &icon_path);

  static const int MAXIMUM_STEP = 15;

  QTimer timer_;

  LogReader *reader_;
  LogStream stream_;

  QWidget *widget_;

  QSpinBox *current_index_box_;
  QLabel *total_samples_label_;

  QSpinBox *start_box_;
  QSpinBox *end_box_;
  QSpinBox *step_box_;

  Timeline *timeline_;

  int rate_;
  int step_;

  QString stream_name_;
  QString filename_;

  bool running_;

  friend class RegisterQLogViewer;
};


} // namespace rock_replay_cpp

#endif // __QLOGVIEWER__