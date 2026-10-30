##############################################################################
# DispCtrl 雷达显示控制系统 - CMake构建配置文件
##############################################################################
# 项目描述：
#   雷达显示控制系统（DispCtrl）是一个基于Qt的实时雷达数据显示与控制应用
#   主要功能包括：
#   - 实时雷达数据接收和处理
#   - PPI（平面位置显示）和极坐标显示
#   - 目标检测和航迹管理
#   - UDP网络通信
#   - Web地图集成显示
#   - 多线程数据处理
#
# 技术栈：
#   - Qt 5.14+ (Widgets, WebEngine, Network)
#   - C++17标准
#   - CMake 3.16+ 构建系统
#   - 多线程架构设计
#
# 构建配置：
#   - 支持Debug和Release配置
#   - 自动MOC（Meta-Object Compiler）处理
#   - 自动UI文件编译
#   - 自动资源文件处理
##############################################################################

# CMake最低版本要求 - 3.16支持Qt自动化工具和现代C++特性
cmake_minimum_required(VERSION 3.16)

# 项目定义 - 指定项目名称和使用的编程语言
project(DispCtrl LANGUAGES CXX)

##############################################################################
# 编译器和语言标准配置
##############################################################################

# C++17标准 - 使用现代C++特性（auto、constexpr、if constexpr等）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##############################################################################
# 编译器配置 - 最简化设置，避免环境冲突
##############################################################################

# MSVC编码设置：确保正确处理UTF-8源文件和中文注释
if(MSVC)
    # 使用 /utf-8 选项强制MSVC使用UTF-8编码
    # 这会同时设置源文件编码和执行字符集为UTF-8
    add_compile_options(/utf-8)

    # 禁用 C4819 警告（代码页警告）
    add_compile_options(/wd4819)
endif()

# GCC/Clang 编译选项（Linux/macOS）
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

##############################################################################
# 日志输出配置 - 确保Release模式也保留调试日志
##############################################################################

# 强制Qt在所有构建模式下保留日志输出
# 默认情况下，Qt会在Release模式禁用qDebug()等日志宏
# 添加这个定义可以确保日志系统在Release模式下也能正常工作
add_compile_definitions(QT_MESSAGELOGCONTEXT)

# 可选：如果需要在Release模式下也启用qDebug，取消下面的注释
# add_compile_definitions(QT_NO_DEBUG_OUTPUT=0)

##############################################################################
# Qt自动化工具配置
##############################################################################

# 自动MOC - 自动处理Qt的信号槽机制和反射系统
set(CMAKE_AUTOMOC ON)

# 自动UIC - 自动编译Qt Designer创建的.ui文件
set(CMAKE_AUTOUIC ON)

# 自动RCC - 自动编译Qt资源文件(.qrc)
set(CMAKE_AUTORCC ON)

##############################################################################
# Qt库依赖配置
##############################################################################

# 查找Qt5库 - 雷达显示系统所需的核心组件
find_package(Qt5 REQUIRED COMPONENTS
    Widgets          # GUI界面组件（按钮、窗口、布局等）
    Core             # Qt核心功能（字符串、容器、事件系统等）
    Gui              # 底层GUI支持（绘图、字体、颜色等）
    Network          # 网络通信（UDP Socket、HTTP等）
    WebEngineWidgets # Web浏览器组件（地图显示）
    WebChannel       # Qt与JavaScript通信桥梁
)

# Qt库列表 - 用于后续的链接配置
set(QT_LIBS
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::WebEngineWidgets
    Qt5::WebChannel
)

##############################################################################
# 源文件配置 - 按功能模块组织
##############################################################################

