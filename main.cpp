/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:18
 * @Description: 
 */
/**
 * @file main.cpp
 * @brief DispCtrl雷达显示控制系统主程序入口
 * @details 程序初始化流程：
 *          1. Qt应用程序属性配置（高DPI、OpenGL等）
 *          2. 错误处理框架初始化
 *          3. 系统配置加载和验证
 *          4. 用户界面初始化（字体、样式、主窗口）
 *          5. 日志系统和控制器启动
 * @author DispCtrl Team
 * @date 2024
 */

#include <QApplication>
#include <QDir>
#include <QSurfaceFormat>
#include "mainwindow.h"
#include "Basic/bindThread.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/ConfigManager.h"
#include "Controller/ErrorHandler.h"
#include <QLoggingCategory>
#include "Controller/controller.h"

/**
 * @brief 设置OpenGL渲染格式
 * @details 配置OpenGL上下文：
 *          - 使用Core Profile（核心配置文件）
 *          - OpenGL 3.3版本
 *          - 确保图形渲染的兼容性和性能
 */
void setupOpenGL() {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
}

/**
 * @brief 设置应用程序字体
 * @param app Qt应用程序实例
 * @details 配置全局字体：
 *          - 使用微软雅黑字体，确保中文显示效果
 *          - 字体大小使用MAIN_FONT_SIZE常量
 *          - 提供一致的用户界面体验
 */
void setupFont(QApplication& app) {
    QFont font("Microsoft YaHei", MAIN_FONT_SIZE);
    app.setFont(font);
}

/**
 * @brief 设置应用程序样式
 * @param app Qt应用程序实例
 * @details 加载并应用深色主题样式：
 *          - 从资源文件加载QSS样式表
 *          - 提供专业的雷达显示界面外观
 *          - 深色主题有助于减少眼疲劳
 */
void setupStyle(QApplication& app) {
    QFile file(":/resources/style/darkstyle.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString style = QString::fromUtf8(file.readAll());
        app.setStyleSheet(style);
        file.close();
    }
}

/**
 * @brief 绑定主线程到指定CPU核心（Linux平台）
 * @details 性能优化功能：
 *          - 在Linux系统上将主线程绑定到CPU核心0
 *          - 提高实时性和性能稳定性
 *          - 适用于高性能雷达数据处理需求
 */
void bindMainThread() {
#ifdef Q_OS_LINUX
    if (bindThreadToCpu(0))
        qDebug() << "Main thread bound to CPU 0.";
    else
        qWarning() << "Failed to bind main thread to CPU 0.";
#endif
}

/**
 * @brief 程序主入口函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出代码（0表示正常退出）
 * @details 完整的应用程序初始化流程：
 *          1. Qt应用程序属性配置（必须在QApplication创建前）
 *          2. 创建QApplication实例
 *          3. 初始化错误处理框架
 *          4. 控制器系统初始化  
 *          5. UI配置（字体、OpenGL、样式）
 *          6. 日志系统配置
 *          7. 配置文件加载和验证
 *          8. 主窗口创建和显示
 *          9. 进入事件循环
 */
int main(int argc, char *argv[]) {
    // =============================================================================
    // 第一步：Qt应用程序属性配置（必须在QApplication实例化之前）
    // =============================================================================
    
    // 启用高DPI缩放，确保在4K显示器上正常显示
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    // 使用系统原生桌面OpenGL，避免ANGLE渲染器问题
    // 对QWebEngineView的性能和兼容性至关重要
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    
    // 启用OpenGL上下文共享，提高多窗口渲染性能
    // QWebEngineView依赖独立进程，此设置增强稳定性
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    
    // =============================================================================
    // 第二步：创建Qt应用程序实例
    // =============================================================================
    QApplication app(argc, argv);

    // =============================================================================
    // 第三步：初始化错误处理框架
    // =============================================================================
    ErrorHandler& errorHandler = ErrorHandler::instance();
    Q_UNUSED(errorHandler); // 标记为已使用，避免编译器警告
    qInfo() << "Error handler initialized";

    // =============================================================================
    // 第四步：控制器系统初始化
    // =============================================================================
    CON_INS->init();

    // =============================================================================
    // 第五步：用户界面配置
    // =============================================================================
    setupFont(app);      // 设置全局字体
    setupOpenGL();       // 配置OpenGL渲染
    setupStyle(app);     // 应用深色主题样式

    // =============================================================================
    // 第六步：日志系统配置
    // =============================================================================
    qInstallMessageHandler(enhancedLog);

    // =============================================================================
    // 第七步：配置文件加载和验证
    // =============================================================================
    if (!ConfigManager::instance().load("config.toml")) {
        LOG_ERROR("Failed to load config.toml, using default configuration");
        // 不返回错误，继续运行，使用默认配置
    } else {
        LOG_INFO("Configuration loaded successfully from config.toml");
    }
    
    // =============================================================================
    // 第八步：主窗口创建和显示
    // =============================================================================
    LOG_INFO("Application starting...");
    FramelessMainWindow window;
    LOG_INFO("Main window created successfully");

    // =============================================================================
    // 第九步：进入Qt事件循环
    // =============================================================================
    return app.exec();  // 程序主循环，直到用户退出
}
