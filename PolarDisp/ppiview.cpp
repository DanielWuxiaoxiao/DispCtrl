/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 15:56:15
 * @Description: 
 */
/**
 * @file ppiview.cpp
 * @brief PPI视图组件实现文件
 * @details 实现雷达PPI显示的视图功能，包括图形渲染优化、用户交互处理、
 *          橡皮筋缩放、信息叠加层管理等核心功能。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#include "ppiview.h"
#include "ppisscene.h"
#include "pviewtopleft.h"
#include "pointinfow.h"
#include "mousepositioninfo.h"
#include "ppivisualsettings.h"
#include "polaraxis.h"
#include "../Basic/log.h"

#include <QMouseEvent>
#include <QtMath>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QTimer>
#include <QMainWindow>

/**
 * @brief 计算PPIView中心在主窗口centralWidget中的位置
 * @return PPIView中心在centralWidget坐标系中的位置(像素)
 * @details 通过widget层级关系计算PPIView几何中心在MainWindow的centralWidget中的位置
 */
QPointF PPIView::getPPIViewCenterInMainWindow() const
{
    // PPIView的几何中心（相对于自身坐标系）
    QPointF ppiCenter(width() / 2.0, height() / 2.0);

    // 将PPIView中心坐标转换到MainWindow坐标系
    QWidget* mainWindow = window(); // 获取顶级窗口
    if (!mainWindow) {
        LOG_INFO("Warning: Cannot get MainWindow, returning default center position");
        return QPointF(400, 300); // 默认值
    }

    // 检查PPIView是否已经有有效的几何信息
    if (width() <= 0 || height() <= 0) {
        LOG_INFO("Warning: PPIView size invalid, returning MainWindow center");
        return QPointF(mainWindow->width() / 2.0, mainWindow->height() / 2.0);
    }

    // 将PPIView的中心坐标映射到MainWindow坐标系
    QPoint globalCenter = mapToGlobal(ppiCenter.toPoint());
    QPoint mainWindowCenter = mainWindow->mapFromGlobal(globalCenter);

    return QPointF(mainWindowCenter);
}

/**
 * @brief 计算地图显示所需的中心经纱度和范围
 * @param mapCenterLng 输出：地图中心经度
 * @param mapCenterLat 输出：地图中心纬度
 * @param mapRange 输出：地图显示范围（公里）
 * @details 根据PPIView相对于MapProxyWidget的位置偏移，计算地图应该显示的中心和范围
 *          核心逻辑：PPIView中心 = 雷达经纬度位置，地图中心 = 雷达位置 - PPIView相对MapProxyWidget的偏移
 */