# 主要源文件列表 - 显式列出所有.cpp文件以确保构建稳定性
# 注意：显式列出比通配符更可靠，避免automoc问题
set(SRC_FILES
    # 主程序和主界面
    main.cpp                                    # 程序入口点
    mainwindow.cpp                             # 主窗口实现

    # UDP通信模块 - 网络数据收发
    UDP/threadudpsocket.cpp                    # 多线程UDP Socket通信

    # 地图显示模块 - Web地图集成
    mapDisp/mapprox.cpp                        # 地图代理和Web交互

    # 极坐标显示模块 - PPI雷达显示核心
    PolarDisp/pointinfow.cpp                   # 点信息窗口
    PolarDisp/polargrid.cpp                    # 极坐标网格绘制
    PolarDisp/polaraxis.cpp                    # 极坐标轴绘制
    PolarDisp/ppisscene.cpp                    # PPI场景管理
    PolarDisp/scanlayer.cpp                    # 扫描层显示
    PolarDisp/scanmarkeroverlay.cpp             # QWidget上层扫描标记
    PolarDisp/sectorwidget.cpp                 # 扇区部件
    PolarDisp/sectorscene.cpp                  # 扇区场景
    PolarDisp/sectorpolargrid.cpp              # 扇区极坐标网格
    PolarDisp/rangeazimuthwidget.cpp           # 距离-方位显示部件
    PolarDisp/rangeazimuthchart.cpp            # 距离-方位图表（直角坐标系）
    PolarDisp/rangeheightchart.cpp             # 距离-高度图表（直角坐标系）
    PolarDisp/track3dwidget.cpp                 # 三维最新航迹显示（WebEngine + Three.js）
    PolarDisp/pviewtopleft.cpp                 # 左上角视图
    PolarDisp/mousepositioninfo.cpp            # 鼠标位置信息
    PolarDisp/ppivisualsettings.cpp            # PPI视觉设置
    PolarDisp/tooltip.cpp                      # 工具提示
    PolarDisp/zoomview.cpp                     # 缩放视图
    PolarDisp/ppiview.cpp                      # PPI主视图
    PolarDisp/geofenceitem.cpp                 # PPI围栏图形项

    # 点管理模块 - 目标检测和航迹管理
    PointManager/detmanager.cpp                # 检测点管理器
    PointManager/point.cpp                     # 点对象基类
    PointManager/trackmanager.cpp              # 航迹管理器
    PointManager/sectordetmanager.cpp          # 扇区检测管理
    PointManager/sectortrackmanager.cpp        # 扇区航迹管理

    # 控制模块 - 数据流控制和业务逻辑
    Controller/controller.cpp                  # 主控制器
    Controller/testtrackgenerator.cpp          # 本地测试航迹生成
    Controller/disp2datamanager.cpp            # 显示到数据管理
    Controller/data2dispmanager.cpp            # 数据到显示管理
    Controller/tbd2dispmanager.cpp             # TBD数据到显示管理
    Controller/collabtrack2dispmanager.cpp     # 协同航迹到显示管理
    Controller/disp2monmanager.cpp             # 显示到监控管理
    Controller/targetdispmanager.cpp           # 目标显示管理
    Controller/sig2dispmanager.cpp             # 信号到显示管理
    Controller/mon2dispmanager.cpp             # 监控到显示管理
        Controller/ExternalCtrlManager.cpp        # 外部雷控/调度链路
    Controller/gcsmanager.cpp                  # GCS地面站通信链路
    Controller/edgeradarreporter.cpp           # JSON雷达数据上报链路
    Controller/edgeradarresultreceiver.cpp      # 边缘终端识别结果回传接收
    Controller/totalcontrolmqttclient.cpp       # 总控MQTT识别结果上报
    Controller/laserreportmanager.cpp           # 激光侦察上报（独立模块）
    Controller/subsystemnetworkmonitor.cpp     # 健康管理分系统网络状态
    CommandControl/commandcontrolprotocol.cpp  # 总控 UDP 协议编解码
    CommandControl/commandcontrolconfig.cpp    # 总控通信独立配置
    CommandControl/commandcontrolnetworkmonitor.cpp # 总控网络连通状态
    CommandControl/ntptimesync.cpp             # NTP 授时与系统时间更新
    CommandControl/commandcontrolrecordstore.cpp # DDA4 会话记录
    CommandControl/commandcontroltrackreportstore.cpp # DDA4 航迹上报过程独立记录
    CommandControl/commandcontrolrecordwriter.cpp # DDA4 独立记录线程
    CommandControl/commandcontroltransport.cpp # UDP 工作线程
    CommandControl/commandcontrolmodule.cpp    # 总控通信核心模块
    CommandControl/commandcontrolwindow.cpp    # 总控通信独立窗口
    ElectromagneticDevice/deviceprotocol.cpp
    ElectromagneticDevice/devicetransport.cpp
    ElectromagneticDevice/devicemodule.cpp
    PhotoelectricV2/module.cpp
    ZhoushanOptical/zhoushanmodule.cpp
    Controller/RadarDataManager.cpp            # 雷达数据统一管理器
    Controller/ErrorHandler.cpp                # 错误处理框架
    Controller/disp2sigmanager.cpp             # 显示到信号管理
    Controller/disp2resmanager.cpp             # 显示到资源管理
    Controller/res2dispmanager.cpp             # 资源到显示管理
    Controller/disp2photomanager.cpp           # 显示到照片管理
    Controller/disp2datamanager.cpp            # 显示到数据管理
    Controller/ascanmanager.cpp                # A显数据UDP接收管理器
    Controller/beamschedulemanager.cpp         # 波束调度报告UDP接收管理器
    Controller/geofencemanager.cpp             # 电子围栏检测管理器
    Controller/recordmanager.cpp               # 场景录制/回放管理器
    Controller/raedatasetreader.cpp            # 离线RAE点迹数据读取器

    # 主界面模块 - 控制面板和布局
    mainPanel/mainoverlayout.cpp               # 主界面覆盖布局
    mainPanel/azelrangewidget.cpp              # 方位角高低角范围控件
    mainPanel/screenrecorderwidget.cpp         # 屏幕录制与回放
    mainPanel/simpleavi.cpp                    # 纯Qt AVI视频写入器
    mainPanel/logpanel.cpp                     # 日志面板
    mainPanel/statspanel.cpp                   # 统计面板
    mainPanel/healthdashboard.cpp              # 健康仪表板
    mainPanel/ascanwidget.cpp                  # A显波形控件
    mainPanel/beamschedulewidget.cpp           # 波束调度甘特图控件

    # 自定义组件模块 - UI扩展控件
    cusWidgets/detachablewidget.cpp            # 可分离窗口部件
    cusWidgets/cuswindow.cpp                   # 自定义窗口
    cusWidgets/custommessagebox.cpp            # 自定义消息框
    cusWidgets/developerlogindialog.cpp        # 运行时开发者模式验证
    cusWidgets/customcombobox.cpp              # 自定义ComboBox组件
    cusWidgets/customspinbox.cpp               # 自定义SpinBox组件
    cusWidgets/customspinboxstyle.cpp          # 自定义SpinBox样式
    cusWidgets/customlinechart.cpp             # 自定义直角坐标图表基类
    cusWidgets/frozentablewidget.cpp           # 冻结首列表格组件

    # 参数配置模块 - 雷达参数配置对话框
    paramWidget/batterycontrol.cpp             # 一键全数配置
    paramWidget/tranrecvui.cpp                 # 发射接收控制
    paramWidget/datasaveui.cpp                 # 数据存储/删除
    paramWidget/dataprocessui.cpp              # 数据处理参数
    paramWidget/sigparamui.cpp                 # 信号处理参数
    paramWidget/freqcontrolui.cpp              # 波形及采样控制
    paramWidget/waveandsample.cpp              # 电调控制
    paramWidget/scanrangeui.cpp                # 方向图扫描控制
    paramWidget/photoelectricparam.cpp         # 光电系统控制
    paramWidget/servocontrol.cpp               # 伺服控制
    paramWidget/twsmodedialog.cpp              # TWS模式设置对话框
    paramWidget/tasmodedialog.cpp              # TAS模式设置对话框
    paramWidget/radarpanelpositiondialog.cpp

    # 基础模块 - 系统基础功能
    Basic/log.cpp                              # 日志系统
    Basic/authmanager.cpp                      # 用户/开发者模式状态和通知
    Basic/Protocol.cpp                         # 通信协议定义
    Basic/OsmRoadParser.cpp                    # OSM道路数据解析器
)

