// ErrorHandler.cpp - 统一错误处理框架实现
#include "ErrorHandler.h"
#include "Basic/log.h"
#include <QDebug>
#include <QCoreApplication>
#include <QMessageBox>

// =============================================================================
// LogErrorHandler - 日志错误处理器
// =============================================================================
class LogErrorHandler : public IErrorHandler {
public:
    void handleError(const ErrorInfo& error) override {
        QString logMessage = QString("[%1][%2] %3: %4")
            .arg(QMetaEnum::fromType<ErrorSeverity>().valueToKey(static_cast<int>(error.severity)))
            .arg(QMetaEnum::fromType<ErrorCategory>().valueToKey(static_cast<int>(error.category)))
            .arg(error.code)
            .arg(error.message);
        
        // 添加上下文信息
        if (!error.context.isEmpty()) {
            QStringList contextItems;
            for (auto it = error.context.begin(); it != error.context.end(); ++it) {
                contextItems << QString("%1=%2").arg(it.key(), it.value().toString());
            }
            logMessage += QString(" [%1]").arg(contextItems.join(", "));
        }
        
        switch (error.severity) {
            case ErrorSeverity::Info:
                qInfo() << logMessage;
                break;
            case ErrorSeverity::Warning:
                qWarning() << logMessage;
                break;
            case ErrorSeverity::Error:
                qCritical() << logMessage;
                break;
            case ErrorSeverity::Critical:
            case ErrorSeverity::Fatal:
                qFatal("%s", logMessage.toLocal8Bit().constData());
                break;
        }
    }
};

// =============================================================================
// UIErrorHandler - UI错误处理器
// =============================================================================
class UIErrorHandler : public IErrorHandler {
public:
    void handleError(const ErrorInfo& error) override {
        if (error.severity >= ErrorSeverity::Error) {
            QString title = "错误";
            QString message = QString("%1\n\n%2").arg(error.code, error.message);
            
            QMessageBox::Icon icon = QMessageBox::Warning;
            if (error.severity == ErrorSeverity::Critical || error.severity == ErrorSeverity::Fatal) {
                icon = QMessageBox::Critical;
                title = "严重错误";
            }
            
            // 在主线程中显示消息框
            QMetaObject::invokeMethod(qApp, [=]() {
                QMessageBox messageBox(icon, title, message);
                messageBox.exec();
            }, Qt::QueuedConnection);
        }
    }
};

// =============================================================================
// ErrorHandler 实现
// =============================================================================
ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
{
    // 注册默认处理器
    registerHandler(ErrorCategory::System, new LogErrorHandler());
    registerHandler(ErrorCategory::Network, new LogErrorHandler());
    registerHandler(ErrorCategory::DataProcessing, new LogErrorHandler());
    registerHandler(ErrorCategory::Configuration, new LogErrorHandler());
    registerHandler(ErrorCategory::UI, new UIErrorHandler());
    
    // 连接关键错误信号
    connect(this, &ErrorHandler::criticalErrorOccurred, 
            this, &ErrorHandler::handleCriticalError);
}

ErrorHandler& ErrorHandler::instance()
{
    static ErrorHandler instance;
    return instance;
}

void ErrorHandler::registerHandler(ErrorCategory category, IErrorHandler* handler)
{
    if (m_handlers.contains(category)) {
        delete m_handlers[category];
    }
    m_handlers[category] = handler;
}

void ErrorHandler::reportError(const QString& code, const QString& message, 
                              ErrorSeverity severity, ErrorCategory category,
                              const QMap<QString, QVariant>& context)
{
    ErrorInfo error;
    error.code = code;
    error.message = message;
    error.severity = severity;
    error.category = category;
    error.timestamp = QDateTime::currentDateTime();
    error.context = context;
    
    // 更新统计
    m_errorCounts[category]++;
    
    // 保存到最近错误列表
    m_recentErrors.append(error);
    if (m_recentErrors.size() > MAX_RECENT_ERRORS) {
        m_recentErrors.removeFirst();
    }
    
    // 发送信号
    emit errorReported(error);
    
    if (severity >= ErrorSeverity::Critical) {
        emit criticalErrorOccurred(error);
    }
    
    // 调用相应的处理器
    if (m_handlers.contains(category)) {
        m_handlers[category]->handleError(error);
    }
}

QMap<ErrorCategory, int> ErrorHandler::getErrorStats() const
{
    return m_errorCounts;
}

void ErrorHandler::handleCriticalError(const ErrorInfo& error)
{
    qCritical() << "Critical error occurred:" << error.code << error.message;
    
    // 对于Fatal错误，可能需要优雅关闭应用
    if (error.severity == ErrorSeverity::Fatal) {
        qApp->exit(1);
    }
}