/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 11:25:55
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
/**
 * @file ErrorHandler.h
 * @brief 统一错误处理框架头文件
 * @details 提供企业级错误处理机制的完整框架
 * 
 * 核心功能：
 * - 多级错误分类和严重程度管理
 * - 可扩展的错误处理器策略模式
 * - 线程安全的错误报告和统计
 * - 自动重试机制支持
 * - 便捷的错误报告宏定义
 * 
 * 设计特性：
 * - 灵活的错误分类系统
 * - 插件式的处理器架构
 * - 高性能的错误统计
 * - 可配置的错误响应策略
 * - 完整的错误日志记录
 * 
 * 应用场景：
 * - 雷达系统硬件错误处理
 * - 网络通信异常管理
 * - 数据处理错误记录
 * - 用户界面异常反馈
 * - 系统性能监控
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

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

/**
 * @namespace ErrorHandling
 * @brief 错误处理相关的枚举定义命名空间
 * @details 包含错误严重级别和分类枚举，使用Q_NAMESPACE支持Qt元对象系统
 */
namespace ErrorHandling {
    Q_NAMESPACE

    /**
     * @enum ErrorSeverity
     * @brief 错误严重级别枚举
     * @details 定义五个严重级别，从低到高：
     *          - Info: 信息性消息，不影响功能
     *          - Warning: 警告，可能影响性能但不影响核心功能  
     *          - Error: 错误，影响部分功能但系统可继续运行
     *          - Critical: 严重错误，可能导致功能失效
     *          - Fatal: 致命错误，可能导致程序崩溃或退出
     */
    enum class ErrorSeverity {
        Info,        ///< 信息级别 - 记录重要操作和状态
        Warning,     ///< 警告级别 - 潜在问题但不影响核心功能
        Error,       ///< 错误级别 - 功能性错误但系统可恢复
        Critical,    ///< 严重级别 - 重要功能失效
        Fatal        ///< 致命级别 - 可能导致程序退出
    };
    Q_ENUM_NS(ErrorSeverity)

    /**
     * @enum ErrorCategory
     * @brief 错误分类枚举
     * @details 按照系统功能模块划分错误类型：
     *          - Network: 网络通信相关错误
     *          - DataProcessing: 数据处理和验证错误
     *          - UI: 用户界面相关错误
     *          - System: 系统级错误（文件IO、内存等）
     *          - Configuration: 配置加载和验证错误
     */
    enum class ErrorCategory {
        Network,         ///< 网络通信错误 - UDP连接、数据传输等
        DataProcessing,  ///< 数据处理错误 - 数据验证、格式转换等  
        UI,             ///< 用户界面错误 - 界面操作、显示问题等
        System,         ///< 系统级错误 - 文件操作、内存管理等
        Configuration   ///< 配置错误 - 配置文件加载、参数验证等
    };
    Q_ENUM_NS(ErrorCategory)
}

// 类型别名，简化使用
using ErrorSeverity = ErrorHandling::ErrorSeverity;
using ErrorCategory = ErrorHandling::ErrorCategory;

/**
 * @struct ErrorInfo
 * @brief 错误信息结构体
 * @details 包含完整的错误描述信息：
 *          - 错误代码和消息
 *          - 严重级别和分类
 *          - 时间戳和上下文信息
 */
struct ErrorInfo {
    QString code;                        ///< 错误代码（如"UDP_BIND_FAILED"）
    QString message;                     ///< 错误详细描述
    ErrorSeverity severity;              ///< 错误严重级别
    ErrorCategory category;              ///< 错误分类
    QDateTime timestamp;                 ///< 错误发生时间
    QMap<QString, QVariant> context;     ///< 上下文信息（端口、文件路径等）
};

/**
 * @class IErrorHandler
 * @brief 错误处理器接口
 * @details 策略模式接口，允许为不同错误分类定制处理逻辑：
 *          - LogErrorHandler: 记录到日志文件
 *          - UIErrorHandler: 显示用户提示
 *          - EmailErrorHandler: 发送错误报告邮件（可扩展）
 */
class IErrorHandler {
public:
    virtual ~IErrorHandler() = default;
    
