/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:32
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-28
 * @Description: 距离-方位图表显示组件实现
 */

#include "rangeazimuthchart.h"
#include "../Basic/ConfigManager.h"
#include "../Basic/DispBasci.h"
#include "../Basic/log.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QtMath>

//==============================================================================
// RangeAzimuthChartToolBar 实现
//==============================================================================

RangeAzimuthChartToolBar::RangeAzimuthChartToolBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RangeAzimuthChartToolBar");
    // 让 QSS background-color 生效
    setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

    // 方位角范围控制
    QLabel* azimuthLabel = new QLabel(tr("方位角:"), this);
    azimuthLabel->setObjectName("RangeAzimuthLabel");
    layout->addWidget(azimuthLabel);

    m_minAzimuthEdit = new QLineEdit(this);
    m_minAzimuthEdit->setObjectName("RangeAzimuthMinEdit");
    m_minAzimuthEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_minAzimuthEdit->setFixedWidth(ScaleHelper::uiScaled(60));
    m_minAzimuthEdit->setToolTip(tr("最小方位角 (0-360°)"));
    m_minAzimuthEdit->setText(QString::number(CF_INS.rangeAzimuthAngle("min", 0)));
    layout->addWidget(m_minAzimuthEdit);

    QLabel* separatorLabel = new QLabel("~", this);
    separatorLabel->setObjectName("RangeAzimuthSeparatorLabel");
    layout->addWidget(separatorLabel);

    m_maxAzimuthEdit = new QLineEdit(this);
    m_maxAzimuthEdit->setObjectName("RangeAzimuthMaxEdit");
    m_maxAzimuthEdit->setFixedWidth(ScaleHelper::uiScaled(60));
    m_maxAzimuthEdit->setToolTip(tr("最大方位角 (0-360°)"));
    m_maxAzimuthEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_maxAzimuthEdit->setText(QString::number(CF_INS.rangeAzimuthAngle("max", 360)));
    layout->addWidget(m_maxAzimuthEdit);

    // 连接方位角范围变化信号
    connect(m_minAzimuthEdit, &QLineEdit::returnPressed, this, &RangeAzimuthChartToolBar::onAzimuthRangeChanged);
    connect(m_maxAzimuthEdit, &QLineEdit::returnPressed, this, &RangeAzimuthChartToolBar::onAzimuthRangeChanged);

    layout->addStretch();


    // 清除按钮
    m_clearButton = new QPushButton(tr("清除"), this);
    m_clearButton->setObjectName("RangeAzimuthClearButton");
    //m_clearButton->setFixedWidth(80);
    layout->addWidget(m_clearButton);
    connect(m_clearButton, &QPushButton::clicked, this, &RangeAzimuthChartToolBar::clearRequested);

    // 重置按钮
    m_resetButton = new QPushButton(tr("重置"), this);
    m_resetButton->setObjectName("RangeAzimuthResetButton");
   // m_resetButton->setFixedWidth(80);
    layout->addWidget(m_resetButton);
    connect(m_resetButton, &QPushButton::clicked, this, &RangeAzimuthChartToolBar::resetRequested);
}

double RangeAzimuthChartToolBar::getMinAzimuth() const
{
    bool ok;
    double value = m_minAzimuthEdit->text().toDouble(&ok);
    return ok ? value : 0.0;
}

double RangeAzimuthChartToolBar::getMaxAzimuth() const
{
    bool ok;
    double value = m_maxAzimuthEdit->text().toDouble(&ok);
    return ok ? value : 360.0;
}

void RangeAzimuthChartToolBar::updateStatus(int detectionCount, int trackCount)
{
    //m_statusLabel->setText(QString(tr("检测点: %1 | 航迹: %2")).arg(detectionCount).arg(trackCount));
}

