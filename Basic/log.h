#ifndef LOG_H
#define LOG_H

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <QDebug>

// ================== 日志配置 ==================
const qint64 MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10 MB 分卷大小
const QString LOG_FILE_BASENAME = "disp_ctrl_log"; // 日志文件基础名
const QString LOG_FILE_SUFFIX   = ".txt";          // 日志后缀

static void setEarlyEnv()
{
    // 让 Qt 的日志分类在所有进程里生效（包含 QtWebEngineProcess）
    qputenv("QT_LOGGING_RULES",
            QByteArray(
                "qt.webengine.*=false\n"
                "qt.webenginecontext.*=false\n"
                "qt.qpa.gl=true\n" // 若还想保留自己需要的 GL 类别可改为 true/false
            ));

    // 进一步压低 Chromium 自己的输出（子进程）
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            QByteArray("--disable-logging --log-level=3"));
}

// 获取日志文件名（带分卷编号）
inline QString getLogFileName(int index = -1) {
    if (index < 0) {
        return LOG_FILE_BASENAME + LOG_FILE_SUFFIX;
    } else {
        return LOG_FILE_BASENAME + "_" + QString::number(index) + LOG_FILE_SUFFIX;
    }
}
// ================== 日志函数 ==================
void enhancedLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);

// 便捷的日志宏定义（可添加到头文件中）
#define LOG_DEBUG(msg) qDebug() << msg
#define LOG_INFO(msg) qInfo() << msg
#define LOG_WARNING(msg) qWarning() << msg
#define LOG_CRITICAL(msg) qCritical() << msg
#define LOG_FATAL(msg) qFatal(msg)

// 带类别的日志宏
#define LOG_CATEGORY(category, msg) qDebug() << "[" << category << "]" << msg

// 测试日志输出的示例函数
void testLogging();


#endif // LOG_H
