QT += core gui widgets network
CONFIG += c++17
TEMPLATE = app
TARGET = DispCtrl

msvc:QMAKE_CXXFLAGS += /utf-8
DEFINES += QT_MESSAGELOGCONTEXT

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Basic/log.cpp \
    Controller/controller.cpp \
    Controller/MarineRadarManager.cpp \
    PolarDisp/colorbarwidget.cpp \
    PolarDisp/echolinechart.cpp \
    PolarDisp/echorenderer.cpp \
    PolarDisp/polaraxis.cpp \
    PolarDisp/polargrid.cpp \
    PolarDisp/ppisscene.cpp \
    PolarDisp/ppiview.cpp \
    mainPanel/mainoverlayout.cpp \
    cusWidgets/custommessagebox.cpp

HEADERS += \
    mainwindow.h \
    Basic/ConfigManager.h \
    Basic/DispBasci.h \
    Basic/log.h \
    Basic/MarineProtocol.h \
    Controller/controller.h \
    Controller/MarineRadarManager.h \
    PolarDisp/colorbarwidget.h \
    PolarDisp/echolinechart.h \
    PolarDisp/echorenderer.h \
    PolarDisp/polaraxis.h \
    PolarDisp/polargrid.h \
    PolarDisp/ppisscene.h \
    PolarDisp/ppiview.h \
    mainPanel/mainoverlayout.h \
    cusWidgets/custommessagebox.h

FORMS += mainPanel/mainoverlayout.ui
RESOURCES += resource.qrc