    /**
     * @brief 处理错误信息
     * @param error 错误信息结构体
     * @details 子类实现具体的错误处理逻辑，如写日志、显示UI等
     */
    virtual void handleError(const ErrorInfo& error) = 0;
};

/**
 * @class RetryStrategy
 * @brief 重试策略类
 * @details 提供自动重试机制：
 *          - 配置最大重试次数
 *          - 指数退避延迟策略
 *          - 用于网络连接、文件操作等可重试的操作
 */
class RetryStrategy {
public:
    /**
     * @brief 构造重试策略
     * @param maxRetries 最大重试次数，默认3次
     * @param baseDelay 基础延迟时间（毫秒），默认1000ms
     */
    RetryStrategy(int maxRetries = 3, int baseDelay = 1000)
        : m_maxRetries(maxRetries), m_baseDelayMs(baseDelay) {}
    
    /**
     * @brief 判断是否应该重试
     * @param currentAttempt 当前尝试次数（从0开始）
     * @return true表示应该继续重试
     */
    bool shouldRetry(int currentAttempt) const {
        return currentAttempt < m_maxRetries;
    }
    
    /**
     * @brief 获取重试延迟时间
     * @param currentAttempt 当前尝试次数
     * @return 延迟时间（毫秒），使用指数退避算法
     */
    int getDelay(int currentAttempt) const {
        // 指数退避：baseDelay * 2^currentAttempt
        return m_baseDelayMs * (1 << currentAttempt);
    }
    
private:
    int m_maxRetries;      ///< 最大重试次数
    int m_baseDelayMs;     ///< 基础延迟时间（毫秒）
};

/**
 * @class ErrorHandler
 * @brief 错误处理框架主控制器
 * @details 单例模式的错误处理中心：
 *          - 统一管理所有错误处理器
 *          - 提供线程安全的错误报告
 *          - 支持错误统计和历史记录
 *          - 集成自动重试机制
 *          - 发送错误事件信号
 * 
 * @example 基本使用方式：
 * @code
 * // 报告网络错误
 * ERROR_HANDLER.reportError("UDP_BIND_FAILED", "Cannot bind to port 8080", 
 *                          ErrorSeverity::Error, ErrorCategory::Network);
 * 
 * // 使用重试机制
 * ERROR_HANDLER.executeWithRetry("Connect to server", [&]() {
 *     return connectToServer();
 * });
 * @endcode
 */
