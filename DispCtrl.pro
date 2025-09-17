QT += core gui network charts core bluetooth serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

## 保证release模式也有行号和文件名
DEFINES += QT_MESSAGELOGCONTEXT
# 不要关闭调试输出宏（否则 qDebug 在 Release 也会被编译掉）
# 让 MSVC 的 Release 也带 PDB（可选）
CONFIG(release, debug|release): CONFIG += force_debug_info


 # 如果 MY_MACRO 被定义，则执行以下代码
QT += webengine webenginewidgets webchannel
#httpserver在6.14之后才引入
msvc:QMAKE_CXXFLAGS += -execution-charset:utf-8  #mingw转成msvc后编译编码错误。 需添加这几行
msvc:QMAKE_CXXFLAGS += -source-charset:utf-8


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Basic/Protocol.cpp \
    Basic/log.cpp \
    Controller/controller.cpp \
    Controller/data2dispmanager.cpp \
    Controller/disp2datamanager.cpp \
    Controller/disp2monmanager.cpp \
    Controller/disp2photomanager.cpp \
    Controller/disp2resmanager.cpp \
    Controller/disp2sigmanager.cpp \
    Controller/mon2dispmanager.cpp \
    Controller/sig2dispmanager.cpp \
    Controller/targetdispmanager.cpp \
    PointManager/detmanager.cpp \
    PointManager/point.cpp \
    PointManager/trackmanager.cpp \
    PolarDisp/datasyncmanager.cpp \
    PolarDisp/pointinfow.cpp \
    PolarDisp/polaraxis.cpp \
    PolarDisp/polargrid.cpp \
    PolarDisp/ppisscene.cpp \
    PolarDisp/ppiview.cpp \
    PolarDisp/pviewtopleft.cpp \
    PolarDisp/scanlayer.cpp \
    PolarDisp/sectorpolargrid.cpp \
    PolarDisp/sectorscene.cpp \
    PolarDisp/tooltip.cpp \
    PolarDisp/zoomview.cpp \
    PolarDisp/PolarSectorWidget.cpp \
    UDP/threadudpsocket.cpp \
    cusWidgets/custommessagebox.cpp \
    cusWidgets/cuswindow.cpp \
    cusWidgets/detachablewidget.cpp \
    main.cpp \
    mainPanel/azelrangewidget.cpp \
    mainPanel/mainoverlayout.cpp \
    mainwindow.cpp \
    mapDisp/mapprox.cpp

HEADERS += \
    Basic/ConfigManager.h \
    Basic/DispBasci.h \
    Basic/Protocol.h \
    Basic/bindThread.h \
    Basic/log.h \
    Basic/mathUtil.h \
    Controller/controller.h \
    Controller/data2dispmanager.h \
    Controller/disp2datamanager.h \
    Controller/disp2monmanager.h \
    Controller/disp2photomanager.h \
    Controller/disp2resmanager.h \
    Controller/disp2sigmanager.h \
    Controller/mon2dispmanager.h \
    Controller/sig2dispmanager.h \
    Controller/targetdispmanager.h \
    PointManager/detmanager.h \
    PointManager/point.h \
    PointManager/trackmanager.h \
    PolarDisp/datasyncmanager.h \
    PolarDisp/pointinfow.h \
    PolarDisp/polaraxis.h \
    PolarDisp/polargrid.h \
    PolarDisp/ppisscene.h \
    PolarDisp/ppiview.h \
    PolarDisp/pviewtopleft.h \
    PolarDisp/scanlayer.h \
    PolarDisp/sectorpolargrid.h \
    PolarDisp/sectorscene.h \
    PolarDisp/tooltip.h \
    PolarDisp/zoomview.h \
    PolarDisp/PolarSectorWidget.h \
    UDP/threadudpsocket.h \
    cusWidgets/custommessagebox.h \
    cusWidgets/cuswindow.h \
    cusWidgets/detachablewidget.h \
    mainPanel/azelrangewidget.h \
    mainPanel/mainoverlayout.h \
    mainwindow.h \
    mapDisp/mapprox.h

FORMS += \
    PolarDisp/pointinfow.ui \
    PolarDisp/pviewtopleft.ui \
    mainPanel/mainoverlayout.ui

TRANSLATIONS += \
    DispCtrl_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    resources/icon/array.png

RESOURCES += \
    resource.qrc
#选择资源里面加载的icon需要指定resrouce时  只有使用qtcreator的管理器才能看到 icon资源
