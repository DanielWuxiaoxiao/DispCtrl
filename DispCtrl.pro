##############################################################################
# DispCtrl.pro - 雷达显示控制系统Qt项目配置文件
#
# 项目描述：
#   实时雷达数据显示和控制系统，支持多种数据类型的接收、处理和可视化
#   包括检测点、航迹、系统监控等功能
#
# 主要模块：
#   - UDP通信模块：网络数据接收和发送
#   - 数据管理模块：统一的雷达数据管理
#   - 显示模块：PPI、扇区等多种显示方式
#   - 控制模块：系统参数控制和配置
#   - 错误处理模块：统一的错误处理和重试机制
#
# 技术栈：Qt 5.14.2, C++11, WebEngine, Network
#
# 作者：DanielWuxiaoxiao
# 创建日期：2024
# 最后更新：2025-09-17
##############################################################################

# Qt模块配置 - 基础模块和扩展功能
QT += core gui network charts bluetooth serialport

# Qt 4兼容性检查
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# C++版本设置 - 更新为C++17以匹配CMakeLists.txt
CONFIG += c++17

# 确保 MOC、UIC、RCC 正确处理
CONFIG += moc uic resources

# MOC 和 UI 输出目录（确保跨平台一致性）
MOC_DIR = build/moc
UI_DIR = build/ui
OBJECTS_DIR = build/obj
RCC_DIR = build/rcc

# 调试配置 - 保证release模式也有行号和文件名用于错误追踪
DEFINES += QT_MESSAGELOGCONTEXT
# 不要关闭调试输出宏（否则 qDebug 在 Release 也会被编译掉）
# 让 MSVC 的 Release 也带 PDB（可选）
CONFIG(release, debug|release): CONFIG += force_debug_info

# Web引擎模块 - 用于地图显示和Web界面集成
QT += webengine webenginewidgets webchannel
#httpserver在6.14之后才引入

# MSVC编译器编码设置 - 解决中文编码问题
msvc:QMAKE_CXXFLAGS += /utf-8  # 统一源/执行字符集，避免 C4819

# Linux/Unix 平台特定设置
unix:!macx {
    # 确保正确的链接顺序
    QMAKE_LFLAGS += -Wl,--no-undefined
    # 确保 MOC 文件被正确处理
    QMAKE_MOC = $$[QT_INSTALL_BINS]/moc
}

# 编译器警告设置
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

##############################################################################
# 源文件配置 (SOURCES) - 按模块分组
##############################################################################

SOURCES += \
    # 基础模块 - 协议定义、日志系统、配置管理
    Basic/Protocol.cpp \
    Basic/log.cpp \
    \
    # 控制模块 - 各种管理器和控制逻辑
    Controller/controller.cpp \
    Controller/data2dispmanager.cpp \
    Controller/disp2datamanager.cpp \
    Controller/disp2monmanager.cpp \
    Controller/disp2photomanager.cpp \
    Controller/disp2resmanager.cpp \
    Controller/res2dispmanager.cpp \
    Controller/disp2sigmanager.cpp \
    Controller/mon2dispmanager.cpp \
    Controller/sig2dispmanager.cpp \
    Controller/tbd2dispmanager.cpp \
    Controller/targetdispmanager.cpp \
    Controller/ExternalCtrlManager.cpp \
    Controller/RadarDataManager.cpp \
    Controller/ErrorHandler.cpp \
    \
    # 参数配置模块
    paramWidget/batterycontrol.cpp \
    paramWidget/tranrecvui.cpp \
    paramWidget/datasaveui.cpp \
    paramWidget/dataprocessui.cpp \
    paramWidget/sigparamui.cpp \
    paramWidget/freqcontrolui.cpp \
    paramWidget/waveandsample.cpp \
    paramWidget/scanrangeui.cpp \
    paramWidget/photoelectricparam.cpp \
    paramWidget/servocontrol.cpp \
    paramWidget/twsmodedialog.cpp \
    paramWidget/tasmodedialog.cpp \
    \
    # 点管理模块 - 检测点和航迹管理
    PointManager/detmanager.cpp \
    PointManager/point.cpp \
    PointManager/trackmanager.cpp \
    PointManager/sectordetmanager.cpp \
    PointManager/sectortrackmanager.cpp \
    \
    # 极坐标显示模块 - PPI显示和相关组件
    PolarDisp/pointinfow.cpp \
    PolarDisp/polaraxis.cpp \
    PolarDisp/polargrid.cpp \
    PolarDisp/ppisscene.cpp \
    PolarDisp/ppiview.cpp \
    PolarDisp/pviewtopleft.cpp \
    PolarDisp/mousepositioninfo.cpp \
    PolarDisp/ppivisualsettings.cpp \
    PolarDisp/scanlayer.cpp \
    PolarDisp/sectorpolargrid.cpp \
    PolarDisp/sectorscene.cpp \
    PolarDisp/sectorwidget.cpp \
    PolarDisp/rangeazimuthwidget.cpp \
    PolarDisp/rangeazimuthchart.cpp \
    PolarDisp/tooltip.cpp \
    PolarDisp/zoomview.cpp \
    \
    # UDP通信模块 - 网络数据收发
    UDP/threadudpsocket.cpp \
    \
    # 自定义组件模块 - UI控件
    cusWidgets/custommessagebox.cpp \
    cusWidgets/cuswindow.cpp \
    cusWidgets/detachablewidget.cpp \
    cusWidgets/customcombobox.cpp \
    cusWidgets/customspinbox.cpp \
    cusWidgets/customspinboxstyle.cpp \
    cusWidgets/customlinechart.cpp \
    \
    # 主程序和主界面
    main.cpp \
    mainwindow.cpp \
    \
    # 主界面模块 - 布局和控制面板
    mainPanel/azelrangewidget.cpp \
    mainPanel/mainoverlayout.cpp \
    \
    # 地图显示模块
    mapDisp/mapprox.cpp

