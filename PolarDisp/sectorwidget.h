/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:14
 * @Description: 
 */
/**
 * @file sectorwidget.h  
 * @brief 扇形显示控件定义
 * @details 提供完整的扇形雷达显示控件，包含参数控制和数据演示功能
 * 
 * 功能特性：
 * 1. 图形显示：集成QGraphicsView和SectorScene的完整扇形显示
 * 2. 参数控制：提供角度和距离范围的实时调整界面
 * 3. 数据演示：支持随机生成点迹和航迹数据用于测试和演示
 * 4. 交互控制：提供清除数据、更新参数等操作按钮
 * 5. 自适应布局：支持窗口大小变化的自动适配
 * 
 * 界面设计：
 * - 主显示区：QGraphicsView显示扇形场景内容
 * - 控制面板：分组的参数设置控件
 * - 操作按钮：数据添加、清除、更新等操作
 * - 响应式布局：垂直和水平布局的组合使用
 * 
 * 应用场景：
 * - 雷达扇形监控：实际雷达系统的扇形显示组件
 * - 测试演示：用于功能测试和客户演示
 * - 开发调试：扇形显示功能的开发和调试工具
 * - 培训教学：雷达系统原理的教学演示
 * 
 * 技术架构：
 * - MVC模式：View（QGraphicsView）+ Model（SectorScene）分离
 * - 组合模式：集成多个控制组件形成完整功能
 * - 观察者模式：通过信号槽响应参数变化
 * 
 * @author DispCtrl Development Team  
 * @version 1.0
 * @date 2024
 */

// sectorwidget.h - 使用扇形场景的Widget示例
#ifndef SECTORWIDGET_H
#define SECTORWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QShowEvent>

class SectorScene;

/**
 * @class SectorToolBar
 * @brief 扇形显示工具栏
 * @details 提供扇形范围控制和数据操作的工具栏组件
 * 
 * 功能特性：
 * 1. 参数控制：最小/最大角度和距离范围的精确设置
 * 2. 数据操作：添加随机点迹、航迹和清除数据的快捷按钮
 * 3. 状态显示：实时显示当前操作状态信息
 * 4. 紧凑布局：水平排列控制组件，节省界面空间
 * 
 * 交互设计：
 * - 参数输入：数值输入框提供精确的范围控制
 * - 操作按钮：直观的按钮界面，支持快速操作
 * - 即时反馈：参数修改和操作结果的实时显示
 * 
 * 信号通信：
 * - 参数信号：sectorRangeUpdateRequested()
 * - 数据信号：addRandomDetRequested(), addRandomTrackRequested(), clearAllRequested()
 */
class SectorToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit SectorToolBar(QWidget* parent = nullptr);

    // 获取当前参数值
    double getMinAngle() const;
    double getMaxAngle() const;
    double getMinRange() const;
    double getMaxRange() const;
    
signals:
    /**
     * @brief 扇形范围更新请求信号
     * @param minAngle 最小角度
     * @param maxAngle 最大角度  
     * @param minRange 最小距离(公里)
     * @param maxRange 最大距离(公里)
     * @details 用户在输入框中按回车键时发出
     */
    void sectorRangeUpdateRequested(double minAngle, double maxAngle, double minRange, double maxRange);
    
    /**
     * @brief 重置视图请求信号
     * @details 需要重置视图时发出，通知视图恢复到最佳显示状态
     */
    void resetViewRequested();

public slots:
    
    /**
     * @brief 更新扇形范围显示（已禁用）
     * @param minAngle 最小角度
     * @param maxAngle 最大角度
     * @param minRange 最小距离(公里)
     * @param maxRange 最大距离(公里)
     * @details 此方法已禁用，不再自动更新参数控件的显示值
     */
    void updateRangeDisplay(double minAngle, double maxAngle, double minRange, double maxRange);

