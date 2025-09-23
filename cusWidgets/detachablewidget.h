/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:18
 * @Description: 
 */
/**
 * @file detachablewidget.h
 * @brief 可分离窗口组件
 * @details 提供可以在嵌入模式和独立窗口模式之间切换的组件容器
 * 
 * 功能特性：
 * - 支持组件分离到独立窗口
 * - 支持独立窗口重新嵌入
 * - 自动状态管理和切换
 * - 统一的用户界面控制
 * - 灵活的布局适配
 * 
 * 应用场景：
 * - 多监视器环境下的窗口管理
 * - 用户自定义界面布局
 * - 可拆分的面板系统
 * - 灵活的工作区域配置
 * 
 * 设计优势：
 * - 提升多屏幕工作效率
 * - 灵活的界面布局管理
 * - 一致的用户交互体验
 * - 简化的窗口状态控制
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class CusWindow;

/**
 * @class DetachableWidget
 * @brief 可分离窗口组件
 * @details 容器组件，可将内部组件在嵌入模式和独立窗口模式之间切换
 * 
 * 该类提供了一个智能的组件容器：
 * - 默认以嵌入模式显示子组件
 * - 通过控制按钮可将子组件分离到独立窗口
 * - 支持从独立窗口重新嵌入到原位置
 * - 自动管理组件的父子关系和布局
 * - 保持组件状态的一致性
 * 
 * 工作流程：
 * 1. 初始状态：子组件嵌入在容器中
 * 2. 分离操作：创建独立窗口，将子组件移动到窗口中
 * 3. 重新嵌入：销毁独立窗口，将子组件移回容器
 * 4. 状态同步：更新控制按钮和布局状态
 * 
 * @example
 * ```cpp
 * // 创建子组件
 * QWidget* contentWidget = new QWidget();
 * // 配置子组件...
 * 
 * // 创建可分离容器
 * DetachableWidget* detachable = new DetachableWidget(
 *     "可分离面板",
 *     contentWidget,
 *     QIcon(":/panel_icon.png"),
 *     this
 * );
 * 
 * // 添加到主界面布局
 * mainLayout->addWidget(detachable);
 * ```
 */
class DetachableWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param name 组件名称，用作窗口标题
     * @param childWidget 要管理的子组件
     * @param icon 窗口图标
     * @param parent 父窗口指针
     * @details 创建可分离组件容器，初始化子组件和控制界面
     */
    explicit DetachableWidget(QString name, QWidget* childWidget, QIcon icon, QWidget* parent = nullptr);

private slots:
    /**
     * @brief 分离组件到独立窗口
     * @details 将子组件从容器中分离，创建独立的浮动窗口显示
     * 
     * 执行操作：
     * - 创建自定义浮动窗口
     * - 将子组件重新父化到浮动窗口
     * - 更新控制按钮状态
     * - 连接窗口关闭信号
     */
    void detach();
    
    /**
     * @brief 重新嵌入组件
     * @details 将子组件从独立窗口重新嵌入到容器中
     * 
     * 执行操作：
     * - 将子组件重新父化到容器
     * - 销毁浮动窗口
     * - 恢复容器布局
     * - 更新控制按钮状态
     */
    void reattach();

private:
    QWidget* m_child;               ///< 被管理的子组件内容
    QPushButton* m_btn;             ///< 分离/嵌入控制按钮
    QVBoxLayout* m_layout;          ///< 容器布局管理器
    QString m_name;                 ///< 组件名称（窗口标题）
    CusWindow* m_floatWindow = nullptr; ///< 浮动窗口指针
    QIcon m_icon;                   ///< 窗口图标
    QLabel* m_titleLabel;
};
