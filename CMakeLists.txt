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
    PolarDisp/sectorwidget.cpp                 # 扇区部件
    PolarDisp/sectorscene.cpp                  # 扇区场景
    PolarDisp/sectorpolargrid.cpp              # 扇区极坐标网格
    PolarDisp/rangeazimuthwidget.cpp           # 距离-方位显示部件
    PolarDisp/rangeazimuthchart.cpp            # 距离-方位图表（直角坐标系）
    PolarDisp/pviewtopleft.cpp                 # 左上角视图
    PolarDisp/mousepositioninfo.cpp            # 鼠标位置信息
    PolarDisp/ppivisualsettings.cpp            # PPI视觉设置
    PolarDisp/tooltip.cpp                      # 工具提示
    PolarDisp/zoomview.cpp                     # 缩放视图
    PolarDisp/ppiview.cpp                      # PPI主视图

    # 点管理模块 - 目标检测和航迹管理
    PointManager/detmanager.cpp                # 检测点管理器
    PointManager/point.cpp                     # 点对象基类
    PointManager/trackmanager.cpp              # 航迹管理器
    PointManager/sectordetmanager.cpp          # 扇区检测管理
    PointManager/sectortrackmanager.cpp        # 扇区航迹管理

    # 控制模块 - 数据流控制和业务逻辑
    Controller/controller.cpp                  # 主控制器
    Controller/disp2datamanager.cpp            # 显示到数据管理
    Controller/data2dispmanager.cpp            # 数据到显示管理
    Controller/tbd2dispmanager.cpp             # TBD数据到显示管理
    Controller/disp2monmanager.cpp             # 显示到监控管理
    Controller/targetdispmanager.cpp           # 目标显示管理
    Controller/sig2dispmanager.cpp             # 信号到显示管理
    Controller/mon2dispmanager.cpp             # 监控到显示管理
        Controller/ExternalCtrlManager.cpp        # 外部雷控/调度链路
    Controller/RadarDataManager.cpp            # 雷达数据统一管理器
    Controller/ErrorHandler.cpp                # 错误处理框架
    Controller/disp2sigmanager.cpp             # 显示到信号管理
    Controller/disp2resmanager.cpp             # 显示到资源管理
    Controller/res2dispmanager.cpp             # 资源到显示管理
    Controller/disp2photomanager.cpp           # 显示到照片管理
    Controller/disp2datamanager.cpp            # 显示到数据管理

    # 主界面模块 - 控制面板和布局
    mainPanel/mainoverlayout.cpp               # 主界面覆盖布局
    mainPanel/azelrangewidget.cpp              # 方位角高低角范围控件
    mainPanel/screenrecorderwidget.cpp         # 屏幕录制与回放
    mainPanel/simpleavi.cpp                    # 纯Qt AVI视频写入器

    # 自定义组件模块 - UI扩展控件
    cusWidgets/detachablewidget.cpp            # 可分离窗口部件
    cusWidgets/cuswindow.cpp                   # 自定义窗口
    cusWidgets/custommessagebox.cpp            # 自定义消息框
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

    # 基础模块 - 系统基础功能
    Basic/log.cpp                              # 日志系统
    Basic/Protocol.cpp                         # 通信协议定义
    Basic/OsmRoadParser.cpp                    # OSM道路数据解析器
)

##############################################################################
# 头文件配置 - 按功能模块组织
##############################################################################

set(HDR_FILES
    # 主界面头文件
    mainwindow.h                               # 主窗口类定义

    # 控制模块头文件 - 数据流和业务逻辑
    Controller/targetdispmanager.h             # 目标显示管理器
    Controller/sig2dispmanager.h               # 信号到显示管理器
    Controller/mon2dispmanager.h               # 监控到显示管理器
    Controller/RadarDataManager.h            # 雷达数据管理器
    Controller/ErrorHandler.h                  # 错误处理框架
    Controller/disp2sigmanager.h               # 显示到信号管理器
    Controller/disp2resmanager.h               # 显示到资源管理器
    Controller/res2dispmanager.h               # 资源到显示管理器
    Controller/disp2photomanager.h             # 显示到照片管理器
    Controller/disp2monmanager.h               # 显示到监控管理器
    Controller/disp2datamanager.h              # 显示到数据管理器
    Controller/data2dispmanager.h              # 数据到显示管理器
    Controller/tbd2dispmanager.h               # TBD数据到显示管理器
    Controller/controller.h                    # 主控制器
    Controller/ExternalCtrlManager.h           # 外部雷控/调度链路

    # 自定义组件头文件 - UI扩展控件
    cusWidgets/detachablewidget.h              # 可分离窗口部件
    cusWidgets/cuswindow.h                     # 自定义窗口
    cusWidgets/custommessagebox.h              # 自定义消息框
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
    Basic/mathUtil.h                           # 数学工具函数
    Basic/log.h                               # 日志系统
    Basic/DispBasci.h                         # 显示基础定义
    Basic/ConfigManager.h                     # 配置管理器
    Basic/authmanager.h                       # 管理者模式认证管理器
    Basic/OsmRoadParser.h                     # OSM道路数据解析器

    # 极坐标显示模块头文件 - PPI雷达显示
    PolarDisp/polargrid.h                     # 极坐标网格
    PolarDisp/polaraxis.h                     # 极坐标轴
    PolarDisp/pointinfow.h                    # 点信息窗口
    PolarDisp/zoomview.h                      # 缩放视图
    PolarDisp/tooltip.h                       # 工具提示
    PolarDisp/scanlayer.h                     # 扫描层
    PolarDisp/pviewtopleft.h                  # 左上角视图
    PolarDisp/mousepositioninfo.h             # 鼠标位置信息
    PolarDisp/ppivisualsettings.h             # PPI视觉设置
    PolarDisp/sectorwidget.h                  # 扇区部件
    PolarDisp/sectorscene.h                   # 扇区场景
    PolarDisp/sectorpolargrid.h               # 扇区极坐标网格
    PolarDisp/rangeazimuthwidget.h            # 距离-方位显示部件
    PolarDisp/rangeazimuthchart.h             # 距离-方位图表（直角坐标系）
    PolarDisp/ppiview.h                       # PPI主视图
    PolarDisp/ppisscene.h                     # PPI场景管理

    # 主界面模块头文件 - 控制面板和布局
    mainPanel/mainoverlayout.h                # 主界面覆盖布局
    mainPanel/azelrangewidget.h               # 方位角高低角范围控件
    mainPanel/screenrecorderwidget.h          # 屏幕录制与回放
    mainPanel/simpleavi.h                     # 纯Qt AVI视频写入器

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
            PATTERN ".git" EXCLUDE)

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