void PPIView::calculateMapDisplayParameters(double& mapCenterLng, double& mapCenterLat, double& mapRange) const
{
    QWidget* mainWindow = window();
    if (!mainWindow) {
        // 如果无法获取MainWindow，直接返回雷达位置作为地图中心
        mapCenterLng = m_radarLongitude;
        mapCenterLat = m_radarLatitude;
        mapRange = m_currentRange * 2.0; // 默认地图范围
        return;
    }

    // 1. 获取MapProxyWidget（地图显示组件）
    // MapProxyWidget在MainWindow的centralWidget中
    // 需要将QWidget*转换为QMainWindow*才能调用centralWidget()
    QMainWindow* mainWin = qobject_cast<QMainWindow*>(mainWindow);
    if (!mainWin) {
        // 如果转换失败，使用MainWindow本身作为参考
        mapCenterLng = m_radarLongitude;
        mapCenterLat = m_radarLatitude;
        mapRange = m_currentRange * 2.0;
        return;
    }

    QWidget* centralWidget = mainWin->centralWidget();
    if (!centralWidget) {
        mapCenterLng = m_radarLongitude;
        mapCenterLat = m_radarLatitude;
        mapRange = m_currentRange * 2.0;
        return;
    }

    // 2. 计算PPIView中心在MainWindow坐标系中的位置
    QPointF ppiCenter(width() / 2.0, height() / 2.0);
    QPoint globalPPICenter = mapToGlobal(ppiCenter.toPoint());
    QPoint ppiCenterInMainWindow = mainWindow->mapFromGlobal(globalPPICenter);

    // 3. 计算MapProxyWidget（centralWidget）的中心位置
    QPointF mapProxyCenter(centralWidget->width() / 2.0, centralWidget->height() / 2.0);

    // 4. 计算PPIView中心相对于MapProxyWidget中心的像素偏移
    // 注意：这里偏移的含义是 PPIView中心 - MapProxyWidget中心
    QPointF pixelOffset = QPointF(ppiCenterInMainWindow) - mapProxyCenter;

    // 5. 获取PPI的像素到距离的转换比例（基于当前PPI显示范围）
    double pixelsPerMeter = 1.0; // 默认值
    if (m_scene && m_scene->axis() && m_currentRange > 0) {
        // 使用PPI的实际显示范围计算比例
        // PPI显示半径范围是m_currentRange km = m_currentRange * 1000 m
        // 假设PPI显示半径在视图中占据的像素是视图半径
        double ppiRadiusPixels = qMin(width(), height()) / 2.0;
        double ppiRadiusMeters = m_currentRange * 1000.0; // 转换为米
        pixelsPerMeter = ppiRadiusPixels / ppiRadiusMeters;
    }

    // 6. 将像素偏移转换为米偏移
    double metersPerPixel = 1.0 / pixelsPerMeter;
    double meterOffsetX = pixelOffset.x() * metersPerPixel;
    double meterOffsetY = pixelOffset.y() * metersPerPixel;

    // 7. 将米偏移转换为经纬度偏移
    const double earthRadius = 6378137.0; // 地球半径，米
    const double degreesToRadians = M_PI / 180.0;

    // 纬度偏移（屏幕Y向下为正，但纬度向北为正，所以需要负号）
    double latOffset = -meterOffsetY / (earthRadius * degreesToRadians);

    // 经度偏移（屏幕X向右为正，经度向东为正）
    double latRad = m_radarLatitude * degreesToRadians;
    double lngOffset = meterOffsetX / (earthRadius * cos(latRad) * degreesToRadians);

    // 8. 计算地图中心经纬度
    // 核心逻辑：由于PPIView中心 = 雷达位置，而地图要显示的中心应该是考虑偏移后的位置
    // 地图中心 = 雷达位置 - 偏移量（这样PPIView中心就能正确对应到雷达位置）
    mapCenterLng = m_radarLongitude - lngOffset;
    mapCenterLat = m_radarLatitude - latOffset;

    // 9. 计算地图显示范围
    // 根据MapProxyWidget的大小计算需要显示的地理范围
    double mapWidthPixels = centralWidget->width();
    double mapHeightPixels = centralWidget->height();
    double mapMaxDimensionPixels = qMax(mapWidthPixels, mapHeightPixels);

    // 将最大像素尺寸转换为地理距离（公里）
    double mapMaxDimensionMeters = mapMaxDimensionPixels * metersPerPixel;
    mapRange = mapMaxDimensionMeters / 1000.0; // 转换为公里

    // 确保地图范围合理（至少能完全包含PPI显示区域）
    double minRequiredRange = m_currentRange * 2.0; // 至少是PPI范围的2倍
    mapRange = qMax(mapRange, minRequiredRange);

    LOG_INFO(QString("Map params calc: PPIView center in MainWindow(%1,%2), MapProxy center(%3,%4), pixel offset(%5,%6)")
            .arg(ppiCenterInMainWindow.x()).arg(ppiCenterInMainWindow.y())
            .arg(mapProxyCenter.x()).arg(mapProxyCenter.y())
            .arg(pixelOffset.x()).arg(pixelOffset.y()));
    LOG_INFO(QString("Map params calc: Radar pos(%1,%2), Map center(%3,%4), Map range %5km, PPI range %6km")
            .arg(m_radarLongitude).arg(m_radarLatitude)
            .arg(mapCenterLng).arg(mapCenterLat)
            .arg(mapRange).arg(m_currentRange));
}

/**
 * @brief PPIView构造函数
 * @details 初始化PPI视图组件，配置渲染参数和交互模式，创建信息叠加层
 * @param parent 父窗口指针，用于Qt对象树管理
 *
 * 渲染优化配置：
 * 1. 抗锯齿渲染：启用QPainter::Antialiasing，提供平滑的图形显示
 * 2. 拖拽模式：默认启用RubberBandDrag，支持区域选择
 * 3. 视图更新：SmartViewportUpdate模式，平衡性能与完整性
 * 4. 变换锚点：AnchorUnderMouse，以鼠标位置为缩放中心
 * 5. 调整锚点：AnchorViewCenter，窗口变化时保持内容居中
 *
 * 更新模式说明：
 * - FullViewportUpdate: 全视图重绘（高质量，低性能）
 * - MinimalViewportUpdate: 最小化更新（高性能，可能遗漏）
 * - SmartViewportUpdate: 智能更新（推荐，适合复杂动态场景）
 *
 * 锚点机制：
 * - TransformationAnchor: 控制缩放/旋转的中心点
 * - ResizeAnchor: 控制窗口调整时内容的对齐方式
 *
 * 初始化流程：
 * 1. 配置图形渲染参数
 * 2. 设置用户交互模式
 * 3. 创建信息叠加层
 * 4. 启用橡皮筋缩放功能
 */
