#include <iostream>
#include <rock_widget_collection/SonarWidget.h>
#include "QSonarLogViewer.hpp"

namespace rock_replay_cpp
{

QWidget* QSonarLogViewer::createWidget()
{
  return new SonarWidget(NULL);
}

base::Time QSonarLogViewer::update()
{
  base::samples::Sonar data;
  if (nextSample<base::samples::Sonar>(data))
  {
    SonarWidget* w = static_cast<SonarWidget*>(widget());
    w->setData(data);
    return data.time;
  }

  return base::Time();
}

} // namespace rock_replay_cpp