##############################################################################
# 头文件配置 - 按功能模块组织
##############################################################################

set(HDR_FILES
    # 主界面头文件
    mainwindow.h                               # 主窗口类定义
    PolarDisp/rangeheightchart.h               # 距离-高度图表
    PolarDisp/track3dcoordinate.h               # 三维航迹物理坐标换算
    PolarDisp/track3dwidget.h                   # 三维最新航迹显示

    # 控制模块头文件 - 数据流和业务逻辑
    Controller/targetdispmanager.h             # 目标显示管理器
    Controller/sig2dispmanager.h               # 信号到显示管理器
    Controller/mon2dispmanager.h               # 监控到显示管理器
    Controller/RadarDataManager.h            # 雷达数据管理器
    Controller/ErrorHandler.h                  # 错误处理框架
    Controller/errorloglimiter.h               # 有界错误分类限频
    Controller/disp2sigmanager.h               # 显示到信号管理器
    Controller/disp2resmanager.h               # 显示到资源管理器
    Controller/res2dispmanager.h               # 资源到显示管理器
    Controller/disp2photomanager.h             # 显示到照片管理器
    PhotoelectricV2/module.h
    PhotoelectricV2/protocol.h
    ZhoushanOptical/zhoushanmodule.h
    ZhoushanOptical/protocol.h
    Controller/disp2monmanager.h               # 显示到监控管理器
    Controller/disp2datamanager.h              # 显示到数据管理器
    Controller/data2dispmanager.h              # 数据到显示管理器
    Controller/tbd2dispmanager.h               # TBD数据到显示管理器
    Controller/collabtrack2dispmanager.h       # 协同航迹到显示管理器
    Controller/controller.h                    # 主控制器
    Controller/testtrackgenerator.h            # 本地测试航迹生成
    Controller/ExternalCtrlManager.h           # 外部雷控/调度链路
    Controller/gcsmanager.h                    # GCS地面站通信链路
    Controller/edgeradarreporter.h             # JSON雷达数据上报链路
    Controller/edgerecognitionresult.h         # 边缘终端光电识别结果结构
    Controller/edgeradarresultreceiver.h        # 边缘终端识别结果回传接收
    Controller/totalcontrolmqttclient.h         # 总控MQTT识别结果上报
    Controller/subsystemnetworkmonitor.h        # 健康管理分系统网络状态
    CommandControl/commandcontrolprotocol.h     # 总控 UDP 协议编解码
    CommandControl/commandcontrolconfig.h       # 总控通信独立配置
    CommandControl/commandcontrolnetworkmonitor.h # 总控网络连通状态
    CommandControl/ntptimesync.h                # NTP 授时与系统时间更新
    CommandControl/commandcontrolrecordstore.h  # DDA4 会话记录
    CommandControl/commandcontroltrackreportstore.h # DDA4 航迹上报过程独立记录
    CommandControl/commandcontrolrecordwriter.h # DDA4 独立记录线程
    CommandControl/commandcontroltransport.h    # UDP 工作线程
    CommandControl/commandcontrolmodule.h       # 总控通信核心模块
    CommandControl/commandcontrolwindow.h       # 总控通信独立窗口
    ElectromagneticDevice/deviceprotocol.h
    ElectromagneticDevice/devicetransport.h
    ElectromagneticDevice/devicemodule.h
    Controller/ascanmanager.h                  # A显数据UDP接收管理器
    Controller/beamschedulemanager.h           # 波束调度报告UDP接收管理器
    Controller/geofencemanager.h               # 电子围栏检测管理器
    Controller/recordmanager.h                 # 场景录制/回放管理器

    # 自定义组件头文件 - UI扩展控件
    cusWidgets/detachablewidget.h              # 可分离窗口部件
    cusWidgets/cuswindow.h                     # 自定义窗口
    cusWidgets/windowgeometry.h                # 多屏窗口几何选择
    cusWidgets/custommessagebox.h              # 自定义消息框
    cusWidgets/developerlogindialog.h           # 运行时开发者模式验证
    cusWidgets/customcombobox.h                # 自定义ComboBox组件
    cusWidgets/customspinbox.h                 # 自定义SpinBox组件
    cusWidgets/customspinboxstyle.h            # 自定义SpinBox样式
    cusWidgets/customlinechart.h               # 自定义直角坐标图表基类
    cusWidgets/frozentablewidget.h             # 冻结首列表格组件

    # 参数配置模块头文件 - 雷达参数配置对话框
    paramWidget/batterycontrol.h               # 一键全数配置
    paramWidget/tranrecvui.h                   # 发射接收控制
    paramWidget/datasaveui.h                   # 数据存储/删除
    paramWidget/dataprocessui.h                # 数据处理参数
    paramWidget/sigparamui.h                   # 信号处理参数
    paramWidget/freqcontrolui.h                # 波形及采样控制
    paramWidget/waveandsample.h                # 电调控制
    paramWidget/scanrangeui.h                  # 方向图扫描控制
    paramWidget/photoelectricparam.h           # 光电系统控制
    paramWidget/servocontrol.h                 # 伺服控制

    # 基础模块头文件 - 系统基础功能
    Basic/bindThread.h                         # 线程绑定工具
    Basic/Protocol.h                           # 通信协议定义
    Basic/wgs84coordinate.h                    # WGS84 与 ENU 坐标换算
    Basic/mathUtil.h                           # 数学工具函数
    Basic/log.h                               # 日志系统
    Basic/DispBasci.h                         # 显示基础定义
    Basic/uifontscale.h                       # Qt样式及绘制字体统一缩放
    Basic/uiscale.h                           # 不依赖协议打包的UI缩放定义
    Basic/ConfigManager.h                     # 配置管理器
    Basic/diagnosticlogwindow.h                # 固定槽单调时钟日志限频
    Basic/pointinfovalidation.h                # 检测点公开分发前校验
    Basic/authmanager.h                       # 管理者模式认证管理器
    Basic/radarreference.h                    # 经纬高/姿态逐字段优先级
    Basic/tascontrolformat.h                  # TAS频点编码及参数日志
    Basic/healthstatewindow.h                 # 5秒健康窗口及1000帧全零防抖
    Basic/gpspositionfilter.h                 # 100点GPS平滑与离群剔除
    Basic/radarpositioncontrol.h              # AA08四阵面位置编码
    Basic/radarpanelstate.h                   # 四阵面锁定与选中接入状态
    Basic/radarpanelstore.h                   # 独立TOML锁定快照原子读写
    Basic/OsmRoadParser.h                     # OSM道路数据解析器

    # 极坐标显示模块头文件 - PPI雷达显示
    PolarDisp/polargrid.h                     # 极坐标网格
    PolarDisp/polaraxis.h                     # 极坐标轴
    PolarDisp/pointinfow.h                    # 点信息窗口
    PolarDisp/zoomview.h                      # 缩放视图
    PolarDisp/tooltip.h                       # 工具提示
    PolarDisp/scanlayer.h                     # 扫描层
    PolarDisp/scanmarkeroverlay.h              # QWidget上层扫描标记
    PolarDisp/pviewtopleft.h                  # 左上角视图
    paramWidget/radarpanelpositiondialog.h
    PolarDisp/mousepositioninfo.h             # 鼠标位置信息
    PolarDisp/ppivisualsettings.h             # PPI视觉设置
    PolarDisp/sectorwidget.h                  # 扇区部件
    PolarDisp/sectorscene.h                   # 扇区场景
    PolarDisp/sectorpolargrid.h               # 扇区极坐标网格
    PolarDisp/rangeazimuthwidget.h            # 距离-方位显示部件
    PolarDisp/rangeazimuthchart.h             # 距离-方位图表（直角坐标系）
    PolarDisp/ppiview.h                       # PPI主视图
    PolarDisp/ppisscene.h                     # PPI场景管理
    PolarDisp/geofenceitem.h                  # PPI围栏图形项

    # 主界面模块头文件 - 控制面板和布局
    mainPanel/mainoverlayout.h                # 主界面覆盖布局
    mainPanel/azelrangewidget.h               # 方位角高低角范围控件
    mainPanel/screenrecorderwidget.h          # 屏幕录制与回放
    mainPanel/simpleavi.h                     # 纯Qt AVI视频写入器
    mainPanel/logpanel.h                      # 日志面板
    mainPanel/statspanel.h                    # 统计面板
    mainPanel/healthdashboard.h               # 健康仪表板
    mainPanel/ascanwidget.h                   # A显波形控件
    mainPanel/beamschedulewidget.h            # 波束调度甘特图控件

    # 点管理模块头文件 - 目标检测和航迹
    PointManager/sectordetmanager.h           # 扇区检测管理
    PointManager/sectortrackmanager.h         # 扇区航迹管理
    PointManager/detmanager.h                 # 检测点管理器
    PointManager/trackmanager.h               # 航迹管理器
    PointManager/point.h                      # 点对象基类

    # 地图显示模块头文件
    mapDisp/mapprox.h                         # 地图代理

    # UDP通信模块头文件
    UDP/threadudpsocket.h                     # 多线程UDP Socket
)