private slots:
    /**
     * @brief 参数输入框回车处理
     * @details 任一输入框按回车后，收集所有参数值并发出更新信号
     */
    void onParameterChanged();

private:
    QLineEdit* m_minAngleLineEdit;       ///< 最小角度输入框
    QLineEdit* m_maxAngleLineEdit;       ///< 最大角度输入框
    QLineEdit* m_minRangeLineEdit;       ///< 最小距离输入框(公里)
    QLineEdit* m_maxRangeLineEdit;       ///< 最大距离输入框(公里)
};


/**
 * @class SectorView
 * @brief 扇形显示视图
 * @details 基于QGraphicsView的扇形场景显示组件
 * 
 * 功能特性：
 * 1. 场景显示：集成SectorScene实现扇形数据的图形化显示
 * 2. 交互控制：支持缩放、平移等基本视图交互操作
 * 3. 自适应布局：自动调整显示内容以适应窗口大小变化
 * 
 * 交互设计：
 * - 鼠标操作：支持鼠标拖动和滚轮缩放视图
 * - 键盘控制：可通过键盘快捷键进行视图调整
 * - 响应式布局：视图内容根据窗口大小动态调整
 * 
 * 技术架构：
 * - 继承QGraphicsView，扩展视图功能
 * - 组合SectorScene，实现数据与视图分离
 */
class SectorView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SectorView(QWidget* parent = nullptr);
    void setSectorScene(SectorScene* scene);

public slots:
    /**
     * @brief 重置视图到最佳显示状态
     * @details 将视图恢复到完整显示扇形场景的状态
     */
    void resetView();

private:
    SectorScene* m_scene;
};

/**
 * @class SectorWidget
 * @brief 扇形显示窗口组件
 * @details 采用垂直布局的扇形雷达显示窗口，包含工具栏和显示视图
 * 
 * 组件结构（类似ZoomViewWidget）：
 * 1. 工具栏（SectorToolBar）：参数控制和操作按钮
 * 2. 显示视图（SectorView）：扇形场景的图形显示
 * 
 * 布局设计：
 * - 垂直布局：上方工具栏 + 下方显示视图
 * - 紧凑设计：无边距，统一样式
 * - 响应式：自适应窗口大小变化
 */
class SectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SectorWidget(QWidget* parent = nullptr);

private slots:
    /**
     * @brief 更新扇形范围
     * @param minAngle 最小角度
     * @param maxAngle 最大角度
     * @param minRange 最小距离
     * @param maxRange 最大距离
     * @details 响应工具栏的参数更新请求，更新扇形场景显示
     */
    void updateSectorRange(double minAngle, double maxAngle, double minRange, double maxRange);
    
    /**
     * @brief 场景距离范围改变响应
     * @param minRange 新的最小距离
     * @param maxRange 新的最大距离
     * @details 当场景的距离范围发生变化时，同步更新控件显示
     */
    void onSceneRangeChanged(float minRange, float maxRange);

protected:
    /**
     * @brief 窗口大小改变事件
     * @param event 大小改变事件对象
     * @details 当窗口大小变化时，调整扇形场景的显示尺寸
     */
    void resizeEvent(QResizeEvent* event) override;
    
    /**
     * @brief 窗口显示事件
     * @param event 显示事件对象
     * @details 确保首次显示时场景内容完整显示
     */
    void showEvent(QShowEvent* event) override;

private:
    /**
     * @brief 设置用户界面
     * @details 创建并配置垂直布局：工具栏 + 显示视图
     */
    void setupUI();

private:
    SectorToolBar* m_toolBar;            ///< 工具栏组件
    SectorView* m_view;                  ///< 扇形显示视图
    SectorScene* m_scene;                ///< 扇形场景对象
    
    // 数据生成相关
    int m_trackCounter = 1;              ///< 航迹编号计数器
    QTimer* m_autoAddTimer;              ///< 自动添加数据定时器
};

#endif // SECTORWIDGET_H