PPIView::PPIView(QWidget* parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    // 其他模式：
    // - QGraphicsView::FullViewportUpdate：每次更新都重绘整个视图（性能差，适合简单场景）
    // - QGraphicsView::MinimalViewportUpdate：仅重绘变化的图元（极端场景可能漏更）
    // 优势：平衡性能与更新完整性，是大多数复杂场景（如动态图元、动画）的最优选择
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    // 作用：对视图进行缩放/旋转时，以鼠标当前位置为中心点（而非视图中心）
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // 作用：当窗口（视图）被拉伸/缩小后，场景内容会保持以视图中心为基准对齐
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setupOverlay();
    enableRubberBandZoom(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 延迟发送初始雷达位置信号，确保在MainWindow连接信号后再发送
    QTimer::singleShot(100, this, [this]() {
        // 使用计算的地图显示参数发送初始信号
        double mapCenterLng, mapCenterLat, mapRange;
        calculateMapDisplayParameters(mapCenterLng, mapCenterLat, mapRange);
        emit radarCenterChanged(mapCenterLng, mapCenterLat, mapRange);

        LOG_INFO(QString("PPIView init complete: Radar pos %1,%2, range %3km, Map center %4,%5, Map range %6km")
                .arg(m_radarLongitude).arg(m_radarLatitude).arg(m_currentRange)
                .arg(mapCenterLng).arg(mapCenterLat).arg(mapRange));
    });
}

/**
 * @brief 初始化信息叠加层
 * @details 创建不受视图缩放影响的固定UI元素，提供实时的雷达系统信息显示
 *
 * 创建的叠加层组件：
 * 1. radarInfoW (mainviewTopLeft): 左上角雷达系统信息
 *    - 显示雷达状态、工作模式等系统信息
 *    - 固定在视图左上角位置
 * 2. pointInfo (PointInfoW): 右上角选中点信息
 *    - 显示鼠标悬停或选中目标的详细信息
 *    - 固定在视图右上角位置
 *
 * 叠加层特性：
 * - 位置固定：不随视图缩放、平移而移动
 * - 层级最高：始终显示在场景内容之上
 * - 透明背景：不遮挡重要的雷达显示内容
 * - 响应式布局：随视图窗口大小自动调整位置
 *
 * @note 叠加层使用QWidget直接添加到视图上，而非场景中的图形项
 */
void PPIView::setupOverlay() {
    // 顶角信息使用 QLabel 作为 overlay，不随缩放
    radarInfoW = new mainviewTopLeft(this);
    pointInfo = new PointInfoW(this);
    mousePositionInfo = new MousePositionInfo(this);
    visualSettings = new PPIVisualSettings(this);

    // 连接视觉设置信号
    connect(visualSettings, &PPIVisualSettings::maxDistanceChanged,
            this, &PPIView::onMaxDistanceChanged);
    connect(visualSettings, &PPIVisualSettings::mapTypeChanged,
            this, &PPIView::onMapTypeChanged);
    connect(visualSettings, &PPIVisualSettings::measureModeChanged,
            this, &PPIView::onMeasureModeChanged);

    layoutOverlay();
}

/**
 * @brief 布局信息叠加层组件
 * @details 重新定位所有叠加层组件到正确的屏幕位置，确保信息显示不被遮挡
 *
 * 布局规则：
 * 1. 左上角雷达信息 (radarInfoW):
 *    - 位置：(0, 5) - 紧贴左边界，距顶部5像素
 *    - 目的：显示系统状态信息，避免与菜单栏冲突
 *
 * 2. 右上角点信息 (pointInfo):
 *    - 位置：距右边界8像素，距顶部5像素
 *    - 计算：width() - widget_width - 8
 *    - 目的：显示选中目标信息，与左侧信息对称布局
 *
 * 自适应特性：
 * - 窗口宽度变化时，右侧组件自动调整位置
 * - 组件大小变化时，位置自动重新计算
 * - 确保所有叠加层始终在视图可见区域内
 *
 * 调用时机：
 * - 组件初始化时
 * - 视图窗口大小变化时
 * - 叠加层组件尺寸变化时
 *
 * @note 使用组件的实际尺寸进行精确定位，避免内容溢出
 */
void PPIView::layoutOverlay() {
    if (radarInfoW) {
        radarInfoW->move(0, 5);
    }
    if (pointInfo) {
        QSize s = pointInfo->size();
        pointInfo->move(width()-s.width()-8, 5);
    }
    if (mousePositionInfo) {
        QSize s = mousePositionInfo->size();
        mousePositionInfo->move(8, height()-s.height()-8);
    }
    if (visualSettings) {
        QSize s = visualSettings->size();
        visualSettings->move(width()-s.width()-8, height()-s.height()-8);
    }
}

/**
 * @brief 设置PPI场景
 * @details 将指定的PPI场景设置为当前视图的显示内容，并自动调整视图适应场景
 * @param scene PPI场景对象指针
 *
 * 设置流程：
 * 1. 保存场景引用：存储到m_scene成员变量
 * 2. 关联场景：调用QGraphicsView::setScene()建立视图-场景关系
 * 3. 自动适配：调用fitInView()使场景内容完整显示在视图中
 *
 * 适配参数：
 * - scene->sceneRect(): 使用场景的完整矩形范围
 * - Qt::KeepAspectRatio: 保持宽高比，避免图形变形
 *
 * 功能效果：
 * - 场景内容完整可见：所有图形元素都在视图范围内
 * - 比例正确：圆形保持圆形，不会变成椭圆
 * - 居中显示：场景内容在视图中居中对齐
 *
 * 应用场景：
 * - 视图初始化时设置场景
 * - 切换不同的PPI显示模式
 * - 重新加载场景内容
 *
 * @note 该函数会触发视图的自动重绘，场景变化立即可见
 */
void PPIView::setPPIScene(PPIScene* scene) {
    m_scene = scene;
    QGraphicsView::setScene(scene);
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    // 同步场景的初始范围到视图和界面控件
    if (m_scene && m_scene->axis() && visualSettings) {
        double maxRangeInMeters = m_scene->axis()->maxRange();
        double maxRangeInKm = maxRangeInMeters / 1000.0;

        // 更新视图内部的范围记录
        m_currentRange = maxRangeInKm;

        // 同步界面控件显示（不触发信号）
        visualSettings->setMaxDistance(maxRangeInKm);

        LOG_INFO(QString("PPI range sync: Scene %1m -> View %2km")
                .arg(maxRangeInMeters).arg(maxRangeInKm));
    }
}

/**
 * @brief 启用/禁用橡皮筋缩放功能
 * @details 切换鼠标拖拽的行为模式，在区域选择和视图平移之间切换
 * @param on true启用橡皮筋缩放，false禁用
 *
 * 模式说明：
 * 1. 橡皮筋缩放模式 (on=true):
 *    - 拖拽行为：创建矩形选择框
 *    - 拖拽模式：RubberBandDrag
 *    - 用途：选择特定区域进行精确缩放
 *    - 适用场景：需要查看细节或局部分析
 *
 * 2. 手拖平移模式 (on=false):
 *    - 拖拽行为：移动视图内容
 *    - 拖拽模式：ScrollHandDrag
 *    - 用途：在放大状态下浏览不同区域
 *    - 适用场景：已放大状态下的内容导航
 *
 * 交互体验：
 * - 橡皮筋模式：适合精确的区域选择操作
 * - 手拖模式：适合流畅的内容浏览体验
 * - 可根据用户需求或应用状态动态切换
 *
 * 状态保存：
 * - m_rubberBandZoom标志记录当前模式
 * - 影响鼠标事件的处理逻辑
 * - 确保交互行为与设置一致
 *
 * @note 模式切换立即生效，无需重启或刷新视图
 */
void PPIView::enableRubberBandZoom(bool on) {
    m_rubberBandZoom = on;
    setDragMode(on ? QGraphicsView::RubberBandDrag : QGraphicsView::ScrollHandDrag);
}

/**
 * @brief 鼠标按下事件处理
 * @details 根据橡皮筋缩放模式处理鼠标左键按下事件，初始化选择框
 * @param e 鼠标事件对象，包含位置、按键、修饰键等信息
 *
 * 事件处理逻辑：
 * 1. 模式检查：只在橡皮筋缩放模式(m_rubberBandZoom=true)下处理
 * 2. 按键过滤：只响应鼠标左键(Qt::LeftButton)按下
 * 3. 初始化选择：记录起始位置，创建并显示选择框
 *
 * 橡皮筋选择框初始化：
 * - m_origin = e->pos(): 记录鼠标按下的起始位置（视图坐标）
 * - 创建QRubberBand对象：如果不存在则新建矩形类型选择框
 * - 设置初始几何：位置为起始点，大小为零（点状态）
 * - 显示选择框：调用show()使其可见，为后续拖拽做准备
 *
 * 坐标系统：
 * - e->pos()：相对于PPIView widget的像素坐标
 * - 坐标原点在视图左上角
 * - X轴向右为正，Y轴向下为正
 * - 此坐标用于橡皮筋框的widget层显示
 *
 * 选择框生命周期：
 * - 创建：首次使用时动态创建QRubberBand对象
 * - 重用：后续使用复用已创建的对象，提高性能
 * - 父子关系：设置this为父对象，确保正确的内存管理
 *
 * 交互准备：
 * - 为mouseMoveEvent提供起始参考点
 * - 为mouseReleaseEvent提供完整的选择区域信息
 * - 建立完整的拖拽选择事件序列
 *
 * @note 调用父类实现确保Qt内部状态和其他功能正常工作
 */
void PPIView::mousePressEvent(QMouseEvent* e) {
    if (m_measureMode && e->button() == Qt::LeftButton) {
        // 测距模式下的鼠标按下处理
        m_measuring = true;
        m_measureStart = mapToScene(e->pos());

        // 清除之前的测量显示
        clearMeasureLine();

        // 创建用于显示尺子的 path item
        QPen pen(Qt::yellow);
        pen.setWidthF(2.0);
        pen.setStyle(Qt::DashLine);

        m_measurePath = new QGraphicsPathItem();
        m_measurePath->setPen(pen);
        m_measurePath->setZValue(1000);
        m_scene->addItem(m_measurePath);

        // 起止圆点标记（空心圆环，更像尺子的端点）
        QPen markerPen(Qt::yellow);
        markerPen.setWidthF(1.5);
        QBrush noBrush(Qt::transparent);
        m_startMarker = new QGraphicsEllipseItem(-6, -6, 12, 12);
        m_startMarker->setBrush(noBrush);
        m_startMarker->setPen(markerPen);
        m_startMarker->setZValue(1001);
        m_scene->addItem(m_startMarker);

        m_endMarker = new QGraphicsEllipseItem(-6, -6, 12, 12);
        m_endMarker->setBrush(noBrush);
        m_endMarker->setPen(markerPen);
        m_endMarker->setZValue(1001);
        m_scene->addItem(m_endMarker);

        // 距离文本
        m_distanceText = m_scene->addText("");
        QFont f = m_distanceText->font();
        f.setPointSize(10);
        f.setBold(true);
        m_distanceText->setFont(f);
        m_distanceText->setDefaultTextColor(Qt::yellow);
        m_distanceText->setZValue(1002);

        e->accept();
        return;
    }

    if (m_rubberBandZoom && e->button()==Qt::LeftButton) {
        m_origin = e->pos();
        if (!m_band) m_band = new QRubberBand(QRubberBand::Rectangle, this);
        m_band->setGeometry(QRect(m_origin, QSize()));
        m_band->show();
    }
    QGraphicsView::mousePressEvent(e);
}

/**
 * @brief 鼠标移动事件处理
 * @details 在橡皮筋模式下实时更新选择框的大小和形状
 * @param e 鼠标事件对象，包含当前鼠标位置信息
 *
 * 事件处理条件：
 * 1. 橡皮筋模式激活：m_rubberBandZoom为true
 * 2. 选择框已创建：m_band对象存在（由mousePressEvent创建）
 * 3. 鼠标处于拖拽状态：从按下位置到当前位置的连续移动
 *
 * 选择框更新逻辑：
 * - 起始点：m_origin（鼠标按下时记录的位置）
 * - 终点：e->pos()（当前鼠标位置）
 * - 矩形构造：QRect(起始点, 终点)创建选择矩形
 * - 标准化：normalized()确保矩形坐标正确（处理反向拖拽）
 *
 * 标准化处理：
 * - 作用：处理从右下向左上拖拽的情况
 * - 原理：确保left≤right且top≤bottom
 * - 效果：无论拖拽方向如何，都能得到有效的矩形区域
 *
 * 实时反馈：
 * - 视觉效果：选择框实时跟随鼠标移动
 * - 用户体验：直观显示即将选择的区域范围
 * - 精确控制：用户可以精确调整选择区域的大小
 *
 * 性能考虑：
 * - 轻量级操作：只进行简单的几何计算和显示更新
 * - 即时响应：每次鼠标移动都能立即看到效果
 * - 无复杂计算：避免在移动过程中进行耗时操作
 *
 * @note 调用父类实现保持其他鼠标移动功能的正常工作
 */
void PPIView::mouseMoveEvent(QMouseEvent* e) {
    if (m_measureMode && m_measuring && m_measurePath) {
        QPointF currentPos = mapToScene(e->pos());

        // 计算主线和 ticks（参考ZoomView的实现）
        QLineF baseLine(m_measureStart, currentPos);
        QPainterPath path;
        path.moveTo(m_measureStart);
        path.lineTo(currentPos);

        // 计算距离与刻度（以真实单位：m 为基础）
        double totalLen = baseLine.length();
        double distanceMeters = totalLen; // fallback: pixels
        if (m_scene && m_scene->axis()) {
            distanceMeters = m_scene->axis()->pixelToRange(totalLen);
        }

        // 计算像素每米比例用于将刻度间隔从米转换为像素
        double pixelsPerMeter = 1.0;
        if (distanceMeters > 0.0) pixelsPerMeter = totalLen / distanceMeters;

        // 希望刻度数在 6..12 之间，选择一个"好看"的刻度间隔（1,2,5 x 10^n）
        const int desiredTicks = 8;
        double rawSpacingMeters = qMax(1.0, distanceMeters / double(desiredTicks));
        // nice step: 1,2,5 * 10^n
        double exp = floor(log10(rawSpacingMeters));
        double base = pow(10.0, exp);
        double f = rawSpacingMeters / base;
        double niceMant;
        if (f <= 1.0) niceMant = 1.0;
        else if (f <= 2.0) niceMant = 2.0;
        else if (f <= 5.0) niceMant = 5.0;
        else niceMant = 10.0;
        double spacingMeters = niceMant * base;
        double spacingPixels = spacingMeters * pixelsPerMeter;

        const double minorTickLen = 6.0;
        const double majorTickLen = 12.0;
        int tickCount = int(totalLen / spacingPixels);
        for (int i = 1; i <= tickCount; ++i) {
            double distPx = i * spacingPixels;
            if (distPx >= totalLen) break;
            double ratio = distPx / totalLen;
            QPointF pt = baseLine.pointAt(ratio);
            double angle = baseLine.angle() * M_PI / 180.0;
            double nx = -sin(angle);
            double ny = cos(angle);
            bool major = (i % 5 == 0);
            double tlen = major ? majorTickLen : minorTickLen;
            QPointF a = pt + QPointF(nx * tlen * 0.5, ny * tlen * 0.5);
            QPointF b = pt - QPointF(nx * tlen * 0.5, ny * tlen * 0.5);
            path.moveTo(a);
            path.lineTo(b);
        }

        m_measurePath->setPath(path);

        // 更新起止点标记位置
        if (m_startMarker) m_startMarker->setPos(m_measureStart);
        if (m_endMarker) m_endMarker->setPos(currentPos);

        // 更新距离文本，带单位显示
        if (m_distanceText) {
            QString text;
            if (distanceMeters >= 1000.0) text = QString("距离: %1 km").arg(distanceMeters/1000.0, 0, 'f', 1);
            else text = QString("距离: %1 m").arg(int(distanceMeters));
            m_distanceText->setPlainText(text);
            QPointF midPoint = (m_measureStart + currentPos) / 2;
            m_distanceText->setPos(midPoint + QPointF(8, -12));
        }

        e->accept();
        return;
    }

    if (m_rubberBandZoom && m_band) {
        m_band->setGeometry(QRect(m_origin, e->pos()).normalized());
    }

    // 更新鼠标位置信息
    if (mousePositionInfo && m_scene) {
        // 将视图坐标转换为场景坐标
        QPointF scenePos = mapToScene(e->pos());

        // 获取场景的极坐标轴
        if (PolarAxis* axis = m_scene->axis()) {
            // 将场景坐标转换为极坐标
            auto polarCoord = axis->sceneToPolar(scenePos);

            // 更新显示：距离转换为公里，保留1位小数
            double distanceKm = polarCoord.distance / 1000.0;
            mousePositionInfo->updatePosition(distanceKm, polarCoord.azimuthDeg);
        }
    }

    QGraphicsView::mouseMoveEvent(e);
}

/**
 * @brief 鼠标释放事件处理
 * @details 完成橡皮筋选择操作，处理选择区域并发出相应信号
 * @param e 鼠标事件对象，包含释放按键信息
 *
 * 事件处理条件：
 * 1. 橡皮筋模式激活：m_rubberBandZoom为true
 * 2. 选择框存在：m_band对象有效
 * 3. 左键释放：e->button()为Qt::LeftButton
 *
 * 选择区域处理流程：
 * 1. 隐藏选择框：调用m_band->hide()结束视觉选择过程
 * 2. 获取选择区域：提取标准化的矩形区域
 * 3. 尺寸验证：检查选择区域是否足够大（最小10x10像素）
 * 4. 坐标转换：从视图坐标转换为场景坐标
 * 5. 信号发射：通知其他组件用户的选择操作
 *
 * 最小尺寸验证：
 * - 目的：避免意外的微小选择导致不必要的操作
 * - 阈值：10x10像素，足以区分有意选择和意外点击
 * - 用户体验：防止误操作，确保选择意图明确
 *
 * 坐标系转换：
 * - 输入：视图坐标系的像素矩形(QRect sel)
 * - 转换：mapToScene(sel)将视图坐标映射到场景坐标
 * - 输出：场景坐标系的实数矩形(QRectF s)
 * - 用途：场景坐标用于后续的缩放和定位操作
 *
 * 信号通信：
 * - 信号：areaSelected(QRectF)
 * - 参数：选择区域的场景坐标矩形
 * - 接收者：需要响应用户选择的其他组件
 * - 用途：触发缩放、高亮、分析等后续操作
 *
 * 注释的功能：
 * - //fitInView(s, Qt::KeepAspectRatio): 自动缩放到选择区域
 * - 设计考虑：可能希望由外部组件控制缩放行为
 * - 灵活性：允许不同的响应策略（缩放、高亮、数据过滤等）
 *
 * @note 调用父类实现确保Qt内部事件处理的完整性
 */
void PPIView::mouseReleaseEvent(QMouseEvent* e) {
    if (m_measureMode && m_measuring && e->button() == Qt::LeftButton) {
        m_measuring = false;

        QPointF endPos = mapToScene(e->pos());
        double distancePixels = QLineF(m_measureStart, endPos).length();
        double distanceVal = distancePixels;
        if (m_scene && m_scene->axis()) {
            distanceVal = m_scene->axis()->pixelToRange(distancePixels);
        }
        emit distanceMeasured(distanceVal);
        // 保持显示，直到下次测量或切换模式

        e->accept();
        return;
    }

    if (m_rubberBandZoom && m_band && e->button()==Qt::LeftButton) {
        m_band->hide();
        QRect sel = m_band->geometry().normalized();
        if (sel.width()>10 && sel.height()>10) {
            QRectF s = mapToScene(sel).boundingRect();
             // 发出区域选择信号
            emit areaSelected(s);
            //fitInView(s, Qt::KeepAspectRatio);
        }
    }
    QGraphicsView::mouseReleaseEvent(e);
}

/**
 * @brief 视图大小改变事件处理
 * @details 响应窗口尺寸变化，调整覆盖层布局并重新适配场景显示
 * @param e 大小改变事件对象，包含新旧尺寸信息
 *
 * 事件处理流程：
 * 1. 调用父类处理：确保Qt内部状态正确更新
 * 2. 发出尺寸变化信号：通知其他组件视图尺寸已改变
 * 3. 重新布局覆盖层：调整覆盖组件的位置和大小
 * 4. 重新适配场景：保持场景内容完整可见且比例正确
 *
 * 信号通信：
 * - 信号：viewResized(QSize)
 * - 参数：e->size()新的视图尺寸
 * - 用途：通知其他组件可能需要根据新尺寸调整自身
 * - 场景：如状态栏、工具栏、其他视图等需要响应尺寸变化
 *
 * 覆盖层布局：
 * - 调用：layoutOverlay()重新计算覆盖组件位置
 * - 目的：确保覆盖层元素在新尺寸下正确显示
 * - 适配：根据新的视图尺寸调整组件布局
 *
 * 场景适配：
 * - 方法：fitInView(场景矩形, 保持宽高比)
 * - 效果：场景内容始终完整可见，不会因窗口变化而裁剪
 * - 比例：Qt::KeepAspectRatio保持原始宽高比，避免变形
 * - 用户体验：无论窗口如何变化，内容都能正确显示
 *
 * 性能考虑：
 * - 顺序优化：先处理布局再进行适配，避免重复计算
 * - 调试信息：注释的qDebug()可用于开发期间监控尺寸变化
 * - 即时响应：每次尺寸变化都立即调整，保持视觉连续性
 *
 * 使用场景：
 * - 窗口拖拽改变大小时
 * - 最大化/最小化窗口时
 * - 分屏或多显示器切换时
 * - 程序窗口布局动态调整时
 *
 * @note 事件处理确保在各种尺寸变化情况下都能保持正确的显示效果
 */
void PPIView::resizeEvent(QResizeEvent* e) {
    // 当尺寸改变时，发出信号
    //  qDebug() << e->size();
    QGraphicsView::resizeEvent(e);
    emit viewResized(e->size());

    layoutOverlay();
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

    // PPIView尺寸变化可能影响其在MainWindow中的相对位置，需要重新计算地图显示参数
    double mapCenterLng, mapCenterLat, mapRange;
    calculateMapDisplayParameters(mapCenterLng, mapCenterLat, mapRange);
    emit radarCenterChanged(mapCenterLng, mapCenterLat, mapRange);
}

/**
 * @brief 处理最大距离变化
 * @param distance 新的最大距离值（公里）
 * @details 响应PPIVisualSettings组件的距离变化，更新PolarAxis的距离范围
 */
void PPIView::onMaxDistanceChanged(double distance)
{
    // 更新内部范围记录
    m_currentRange = distance;

    if (m_scene && m_scene->axis()) {
        // 将公里转换为米（PolarAxis使用米作为单位）
        double maxRangeInMeters = distance * 1000.0;
        double currentMinRange = m_scene->axis()->minRange();

        // 设置新的距离范围
        m_scene->axis()->setRange(currentMinRange, maxRangeInMeters);

        // 触发场景更新
        m_scene->updateSceneSize(size());

        // 重新适应视图
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    // 发出雷达中心位置变化信号，通知地图组件更新范围
    // 使用计算的地图显示参数而不是直接的雷达位置
    double mapCenterLng, mapCenterLat, mapRange;
    calculateMapDisplayParameters(mapCenterLng, mapCenterLat, mapRange);
    emit radarCenterChanged(mapCenterLng, mapCenterLat, mapRange);

    // 转发信号给其他需要处理距离变化的组件
    emit maxDistanceChanged(distance);

    LOG_INFO(QString("PPI range update: %1km, Radar pos %2,%3")
            .arg(distance).arg(m_radarLongitude).arg(m_radarLatitude));
}

/**
 * @brief 处理地图类型变化
 * @param index 新的地图类型索引
 * @details 响应PPIVisualSettings组件的地图类型变化，转发信号给地图组件
 */
void PPIView::onMapTypeChanged(int index)
{
    // 转发信号给地图组件处理
    emit mapTypeChanged(index);
}

/**
 * @brief 处理测距模式切换
 * @param enabled 是否启用测距模式
 * @details 切换PPI视图的交互模式，在普通拖拽模式和测距模式之间切换
 */
void PPIView::onMeasureModeChanged(bool enabled)
{
    m_measureMode = enabled;

    if (enabled) {
        // 启用测距模式
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
        clearMeasureLine();  // 清除之前的测距线
    } else {
        // 恢复普通模式
        setDragMode(m_rubberBandZoom ? QGraphicsView::RubberBandDrag : QGraphicsView::ScrollHandDrag);
        setCursor(Qt::ArrowCursor);
        clearMeasureLine();  // 清除测距线
    }
}

/**
 * @brief 清除测距线
 * @details 清除当前显示的测距图形元素，包括路径、标记和文本
 */
void PPIView::clearMeasureLine()
{
    if (!m_scene) return;

    if (m_measurePath) {
        m_scene->removeItem(m_measurePath);
        delete m_measurePath;
        m_measurePath = nullptr;
    }
    if (m_startMarker) {
        m_scene->removeItem(m_startMarker);
        delete m_startMarker;
        m_startMarker = nullptr;
    }
    if (m_endMarker) {
        m_scene->removeItem(m_endMarker);
        delete m_endMarker;
        m_endMarker = nullptr;
    }
    if (m_distanceText) {
        m_scene->removeItem(m_distanceText);
        delete m_distanceText;
        m_distanceText = nullptr;
    }
}

/**
 * @brief 设置雷达中心经纬度
 * @param longitude 经度
 * @param latitude 纬度
 * @details 设置雷达中心的地理位置，并发出信号通知地图同步
 */
void PPIView::setRadarCenter(double longitude, double latitude)
{
    m_radarLongitude = longitude;
    m_radarLatitude = latitude;

    // 发出雷达中心位置变化信号，使用计算的地图显示参数
    double mapCenterLng, mapCenterLat, mapRange;
    calculateMapDisplayParameters(mapCenterLng, mapCenterLat, mapRange);
    emit radarCenterChanged(mapCenterLng, mapCenterLat, mapRange);

    LOG_INFO(QString("Radar center update: %1,%2, range %3km, Map center %4,%5, Map range %6km")
            .arg(longitude).arg(latitude).arg(m_currentRange)
            .arg(mapCenterLng).arg(mapCenterLat).arg(mapRange));
}