##############################################################################
# 头文件配置 (HEADERS) - 按模块分组
##############################################################################

HEADERS += \
    # 基础模块头文件
    Basic/ConfigManager.h \
    Basic/DispBasci.h \
    Basic/Protocol.h \
    Basic/bindThread.h \
    Basic/log.h \
    Basic/mathUtil.h \
    \
    # 控制模块头文件
    Controller/controller.h \
    Controller/data2dispmanager.h \
    Controller/disp2datamanager.h \
    Controller/disp2monmanager.h \
    Controller/disp2photomanager.h \
    Controller/disp2resmanager.h \
    Controller/res2dispmanager.h \
    Controller/disp2sigmanager.h \
    Controller/mon2dispmanager.h \
    Controller/sig2dispmanager.h \
    Controller/tbd2dispmanager.h \
    Controller/targetdispmanager.h \
    Controller/ExternalCtrlManager.h \
    Controller/RadarDataManager.h \
    Controller/ErrorHandler.h \
    \
    # 参数配置模块头文件
    paramWidget/batterycontrol.h \
    paramWidget/tranrecvui.h \
    paramWidget/datasaveui.h \
    paramWidget/dataprocessui.h \
    paramWidget/sigparamui.h \
    paramWidget/freqcontrolui.h \
    paramWidget/waveandsample.h \
    paramWidget/scanrangeui.h \
    paramWidget/photoelectricparam.h \
    paramWidget/servocontrol.h \
    paramWidget/twsmodedialog.h \
    paramWidget/tasmodedialog.h \
    \
    # 点管理模块头文件
    PointManager/detmanager.h \
    PointManager/point.h \
    PointManager/trackmanager.h \
    PointManager/sectordetmanager.h \
    PointManager/sectortrackmanager.h \
    \
    # 极坐标显示模块头文件
    PolarDisp/pointinfow.h \
    PolarDisp/polaraxis.h \
    PolarDisp/polargrid.h \
    PolarDisp/ppisscene.h \
    PolarDisp/ppiview.h \
    PolarDisp/pviewtopleft.h \
    PolarDisp/mousepositioninfo.h \
    PolarDisp/ppivisualsettings.h \
    PolarDisp/scanlayer.h \
    PolarDisp/sectorpolargrid.h \
    PolarDisp/sectorscene.h \
    PolarDisp/sectorwidget.h \
    PolarDisp/rangeazimuthwidget.h \
    PolarDisp/rangeazimuthchart.h \
    PolarDisp/tooltip.h \
    PolarDisp/zoomview.h \
    \
    # UDP通信模块头文件
    UDP/threadudpsocket.h \
    \
    # 自定义组件模块头文件
    cusWidgets/custommessagebox.h \
    cusWidgets/cuswindow.h \
    cusWidgets/detachablewidget.h \
    cusWidgets/customcombobox.h \
    cusWidgets/customspinbox.h \
    cusWidgets/customspinboxstyle.h \
    cusWidgets/customlinechart.h \
    \
    # 主界面头文件
    mainwindow.h \
    \
    # 主界面模块头文件
    mainPanel/azelrangewidget.h \
    mainPanel/mainoverlayout.h \
    \
    # 地图显示模块头文件
    mapDisp/mapprox.h

##############################################################################
# UI文件配置
##############################################################################

FORMS += \
    mainPanel/mainoverlayout.ui \
    PolarDisp/pointinfow.ui \
    PolarDisp/pviewtopleft.ui \
    PolarDisp/mousepositioninfo.ui \
    PolarDisp/ppivisualsettings.ui \
    \
    # 参数配置模块 UI
    paramWidget/batterycontrol.ui \
    paramWidget/tranrecvui.ui \
    paramWidget/datasaveui.ui \
    paramWidget/dataprocessui.ui \
    paramWidget/sigparamui.ui \
    paramWidget/freqcontrolui.ui \
    paramWidget/waveandsample.ui \
    paramWidget/scanrangeui.ui \
    paramWidget/photoelectricparam.ui \
    paramWidget/servocontrol.ui \
    paramWidget/twsmodedialog.ui \
    paramWidget/tasmodedialog.ui

##############################################################################
# 资源文件配置
##############################################################################

RESOURCES += \
    resource.qrc

##############################################################################
# 翻译文件配置
##############################################################################

TRANSLATIONS += \
    DispCtrl_zh_CN.ts

##############################################################################
# 部署配置
##############################################################################

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    resources/icon/array.png
#选择资源里面加载的icon需要指定resrouce时  只有使用qtcreator的管理器才能看到 icon资源
