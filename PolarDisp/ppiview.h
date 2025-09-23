/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 15:56:15
 * @Description: 
 */
/**
 * @file ppiview.h
 * @brief PPI视图组件头文件
 * @details 定义PPI(平面位置显示器)视图类，提供雷达显示的用户交互界面。
 *          包括缩放、拖拽、区域选择等交互功能，以及信息叠加层的管理。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#ifndef PPIVIEW_H
#define PPIVIEW_H
#pragma once
#include <QGraphicsView>
#include <QRubberBand>

// 前向声明 - 避免头文件循环依赖
class PPIScene;           ///< PPI场景管理器
class mainviewTopLeft;    ///< 左上角雷达信息显示组件
class PointInfoW;         ///< 右上角点信息显示组件
class MousePositionInfo;  ///< 鼠标位置信息显示组件
class PPIVisualSettings;  ///< PPI视觉设置组件

/**
 * @class PPIView
 * @brief PPI显示视图类
 * @details 雷达PPI显示的视图组件，继承自QGraphicsView，提供：
 *          - 高质量的图形渲染（抗锯齿）
 *          - 鼠标交互功能（缩放、拖拽、区域选择）
 *          - 橡皮筋缩放功能
 *          - 信息叠加层管理
 *          - 自适应窗口大小变化
 *
 * 主要特性：
 * - 智能视图更新：平衡性能与更新完整性
 * - 鼠标中心缩放：以鼠标位置为缩放中心
 * - 视图中心对齐：窗口大小变化时保持内容居中
 * - 橡皮筋区域选择：支持拖拽选择特定区域
 * - 信息叠加显示：不受缩放影响的固定信息显示
 *
 * 渲染优化：
 * - 抗锯齿渲染：提供平滑的图形显示效果
 * - 智能更新模式：减少不必要的重绘提升性能
 * - 变换锚点优化：提供直观的用户交互体验
 *
 * @example 基本使用：
 * @code
 * PPIView* view = new PPIView();
 * PPIScene* scene = new PPIScene();
 * view->setPPIScene(scene);
 * view->enableRubberBandZoom(true);
 * @endcode
 */
class PPIView : public QGraphicsView {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 初始化PPI视图组件：
     *          - 启用抗锯齿渲染
     *          - 配置智能视图更新模式
     *          - 设置鼠标中心变换锚点
     *          - 配置视图中心调整锚点
     *          - 初始化信息叠加层
     *          - 启用橡皮筋缩放功能
     */
    explicit PPIView(QWidget* parent=nullptr);

    /**
     * @brief 设置PPI场景
     * @param scene PPI场景对象指针
     * @details 将PPI场景设置为当前视图的显示内容：
     *          - 保存场景引用
     *          - 调用QGraphicsView::setScene()设置场景
     *          - 自动调整视图以适应场景内容
     */
    void setPPIScene(PPIScene* scene);

    /**
     * @brief 启用/禁用橡皮筋缩放功能
     * @param on true启用，false禁用
     * @details 控制鼠标拖拽的行为模式：
     *          - 启用时：拖拽创建选择矩形，用于区域缩放
     *          - 禁用时：拖拽用于平移视图内容
     *          - 同时切换相应的拖拽模式
     */
    void enableRubberBandZoom(bool on);

    /**
     * @brief 设置雷达中心经纬度
     * @param longitude 经度
     * @param latitude 纬度
     * @details 设置雷达中心的地理位置，用于与地图同步
     */
    void setRadarCenter(double longitude, double latitude);

    /**
     * @brief 获取雷达中心经度
     * @return 雷达中心经度
     */
    double getRadarLongitude() const { return m_radarLongitude; }

    /**
     * @brief 获取雷达中心纬度
     * @return 雷达中心纬度
     */
    double getRadarLatitude() const { return m_radarLatitude; }

    /**
     * @brief 获取当前雷达最大范围
     * @return 最大范围（公里）
     */
    double getCurrentRange() const { return m_currentRange; }

    /**
     * @brief 计算PPIView中心在主窗口centralWidget中的位置
     * @return PPIView中心在centralWidget坐标系中的位置(像素)
     * @details 通过widget层级关系计算PPIView几何中心在MainWindow的centralWidget中的位置
     */
    QPointF getPPIViewCenterInMainWindow() const;

    /**
     * @brief 计算地图显示所需的中心经纬度和范围
     * @param mapCenterLng 输出：地图中心经度
     * @param mapCenterLat 输出：地图中心纬度
     * @param mapRange 输出：地图显示范围（公里）
     * @details 根据PPIView在centralWidget中的位置偏移，计算地图应该显示的中心和范围
     */
    void calculateMapDisplayParameters(double& mapCenterLng, double& mapCenterLat, double& mapRange) const;

signals:
    /**
     * @brief 视图尺寸变化信号
     * @param newSize 新的视图尺寸
     * @details 当视图窗口大小改变时发出，通知相关组件：
     *          - 场景需要重新计算显示比例
     *          - 叠加层需要重新布局
     *          - 其他依赖视图尺寸的组件需要更新
     */
    void viewResized(const QSize& newSize);

    /**
     * @brief 区域选择信号
     * @param sceneRect 选择的场景矩形区域
     * @details 当用户通过橡皮筋选择区域时发出：
     *          - 传递场景坐标系下的选择矩形
     *          - 可用于区域缩放或特定区域分析
     *          - 矩形大小必须超过10x10像素才会触发
     */
    void areaSelected(const QRectF& sceneRect);

    /**
     * @brief 最大距离变化信号
     * @param distance 新的最大距离值（公里）
     * @details 当用户通过视觉设置组件修改最大距离时发出
     */
    void maxDistanceChanged(double distance);

