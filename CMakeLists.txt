cmake_minimum_required(VERSION 3.16)
project(DispCtrl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Use Qt5 only
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Network WebEngineWidgets WebChannel)
set(QT_LIBS Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Network Qt5::WebEngineWidgets Qt5::WebChannel)

# Source collection (adjust if you prefer explicit lists)
# Explicit source/header list to avoid automoc issues and ensure stable builds.
set(SRC_FILES
    main.cpp
    mainwindow.cpp
    UDP/threadudpsocket.cpp
    mapDisp/mapprox.cpp
    PolarDisp/pointinfow.cpp
    PolarDisp/polargrid.cpp
    PolarDisp/polaraxis.cpp
    PointManager/detmanager.cpp
    PointManager/point.cpp
    Controller/controller.cpp
    mainPanel/mainoverlayout.cpp
    cusWidgets/detachablewidget.cpp
    Basic/log.cpp
    PointManager/trackmanager.cpp
    Controller/disp2datamanager.cpp
    Controller/data2dispmanager.cpp
    Controller/disp2monmanager.cpp
    cusWidgets/cuswindow.cpp
    PolarDisp/ppisscene.cpp
    cusWidgets/custommessagebox.cpp
    mainPanel/azelrangewidget.cpp
    Controller/targetdispmanager.cpp
    PolarDisp/scanlayer.cpp
    PolarDisp/sectorwidget.cpp
    PolarDisp/sectorscene.cpp
    PolarDisp/sectorpolargrid.cpp
    PointManager/sectordetmanager.cpp
    PointManager/sectortrackmanager.cpp
    Controller/sig2dispmanager.cpp
    Controller/mon2dispmanager.cpp
    Controller/RadarDataManager.cpp
    PolarDisp/pviewtopleft.cpp
    PolarDisp/tooltip.cpp
    Controller/disp2sigmanager.cpp
    Basic/Protocol.cpp
    PolarDisp/zoomview.cpp
    PolarDisp/ppiview.cpp
    Controller/disp2resmanager.cpp
    Controller/disp2photomanager.cpp
    Controller/disp2datamanager.cpp
)

set(HDR_FILES
    mainwindow.h
    cusWidgets/detachablewidget.h
    Controller/targetdispmanager.h
    Controller/sig2dispmanager.h
    Controller/mon2dispmanager.h
    Controller/CentralDataManager.h
    Controller/disp2sigmanager.h
    Controller/disp2resmanager.h
    Controller/disp2photomanager.h
    Controller/disp2monmanager.h
    cusWidgets/cuswindow.h
    Controller/disp2datamanager.h
    Basic/bindThread.h
    PolarDisp/polargrid.h
    PolarDisp/polaraxis.h
    mainPanel/mainoverlayout.h
    PolarDisp/pointinfow.h
    mainPanel/azelrangewidget.h
    Basic/Protocol.h
    Basic/mathUtil.h
    Basic/log.h
    Basic/DispBasci.h
    Basic/ConfigManager.h
    PolarDisp/zoomview.h
    PolarDisp/tooltip.h
    PolarDisp/scanlayer.h
    PolarDisp/pviewtopleft.h
    PolarDisp/sectorwidget.h
    PolarDisp/sectorscene.h
    PolarDisp/sectorpolargrid.h
    PointManager/sectordetmanager.h
    PointManager/sectortrackmanager.h
    PolarDisp/ppiview.h
    PolarDisp/ppisscene.h
    cusWidgets/custommessagebox.h
    Controller/data2dispmanager.h
    Controller/controller.h
    PointManager/detmanager.h
    mapDisp/mapprox.h
    PointManager/trackmanager.h
    PointManager/point.h
    UDP/threadudpsocket.h
)

set(UI_FILES
    PolarDisp/pointinfow.ui
    PolarDisp/pviewtopleft.ui
    mainPanel/mainoverlayout.ui
)

set(QRC_FILES
    resource.qrc
)

# Define target without suffix
add_executable(DispCtrl
    ${SRC_FILES}
    ${HDR_FILES}
    ${UI_FILES}
    ${QRC_FILES}
)

target_link_libraries(DispCtrl PRIVATE ${QT_LIBS})

# MSVC: ensure source/exec charset UTF-8 (mirror your .pro flags)
if(MSVC)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("/source-charset:utf-8" COMPILER_SUPPORTS_SOURCE_CHARSET)
    if(COMPILER_SUPPORTS_SOURCE_CHARSET)
        target_compile_options(DispCtrl PRIVATE "/source-charset:utf-8" "/execution-charset:utf-8")
    else()
        check_cxx_compiler_flag("/utf-8" COMPILER_SUPPORTS_UTF8)
        if(COMPILER_SUPPORTS_UTF8)
            target_compile_options(DispCtrl PRIVATE "/utf-8")
        endif()
    endif()
endif()

# Output directories: separate per-config
set_target_properties(DispCtrl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

# For single-config generators (Makefile/Ninja) use CMAKE_BUILD_TYPE
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
endif()

# Recommended include dirs (optional)
target_include_directories(DispCtrl PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

# Linux: common flags (tweak as needed)
if(UNIX AND NOT APPLE)
    set_target_properties(DispCtrl PROPERTIES
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()
