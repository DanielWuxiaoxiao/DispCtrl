/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:15
 * @Description: 
 */
/**
 * @file zoomview.h
 * @brief 局部放大视图组件定义  
 * @details 提供雷达显示的局部放大和测距功能，包含完整的用户交互界面
 *   
 * 本文件定义了三个主要类：
 * - ZoomViewToolBar: 缩放工具栏，提供缩放控制和模式切换
 * - ZoomView: 核心放大视图，支持拖拽和测距两种交互模式
 * - ZoomViewWidget: 完整的放大窗口，组合工具栏和视图
 * 
 * 主要功能：
 * 1. 局部区域放大显示：从主视图选择区域进行详细查看
 * 2. 交互式缩放控制：支持鼠标滚轮和按钮控制的多级缩放
 * 3. 精确测距功能：在放大视图中进行距离测量
 * 4. 智能拖拽导航：在放大状态下的视图内容浏览
 * 5. 模式切换：拖拽模式和测距模式的无缝切换
 * 
 * 设计模式：
 * - 组合模式：ZoomViewWidget组合工具栏和视图组件
 * - 观察者模式：通过信号槽实现组件间通信
 * - 状态模式：支持不同的交互模式（拖拽/测距）
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

// zoomview.h
#ifndef ZOOMVIEW_H
#define ZOOMVIEW_H

#include <QGraphicsView>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLineF>

class PPIScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsTextItem;

/**
 * @class ZoomViewToolBar
 * @brief 缩放视图工具栏
 * @details 提供缩放控制、模式切换和状态显示的工具栏组件
 * 
 * 功能特性：
 * 1. 缩放控制：提供放大、缩小、重置视图的按钮操作
 * 2. 模式切换：拖拽模式和测距模式的互斥切换
 * 3. 状态显示：实时显示当前缩放比例信息
 * 4. 图标界面：使用直观的图标按钮提升用户体验
 * 
 * 交互设计：
 * - 按钮式操作：简单直观的点击交互
 * - 状态反馈：实时更新缩放比例显示
 * - 模式指示：通过按钮状态显示当前交互模式
 * 
 * 信号通信：
 * - 缩放信号：zoomIn(), zoomOut(), resetViewRequested()
 * - 模式信号：dragModeChanged(), measureModeChanged()
 * 
 * 外观定制：
 * - 支持CSS样式定制：通过objectName设置样式
 * - 图标资源：使用资源文件中的图标
 * - 工具提示：提供操作说明提示
 */
// 工具栏控件
class ZoomViewToolBar : public QWidget {
    Q_OBJECT
public:
    explicit ZoomViewToolBar(QWidget* parent = nullptr);

signals:
    /**
     * @brief 放大视图信号
     * @details 用户点击放大按钮时发出，通知视图进行放大操作
     */
    void zoomIn();
    
    /**
     * @brief 缩小视图信号
     * @details 用户点击缩小按钮时发出，通知视图进行缩小操作
     */
    void zoomOut();
    
    /**
     * @brief 拖拽模式改变信号
     * @param drag true为启用拖拽模式，false为禁用
     * @details 用户切换拖拽模式时发出，通知视图调整交互行为
     */
    void dragModeChanged(bool drag);
    
    /**
     * @brief 测距模式改变信号
     * @param measure true为启用测距模式，false为禁用
     * @details 用户切换测距模式时发出，通知视图调整交互行为
     */
    void measureModeChanged(bool measure);
    
    /**
     * @brief 重置视图请求信号
     * @details 用户点击重置按钮时发出，通知视图恢复到初始状态
     */
    void resetViewRequested();

public slots:
    /**
     * @brief 设置缩放级别显示
     * @param level 当前缩放级别（1.0表示100%）
     * @details 更新工具栏上的缩放比例显示文本
     */
    void setZoomLevel(double level);

private:
    QPushButton* m_zoomInBtn;        ///< 放大按钮
    QPushButton* m_zoomOutBtn;       ///< 缩小按钮
    QPushButton* m_resetBtn;         ///< 重置视图按钮
    QPushButton* m_dragBtn;          ///< 拖拽模式按钮（可切换）
    QPushButton* m_measureBtn;       ///< 测距模式按钮（可切换）
    QLabel* m_zoomLabel;             ///< 缩放比例显示标签
};