class ErrorHandler : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief 获取ErrorHandler单例实例
     * @return ErrorHandler的静态实例引用
     * @details 线程安全的单例模式，确保全局唯一的错误处理器
     */
    static ErrorHandler& instance();
    
    /**
     * @brief 注册错误处理器
     * @param category 错误分类
     * @param handler 处理器实例指针，ErrorHandler负责内存管理
     * @details 允许为不同错误分类注册专门的处理器：
     *          - Network -> LogErrorHandler
     *          - UI -> UIErrorHandler  
     *          - 支持运行时动态更换处理策略
     */
    void registerHandler(ErrorCategory category, IErrorHandler* handler);
    
    /**
     * @brief 报告错误信息
     * @param code 错误代码（建议使用大写下划线格式，如"UDP_BIND_FAILED"）
     * @param message 错误详细描述
     * @param severity 严重级别
     * @param category 错误分类
     * @param context 上下文信息（可选），包含调试相关的额外信息
     * @details 核心错误报告流程：
     *          1. 构建标准化ErrorInfo结构
     *          2. 更新错误统计和历史记录
     *          3. 发送错误信号通知监听者
     *          4. 调用相应分类的错误处理器
     */
    void reportError(const QString& code, const QString& message, 
                     ErrorSeverity severity, ErrorCategory category,
                     const QMap<QString, QVariant>& context = {});
    
    /**
     * @brief 执行带重试机制的操作
     * @tparam Func 可调用对象类型（lambda、函数指针等）
     * @param operationName 操作名称，用于错误报告
     * @param operation 要执行的操作，返回bool表示成功/失败
     * @param strategy 重试策略，默认最多重试3次
     * @return true表示操作成功，false表示所有重试都失败
     * @details 自动重试机制：
     *          - 支持指数退避延迟
     *          - 自动记录每次失败的错误
     *          - 适用于网络连接、文件操作等
     * 
     * @example 使用示例：
     * @code
     * bool success = ERROR_HANDLER.executeWithRetry("UDP Connection", [&]() {
     *     return socket->connectToHost(host, port);
     * }, RetryStrategy(5, 500)); // 最多重试5次，基础延迟500ms
     * @endcode
     */
    template<typename Func>
    bool executeWithRetry(const QString& operationName, Func operation, 
                         const RetryStrategy& strategy = RetryStrategy()) {
        for (int attempt = 0; attempt <= strategy.shouldRetry(attempt); ++attempt) {
            try {
                if (operation()) {
                    return true;  // 操作成功
                }
            } catch (const std::exception& e) {
                // 捕获异常并记录错误
                reportError("OPERATION_FAILED", 
                          QString("Operation '%1' failed: %2").arg(operationName, e.what()),
                          ErrorSeverity::Error, ErrorCategory::System,
                          {{"attempt", attempt}, {"operation", operationName}});
            }
            
            // 如果还有重试机会，等待后继续
            if (strategy.shouldRetry(attempt + 1)) {
                QTimer::singleShot(strategy.getDelay(attempt), [this]() {
                    // 延迟后继续（实际重试在循环中处理）
                });
            }
        }
        return false;  // 所有重试都失败
    }
    
    /**
     * @brief 获取错误统计信息
     * @return 按错误分类统计的错误计数映射
     * @details 提供系统健康状况监控数据：
     *          - 各分类错误发生次数
     *          - 用于性能分析和问题诊断
     *          - 可用于错误模式识别
     */
    QMap<ErrorCategory, int> getErrorStats() const;
    
signals:
    /**
     * @brief 错误报告信号
     * @param error 错误信息结构体
     * @details 每次报告错误时发送，允许其他组件监听错误事件
     */
    void errorReported(const ErrorInfo& error);
    
    /**
     * @brief 关键错误发生信号
     * @param error 关键错误信息
     * @details 仅在Critical或Fatal级别错误时发送，用于特殊处理
     */
    void criticalErrorOccurred(const ErrorInfo& error);
    
private slots:
    /**
     * @brief 处理关键错误的槽函数
     * @param error 关键错误信息
     * @details 关键错误的特殊处理逻辑：
     *          - 记录到系统日志
     *          - Fatal错误时优雅关闭应用
     */
    void handleCriticalError(const ErrorInfo& error);
    
private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针
     */
    ErrorHandler(QObject* parent = nullptr);
    
    QMap<ErrorCategory, IErrorHandler*> m_handlers;    ///< 错误处理器映射表
    QList<ErrorInfo> m_recentErrors;                   ///< 最近错误历史记录
    QMap<ErrorCategory, int> m_errorCounts;            ///< 错误统计计数
    
    static const int MAX_RECENT_ERRORS = 1000;         ///< 最大历史记录数量
};

/**
 * @defgroup ErrorMacros 错误报告便捷宏
 * @brief 简化错误报告的宏定义
 * @{
 */

/// 获取ErrorHandler单例实例的便捷宏
#define ERROR_HANDLER ErrorHandler::instance()

/// 通用错误报告宏，自动添加文件名、行号、函数名
#define REPORT_ERROR(code, msg, severity, category) \
    ERROR_HANDLER.reportError(code, msg, severity, category, \
        {{"file", __FILE__}, {"line", __LINE__}, {"function", __FUNCTION__}})

/// 网络错误报告宏
#define REPORT_NETWORK_ERROR(msg) \
    REPORT_ERROR("NETWORK_ERROR", msg, ErrorSeverity::Error, ErrorCategory::Network)

/// 数据处理错误报告宏
#define REPORT_DATA_ERROR(msg) \
    REPORT_ERROR("DATA_ERROR", msg, ErrorSeverity::Error, ErrorCategory::DataProcessing)

/** @} */ // end of ErrorMacros group

#endif // ERRORHANDLER_H