/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:30
 * @Description: 
 */
#include "log.h"

// ================== 日志函数 ==================
void enhancedLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{    // —— 先做兜底过滤 —— //
    const QByteArray cat = context.category ? QByteArray(context.category) : QByteArray();

    // 屏蔽 Qt 内部的日志
    if (cat.startsWith("qt.")) {
        return;
    }
    // 对 default 做区分：Qt 内部的 default 没有文件行号
    if (cat == "default" && (!context.file || context.line <= 0)) {
        return;
    }

    if (cat.startsWith("qt.webengine") || cat.startsWith("qt.webenginecontext")) {
        return; // 丢弃 WebEngine 的所有日志
    }
    // 常见 Chromium/ANGLE 噪音关键字
    static const char* kSpam[] = {
        "gles2_cmd_decoder.cc",
        "WebGL-",
        "RENDER WARNING",
        "GL ERROR",
        "ANGLE"
    };
    for (auto *p : kSpam) {
        if (msg.contains(QString::fromLatin1(p))) return;
    }


    static QFile logFile;
    static QMutex logMutex;
    static int logFileIndex = 0;   // 当前卷号（0 ~ MAX_LOG_FILE_COUNT-1，循环使用）
    static bool firstTime = true;

    QMutexLocker locker(&logMutex);

    // ——— 辅助：打开指定卷号的日志文件 ———
    // 若目标文件已存在则先删除（循环覆盖旧日志），再以追加模式创建
    auto openLogFile = [&](int idx) -> bool {
        if (logFile.isOpen()) logFile.close();

        QString logPath = QDir::currentPath() + "/" + getLogFileName(idx);
        logFile.setFileName(logPath);

        // 覆盖写：截断旧文件内容
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            // 回退到临时目录
            logPath = QDir::temp().filePath(getLogFileName(idx));
            logFile.setFileName(logPath);
            if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                return false;
        }

        QTextStream out(&logFile);
        out << "\n=== Log Session Started at "
            << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << " (file " << (idx + 1) << "/" << MAX_LOG_FILE_COUNT << ") ===\n"
            << "Log file location: " << logFile.fileName() << "\n" << Qt::endl;
        out.flush();

        fprintf(stderr, "\n========================================\n");
        fprintf(stderr, "Log file: %s  [%d/%d]\n",
                logFile.fileName().toLocal8Bit().constData(),
                idx + 1, MAX_LOG_FILE_COUNT);
        fprintf(stderr, "========================================\n\n");
        fflush(stderr);
        return true;
    };

    // 首次运行时打开第0卷
    if (firstTime) {
        firstTime = false;
        logFileIndex = 0;
        if (!openLogFile(logFileIndex)) {
            fprintf(stderr, "[ERROR] Failed to create log file\n");
            fflush(stderr);
        }
    }

    // 检查是否需要分卷
    if (logFile.isOpen() && logFile.size() >= MAX_LOG_FILE_SIZE) {
        // 循环到下一卷（超出上限则回绕到 0，覆盖最旧的那个）
        logFileIndex = (logFileIndex + 1) % MAX_LOG_FILE_COUNT;
        openLogFile(logFileIndex);
    }

    // 如果文件未打开，尝试重新打开当前卷
    if (!logFile.isOpen()) {
        if (!openLogFile(logFileIndex))
            return;
    }

    QTextStream out(&logFile);
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString level;
    QString colorCode;

    // 设置日志级别和颜色
    switch (type) {
        case QtDebugMsg:
            level = "DEBUG"; colorCode = "\033[0;37m"; break;
        case QtInfoMsg:
            level = "INFO"; colorCode = "\033[0;32m"; break;
        case QtWarningMsg:
            level = "WARNING"; colorCode = "\033[0;33m"; break;
        case QtCriticalMsg:
            level = "CRITICAL"; colorCode = "\033[0;31m"; break;
        case QtFatalMsg:
            level = "FATAL"; colorCode = "\033[1;31m"; break;
    }

    // 提取源码信息
    QString sourceInfo;
    if (context.file && context.line > 0) {
        QFileInfo fileInfo(context.file);
        QString fileName = fileInfo.baseName();
        sourceInfo = QString("%1:%2").arg(fileName).arg(context.line);
    } else {
        sourceInfo = "unknown:0";
    }

    // 提取函数名
    QString functionName = "unknown";
    if (context.function) {
        functionName = QString(context.function);
        int parenIndex = functionName.indexOf('(');
        if (parenIndex > 0) {
            functionName = functionName.left(parenIndex);
        }
        int colonIndex = functionName.lastIndexOf("::");
        if (colonIndex > 0) {
            functionName = functionName.mid(colonIndex + 2);
        }
    }

    // 线程信息
    QString threadInfo = QString::number(reinterpret_cast<quintptr>(QThread::currentThread()), 16);
    threadInfo = threadInfo.right(4).toUpper();

    // 格式化日志消息
    QString logMessage = QString("[%1] %2 [%3] T:%4 %5() - %6")
                        .arg(time)
                        .arg(level.leftJustified(8))
                        .arg(sourceInfo.leftJustified(20))
                        .arg(threadInfo)
                        .arg(functionName.leftJustified(15))
                        .arg(msg);

    // 写入文件（始终写入，不受构建模式影响）
    out << logMessage << Qt::endl;
    out.flush();

    // 控制台彩色输出（仅 Debug 模式）
#ifdef QT_DEBUG
    static QTextStream console(stdout);
    QString consoleMessage = QString("%1%2\033[0m").arg(colorCode).arg(logMessage);
    console << consoleMessage << Qt::endl;
    console.flush();
#else
    // Release模式也输出到控制台（无颜色）
    static QTextStream console(stdout);
    console << logMessage << Qt::endl;
    console.flush();
#endif
}

// 测试日志输出的示例函数
void testLogging() {
    LOG_DEBUG("This is a debug message");
    LOG_INFO("Application initialized successfully");
    LOG_WARNING("This is a warning message");
    LOG_CRITICAL("This is a critical error");

    // 类别日志示例
    LOG_CATEGORY("NETWORK", "Connection established");
    LOG_CATEGORY("RADAR", "Target detected at 125km");
    LOG_CATEGORY("DATABASE", "Query executed in 15ms");
}
