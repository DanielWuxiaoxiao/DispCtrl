#include <QApplication>
#include <QDir>
#include <QSurfaceFormat>
#include "mainwindow.h"
#include "Basic/bindThread.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/ConfigManager.h"
#include <QLoggingCategory>
#include "Controller/controller.h"

void setupOpenGL() {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
}

void setupFont(QApplication& app) {
    QFont font("Microsoft YaHei", MAIN_FONT_SIZE);
    app.setFont(font);
}

void setupStyle(QApplication& app) {
    QFile file(":/resources/style/darkstyle.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString style = QString::fromUtf8(file.readAll());
        app.setStyleSheet(style);
        file.close();
    }
}
void bindMainThread() {
#ifdef Q_OS_LINUX
    if (bindThreadToCpu(0))
        qDebug() << "Main thread bound to CPU 0.";
    else
        qWarning() << "Failed to bind main thread to CPU 0.";
#endif
}

int main(int argc, char *argv[]) {
    // 設置必要的屬性，必須在 QApplication 實例化之前
    // 啟動高 DPI 縮放，確保在 4K 螢幕上顯示正常
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // 禁用 ANGLE，優先使用系統原生的桌面 OpenGL
    // 這對於 QWebEngineView 的性能和兼容性至關重要
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    // QWebEngineView 依賴於一個獨立的進程。
    // 在某些環境下，如果沒有這行，可能會導致進程啟動失敗，從而崩潰。
    // 雖然它不直接與你的問題相關，但作為一個最佳實踐，加上它會讓程式更健壯。
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);

    CON_INS->init();

    setupFont(app);
    setupOpenGL();
    setupStyle(app);

    qInstallMessageHandler(enhancedLog);

    if (!ConfigManager::instance().load("config.json")) {
        LOG_CRITICAL("Config load failed, exiting...");
        return -1;
    }
    // 记录应用程序启动
    LOG_INFO("Application starting...");
    FramelessMainWindow window;
    // 测试日志输出
    LOG_INFO("Main window created successfully");

    return app.exec();
}