void RangeAzimuthChartToolBar::onAzimuthRangeChanged()
{
    double minAz = getMinAzimuth();
    double maxAz = getMaxAzimuth();

    // 验证范围
    if (minAz < 0 || minAz >= 360) {
        m_minAzimuthEdit->setText("0");
        minAz = 0;
    }
    if (maxAz <= 0 || maxAz > 360) {
        m_maxAzimuthEdit->setText("360");
        maxAz = 360;
    }
    if (minAz >= maxAz) {
        m_minAzimuthEdit->setText("0");
        m_maxAzimuthEdit->setText("360");
        minAz = 0;
        maxAz = 360;
    }

    emit azimuthRangeChanged(minAz, maxAz);
}

//==============================================================================
// RangeAzimuthChart 实现
//==============================================================================

RangeAzimuthChart::RangeAzimuthChart(QWidget* parent)
    : CustomLineChart(parent)
    , m_minAzimuth(0.0)
    , m_maxAzimuth(360.0)
{
    setObjectName("RangeAzimuthChart");

    // 移除边框
    setFrameShape(QFrame::NoFrame);
    setFrameStyle(QFrame::NoFrame);

    // 配置X轴（方位角）
    ChartAxisConfig xAxis;
    xAxis.minValue = 0;
    xAxis.maxValue = 360;
    xAxis.majorTickInterval = 45;   // 主刻度：45°
    xAxis.minorTickInterval = 15;   // 副刻度：15°
    xAxis.label = tr("方位角（°）");
    xAxis.unit = "";                // 单位已包含在label中，刻度不再显示
    setXAxisConfig(xAxis);

    // 配置Y轴（距离）- 使用km单位
    ChartAxisConfig yAxis;
    yAxis.minValue = 0;
    yAxis.maxValue = 5;              // 默认5公里
    yAxis.majorTickInterval = 1;     // 主刻度：1公里
    yAxis.minorTickInterval = 0.5;   // 副刻度：0.5公里
    yAxis.label = tr("距离 (km)");
    yAxis.unit = "";                 // 单位已包含在label中，刻度不再显示
    setYAxisConfig(yAxis);

    // 启用网格和坐标轴
    setGridVisible(true);
    setAxisVisible(true);

    // ---------- 黑绿配色主题（与 darkstyle.qss 保持一致）----------
    // 背景：深黑
    setChartBgColor(QColor(10, 16, 16));
    // 主网格线：低饱和深绿
    setChartGridMajorColor(QColor(0, 80, 60));
    // 次网格线：更暗的绿
    setChartGridMinorColor(QColor(0, 50, 38));
    // 坐标轴线：绿色
    setChartAxisColor(QColor(0, 255, 136));
    // 刻度/标签文字：青绿色
    setChartTextColor(QColor(102, 255, 204));
}

RangeAzimuthChart::~RangeAzimuthChart()
{
    clearRadarData();
}

void RangeAzimuthChart::addDetectionPoint(const PointInfo& info)
{
    // 提取方位角和距离
    double azimuth = info.azimuth;
    double range = info.range / 1000.0;  // 转换为km用于坐标系

    // 将数据坐标转换为场景坐标
    QPointF scenePos = dataToScene(azimuth, range);

    // 计算实际点大小
    double size = m_baseDetectionSize * m_detectionSizeRatio;

    // 创建图形项
    QGraphicsEllipseItem* item = new QGraphicsEllipseItem(
        scenePos.x() - size/2,
        scenePos.y() - size/2,
        size,
        size
    );
    item->setPen(Qt::NoPen);
    item->setBrush(QBrush(m_detectionColor));

    // 设置tooltip（与PPI检测点格式完全一致）
    QString tooltip = QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
        .arg(DET_LABEL)
        .arg(info.range, 0, 'f', 1)
        .arg(info.azimuth, 0, 'f', 1)
        .arg(info.elevation, 0, 'f', 1)
        .arg(info.SNR, 0, 'f', 1)
        .arg(info.speed, 0, 'f', 1)
        .arg(info.altitute, 0, 'f', 1)
        .arg(info.amp);
    item->setToolTip(tooltip);

    // 启用hover事件
    item->setAcceptHoverEvents(true);

    scene()->addItem(item);

    // 检查是否在显示范围内
    ChartAxisConfig yAxis = yAxisConfig();
    bool inAzimuthRange = isAzimuthInRange(azimuth);
    bool inDistanceRange = (range >= yAxis.minValue && range <= yAxis.maxValue);
    bool shouldShow = m_detectionVisible && inAzimuthRange && inDistanceRange;

    item->setVisible(shouldShow);

    // 存储到检测点列表
    DetectionItem detItem;
    detItem.graphicsItem = item;
    detItem.info = info;
    detItem.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_detections.append(detItem);

    // 限制点数
    limitDetectionPoints();

    // 更新统计
    updatePointCount();
}

