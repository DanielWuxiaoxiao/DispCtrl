/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:44
 * @Description: 
 */
#ifndef LOG_H
#define LOG_H

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include <QDir>

// ================== 日志配置 ==================
const qint64 MAX_LOG_FILE_SIZE  = 10 * 1024 * 1024; // 10 MB 分卷大小
const int    MAX_LOG_FILE_COUNT = 5;                 // 本地最多保留的日志文件数量
const QString LOG_FILE_BASENAME = "disp_ctrl_log";  // 日志文件基础名
const QString LOG_FILE_SUFFIX   = ".txt";            // 日志后缀

// 预留的环境设置函数 - 可根据需要调用
[[maybe_unused]] static void setEarlyEnv()
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

// 获取日志文件名（按分卷编号，统一格式 disp_ctrl_log_N.txt，N = 0..MAX_LOG_FILE_COUNT-1）
inline QString getLogFileName(int index = 0) {
    return LOG_FILE_BASENAME + "_" + QString::number(index) + LOG_FILE_SUFFIX;
}
// ================== 日志函数 ==================
bool isLogTypeEnabled(QtMsgType type);
void refreshRuntimeLogLevelFromConfig();
void enhancedLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);

// 便捷的日志宏定义（可添加到头文件中）
#ifdef QT_DEBUG
#define LOG_DEBUG(msg) do { if (isLogTypeEnabled(QtDebugMsg)) qDebug() << msg; } while (0)
#else
#define LOG_DEBUG(msg) do { } while (0)
#endif
#define LOG_INFO(msg) do { if (isLogTypeEnabled(QtInfoMsg)) qInfo() << msg; } while (0)
#define LOG_WARNING(msg) do { if (isLogTypeEnabled(QtWarningMsg)) qWarning() << msg; } while (0)
#define LOG_ERROR(msg) do { if (isLogTypeEnabled(QtCriticalMsg)) qCritical() << msg; } while (0)
#define LOG_CRITICAL(msg) do { if (isLogTypeEnabled(QtCriticalMsg)) qCritical() << msg; } while (0)
#define LOG_FATAL(msg) qFatal(msg)

// 带类别的日志宏
#define LOG_CATEGORY(category, msg) do { if (isLogTypeEnabled(QtDebugMsg)) qDebug() << "[" << category << "]" << msg; } while (0)

// 测试日志输出的示例函数
void testLogging();


#endif // LOG_H
