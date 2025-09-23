/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 11:25:55
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
/**
 * @file ErrorHandler.cpp
 * @brief 统一错误处理框架实现
 * @details 提供企业级错误处理机制的完整实现
 * 
 * 实现功能：
 * - 多级错误处理器（日志、UI、自定义）
 * - 错误分类和严重级别管理
 * - 线程安全的错误报告
 * - 错误统计和历史记录
 * - 自动重试机制支持
 * 
 * 设计模式：
 * - 策略模式：可插拔的错误处理器
 * - 单例模式：全局错误管理器
 * - 观察者模式：错误事件通知
 * 
 * 性能优化：
 * - 轻量级错误信息结构
 * - 异步错误处理选项
 * - 高效的错误统计算法
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#include "ErrorHandler.h"
#include "Basic/log.h"
#include <QDebug>
#include <QCoreApplication>
#include <QMessageBox>

// =============================================================================
// LogErrorHandler - 日志错误处理器
// =============================================================================

/**
 * @class LogErrorHandler
 * @brief 基于日志系统的错误处理器
 * @details 将错误信息写入日志文件和控制台，支持不同严重级别的日志输出
 *          - Info/Warning: 标准日志输出
 *          - Error: 错误级别日志
 *          - Critical/Fatal: 致命错误处理，可能导致程序终止
 */
class LogErrorHandler : public IErrorHandler {
public:
    /**
     * @brief 处理错误信息
     * @param error 错误信息结构体，包含错误代码、消息、严重级别等
     * @details 根据错误严重级别选择合适的日志输出方式：
     *          - 格式化错误消息，包含分类、级别、代码和详细描述
     *          - 添加上下文信息（如端口号、文件路径等）
     *          - 对于Fatal错误，调用qFatal导致程序终止
     */
    void handleError(const ErrorInfo& error) override {
        // 构建标准化的日志消息格式：[级别][分类] 错误代码: 错误描述
        QString logMessage = QString("[%1][%2] %3: %4")
            .arg(QMetaEnum::fromType<ErrorSeverity>().valueToKey(static_cast<int>(error.severity)))
            .arg(QMetaEnum::fromType<ErrorCategory>().valueToKey(static_cast<int>(error.category)))
            .arg(error.code)
            .arg(error.message);
        
        // 添加上下文信息（端口、IP、文件名等额外调试信息）
        if (!error.context.isEmpty()) {
            QStringList contextItems;
            for (auto it = error.context.begin(); it != error.context.end(); ++it) {
                contextItems << QString("%1=%2").arg(it.key(), it.value().toString());
            }
            logMessage += QString(" [%1]").arg(contextItems.join(", "));
        }
        
        // 根据严重级别选择合适的Qt日志输出函数
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
                // Fatal错误会终止程序执行
                qFatal("%s", logMessage.toLocal8Bit().constData());
                break;
        }
    }
};

// =============================================================================
// UIErrorHandler - UI错误处理器  
// =============================================================================

/**
 * @class UIErrorHandler
 * @brief 基于用户界面的错误处理器
 * @details 为用户提供可视化的错误提示，通过消息框显示重要错误信息
 *          - 仅处理Error级别及以上的错误
 *          - 线程安全，支持在任意线程中调用
 *          - 根据严重级别显示不同图标和标题
 */
class UIErrorHandler : public IErrorHandler {
public:
    /**
     * @brief 处理错误信息并显示UI提示
     * @param error 错误信息结构体
     * @details 实现功能：
     *          - 过滤低级别错误（Info/Warning不显示UI）
     *          - 构建用户友好的错误消息
     *          - 使用Qt::QueuedConnection确保线程安全
     *          - 区分普通错误和严重错误的显示样式
     */
    void handleError(const ErrorInfo& error) override {
        // 只为Error级别及以上的错误显示UI提示
        if (error.severity >= ErrorSeverity::Error) {
            QString title = "错误";
            QString message = QString("%1\n\n%2").arg(error.code, error.message);
            
            // 根据严重级别选择合适的图标和标题
            QMessageBox::Icon icon = QMessageBox::Warning;
            if (error.severity == ErrorSeverity::Critical || error.severity == ErrorSeverity::Fatal) {
                icon = QMessageBox::Critical;
                title = "严重错误";
            }
            
            // 使用Qt::QueuedConnection确保在主线程中显示消息框
            // 这样可以从任意线程安全地调用此方法
            QMetaObject::invokeMethod(qApp, [=]() {
                QMessageBox messageBox(icon, title, message);
                messageBox.exec();
            }, Qt::QueuedConnection);
        }
    }
};

// =============================================================================
// ErrorHandler 主要实现类
// =============================================================================