##############################################################################
# UI和资源文件配置
##############################################################################

# Qt Designer UI文件 - 图形界面设计文件
set(UI_FILES
    PolarDisp/pointinfow.ui                    # 点信息显示界面
    PolarDisp/pviewtopleft.ui                  # 左上角控制界面
    PolarDisp/mousepositioninfo.ui             # 鼠标位置信息界面
    PolarDisp/ppivisualsettings.ui             # PPI视觉设置界面
    mainPanel/mainoverlayout.ui                # 主界面布局
    paramWidget/batterycontrol.ui              # 一键全数配置界面
    paramWidget/tranrecvui.ui                  # 发射接收控制界面
    paramWidget/datasaveui.ui                  # 数据存储界面
    paramWidget/dataprocessui.ui               # 数据处理参数界面
    paramWidget/sigparamui.ui                  # 信号处理参数界面
    paramWidget/freqcontrolui.ui               # 波形及采样控制界面
    paramWidget/waveandsample.ui               # 电调控制界面
    paramWidget/scanrangeui.ui                 # 方向图扫描控制界面
    paramWidget/photoelectricparam.ui          # 光电系统控制界面
    paramWidget/servocontrol.ui                # 伺服控制界面
)

# Qt资源文件 - 图标、样式表、静态文件等
set(QRC_FILES
    resource.qrc                               # 主资源文件（图标、CSS等）
)

