/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:17
 * @Description: 
 */
/**
 * @file custommessagebox.h
 * @brief 自定义消息对话框组件
 * @details 提供统一风格的消息提示对话框，支持多种消息类型和按钮组合
 * 
 * 功能特性：
 * - 多种消息类型：信息、警告、确认
 * - 多种按钮组合：确定、确定/取消、是/否
 * - 自定义UI风格和主题
 * - 静态便捷方法支持
 * - 响应式布局设计
 * 
 * 设计优势：
 * - 替代系统默认消息框
 * - 统一应用程序UI风格
 * - 更好的国际化支持
 * - 灵活的自定义扩展
 * 
 * 使用场景：
 * - 用户操作确认
 * - 错误信息提示
 * - 警告消息显示
 * - 程序状态通知
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @class CustomMessageBox
 * @brief 自定义消息对话框组件
 * @details 基于QDialog的自定义消息提示框，提供统一的UI风格和交互体验
 * 
 * 该类提供了完整的消息对话框解决方案：
 * - 支持多种预定义的消息类型（信息、警告、确认）
 * - 提供不同的按钮组合（确定、确定/取消、是/否）
 * - 统一的视觉风格和自定义样式
 * - 便捷的静态方法接口
 * - 响应式布局适配
 * 
 * 相比系统默认QMessageBox的优势：
 * - 完全自定义的外观和样式
 * - 更好的主题适配能力
 * - 统一的应用程序风格
 * - 灵活的扩展和定制
 * 
 * @example
 * ```cpp
 * // 显示信息对话框
 * CustomMessageBox::showInfo(this, "提示", "操作成功完成");
 * 
 * // 显示警告对话框
 * CustomMessageBox::showWarning(this, "警告", "配置参数无效");
 * 
 * // 显示确认对话框
 * if (CustomMessageBox::showConfirm(this, "确认", "确定要删除该项目吗？")) {
 *     // 用户点击确定
 *     deleteItem();
 * }
 * ```
 */
class CustomMessageBox : public QDialog
{
    Q_OBJECT

public:
    /**
     * @enum MessageType
     * @brief 消息类型枚举
     * @details 定义消息对话框的不同类型，影响图标和样式
     */
    enum MessageType {
        Info,     ///< 信息消息类型
        Warning,  ///< 警告消息类型
        Confirm   ///< 确认消息类型
    };

    /**
     * @enum ButtonType
     * @brief 按钮类型枚举
     * @details 定义消息对话框的按钮组合类型
     */
    enum ButtonType {
        Ok,       ///< 仅"确定"按钮
        OkCancel, ///< "确定"和"取消"按钮
        YesNo     ///< "是"和"否"按钮
    };

    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 创建自定义消息对话框实例，初始化UI组件
     */
    explicit CustomMessageBox(QWidget *parent = nullptr);

    // === 静态便捷方法 ===
    
    /**
     * @brief 显示确认对话框
     * @param parent 父窗口指针
     * @param title 对话框标题
     * @param text 消息内容
     * @return true=用户确认，false=用户取消
     * @details 显示带有"确定"和"取消"按钮的确认对话框
     */
    static bool showConfirm(QWidget *parent, const QString &title, const QString &text);
    
    /**
     * @brief 显示信息对话框
     * @param parent 父窗口指针
     * @param title 对话框标题
     * @param text 消息内容
     * @details 显示带有"确定"按钮的信息提示对话框
     */
    static void showInfo(QWidget *parent, const QString &title, const QString &text);
    
    /**
     * @brief 显示警告对话框
     * @param parent 父窗口指针
     * @param title 对话框标题
     * @param text 警告内容
     * @details 显示带有"确定"按钮的警告提示对话框
     */
    static void showWarning(QWidget *parent, const QString &title, const QString &text);

private:
    QLabel *m_titleLabel;         ///< 标题显示标签
    QLabel *m_textLabel;          ///< 消息内容显示标签
    QHBoxLayout *m_buttonLayout;  ///< 按钮布局管理器

    /**
     * @brief 设置UI界面
     * @details 创建和配置对话框的UI组件布局
     */
    void setupUI();
    
    /**
     * @brief 应用样式
     * @details 为对话框组件应用自定义样式和主题
     */
    void applyStyle();
    
    /**
     * @brief 显示对话框
     * @param title 对话框标题
     * @param text 消息内容
     * @param type 消息类型
     * @param buttons 按钮类型
     * @return 用户点击的按钮结果代码
     * @details 核心对话框显示方法，处理所有类型的消息框
     */
    int showDialog(const QString &title, const QString &text, MessageType type, ButtonType buttons);
};

#endif // CUSTOMMESSAGEBOX_H