/**
 * @class ZoomView
 * @brief 局部放大视图核心组件
 * @details 基于QGraphicsView的雷达显示局部放大视图，支持拖拽和测距两种交互模式
 * 
 * 主要功能：
 * 1. 局部放大显示：显示从主视图选择的特定区域
 * 2. 多级缩放控制：支持鼠标滚轮和程序控制的缩放操作
 * 3. 交互模式切换：
 *    - 拖拽模式：用于视图内容的平移浏览
 *    - 测距模式：用于精确的距离测量
 * 4. 精确测距：提供可视化的距离测量工具
 * 5. 场景共享：与主视图共享同一个PPIScene，保持数据一致性
 * 
 * 技术特性：
 * - 坐标系统：支持场景坐标到视图坐标的精确转换
 * - 缩放算法：平滑的几何级数缩放，保持视觉连续性
 * - 测距精度：基于场景坐标的精确距离计算
 * - 渲染优化：继承QGraphicsView的高效渲染机制
 * 
 * 交互模式：
 * - DragMode: 鼠标拖拽平移视图内容
 * - MeasureMode: 鼠标绘制测距线，显示距离信息
 * 
 * 信号通信：
 * - zoomLevelChanged: 缩放级别变化通知
 * - distanceMeasured: 距离测量结果通知
 */
// 局部放大视图
class ZoomView : public QGraphicsView {
    Q_OBJECT

public:
    /**
     * @enum Mode
     * @brief 视图交互模式枚举
     */
    enum Mode {
        DragMode,      ///< 拖动模式：用于视图内容平移
        MeasureMode    ///< 测距模式：用于距离测量
    };

    explicit ZoomView(QWidget* parent = nullptr);
    ~ZoomView();

    /**
     * @brief 设置PPI场景
     * @param scene PPI场景对象指针
     * @details 设置要显示的场景，通常与主视图共享同一个场景实例
     */
    // 设置场景（与主视图共享同一个场景）
    void setPPIScene(PPIScene* scene);

    /**
     * @brief 显示指定区域
     * @param sceneRect 要显示的场景矩形区域
     * @details 将视图调整到显示指定的场景区域，通常来自主视图的选择操作
     */
    // 显示指定区域（从主视图的rubberband传入）
    void showArea(const QRectF& sceneRect);

    /**
     * @brief 获取当前缩放比例
     * @return 当前缩放级别（1.0表示100%）
     * @details 返回相对于原始大小的缩放倍数
     */
    // 获取当前缩放比例
    double zoomLevel() const;

signals:
    /**
     * @brief 缩放级别改变信号
     * @param level 新的缩放级别
     * @details 当视图缩放级别发生变化时发出，用于更新工具栏显示
     */
    void zoomLevelChanged(double level);
    
    /**
     * @brief 距离测量完成信号
     * @param distance 测量得到的距离值
     * @details 在测距模式下完成一次距离测量时发出
     */
    void distanceMeasured(double distance);

public slots:
    /**
     * @brief 放大视图
     * @details 按预设倍数放大视图，并更新缩放级别
     */
    void zoomIn();
    
    /**
     * @brief 缩小视图
     * @details 按预设倍数缩小视图，并更新缩放级别
     */
    void zoomOut();
    
    /**
     * @brief 设置自定义拖拽模式
     * @param drag true启用拖拽模式，false禁用
     * @details 切换到拖拽模式，用于视图内容的平移浏览
     */
    void setCustomDragMode(bool drag);
    
    /**
     * @brief 设置测距模式
     * @param measure true启用测距模式，false禁用
     * @details 切换到测距模式，用于距离测量操作
     */
    void setMeasureMode(bool measure);
    
    /**
     * @brief 重置视图
     * @details 将视图恢复到初始状态，清除所有测量标记
     */
    void resetView();

protected:
    /**
     * @brief 鼠标按下事件处理
     * @param event 鼠标事件对象
     * @details 根据当前模式处理鼠标按下事件，初始化拖拽或测距操作
     */
    void mousePressEvent(QMouseEvent* event) override;
    