##############################################################################
# 可执行目标定义
##############################################################################

# 创建DispCtrl可执行文件目标
# 包含所有源文件、头文件、UI文件和资源文件
add_executable(DispCtrl
    ${SRC_FILES}                               # 所有.cpp源文件
    ${HDR_FILES}                               # 所有.h头文件
    ${UI_FILES}                                # 所有.ui界面文件
    ${QRC_FILES}                               # 所有.qrc资源文件
)

##############################################################################
# 库链接配置
##############################################################################

# 链接Qt库到DispCtrl目标
target_link_libraries(DispCtrl PRIVATE ${QT_LIBS})

# Windows 修改系统时间时需要调整 SeSystemtimePrivilege。
if(WIN32)
    target_link_libraries(DispCtrl PRIVATE advapi32)
endif()

option(DISPCTRL_BUILD_TESTS "Build focused DispCtrl QTest targets" OFF)
if(DISPCTRL_BUILD_TESTS)
    enable_testing()
    find_package(Qt5 REQUIRED COMPONENTS Test)
    add_executable(command_control_config_test tests/commandcontrol_config_test.cpp
        CommandControl/commandcontrolconfig.cpp Basic/log.cpp)
    target_include_directories(command_control_config_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(command_control_config_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME command_control_config_test COMMAND command_control_config_test)
    add_executable(track_data_rate_test tests/trackdatarate_test.cpp Basic/Protocol.cpp)
    target_include_directories(track_data_rate_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(track_data_rate_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME track_data_rate_test COMMAND track_data_rate_test)
    add_executable(data_save_test tests/datasave_test.cpp paramWidget/datasaveui.cpp
        paramWidget/datasaveui.h paramWidget/datasaveui.ui Basic/Protocol.cpp Basic/log.cpp
        cusWidgets/custommessagebox.cpp cusWidgets/custommessagebox.h)
    target_include_directories(data_save_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(data_save_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)
    add_test(NAME data_save_test COMMAND data_save_test)
    set_tests_properties(data_save_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(radar_panel_state_test tests/radarpanelstate_test.cpp)
    target_include_directories(radar_panel_state_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(radar_panel_state_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME radar_panel_state_test COMMAND radar_panel_state_test)
    add_executable(radar_panel_dialog_test tests/radarpanelpositiondialog_test.cpp paramWidget/radarpanelpositiondialog.cpp
        cusWidgets/cuswindow.cpp cusWidgets/cuswindow.h)
    target_include_directories(radar_panel_dialog_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(radar_panel_dialog_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)
    add_test(NAME radar_panel_dialog_test COMMAND radar_panel_dialog_test)
    set_tests_properties(radar_panel_dialog_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(bit_report_decoder_test tests/bitreportdecoder_test.cpp)
    target_include_directories(bit_report_decoder_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(bit_report_decoder_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME bit_report_decoder_test COMMAND bit_report_decoder_test)
    add_executable(ui_font_scale_test tests/uifontscale_test.cpp)
    target_include_directories(ui_font_scale_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(ui_font_scale_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)
    add_test(NAME ui_font_scale_test COMMAND ui_font_scale_test)
    set_tests_properties(ui_font_scale_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(map_no_map_test tests/mapnomap_test.cpp mapDisp/mapprox.cpp mapDisp/mapprox.h Basic/log.cpp)
    set_target_properties(map_no_map_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/map_no_map_fixture")
    target_include_directories(map_no_map_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(map_no_map_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Network Qt5::WebEngineWidgets Qt5::WebChannel Qt5::Test)
    add_test(NAME map_no_map_test COMMAND map_no_map_test)
    set_tests_properties(map_no_map_test PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QTWEBENGINE_DISABLE_SANDBOX=1;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu")
    add_executable(health_state_window_test tests/healthstatewindow_test.cpp)
    target_include_directories(health_state_window_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(health_state_window_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME health_state_window_test COMMAND health_state_window_test)
    add_executable(scan_layer_render_test tests/scanlayer_render_test.cpp
        PolarDisp/scanlayer.cpp PolarDisp/scanlayer.h PolarDisp/scanmarkeroverlay.cpp
        PolarDisp/polaraxis.cpp PolarDisp/polaraxis.h Basic/log.cpp)
    target_include_directories(scan_layer_render_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(scan_layer_render_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)
    add_test(NAME scan_layer_render_test COMMAND scan_layer_render_test)
    set_tests_properties(scan_layer_render_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(tas_control_format_test tests/tascontrolformat_test.cpp)
    target_include_directories(tas_control_format_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(tas_control_format_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME tas_control_format_test COMMAND tas_control_format_test)
    add_executable(radar_reference_config_test tests/radarreference_config_test.cpp Basic/log.cpp)
    target_include_directories(radar_reference_config_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(radar_reference_config_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME radar_reference_config_test COMMAND radar_reference_config_test)
    add_executable(radar_reference_test tests/radarreference_test.cpp)
    target_include_directories(radar_reference_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(radar_reference_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME radar_reference_test COMMAND radar_reference_test)
    add_executable(radar_position_control_test tests/radarpositioncontrol_test.cpp)
    target_include_directories(radar_position_control_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(radar_position_control_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME radar_position_control_test COMMAND radar_position_control_test)
    add_executable(sigpro_param_test tests/sigproparam_test.cpp)
    target_include_directories(sigpro_param_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(sigpro_param_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME sigpro_param_test COMMAND sigpro_param_test)
    add_executable(auth_mode_test tests/authmode_test.cpp Basic/authmanager.cpp Basic/authmanager.h
        cusWidgets/developerlogindialog.cpp cusWidgets/developerlogindialog.h)
    target_include_directories(auth_mode_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(auth_mode_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)
    add_test(NAME auth_mode_test COMMAND auth_mode_test)
    set_tests_properties(auth_mode_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(command_control_lifecycle_test tests/commandcontrol_lifecycle_test.cpp
        CommandControl/commandcontrolmodule.cpp CommandControl/commandcontrolmodule.h
        CommandControl/commandcontrolprotocol.cpp CommandControl/commandcontrolconfig.cpp
        CommandControl/commandcontrolnetworkmonitor.cpp CommandControl/commandcontrolnetworkmonitor.h
        CommandControl/ntptimesync.cpp CommandControl/ntptimesync.h
        CommandControl/commandcontrolrecordstore.cpp CommandControl/commandcontroltrackreportstore.cpp
        CommandControl/commandcontrolrecordwriter.cpp CommandControl/commandcontrolrecordwriter.h
        CommandControl/commandcontroltransport.cpp CommandControl/commandcontroltransport.h
        CommandControl/commandcontrolwindow.cpp CommandControl/commandcontrolwindow.h Basic/log.cpp)
    target_include_directories(command_control_lifecycle_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(command_control_lifecycle_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Network Qt5::Test)
    if(WIN32)
        target_link_libraries(command_control_lifecycle_test PRIVATE advapi32)
    endif()
    add_test(NAME command_control_lifecycle_test COMMAND command_control_lifecycle_test)
    add_executable(zhoushan_optical_protocol_test tests/zhoushanopticalprotocol_test.cpp)
    target_include_directories(zhoushan_optical_protocol_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(zhoushan_optical_protocol_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME zhoushan_optical_protocol_test COMMAND zhoushan_optical_protocol_test)
    add_executable(zhoushan_optical_module_test tests/zhoushanopticalmodule_test.cpp
        ZhoushanOptical/zhoushanmodule.cpp ZhoushanOptical/zhoushanmodule.h Basic/log.cpp cusWidgets/cuswindow.cpp)
    target_include_directories(zhoushan_optical_module_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(zhoushan_optical_module_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Network Qt5::Test)
    add_test(NAME zhoushan_optical_module_test COMMAND zhoushan_optical_module_test)
    add_executable(optical_indication_ui_test tests/opticalindicationui_test.cpp
        ZhoushanOptical/zhoushanmodule.cpp ZhoushanOptical/zhoushanmodule.h Basic/log.cpp cusWidgets/cuswindow.cpp)
    target_include_directories(optical_indication_ui_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(optical_indication_ui_test PRIVATE Qt5::Core Qt5::Widgets Qt5::Network Qt5::Test)
    add_test(NAME optical_indication_ui_test COMMAND optical_indication_ui_test)
    set_tests_properties(optical_indication_ui_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(photoelectric_v2_protocol_test tests/photoelectricv2protocol_test.cpp)
    target_include_directories(photoelectric_v2_protocol_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(photoelectric_v2_protocol_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME photoelectric_v2_protocol_test COMMAND photoelectric_v2_protocol_test)
    add_executable(photoelectric_v2_module_test tests/photoelectricv2module_test.cpp
        PhotoelectricV2/module.cpp PhotoelectricV2/module.h Basic/log.cpp
        cusWidgets/cuswindow.cpp cusWidgets/custommessagebox.cpp)
    target_include_directories(photoelectric_v2_module_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(photoelectric_v2_module_test PRIVATE Qt5::Core Qt5::Network Qt5::Widgets Qt5::Test)
    add_test(NAME photoelectric_v2_module_test COMMAND photoelectric_v2_module_test)
    add_executable(window_geometry_test tests/windowgeometry_test.cpp)
    target_include_directories(window_geometry_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(window_geometry_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME window_geometry_test COMMAND window_geometry_test)
    add_executable(diagnostic_log_window_test tests/diagnosticlogwindow_test.cpp)
    target_include_directories(diagnostic_log_window_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(diagnostic_log_window_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME diagnostic_log_window_test COMMAND diagnostic_log_window_test)
    add_executable(error_log_limiter_test tests/errorloglimiter_test.cpp)
    target_include_directories(error_log_limiter_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(error_log_limiter_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME error_log_limiter_test COMMAND error_log_limiter_test)
    add_executable(point_info_validation_test tests/pointinfovalidation_test.cpp)
    target_include_directories(point_info_validation_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(point_info_validation_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME point_info_validation_test COMMAND point_info_validation_test)
    add_executable(track3d_coordinate_test
        tests/track3dcoordinate_test.cpp
    )
    target_include_directories(track3d_coordinate_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(track3d_coordinate_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME track3d_coordinate_test COMMAND track3d_coordinate_test)
    add_executable(electromagnetic_protocol_test
        tests/electromagneticprotocol_test.cpp
        ElectromagneticDevice/deviceprotocol.cpp
    )
    target_include_directories(electromagnetic_protocol_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(electromagnetic_protocol_test PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME electromagnetic_protocol_test COMMAND electromagnetic_protocol_test)
    add_executable(electromagnetic_reporting_test
        tests/electromagneticreporting_test.cpp
        Controller/testtrackgenerator.cpp
        ElectromagneticDevice/devicemodule.cpp
        ElectromagneticDevice/devicetransport.cpp
        ElectromagneticDevice/deviceprotocol.cpp
        Basic/log.cpp
    )
    target_include_directories(electromagnetic_reporting_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(electromagnetic_reporting_test PRIVATE Qt5::Widgets Qt5::Network Qt5::Test)
    add_test(NAME electromagnetic_reporting_test COMMAND electromagnetic_reporting_test)
    add_executable(subsystem_feature_flags_test
        tests/subsystemfeatureflags_test.cpp
        Controller/subsystemnetworkmonitor.cpp
        Basic/log.cpp
    )
    target_include_directories(subsystem_feature_flags_test PRIVATE "${CMAKE_SOURCE_DIR}")
    target_link_libraries(subsystem_feature_flags_test PRIVATE Qt5::Core Qt5::Network Qt5::Test)
    add_test(NAME subsystem_feature_flags_test COMMAND subsystem_feature_flags_test)
endif()

##############################################################################
# 输出目录配置
##############################################################################

# 多配置生成器（如Visual Studio）的输出目录设置
# 分别为Debug和Release配置创建独立的输出目录
set_target_properties(DispCtrl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"    # 可执行文件目录
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"    # 静态库目录
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"    # 动态库目录
)

# 单配置生成器（如Makefile/Ninja）的输出目录设置
if(NOT CMAKE_CONFIGURATION_TYPES)
    # 如果未指定构建类型，默认使用Debug
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif()
    # 设置运行时输出目录
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
endif()

##############################################################################
# 包含目录配置
##############################################################################

# 添加项目根目录到包含路径
# 支持相对路径包含（如#include "Basic/log.h"）
target_include_directories(DispCtrl PRIVATE
    "${CMAKE_SOURCE_DIR}"                      # 项目根目录
)

##############################################################################
# 平台特定配置
##############################################################################

# Linux平台配置 - 运行时库路径设置
if(UNIX AND NOT APPLE)
    set_target_properties(DispCtrl PROPERTIES
        BUILD_WITH_INSTALL_RPATH ON            # 构建时设置RPATH
        INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"  # 相对路径查找共享库
    )
endif()

##############################################################################
# Linux 安装规则（供 package_linux.sh 使用 cmake --install）
##############################################################################

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    # 安装可执行文件
    install(TARGETS DispCtrl RUNTIME DESTINATION bin)

    # 安装配置文件
    install(FILES "${CMAKE_SOURCE_DIR}/config.toml" DESTINATION bin)

    # 安装地图HTML及资源（保持目录结构）
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/htmls/"
            DESTINATION bin
            PATTERN ".git" EXCLUDE
            PATTERN "mapNoL" EXCLUDE PATTERN "map16" EXCLUDE PATTERN "map16S" EXCLUDE
            PATTERN "osm" EXCLUDE PATTERN "CommandControlRecords" EXCLUDE PATTERN "recordings" EXCLUDE
            PATTERN "*.dll" EXCLUDE PATTERN "*.exe" EXCLUDE PATTERN "*.pdb" EXCLUDE
            PATTERN "*.lnk" EXCLUDE PATTERN "data.txt" EXCLUDE PATTERN "disp_ctrl_log_*.txt" EXCLUDE)

    # 安装QSS样式表
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/style")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/style/"
                DESTINATION bin/resources/style
                FILES_MATCHING PATTERN "*.qss")
    endif()

    # 安装图标资源
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icon")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/icon/"
                DESTINATION bin/resources/icon)
    endif()
endif()

##############################################################################
# 构建信息输出
##############################################################################

# 显示构建配置信息（有助于调试编译问题）
message(STATUS "=== DispCtrl 构建配置信息 ===")
message(STATUS "CMake版本: ${CMAKE_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Qt版本: ${Qt5_VERSION}")

# 计算源文件数量
list(LENGTH SRC_FILES SRC_FILES_COUNT)
message(STATUS "源文件数量: ${SRC_FILES_COUNT}")
message(STATUS "二进制输出目录: ${CMAKE_BINARY_DIR}/bin")

##############################################################################
# 配置文件部署
##############################################################################

# 复制配置文件到输出目录（仅在目标不存在时复制，避免覆盖用户修改）
# 使用自定义脚本实现"如果不存在则复制"的逻辑
add_custom_target(copy_config ALL
    # bin目录 - 只在文件不存在时复制
    COMMAND ${CMAKE_COMMAND}
        "-DSRC_FILE=${CMAKE_SOURCE_DIR}/config.toml"
        "-DDST_FILE=${CMAKE_BINARY_DIR}/bin/config.toml"
        -P "${CMAKE_SOURCE_DIR}/scripts/copy_if_not_exists.cmake"
    # Debug目录 - 只在文件不存在时复制
    COMMAND ${CMAKE_COMMAND}
        "-DSRC_FILE=${CMAKE_SOURCE_DIR}/config.toml"
        "-DDST_FILE=${CMAKE_BINARY_DIR}/bin/Debug/config.toml"
        -P "${CMAKE_SOURCE_DIR}/scripts/copy_if_not_exists.cmake"
    # Release目录 - 只在文件不存在时复制
    COMMAND ${CMAKE_COMMAND}
        "-DSRC_FILE=${CMAKE_SOURCE_DIR}/config.toml"
        "-DDST_FILE=${CMAKE_BINARY_DIR}/bin/Release/config.toml"
        -P "${CMAKE_SOURCE_DIR}/scripts/copy_if_not_exists.cmake"
    COMMENT "Copying config.toml to output directories (only if not exists)"
    VERBATIM
)

# 复制OSM道路数据到输出目录
if(EXISTS "${CMAKE_SOURCE_DIR}/osm")
    add_custom_target(copy_osm ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/osm"
            "${CMAKE_BINARY_DIR}/bin/osm"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/osm"
            "${CMAKE_BINARY_DIR}/bin/Debug/osm"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/osm"
            "${CMAKE_BINARY_DIR}/bin/Release/osm"
        COMMENT "Copying OSM road data to output directories"
        VERBATIM
    )
endif()

##############################################################################
# 编译完成后的提示
##############################################################################

# 添加自定义目标来显示编译完成信息
add_custom_target(build_info ALL
    COMMAND ${CMAKE_COMMAND} -E echo "DispCtrl Build Completed!"
    COMMAND ${CMAKE_COMMAND} -E echo "Executable: ${CMAKE_BINARY_DIR}/bin/"
    COMMAND ${CMAKE_COMMAND} -E echo "Config file: ${CMAKE_BINARY_DIR}/bin/config.toml"
    DEPENDS DispCtrl copy_config
    VERBATIM
)
