/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:19
 * @Description: 
 */
/**
 * @file mainwindow.h
 * @brief 雷达显示系统主窗口
 * @details 实现雷达显示控制系统的主窗口界面，集成地图显示和覆盖层控制界面
 * 
 * 功能特性：
 * - 无边框全屏显示窗口
 * - 地图底层显示集成
 * - 透明覆盖层控制界面
 * - 全屏/窗口模式切换
 * - 键盘快捷键支持
 * - 多层界面架构管理
 * 
 * 界面架构：
 * - 底层：Web地图显示（MapProxyWidget）
 * - 上层：透明控制覆盖层（MainOverLayOut）
 * - 支持全屏和最大化模式切换
 * - 响应式界面布局
 * 
 * 交互功能：
 * - ESC键切换全屏/最大化模式
 * - 支持系统级最小化控制
 * - 覆盖层鼠标事件处理
 * - 窗口层级管理
 * 
 * 使用场景：
 * - 雷达操作员工作站主界面
 * - 大屏幕显示系统
 * - 多监视器环境显示
 * - 实时雷达数据监控
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>

// 前向声明
class MapProxyWidget;  ///< Web地图代理组件
class MainOverLayOut;  ///< 主覆盖层布局管理器

/**
 * @class FramelessMainWindow
 * @brief 无边框主窗口类
 * @details 雷达显示系统的主窗口实现，采用无边框设计和多层界面架构
 * 
 * 该类实现了雷达显示系统的主要用户界面：
 * - 采用无边框全屏设计，最大化显示区域
 * - 底层集成Web地图显示，提供地理位置参考
 * - 上层覆盖透明控制界面，包含雷达控制面板
 * - 支持键盘快捷键进行界面模式切换
 * - 响应系统控制器的窗口管理命令
 * 
 * 设计特点：
 * - 分层架构：地图底层 + 控制覆盖层
 * - 全屏优先：充分利用显示空间
 * - 透明覆盖：保持地图可见性的同时提供控制界面
 * - 灵活布局：适应不同屏幕尺寸和分辨率
 * 
 * 键盘交互：
 * - ESC键：在全屏和最大化模式间切换
 * - 支持标准窗口快捷键
 * 
 * @example
 * ```cpp
 * FramelessMainWindow* mainWindow = new FramelessMainWindow();
 * mainWindow->show();  // 自动以全屏模式显示
 * 
 * // 主窗口会自动初始化：
 * // 1. 底层地图显示
 * // 2. 上层控制界面
 * // 3. 键盘事件处理
 * // 4. 系统控制器连接
 * ```
 */
class FramelessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 创建无边框主窗口，初始化界面布局和系统连接
     * 
     * 初始化流程：
     * - 设置窗口属性和图标
     * - 配置全屏显示模式
     * - 初始化底层地图视图
     * - 创建上层控制覆盖层
     * - 连接系统控制器信号
     */
    explicit FramelessMainWindow(QWidget *parent = nullptr);

protected:
    /**
     * @brief 键盘按键事件处理
     * @param event 键盘事件对象
     * @details 处理窗口显示模式切换的键盘快捷键
     * 
     * 支持的快捷键：
     * - ESC键：在全屏和最大化模式间切换
     * - 其他按键：传递给父类处理
     * 
     * 切换逻辑：
     * - 当前全屏模式 -> 切换到最大化窗口模式
     * - 当前窗口模式 -> 切换到全屏模式
     */
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape)
        {
            if (isFullScreen())
            {
                showMaximized();
            }
            else
            {
                showFullScreen();
            }
        }
        else
        {
            QMainWindow::keyPressEvent(event);
        }
    }

private:
    // === 初始化方法 ===
    
    /**
     * @brief 设置中央视图
     * @details 初始化底层地图显示组件
     * 
     * 执行操作：
     * - 创建中央widget容器
     * - 初始化MapProxyWidget地图组件
     * - 设置默认地图显示模式
     * - 配置无边距布局
     */
    void setupCentralView();
    
    /**
     * @brief 设置覆盖层UI
     * @details 初始化上层透明控制界面
     * 
     * 执行操作：
     * - 创建MainOverLayOut覆盖层
     * - 配置透明度和鼠标事件属性
     * - 设置无边框和置顶属性
     * - 调整覆盖层几何位置和层级
     */
    void setupOverlayUI();

    // === 成员变量 ===
    MainOverLayOut *m_overlayWidget; ///< 上层控制覆盖层组件
    MapProxyWidget *m_map;           ///< 底层地图显示组件
};

#endif // MAINWINDOW_H
