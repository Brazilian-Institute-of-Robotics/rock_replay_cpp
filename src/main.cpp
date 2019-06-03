#include <iostream>
#include <QApplication>
#include <base/samples/Sonar.hpp>
#include "QLogViewer.hpp"

using namespace pocolog_cpp;
using namespace rock_replay_cpp;

int main(int argc, char **argv)
{

  if (argc <= 1)
  {
    std::cerr << "Inform log filename." << std::endl;
    return -1;
  }

  QApplication app(argc, argv);

  QLogViewer* viewer = QLogViewer::create(QString(argv[1]), 10);

  if (!viewer) return -1;

  viewer->show();

  return app.exec();
}
