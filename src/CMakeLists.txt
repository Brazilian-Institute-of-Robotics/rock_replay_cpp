find_package(Qt4 REQUIRED QtCore QtGui)
find_package( Boost COMPONENTS system filesystem)


qt4_add_resources(QtApp_RCC_SRCS ${PROJECT_SOURCE_DIR}/resources.qrc)

qt4_wrap_cpp(
  rock_replay_cpp_MOC_CPP
  QLogViewer.hpp
)

rock_executable(rock-replay-cpp
    main.cpp
    QLogViewer.cpp
    QSonarLogViewer.cpp
    LogReader.cpp
    ${rock_replay_cpp_MOC_CPP}
    ${QtApp_RCC_SRCS}
  DEPS_PLAIN
    Boost_SYSTEM
    Boost_FILESYSTEM
  DEPS_PKGCONFIG
    base-lib
    pocolog_cpp
    rock_widget_collection
    QtCore
    QtGui)
