#ifndef LOG_H
#define LOG_H

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QTextStream>
#include <QThread>

inline constexpr qint64 MAX_LOG_FILE_SIZE = 10 * 1024 * 1024;
inline constexpr int MAX_LOG_FILE_COUNT = 5;
inline const QString LOG_FILE_BASENAME = "disp_ctrl_log";
inline const QString LOG_FILE_SUFFIX = ".txt";

inline QString getLogFileName(int index = 0)
{
    return LOG_FILE_BASENAME + "_" + QString::number(index) + LOG_FILE_SUFFIX;
}

void enhancedLog(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void testLogging();

#define LOG_DEBUG(msg) qDebug() << msg
#define LOG_INFO(msg) qInfo() << msg
#define LOG_WARNING(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
#define LOG_CRITICAL(msg) qCritical() << msg
#define LOG_FATAL(msg) qFatal(msg)

#endif // LOG_H