    /**
     * @brief 鼠标移动事件处理
     * @param event 鼠标事件对象
     * @details 在测距模式下实时更新测距线的显示
     */
    void mouseMoveEvent(QMouseEvent* event) override;
    
    /**
     * @brief 鼠标释放事件处理
     * @param event 鼠标事件对象
     * @details 完成测距操作或拖拽操作的结束处理
     */
    void mouseReleaseEvent(QMouseEvent* event) override;
    
    /**
     * @brief 鼠标滚轮事件处理
     * @param event 滚轮事件对象
     * @details 响应滚轮滚动进行缩放操作
     */
    void wheelEvent(QWheelEvent* event) override;

private:
    /**
     * @brief 更新缩放级别
     * @details 计算当前缩放级别并发出信号通知
     */
    void updateZoomLevel();
    
    /**
     * @brief 清除测量线
     * @details 移除当前显示的所有测距图形元素
     */
    void clearMeasureLine();

private:
    PPIScene* m_scene;               ///< PPI场景指针
    Mode m_mode;                     ///< 当前交互模式
    double m_zoomFactor;             ///< 当前缩放因子

    // 测距相关组件（提供具象的尺子表现）
    bool m_measuring;                ///< 是否正在进行测距操作
    QPointF m_measureStart;          ///< 测距起始点（场景坐标）
    QGraphicsPathItem* m_measurePath;   ///< 虚线测距路径
    QGraphicsEllipseItem* m_startMarker; ///< 起始点标记
    QGraphicsEllipseItem* m_endMarker;   ///< 结束点标记
    QGraphicsTextItem* m_distanceText;   ///< 距离显示文本
};

/**
 * @class ZoomViewWidget
 * @brief 完整的局部放大窗口组件
 * @details 组合工具栏和放大视图的完整窗口组件，提供一体化的局部放大功能
 * 
 * 组件架构：
 * 1. 工具栏（ZoomViewToolBar）：提供操作控制界面
 * 2. 视图（ZoomView）：提供核心的显示和交互功能
 * 3. 布局管理：垂直布局组织工具栏和视图
 * 
 * 功能特性：
 * - 一体化设计：工具栏和视图的完美集成
 * - 信号中继：将内部信号转发给外部组件
 * - 统一接口：提供简化的外部调用接口
 * - 自动连接：内部组件信号槽的自动连接
 * 
 * 使用场景：
 * - 独立窗口：作为独立的局部放大窗口
 * - 停靠窗口：集成到主界面的停靠区域
 * - 对话框：作为对话框中的主要内容
 * 
 * 外部接口：
 * - setPPIScene(): 设置要显示的场景
 * - showArea(): 显示指定区域
 * - view(): 获取内部视图组件
 * - distanceMeasured: 距离测量信号
 */
// 完整的局部放大窗口（包含工具栏和视图）
class ZoomViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZoomViewWidget(QWidget* parent = nullptr);
    ~ZoomViewWidget();

    /**
     * @brief 设置PPI场景
     * @param scene PPI场景对象指针
     * @details 将场景设置给内部的ZoomView组件
     */
    // 设置场景
    void setPPIScene(PPIScene* scene);

    /**
     * @brief 显示指定区域
     * @param sceneRect 要显示的场景矩形区域
     * @details 调用内部ZoomView显示指定区域
     */
    // 显示指定区域
    void showArea(const QRectF& sceneRect);

    /**
     * @brief 获取内部视图组件
     * @return ZoomView指针
     * @details 提供对内部视图组件的直接访问
     */
    ZoomView* view() const { return m_view; }

signals:
    /**
     * @brief 距离测量完成信号
     * @param distance 测量得到的距离值
     * @details 转发内部ZoomView的距离测量信号
     */
    void distanceMeasured(double distance);

private:
    /**
     * @brief 设置用户界面
     * @details 创建并配置布局、工具栏和视图组件
     */
    void setupUI();
    
    /**
     * @brief 连接信号槽
     * @details 建立工具栏和视图之间的信号槽连接
     */
    void connectSignals();

private:
    ZoomView* m_view;                ///< 内部的放大视图组件
    ZoomViewToolBar* m_toolBar;      ///< 内部的工具栏组件
};

#endif // ZOOMVIEW_H