void RangeAzimuthChart::addTrackPoint(const trackInfo& info)
{
    // 提取方位角和距离（trackInfo 使用 azi 和 dis）
    double azimuth = info.azi;
    double range = info.dis / 1000.0;  // 转换为km用于坐标系

    // 将数据坐标转换为场景坐标
    QPointF scenePos = dataToScene(azimuth, range);

    // 计算实际点大小
    double size = m_baseTrackSize * m_trackSizeRatio;

    // 创建图形项
    QGraphicsEllipseItem* item = new QGraphicsEllipseItem(
        scenePos.x() - size/2,
        scenePos.y() - size/2,
        size,
        size
    );
    item->setPen(QPen(m_trackColor, 1));
    item->setBrush(QBrush(m_trackColor));

    // 设置tooltip（与PPI航迹点格式一致）
    QString tooltip = QString("%1\nID:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
        .arg(TRA_LABEL)
        .arg(info.batch)
        .arg(info.dis, 0, 'f', 1)
        .arg(info.azi, 0, 'f', 1)
        .arg(info.ele, 0, 'f', 1)
        .arg(info.SNR, 0, 'f', 1)
        .arg(info.vel, 0, 'f', 1)
        .arg(info.altitute, 0, 'f', 1)
        .arg(info.amp);
    item->setToolTip(tooltip);

    // 启用hover事件
    item->setAcceptHoverEvents(true);

    scene()->addItem(item);

    // 检查是否在显示范围内
    ChartAxisConfig yAxis = yAxisConfig();
    bool inAzimuthRange = isAzimuthInRange(azimuth);
    bool inDistanceRange = (range >= yAxis.minValue && range <= yAxis.maxValue);
    bool shouldShow = m_trackVisible && inAzimuthRange && inDistanceRange;

    item->setVisible(shouldShow);

    // 存储到列表
    TrackItem trackItem;
    trackItem.graphicsItem = item;
    trackItem.trackData = info;
    trackItem.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_tracks.append(trackItem);

    // 限制航迹数
    limitTrackPoints();

    // 更新统计
    updatePointCount();
}

void RangeAzimuthChart::addPointInfo(const PointInfo& info)
{
    // statMethod==2 表示消批命令，不添加新点（批次已由 removeBatch 删除）
    if (info.statMethod == 2) {
        LOG_INFO(QString("[RangeAzimuthChart::addPointInfo] statMethod==2 ignored (batch=%1)").arg(info.batch));
        return;
    }

    // 根据 type 字段判断点类型并分发
    if (info.type == Detection) {
        // 检测点
        addDetectionPoint(info);
    } else if (info.type == Track || info.type == TBDPointType) {
        // 航迹或TBD航迹 - 需要转换为 trackInfo 格式
        // 注意：PointInfo 和 trackInfo 字段名不同
        // PointInfo: azimuth, range
        // trackInfo: azi, dis
        // 这里直接使用 PointInfo 的数据绘制

        // 提取方位角和距离
        double azimuth = info.azimuth;
        double range = info.range / 1000.0;  // 转换为km用于坐标系

        // 将数据坐标转换为场景坐标
        QPointF scenePos = dataToScene(azimuth, range);

        // 计算实际点大小
        double size = m_baseTrackSize * m_trackSizeRatio;

        // 创建图形项
        QGraphicsEllipseItem* item = new QGraphicsEllipseItem(
            scenePos.x() - size/2,
            scenePos.y() - size/2,
            size,
            size
        );
        item->setPen(QPen(m_trackColor, 1));
        item->setBrush(QBrush(m_trackColor));

        // 设置tooltip（与PPI格式一致）
        QString typeStr = (info.type == Track) ? TRA_LABEL : "TBD航迹";
        QString tooltip = QString("%1\nID:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
            .arg(typeStr)
            .arg(info.batch)
            .arg(info.range, 0, 'f', 1)
            .arg(info.azimuth, 0, 'f', 1)
            .arg(info.elevation, 0, 'f', 1)
            .arg(info.SNR, 0, 'f', 1)
            .arg(info.speed, 0, 'f', 1)
            .arg(info.altitute, 0, 'f', 1)
            .arg(info.amp);
        item->setToolTip(tooltip);

        // 启用hover事件
        item->setAcceptHoverEvents(true);

        scene()->addItem(item);

        // 检查是否在显示范围内
        ChartAxisConfig yAxis = yAxisConfig();
        bool inAzimuthRange = isAzimuthInRange(azimuth);
        bool inDistanceRange = (range >= yAxis.minValue && range <= yAxis.maxValue);
        bool shouldShow = m_trackVisible && inAzimuthRange && inDistanceRange;

        item->setVisible(shouldShow);

        // 存储到列表（使用伪造的trackInfo）
        TrackItem trackItem;
        trackItem.graphicsItem = item;
        // 创建一个临时的 trackInfo 来存储
        trackInfo track;
        track.azi = info.azimuth;
        track.dis = info.range;
        track.batch = info.batch;
        trackItem.trackData = track;
        trackItem.timestamp = QDateTime::currentMSecsSinceEpoch();

        m_tracks.append(trackItem);

        // 限制航迹数
        limitTrackPoints();

        // 更新统计
        updatePointCount();
    }
}

void RangeAzimuthChart::removeBatch(int batchID)
{
    int removedCount = 0;
    // 从后向前遍历，删除匹配的航迹
    for (int i = m_tracks.size() - 1; i >= 0; --i) {
        if (m_tracks[i].trackData.batch == batchID) {
            // 从场景中移除图形项
            if (m_tracks[i].graphicsItem) {
                scene()->removeItem(m_tracks[i].graphicsItem);
                delete m_tracks[i].graphicsItem;
            }
            // 从列表中移除
            m_tracks.removeAt(i);
            ++removedCount;
        }
    }

    LOG_INFO(QString("[RangeAzimuthChart::removeBatch] batch=%1 removed %2 track items, remaining tracks=%3")
             .arg(batchID).arg(removedCount).arg(m_tracks.size()));

    // 更新统计显示
    updatePointCount();
}

void RangeAzimuthChart::clearRadarData()
{
    // 清除所有检测点图形项
    for (const DetectionItem& item : m_detections) {
        if (item.graphicsItem) {
            scene()->removeItem(item.graphicsItem);
            delete item.graphicsItem;
        }
    }
    m_detections.clear();

    // 清除所有航迹图形项
    for (const TrackItem& item : m_tracks) {
        if (item.graphicsItem) {
            scene()->removeItem(item.graphicsItem);
            delete item.graphicsItem;
        }
    }
    m_tracks.clear();

    // 更新统计
    updatePointCount();
}

void RangeAzimuthChart::setRangeFromMain(double minRange, double maxRange)
{
    // 输入是米，转换为km
    double minRangeKm = minRange / 1000.0;
    double maxRangeKm = maxRange / 1000.0;

    // 更新Y轴配置
    ChartAxisConfig yAxis = yAxisConfig();
    yAxis.minValue = minRangeKm;
    yAxis.maxValue = maxRangeKm;

    // 根据距离范围自动调整刻度间隔（km单位）
    double rangeSpan = maxRangeKm - minRangeKm;
    if (rangeSpan <= 1) {
        yAxis.majorTickInterval = 0.2;
        yAxis.minorTickInterval = 0.1;
    } else if (rangeSpan <= 5) {
        yAxis.majorTickInterval = 1;
        yAxis.minorTickInterval = 0.5;
    } else if (rangeSpan <= 10) {
        yAxis.majorTickInterval = 2;
        yAxis.minorTickInterval = 1;
    } else {
        yAxis.majorTickInterval = 5;
        yAxis.minorTickInterval = 1;
    }

    setYAxisConfig(yAxis);

    // 重新绘制所有点（因为坐标系改变了）
    // 清除现有图形项
    for (DetectionItem& detItem : m_detections) {
        if (detItem.graphicsItem) {
            scene()->removeItem(detItem.graphicsItem);
            delete detItem.graphicsItem;
            detItem.graphicsItem = nullptr;
        }
    }
    for (TrackItem& item : m_tracks) {
        if (item.graphicsItem) {
            scene()->removeItem(item.graphicsItem);
            delete item.graphicsItem;
            item.graphicsItem = nullptr;
        }
    }

    // 重新创建检测点图形项
    for (DetectionItem& detItem : m_detections) {
        const PointInfo& info = detItem.info;
        double rangeKm = info.range / 1000.0;  // 转换为km

        QPointF scenePos = dataToScene(info.azimuth, rangeKm);
        double size = m_baseDetectionSize * m_detectionSizeRatio;

        QGraphicsEllipseItem* item = new QGraphicsEllipseItem(
            scenePos.x() - size/2,
            scenePos.y() - size/2,
            size,
            size
        );
        item->setPen(Qt::NoPen);
        item->setBrush(QBrush(m_detectionColor));

        // 设置tooltip（与PPI格式一致）
        QString tooltip = QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
            .arg(DET_LABEL)
            .arg(info.range, 0, 'f', 1)
            .arg(info.azimuth, 0, 'f', 1)
            .arg(info.elevation, 0, 'f', 1)
            .arg(info.SNR, 0, 'f', 1)
            .arg(info.speed, 0, 'f', 1)
            .arg(info.altitute, 0, 'f', 1)
            .arg(info.amp);
        item->setToolTip(tooltip);
        item->setAcceptHoverEvents(true);

        // 检查是否应该显示（使用km比较）
        bool inAzimuthRange = isAzimuthInRange(info.azimuth);
        bool inDistanceRange = (rangeKm >= minRangeKm && rangeKm <= maxRangeKm);
        bool shouldShow = m_detectionVisible && inAzimuthRange && inDistanceRange;
        item->setVisible(shouldShow);

        scene()->addItem(item);

        detItem.graphicsItem = item;
    }

    // 重新创建航迹图形项
    for (TrackItem& item : m_tracks) {
        const trackInfo& track = item.trackData;
        double rangeKm = track.dis / 1000.0;  // 转换为km

        QPointF scenePos = dataToScene(track.azi, rangeKm);
        double size = m_baseTrackSize * m_trackSizeRatio;

        QGraphicsEllipseItem* graphicsItem = new QGraphicsEllipseItem(
            scenePos.x() - size/2,
            scenePos.y() - size/2,
            size,
            size
        );
        graphicsItem->setPen(QPen(m_trackColor, 1));
        graphicsItem->setBrush(QBrush(m_trackColor));

        // 设置tooltip（与PPI格式一致）
        QString tooltip = QString("%1\nID:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
            .arg(TRA_LABEL)
            .arg(track.batch)
            .arg(track.dis, 0, 'f', 1)
            .arg(track.azi, 0, 'f', 1)
            .arg(track.ele, 0, 'f', 1)
            .arg(track.SNR, 0, 'f', 1)
            .arg(track.vel, 0, 'f', 1)
            .arg(track.altitute, 0, 'f', 1)
            .arg(track.amp);
        graphicsItem->setToolTip(tooltip);
        graphicsItem->setAcceptHoverEvents(true);

        // 检查是否应该显示（使用km比较）
        bool inAzimuthRange = isAzimuthInRange(track.azi);
        bool inDistanceRange = (rangeKm >= minRangeKm && rangeKm <= maxRangeKm);
        bool shouldShow = m_trackVisible && inAzimuthRange && inDistanceRange;
        graphicsItem->setVisible(shouldShow);

        scene()->addItem(graphicsItem);

        item.graphicsItem = graphicsItem;
    }
}

void RangeAzimuthChart::setDetectionVisible(bool visible)
{
    m_detectionVisible = visible;

    // 更新所有检测点的可见性（同时考虑范围过滤）
    ChartAxisConfig yAxis = yAxisConfig();
    for (const DetectionItem& detItem : m_detections) {
        if (detItem.graphicsItem) {
            const PointInfo& info = detItem.info;
            double rangeKm = info.range / 1000.0;  // 转换为km
            bool inAzimuthRange = isAzimuthInRange(info.azimuth);
            bool inDistanceRange = (rangeKm >= yAxis.minValue && rangeKm <= yAxis.maxValue);
            bool shouldShow = visible && inAzimuthRange && inDistanceRange;
            detItem.graphicsItem->setVisible(shouldShow);
        }
    }
}

void RangeAzimuthChart::setTrackVisible(bool visible)
{
    m_trackVisible = visible;

    // 更新所有航迹的可见性（同时考虑范围过滤）
    ChartAxisConfig yAxis = yAxisConfig();
    for (const TrackItem& item : m_tracks) {
        if (item.graphicsItem) {
            const trackInfo& track = item.trackData;
            double rangeKm = track.dis / 1000.0;  // 转换为km
            bool inAzimuthRange = isAzimuthInRange(track.azi);
            bool inDistanceRange = (rangeKm >= yAxis.minValue && rangeKm <= yAxis.maxValue);
            bool shouldShow = visible && inAzimuthRange && inDistanceRange;
            item.graphicsItem->setVisible(shouldShow);
        }
    }
}

void RangeAzimuthChart::setDetectionSizeRatio(double ratio)
{
    m_detectionSizeRatio = qBound(0.5, ratio, 3.0);

    // 更新所有检测点的大小
    for (const DetectionItem& detItem : m_detections) {
        if (detItem.graphicsItem) {
            double size = m_baseDetectionSize * m_detectionSizeRatio;
            double rangeKm = detItem.info.range / 1000.0;  // 转换为km
            QPointF scenePos = dataToScene(detItem.info.azimuth, rangeKm);
            detItem.graphicsItem->setRect(
                scenePos.x() - size/2,
                scenePos.y() - size/2,
                size,
                size
            );
        }
    }
}

void RangeAzimuthChart::setTrackSizeRatio(double ratio)
{
    m_trackSizeRatio = qBound(0.5, ratio, 3.0);

    // 更新所有航迹的大小
    for (const TrackItem& item : m_tracks) {
        if (item.graphicsItem) {
            double size = m_baseTrackSize * m_trackSizeRatio;
            double rangeKm = item.trackData.dis / 1000.0;  // 转换为km
            QPointF scenePos = dataToScene(item.trackData.azi, rangeKm);
            item.graphicsItem->setRect(
                scenePos.x() - size/2,
                scenePos.y() - size/2,
                size,
                size
            );
        }
    }
}

void RangeAzimuthChart::setMaxDetectionPoints(int maxPoints)
{
    LOG_INFO(QString("[RangeAzimuthChart::setMaxDetectionPoints] max=%1, current detections=%2")
             .arg(maxPoints).arg(m_detections.size()));
    m_maxDetectionPoints = maxPoints;
    limitDetectionPoints();
    LOG_INFO(QString("[RangeAzimuthChart::setMaxDetectionPoints] after limit: detections=%1")
             .arg(m_detections.size()));
}

void RangeAzimuthChart::setMaxTrackPoints(int maxTracks)
{
    m_maxTrackPoints = maxTracks;
    limitTrackPoints();
}

void RangeAzimuthChart::limitDetectionPoints()
{
    // FIFO：删除最旧的点（列表头部是最旧的）
    while (m_detections.size() > m_maxDetectionPoints) {
        DetectionItem& oldest = m_detections.first();
        if (oldest.graphicsItem) {
            scene()->removeItem(oldest.graphicsItem);
            delete oldest.graphicsItem;
        }
        m_detections.removeFirst();
    }
}

void RangeAzimuthChart::limitTrackPoints()
{
    // FIFO：删除最旧的航迹
    while (m_tracks.size() > m_maxTrackPoints) {
        TrackItem& oldest = m_tracks.first();
        if (oldest.graphicsItem) {
            scene()->removeItem(oldest.graphicsItem);
            delete oldest.graphicsItem;
        }
        m_tracks.removeFirst();
    }
}

void RangeAzimuthChart::updatePointCount()
{
    emit pointCountChanged(m_detections.size(), m_tracks.size());
}

void RangeAzimuthChart::resizeEvent(QResizeEvent* event)
{
    // 基类处理：更新 sceneRect 并重绘网格/坐标轴
    CustomLineChart::resizeEvent(event);
    // 基类 rebuild() 完成后，重新计算雷达数据点的场景坐标
    refreshAllPoints();
}

void RangeAzimuthChart::refreshAllPoints()
{
    ChartAxisConfig xAxis = xAxisConfig();
    ChartAxisConfig yAxis = yAxisConfig();

    // 更新检测点位置和可见性
    for (const DetectionItem& detItem : m_detections) {
        if (detItem.graphicsItem) {
            const PointInfo& info = detItem.info;
            double azimuth = info.azimuth;
            double rangeKm = info.range / 1000.0;  // 转换为km

            // 重新计算场景位置
            QPointF scenePos = dataToScene(azimuth, rangeKm);
            double size = m_baseDetectionSize * m_detectionSizeRatio;
            detItem.graphicsItem->setRect(scenePos.x() - size/2, scenePos.y() - size/2, size, size);

            // 更新可见性
            bool inAzimuthRange = isAzimuthInRange(azimuth);
            bool inDistanceRange = (rangeKm >= yAxis.minValue && rangeKm <= yAxis.maxValue);
            bool shouldShow = m_detectionVisible && inAzimuthRange && inDistanceRange;
            detItem.graphicsItem->setVisible(shouldShow);
        }
    }

    // 更新航迹位置和可见性
    for (TrackItem& item : m_tracks) {
        if (item.graphicsItem) {
            const trackInfo& track = item.trackData;
            double azimuth = track.azi;
            double rangeKm = track.dis / 1000.0;  // 转换为km

            // 重新计算场景位置
            QPointF scenePos = dataToScene(azimuth, rangeKm);
            double size = m_baseTrackSize * m_trackSizeRatio;
            item.graphicsItem->setRect(scenePos.x() - size/2, scenePos.y() - size/2, size, size);

            // 更新可见性
            bool inAzimuthRange = isAzimuthInRange(azimuth);
            bool inDistanceRange = (rangeKm >= yAxis.minValue && rangeKm <= yAxis.maxValue);
            bool shouldShow = m_trackVisible && inAzimuthRange && inDistanceRange;
            item.graphicsItem->setVisible(shouldShow);
        }
    }

    updatePointCount();
}

void RangeAzimuthChart::setAzimuthRange(double minAz, double maxAz)
{
    m_minAzimuth = minAz;
    m_maxAzimuth = maxAz;

    // 更新X轴配置
    ChartAxisConfig xAxis = xAxisConfig();
    xAxis.minValue = minAz;
    xAxis.maxValue = maxAz;
    // 根据范围动态调整刻度间隔
    double range = maxAz - minAz;
    if (range <= 90) {
        xAxis.majorTickInterval = 15;
        xAxis.minorTickInterval = 5;
    } else if (range <= 180) {
        xAxis.majorTickInterval = 30;
        xAxis.minorTickInterval = 10;
    } else {
        xAxis.majorTickInterval = 45;
        xAxis.minorTickInterval = 15;
    }
    setXAxisConfig(xAxis);

    // 保存配置
    CF_INS.setRangeAzimuthAngle("min", minAz);
    CF_INS.setRangeAzimuthAngle("max", maxAz);

    // 刷新所有点的位置和可见性
    refreshAllPoints();

    qDebug() << "[RangeAzimuthChart::setAzimuthRange] Updated to" << minAz << "~" << maxAz;
}

bool RangeAzimuthChart::isAzimuthInRange(double azimuth) const
{
    // 处理跨越0°的情况
    if (m_minAzimuth <= m_maxAzimuth) {
        // 正常情况：例如 30° - 120°
        return azimuth >= m_minAzimuth && azimuth <= m_maxAzimuth;
    } else {
        // 跨越0°的情况：例如 330° - 30°
        return azimuth >= m_minAzimuth || azimuth <= m_maxAzimuth;
    }
}

//==============================================================================
// RangeAzimuthChartWidget 实现
//==============================================================================

RangeAzimuthChartWidget::RangeAzimuthChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RangeAzimuthChartWidget");
    // 让 QSS background-color 对 QWidget 生效
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建工具栏
    m_toolbar = new RangeAzimuthChartToolBar(this);
    layout->addWidget(m_toolbar);

    // 创建图表
    m_chart = new RangeAzimuthChart(this);
    layout->addWidget(m_chart, 1);  // 拉伸因子为1

    // 连接信号
    connect(m_toolbar, &RangeAzimuthChartToolBar::clearRequested,
            this, &RangeAzimuthChartWidget::onClearRequested);
    connect(m_toolbar, &RangeAzimuthChartToolBar::resetRequested,
            this, &RangeAzimuthChartWidget::onResetRequested);
    connect(m_toolbar, &RangeAzimuthChartToolBar::azimuthRangeChanged,
            m_chart, &RangeAzimuthChart::setAzimuthRange);

    // 连接点数统计更新到工具栏
    connect(m_chart, &RangeAzimuthChart::pointCountChanged,
            m_toolbar, &RangeAzimuthChartToolBar::updateStatus);
}

RangeAzimuthChartWidget::~RangeAzimuthChartWidget()
{
}

void RangeAzimuthChartWidget::onClearRequested()
{
    qDebug() << "[RangeAzimuthChartWidget::onClearRequested] Clear button clicked";
    m_chart->clearRadarData();
}

void RangeAzimuthChartWidget::onResetRequested()
{
    qDebug() << "[RangeAzimuthChartWidget::onResetRequested] Reset button clicked";

    // 重置距离范围到默认状态（0-5km）
    m_chart->setRangeFromMain(0, 5000);

    // 重置方位角范围到默认状态（0-360°）
    m_chart->setAzimuthRange(0, 360);

    // 注意：重置不清除数据，只是恢复默认视图范围
    // 数据会在 setRangeFromMain 和 setAzimuthRange 中被重新排布

    qDebug() << "[RangeAzimuthChartWidget::onResetRequested] Reset completed";
}
