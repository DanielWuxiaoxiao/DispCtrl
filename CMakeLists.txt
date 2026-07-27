cmake_minimum_required(VERSION 3.16)
project(DispCtrl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    add_compile_options(/utf-8 /wd4819)
endif()

add_compile_definitions(QT_MESSAGELOGCONTEXT)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(DispCtrl
    main.cpp
    mainwindow.cpp
    mainwindow.h
    Basic/ConfigManager.h
    Basic/DispBasci.h
    Basic/log.cpp
    Basic/log.h
    Basic/MarineProtocol.h
    Controller/controller.cpp
    Controller/controller.h
    Controller/MarineRadarManager.cpp
    Controller/MarineRadarManager.h
    PolarDisp/colorbarwidget.cpp
    PolarDisp/colorbarwidget.h
    PolarDisp/echolinechart.cpp
    PolarDisp/echolinechart.h
    PolarDisp/echorenderer.cpp
    PolarDisp/echorenderer.h
    PolarDisp/polaraxis.cpp
    PolarDisp/polaraxis.h
    PolarDisp/polargrid.cpp
    PolarDisp/polargrid.h
    PolarDisp/ppisscene.cpp
    PolarDisp/ppisscene.h
    PolarDisp/ppiview.cpp
    PolarDisp/ppiview.h
    mainPanel/mainoverlayout.cpp
    mainPanel/mainoverlayout.h
    mainPanel/mainoverlayout.ui
    cusWidgets/custommessagebox.cpp
    cusWidgets/custommessagebox.h
    resource.qrc
)

target_link_libraries(DispCtrl PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)

set_target_properties(DispCtrl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
