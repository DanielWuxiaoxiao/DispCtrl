#include "log.h"

namespace {

QString levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "DEBUG";
    case QtInfoMsg: return "INFO";
    case QtWarningMsg: return "WARNING";
    case QtCriticalMsg: return "CRITICAL";
    case QtFatalMsg: return "FATAL";
    }
    return "UNKNOWN";
}

} // namespace

void enhancedLog(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
#ifndef QT_DEBUG
    if (type == QtDebugMsg) {
        return;
    }
#endif

    const QByteArray category = context.category ? QByteArray(context.category) : QByteArray();
    if (category.startsWith("qt.") || (category == "default" && (!context.file || context.line <= 0))) {
        return;
    }

    static QFile logFile;
    static QMutex logMutex;
    static int logFileIndex = 0;
    QMutexLocker locker(&logMutex);

    const auto openLogFile = [&]() -> bool {
        if (logFile.isOpen()) {
            logFile.close();
        }
        logFile.setFileName(QDir::current().filePath(getLogFileName(logFileIndex)));
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            logFile.setFileName(QDir::temp().filePath(getLogFileName(logFileIndex)));
            if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                return false;
            }
        }
        QTextStream stream(&logFile);
        stream << "\n=== Log Session Started at "
               << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " (file " << (logFileIndex + 1) << "/" << MAX_LOG_FILE_COUNT << ") ===\n"
               << "Log file location: " << logFile.fileName() << "\n";
        stream.flush();
        return true;
    };

    if (!logFile.isOpen() && !openLogFile()) {
        return;
    }
    if (logFile.size() >= MAX_LOG_FILE_SIZE) {
        logFileIndex = (logFileIndex + 1) % MAX_LOG_FILE_COUNT;
        if (!openLogFile()) {
            return;
        }
    }

    const QString source = context.file && context.line > 0
        ? QString("%1:%2").arg(QFileInfo(context.file).baseName()).arg(context.line)
        : QStringLiteral("unknown:0");
    QString function = context.function ? QString::fromUtf8(context.function) : QStringLiteral("unknown");
    function = function.left(function.indexOf('('));
    const int scope = function.lastIndexOf("::");
    if (scope >= 0) {
        function = function.mid(scope + 2);
    }

    const QString line = QString("[%1] %2 [%3] T:%4 %5() - %6")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(levelName(type).leftJustified(8))
        .arg(source.leftJustified(20))
        .arg(QString::number(reinterpret_cast<quintptr>(QThread::currentThread()), 16).right(4).toUpper())
        .arg(function.leftJustified(15))
        .arg(msg);

    QTextStream fileStream(&logFile);
    fileStream << line << Qt::endl;
    fileStream.flush();

    QTextStream console(stdout);
    console << line << Qt::endl;
    console.flush();
}

void testLogging()
{
    LOG_INFO("Logging is active");
}
