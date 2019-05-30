#ifndef QSONARLOGVIEWER_H
#define QSONARLOGVIEWER_H

#include <QObject>
#include <QTimer>
#include <base/samples/Sonar.hpp>
#include "QLogViewer.hpp"
#include "LogReader.hpp"

namespace rock_replay_cpp
{

class QSonarLogViewer : public QLogViewer
{
protected:
  virtual QWidget* createWidget();
  virtual void update();
};

REGISTER_LOGVIEWER("/base/samples/Sonar", base::samples::Sonar, QSonarLogViewer)

} // namespace rock_replay_cpp

#endif // QLogViewer