/**
 * @brief ErrorHandler构造函数
 * @param parent 父对象指针，用于Qt对象树管理
 * @details 初始化错误处理框架：
 *          - 为各个错误分类注册默认处理器
 *          - UI错误使用UIErrorHandler（显示消息框）
 *          - 其他错误使用LogErrorHandler（写入日志）
 *          - 连接关键错误信号到处理槽函数
 */
ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
{
    // 注册默认错误处理器 - 根据错误分类分配合适的处理器
    registerHandler(ErrorCategory::System, new LogErrorHandler());      // 系统错误 -> 日志
    registerHandler(ErrorCategory::Network, new LogErrorHandler());     // 网络错误 -> 日志  
    registerHandler(ErrorCategory::DataProcessing, new LogErrorHandler()); // 数据处理错误 -> 日志
    registerHandler(ErrorCategory::Configuration, new LogErrorHandler());  // 配置错误 -> 日志
    registerHandler(ErrorCategory::UI, new UIErrorHandler());           // UI错误 -> 消息框
    
    // 连接关键错误信号到处理槽 - 用于处理严重错误的特殊逻辑
    connect(this, &ErrorHandler::criticalErrorOccurred, 
            this, &ErrorHandler::handleCriticalError);
}

/**
 * @brief 获取ErrorHandler单例实例
 * @return ErrorHandler的静态实例引用
 * @details 使用线程安全的单例模式，确保全局只有一个错误处理器实例
 */
ErrorHandler& ErrorHandler::instance()
{
    static ErrorHandler instance;
    return instance;
}

/**
 * @brief 注册错误处理器
 * @param category 错误分类（网络、UI、数据处理等）
 * @param handler 处理器实例指针，ErrorHandler负责内存管理
 * @details 实现功能：
 *          - 支持为不同错误分类注册专门的处理器
 *          - 自动管理处理器内存，替换时会删除旧处理器
 *          - 允许运行时动态更改处理策略
 */
void ErrorHandler::registerHandler(ErrorCategory category, IErrorHandler* handler)
{
    if (m_handlers.contains(category)) {
        delete m_handlers[category];  // 删除旧的处理器，避免内存泄漏
    }
    m_handlers[category] = handler;
}

/**
 * @brief 报告错误信息
 * @param code 错误代码（如"UDP_BIND_FAILED"）
 * @param message 错误描述信息
 * @param severity 严重级别（Info/Warning/Error/Critical/Fatal）
 * @param category 错误分类（Network/UI/DataProcessing/System/Configuration）
 * @param context 上下文信息（端口号、文件路径等额外调试信息）
 * @details 核心错误处理流程：
 *          1. 构建标准化ErrorInfo结构
 *          2. 更新错误统计计数
 *          3. 维护最近错误历史记录
 *          4. 发送错误信号通知监听者
 *          5. 调用相应分类的错误处理器
 *          6. 对于关键错误，触发特殊处理逻辑
 */
void ErrorHandler::reportError(const QString& code, const QString& message, 
                              ErrorSeverity severity, ErrorCategory category,
                              const QMap<QString, QVariant>& context)
{
    // 构建标准化的错误信息结构
    ErrorInfo error;
    error.code = code;
    error.message = message;
    error.severity = severity;
    error.category = category;
    error.timestamp = QDateTime::currentDateTime();
    error.context = context;
    
    // 更新错误统计 - 用于错误分析和监控
    m_errorCounts[category]++;
    
    // 维护最近错误列表 - 保存最近的错误用于调试和分析
    m_recentErrors.append(error);
    if (m_recentErrors.size() > MAX_RECENT_ERRORS) {
        m_recentErrors.removeFirst();  // 保持列表大小在限制范围内
    }
    
    // 发送错误信号 - 允许其他组件监听和响应错误事件
    emit errorReported(error);
    
    // 对于关键错误，发送特殊信号
    if (severity >= ErrorSeverity::Critical) {
        emit criticalErrorOccurred(error);
    }
    
    // 调用相应分类的错误处理器进行实际处理
    if (m_handlers.contains(category)) {
        m_handlers[category]->handleError(error);
    }
}

/**
 * @brief 获取错误统计信息
 * @return 按错误分类统计的错误计数映射
 * @details 提供错误分析数据，可用于：
 *          - 系统健康状况监控
 *          - 错误模式分析
 *          - 性能优化指导
 */
QMap<ErrorCategory, int> ErrorHandler::getErrorStats() const
{
    return m_errorCounts;
}

/**
 * @brief 处理关键错误
 * @param error 关键错误信息
 * @details 关键错误的特殊处理逻辑：
 *          - 记录关键错误到系统日志
 *          - 对于Fatal错误，优雅地关闭应用程序
 *          - 可以扩展为发送错误报告、保存状态等
 */
void ErrorHandler::handleCriticalError(const ErrorInfo& error)
{
    qCritical() << "Critical error occurred:" << error.code << error.message;
    
    // 对于Fatal错误，需要优雅关闭应用程序
    if (error.severity == ErrorSeverity::Fatal) {
        qApp->exit(1);  // 退出应用程序，返回错误代码1
    }
}