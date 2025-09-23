/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:18
 * @Description: 
 */
/**
 * @file mainoverlayout.h
 * @brief 主界面覆盖层布局管理器
 * @details 管理雷达显示系统的主要界面组件布局和交互
 * 
 * 功能特性：
 * - 集成PPI雷达显示界面管理
 * - 缩放视图控制器集成
 * - 扇区显示控件管理
 * - 主要视图组件协调
 * - 界面布局自适应调整
 * 
 * 组件架构：
 * - PPIView: 主要雷达显示视图
 * - PPIScene: 雷达场景管理器
 * - ZoomViewWidget: 缩放控制面板
 * - SectorWidget: 扇区控制面板
 * 
 * 使用场景：
 * - 雷达显示主界面布局
 * - 多视图协调管理
 * - 用户界面集成控制
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef MAINOVERLAYOUT_H
#define MAINOVERLAYOUT_H

#include <QWidget>
#include "ui_mainoverlayout.h"

// 前向声明 - 雷达显示相关组件
class PPIView;        ///< PPI雷达显示视图
class PPIScene;       ///< PPI雷达场景管理器
class ZoomViewWidget; ///< 缩放视图控制器
class SectorWidget;   ///< 扇区控制面板

namespace Ui {
class MainOverLayOut;
}

/**
 * @class MainOverLayOut
 * @brief 主界面覆盖层布局管理器
 * @details 雷达显示系统的主要界面组件布局管理器
 * 
 * 该类负责整合和管理雷达显示系统的各个核心UI组件：
 * - 统一管理PPI雷达显示界面
 * - 协调各个控制面板的布局和交互
 * - 提供界面组件的初始化和配置接口
 * - 管理视图间的数据同步和状态更新
 * 
 * 布局架构：
 * - 主视图区域：PPI雷达显示
 * - 右上角控制区：缩放和扇区控制
 * - 覆盖层管理：透明控制面板
 * - 响应式布局：适应窗口尺寸变化
 * 
 * @example
 * ```cpp
 * MainOverLayOut* mainLayout = new MainOverLayOut(parent);
 * mainLayout->mainPView();    // 初始化主视图
 * mainLayout->topRightSet();  // 配置右上角控制区
 * mainLayout->show();
 * ```
 */
class MainOverLayOut : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 初始化主界面布局管理器，创建UI界面并设置基本布局
     */
    explicit MainOverLayOut(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 清理资源，释放UI组件和相关对象
     */
    ~MainOverLayOut();
    
    /**
     * @brief 初始化主PPI视图
     * @details 创建和配置主要的PPI雷达显示视图
     * 
     * 执行操作：
     * - 初始化PPIView和PPIScene
     * - 建立视图与场景的关联
     * - 配置显示参数和交互模式
     * - 设置视图在布局中的位置
     */
    void mainPView();
    
    /**
     * @brief 配置右上角控制区域
     * @details 设置和配置右上角的控制面板区域
     * 
     * 执行操作：
     * - 初始化缩放控制面板
     * - 配置扇区控制面板
     * - 设置控制面板的布局位置
     * - 建立控制面板与主视图的交互连接
     */
    void topRightSet();

    /**
     * @brief 获取PPI视图指针
     * @return PPIView指针，用于外部访问PPI视图
     * @details 提供对内部PPI视图组件的访问接口，支持信号连接等操作
     */
    PPIView* getPPIView() const { return mView; }

private:
    Ui::MainOverLayOut *ui;           ///< UI界面对象指针
    PPIView* mView;                   ///< PPI雷达显示视图
    PPIScene* mScene;                 ///< PPI雷达场景管理器
    ZoomViewWidget* m_zoomView;       ///< 缩放视图控制器
    SectorWidget* m_sectorWidget;     ///< 扇区显示控制器
};

#endif // MAINOVERLAYOUT_H