    /**
     * @brief 地图类型变化信号
     * @param index 新的地图类型索引
     * @details 当用户通过视觉设置组件切换地图类型时发出
     */
    void mapTypeChanged(int index);

    /**
     * @brief 测距结果信号
     * @param distance 测量得到的距离值（米）
     * @details 当用户完成测距操作时发出，提供测量结果
     */
    void distanceMeasured(double distance);

    /**
     * @brief 雷达中心位置变化信号
     * @param longitude 新的经度
     * @param latitude 新的纬度
     * @param range 当前雷达范围（公里）
     * @details 当雷达中心位置或范围发生变化时发出，用于同步地图显示
     */
    void radarCenterChanged(double longitude, double latitude, double range);

public slots:
    /**
     * @brief 处理最大距离变化
     * @param distance 新的最大距离值（公里）
     * @details 响应PPIVisualSettings组件的距离变化，更新场景显示范围
     */
    void onMaxDistanceChanged(double distance);

    /**
     * @brief 处理地图类型变化
     * @param index 新的地图类型索引
     * @details 响应PPIVisualSettings组件的地图类型变化，转发给地图组件
     */
    void onMapTypeChanged(int index);

    /**
     * @brief 处理测距模式切换
     * @param enabled 是否启用测距模式
     * @details 响应PPIVisualSettings组件的测距模式切换，修改鼠标交互行为
     */
    void onMeasureModeChanged(bool enabled);

protected:
    /**
     * @brief 鼠标按下事件处理
     * @param e 鼠标事件对象
     * @details 处理鼠标按下事件：
     *          - 橡皮筋模式：记录起始点，创建选择矩形
     *          - 调用基类处理其他鼠标交互
     */
    void mousePressEvent(QMouseEvent* e) override;

    /**
     * @brief 鼠标移动事件处理
     * @param e 鼠标事件对象
     * @details 处理鼠标移动事件：
     *          - 橡皮筋模式：更新选择矩形大小
     *          - 调用基类处理视图拖拽等操作
     */
    void mouseMoveEvent(QMouseEvent* e) override;

    /**
     * @brief 鼠标释放事件处理
     * @param e 鼠标事件对象
     * @details 处理鼠标释放事件：
     *          - 橡皮筋模式：完成区域选择，发射areaSelected信号
     *          - 验证选择区域大小，过滤无效选择
     *          - 调用基类处理其他鼠标操作
     */
    void mouseReleaseEvent(QMouseEvent* e) override;

    /**
     * @brief 窗口大小变化事件处理
     * @param e 大小变化事件对象
     * @details 处理视图窗口大小变化：
     *          - 调用基类的大小变化处理
     *          - 发射viewResized信号通知其他组件
     *          - 重新布局信息叠加层
     *          - 调整视图以适应场景内容
     */
    void resizeEvent(QResizeEvent* e) override;

private:
    // 核心组件
    PPIScene* m_scene;                ///< PPI场景对象指针

    // 交互控制
    bool m_rubberBandZoom = true;     ///< 橡皮筋缩放功能开关
    QRubberBand* m_band = nullptr;    ///< 橡皮筋选择框对象
    QPoint m_origin;                  ///< 橡皮筋拖拽起始点

    // 测距功能
    bool m_measureMode = false;       ///< 测距模式开关
    bool m_measuring = false;         ///< 正在测距标志
    QPointF m_measureStart;           ///< 测距起始点（场景坐标）
    QGraphicsPathItem* m_measurePath = nullptr;  ///< 测距路径图形项
    QGraphicsEllipseItem* m_startMarker = nullptr;  ///< 起始点标记
    QGraphicsEllipseItem* m_endMarker = nullptr;    ///< 结束点标记
    QGraphicsTextItem* m_distanceText = nullptr;    ///< 距离文本显示

    // 信息叠加层组件 - 不随视图缩放变化的固定UI元素
    mainviewTopLeft* radarInfoW = nullptr;      ///< 左上角雷达系统信息显示
    PointInfoW* pointInfo = nullptr;            ///< 右上角选中点详细信息显示
    MousePositionInfo* mousePositionInfo = nullptr;  ///< 左下角鼠标位置信息显示
    PPIVisualSettings* visualSettings = nullptr;     ///< 右下角PPI视觉设置组件

    // 雷达地理位置信息
    double m_radarLongitude = 108.9138;         ///< 雷达中心经度（默认西电99号楼）
    double m_radarLatitude = 34.2311;           ///< 雷达中心纬度（默认西电99号楼）
    double m_currentRange = 5.0;                ///< 当前雷达最大范围（公里）

    /**
     * @brief 初始化叠加层
     * @details 创建和配置信息叠加层组件：
     *          - 创建左上角雷达信息显示组件
     *          - 创建右上角点信息显示组件
     *          - 创建左下角鼠标位置信息显示组件
     *          - 创建右下角PPI视觉设置组件
     *          - 调用layoutOverlay()进行初始布局
     */
    void setupOverlay();

    /**
     * @brief 布局叠加层组件
     * @details 重新定位所有叠加层组件的位置：
     *          - 左上角信息：固定在左上角位置
     *          - 右上角信息：根据当前视图宽度动态调整位置
     *          - 左下角信息：固定在左下角位置
     *          - 右下角设置：根据当前视图尺寸动态调整位置
     *          - 确保叠加层不会超出视图边界
     */
    void layoutOverlay();

    /**
     * @brief 清除测距线
     * @details 清除当前显示的测距图形元素，包括路径、标记和文本
     */
    void clearMeasureLine();
};
#endif //PPIVIEW_H
