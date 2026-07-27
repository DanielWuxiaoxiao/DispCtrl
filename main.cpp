#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>

#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Controller/controller.h"
#include "mainwindow.h"

namespace {

void setupFont(QApplication& app)
{
    QFont font("Microsoft YaHei", MAIN_FONT_SIZE);
    font.setPointSizeF(MAIN_FONT_SIZE * ScaleHelper::uiScale());
    app.setFont(font);
}

void setupStyle(QApplication& app)
{
    QFile file(":/resources/style/darkstyle.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

} // namespace

int main(int argc, char* argv[])
{
    const bool configLoaded = ConfigManager::instance().load("config.toml");
    const QString dpiPolicy = ConfigManager::instance().uiDpiPolicy("fixed").trimmed().toLower();
    const bool fixedDpi = dpiPolicy != "system";

    if (fixedDpi) {
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
        qputenv("QT_SCALE_FACTOR", "1");
        qputenv("QT_FONT_DPI", "96");
        QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_Use96Dpi);
    } else {
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }

    QApplication app(argc, argv);
    qInstallMessageHandler(enhancedLog);

    ScaleHelper::init(fixedDpi ? QStringLiteral("fixed") : QStringLiteral("system"),
                      ConfigManager::instance().uiScale(1.0));
    setupFont(app);
    setupStyle(app);

    if (!configLoaded) {
        LOG_WARNING("config.toml was not loaded; using ship-radar defaults");
    }

    CON_INS->init();
    LOG_INFO(QString("Ship-radar startup, log file: %1/disp_ctrl_log.txt")
                 .arg(QDir::currentPath()));

    FramelessMainWindow window;
    return app.exec();
}
