/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:19
 * @Description: 
 */
/**
 * @file mainwindow.cpp
 * @brief 雷达显示系统主窗口实现
 * @details 实现无边框主窗口的界面初始化、布局管理和事件处理功能
 * 
 * 实现特性：
 * - 无边框全屏窗口创建
 * - 分层界面架构实现
 * - 地图底层和控制覆盖层集成
 * - 系统控制器信号连接
 * - 响应式布局管理
 * 
 * 界面层次结构：
 * 1. 底层：QMainWindow主窗口
 * 2. 中央组件：MapProxyWidget地图显示
 * 3. 覆盖层：MainOverLayOut透明控制界面
 * 4. 顶层：鼠标和键盘事件处理
 * 
 * 依赖组件：
 * - MapProxyWidget：Web地图集成显示
 * - MainOverLayOut：雷达控制界面布局
 * - Controller：系统控制器管理
 * - DispBasci：应用程序基础配置
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#include "mainwindow.h"
#include "Basic/DispBasci.h"
#include <QVBoxLayout>
#include "mapDisp/mapprox.h"
#include "mainPanel/mainoverlayout.h"
#include "Controller/controller.h"
#include "PolarDisp/ppiview.h"

/**
 * @brief FramelessMainWindow构造函数实现
 * @param parent 父窗口指针
 * @details 创建和初始化雷达显示系统的主窗口界面
 * 
 * 初始化步骤：
 * 1. 设置窗口基本属性（标题、图标）
 * 2. 启用全屏显示模式
 * 3. 初始化底层地图中央视图
 * 4. 创建上层透明控制覆盖层
 * 5. 连接系统控制器的窗口管理信号
 * 
 * 窗口特性：
 * - 无边框设计，最大化显示区域
 * - 全屏优先模式，适合大屏幕显示
 * - 支持ESC键切换显示模式
 * - 响应系统最小化命令
 */
FramelessMainWindow::FramelessMainWindow(QWidget *parent) : QMainWindow(parent)
{
    // === 窗口基本属性设置 ===
    // 设置应用程序窗口标题（从DispBasci.h配置文件读取）
    setWindowTitle(APP_NAME);
    
    // 设置窗口图标为雷达阵列图标，提升专业性
    setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    
    // === 显示模式配置 ===
    // 启用全屏显示模式，最大化利用屏幕空间
    // 适合雷达操作员工作站的专业显示需求
    showFullScreen();
    
    // === 界面组件初始化 ===
    // 初始化底层地图中央视图
    setupCentralView();
    
    // 初始化上层透明控制覆盖层
    setupOverlayUI();
    
    // === 系统控制器连接 ===
    // 连接控制器的最小化窗口信号到主窗口的最小化槽
    // 支持通过系统控制器远程控制窗口状态
    connect(CON_INS, &Controller::minimizeWindow, this, &QMainWindow::showMinimized);
}

/**
 * @brief 设置中央视图组件
 * @details 初始化主窗口的底层地图显示组件
 * 
 * 实现步骤：
 * 1. 创建中央容器widget
 * 2. 设置无边距垂直布局
 * 3. 创建MapProxyWidget地图组件
 * 4. 将地图视图添加到布局中
 * 5. 设置默认地图显示模式
 * 6. 将容器设置为主窗口中央组件
 * 
 * 设计考虑：
 * - 无边距布局：充分利用显示空间
 * - 地图作为背景：提供地理位置参考
 * - 默认地图模式：确保启动时有可用的地图显示
 */
void FramelessMainWindow::setupCentralView()
{
    // 创建中央容器widget，整个底层widget承载地图显示
    QWidget *central = new QWidget(this);
    
    // 创建垂直布局管理器，无边距设计
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);  // 移除所有边距，最大化显示区域
    
    // 创建Web地图代理组件实例
    m_map = new MapProxyWidget();
    
    // 将地图视图添加到布局中
    layout->addWidget(m_map->getView());
    
    // 设置默认地图显示模式（模式1）
    m_map->chooseMap(1);
    
    // 将配置好的中央组件设置为主窗口的中央widget
    setCentralWidget(central);
}

/**
 * @brief 设置覆盖层用户界面
 * @details 创建和配置透明的控制界面覆盖层
 * 
 * 覆盖层特性：
 * - 透明背景：保持底层地图可见
 * - 置顶显示：确保控制界面始终可访问
 * - 无边框设计：与主窗口风格一致
 * - 全窗口覆盖：响应式布局适配
 * - 鼠标事件透传控制：精确的交互管理
 * 
 * 实现细节：
 * 1. 创建MainOverLayOut覆盖层实例
 * 2. 配置透明度和鼠标事件属性
 * 3. 设置窗口标志（无边框、置顶）
 * 4. 调整几何位置覆盖整个主窗口
 * 5. 提升层级并显示
 */
// ====== 悬浮控件层 ======
void FramelessMainWindow::setupOverlayUI()
{
    // 创建主覆盖层布局管理器实例
    m_overlayWidget = new MainOverLayOut(this);
    
    // === 透明度和背景设置 ===
    // 禁用半透明背景：保持控制面板的清晰可见性
    m_overlayWidget->setAttribute(Qt::WA_TranslucentBackground, false);
    
    // 禁用鼠标事件透传：确保控制面板可正常接收鼠标交互
    m_overlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    // === 窗口行为设置 ===
    // 设置无边框和置顶标志：
    // - FramelessWindowHint: 移除窗口边框和标题栏
    // - WindowStaysOnTopHint: 确保覆盖层始终在最上层显示
    m_overlayWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    // === 几何位置设置 ===
    // 设置覆盖层几何位置，完全覆盖主窗口区域
    m_overlayWidget->setGeometry(0, 0, width(), height());
    
    // === 层级管理 ===
    // 提升覆盖层到最前端，确保在所有其他组件之上
    m_overlayWidget->raise();
    
    // 显示覆盖层
    m_overlayWidget->show();
    
    // === 信号连接 ===
    // 连接PPI视图的地图类型变化信号到地图组件的地图选择方法
    // 使PPIVisualSettings组件能够控制背景地图的显示类型
    connect(m_overlayWidget->getPPIView(), &PPIView::mapTypeChanged,
            m_map, &MapProxyWidget::chooseMap);
    
    // 连接PPI视图的雷达中心变化信号到地图组件的同步方法
    // 实现雷达位置/范围变化时自动同步地图显示范围
    connect(m_overlayWidget->getPPIView(), &PPIView::radarCenterChanged,
            m_map, &MapProxyWidget::syncRadarToMap);
    
    // 初始化时同步一次雷达位置到地图
    PPIView* ppiView = m_overlayWidget->getPPIView();
    if (ppiView) {
        m_map->syncRadarToMap(ppiView->getRadarLongitude(), 
                             ppiView->getRadarLatitude(), 
                             ppiView->getCurrentRange());
    }
}