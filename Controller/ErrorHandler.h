// ErrorHandler.h - 统一错误处理框架
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QDateTime>
#include <QVariant>
#include <QMetaEnum>
#include <memory>
#include <functional>
#include <limits>

// 在命名空间中声明枚举
namespace ErrorHandling {
    Q_NAMESPACE

    enum class ErrorSeverity {
        Info,
        Warning,
        Error,
        Critical,
        Fatal
    };
    Q_ENUM_NS(ErrorSeverity)

    enum class ErrorCategory {
        Network,
        DataProcessing,
        UI,
        System,
        Configuration
    };
    Q_ENUM_NS(ErrorCategory)
}

using ErrorSeverity = ErrorHandling::ErrorSeverity;
using ErrorCategory = ErrorHandling::ErrorCategory;

struct ErrorInfo {
    QString code;
    QString message;
    ErrorSeverity severity;
    ErrorCategory category;
    QDateTime timestamp;
    QMap<QString, QVariant> context;
};

// 错误处理策略接口
class IErrorHandler {
public:
    virtual ~IErrorHandler() = default;
    virtual void handleError(const ErrorInfo& error) = 0;
};

// 重试策略
class RetryStrategy {
public:
    RetryStrategy(int maxRetries = 3, int baseDelay = 1000)
        : m_maxRetries(maxRetries), m_baseDelayMs(baseDelay) {}
    
    bool shouldRetry(int currentAttempt) const {
        return currentAttempt < m_maxRetries;
    }
    
    int getDelay(int currentAttempt) const {
        // 指数退避
        return m_baseDelayMs * (1 << currentAttempt);
    }
    
private:
    int m_maxRetries;
    int m_baseDelayMs;
};

class ErrorHandler : public QObject {
    Q_OBJECT
    
public:
    static ErrorHandler& instance();
    
    // 注册错误处理器
    void registerHandler(ErrorCategory category, IErrorHandler* handler);
    
    // 报告错误
    void reportError(const QString& code, const QString& message, 
                     ErrorSeverity severity, ErrorCategory category,
                     const QMap<QString, QVariant>& context = {});
    
    // 带重试的操作
    template<typename Func>
    bool executeWithRetry(const QString& operationName, Func operation, 
                         const RetryStrategy& strategy = RetryStrategy()) {
        for (int attempt = 0; attempt <= strategy.shouldRetry(attempt); ++attempt) {
            try {
                if (operation()) {
                    return true;
                }
            } catch (const std::exception& e) {
                reportError("OPERATION_FAILED", 
                          QString("Operation '%1' failed: %2").arg(operationName, e.what()),
                          ErrorSeverity::Error, ErrorCategory::System,
                          {{"attempt", attempt}, {"operation", operationName}});
            }
            
            if (strategy.shouldRetry(attempt + 1)) {
                QTimer::singleShot(strategy.getDelay(attempt), [this]() {
                    // 延迟后继续
                });
            }
        }
        return false;
    }
    
    // 获取错误统计
    QMap<ErrorCategory, int> getErrorStats() const;
    
signals:
    void errorReported(const ErrorInfo& error);
    void criticalErrorOccurred(const ErrorInfo& error);
    
private slots:
    void handleCriticalError(const ErrorInfo& error);
    
private:
    ErrorHandler(QObject* parent = nullptr);
    
    QMap<ErrorCategory, IErrorHandler*> m_handlers;
    QList<ErrorInfo> m_recentErrors;
    QMap<ErrorCategory, int> m_errorCounts;
    
    static const int MAX_RECENT_ERRORS = 1000;
};

// 便捷宏
#define ERROR_HANDLER ErrorHandler::instance()

#define REPORT_ERROR(code, msg, severity, category) \
    ERROR_HANDLER.reportError(code, msg, severity, category, \
        {{"file", __FILE__}, {"line", __LINE__}, {"function", __FUNCTION__}})

#define REPORT_NETWORK_ERROR(msg) \
    REPORT_ERROR("NETWORK_ERROR", msg, ErrorSeverity::Error, ErrorCategory::Network)

#define REPORT_DATA_ERROR(msg) \
    REPORT_ERROR("DATA_ERROR", msg, ErrorSeverity::Error, ErrorCategory::DataProcessing)

#endif // ERRORHANDLER_H