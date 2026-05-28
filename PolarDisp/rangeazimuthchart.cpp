/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-28 17:21:47
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
#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSimpleTextItem>
#include <QHBoxLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QSet>
#include <QToolTip>
#include <QVarLengthArray>
#include <QWidget>
#include <QtMath>

namespace {

constexpr qreal kTrackLabelOffsetX = 8.0;
constexpr qreal kTrackLabelOffsetY = -18.0;
constexpr int kDistanceMajorTickCount = 5;
constexpr int kDistanceMinorTicksPerMajor = 2;

quint64 makeTrackLabelKey(unsigned type, int batch)
{
    return (static_cast<quint64>(type) << 32) | static_cast<quint32>(batch);
}

void applyDistanceAxisTicks(ChartAxisConfig& axis, double minRangeKm, double maxRangeKm)
{
    const double span = maxRangeKm - minRangeKm;
    if (span <= 0.0) {
        axis.majorTickInterval = 1.0;
        axis.minorTickInterval = 0.5;
        return;
    }

    axis.majorTickInterval = span / kDistanceMajorTickCount;
    axis.minorTickInterval = axis.majorTickInterval / kDistanceMinorTicksPerMajor;
}

}

class RangeAzimuthBatchItem : public QGraphicsItem
{
public:
    explicit RangeAzimuthBatchItem(RangeAzimuthChart* chart)
        : m_chart(chart)
    {
        setZValue(POINT_Z);
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::NoButton);
        updateBounds(false);
    }

    QRectF boundingRect() const override
    {
        return m_bounds;
    }

    void refreshGeometry()
    {
        updateBounds(true);
        update();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)
        if (!m_chart) {
            return;
        }

        QVarLengthArray<QPointF, 2048> detections;
        QVarLengthArray<QPointF, 2048> normalTracks;
        QVarLengthArray<QPointF, 2048> otherTracks;
        QVarLengthArray<QPointF, 2048> tbdTracks;
        QVarLengthArray<QPointF, 2048> cooperativeTracks;

        for (const auto& item : m_chart->m_detections) {
            if (m_chart->shouldShowDetection(item.info)) {
                detections.append(detectionPoint(item.info));
            }
        }
        for (const auto& item : m_chart->m_tracks) {
            if (!m_chart->shouldShowTrack(item.info)) {
                continue;
            }
            trackBucket(item.info, normalTracks, otherTracks, tbdTracks, cooperativeTracks)
                .append(trackPoint(item.info));
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        drawPoints(painter, m_chart->m_detectionColor,
                   qMax<qreal>(1.0, m_chart->m_baseDetectionSize * m_chart->m_detectionSizeRatio),
                   detections);
        drawPoints(painter, m_chart->m_trackColor,
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   normalTracks);
        drawPoints(painter, m_chart->m_otherTrackColor,
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   otherTracks);
        drawPoints(painter, m_chart->m_tbdTrackColor,
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   tbdTracks);
        drawPoints(painter, m_chart->m_cooperativeTrackColor,
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   cooperativeTracks);
        painter->restore();
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override
    {
        if (!m_chart) {
            QGraphicsItem::hoverMoveEvent(event);
            return;
        }

        PointInfo nearest;
        bool found = false;
        qreal nearestDistanceSq = 64.0;

        auto testPoint = [&](const PointInfo& info, const QPointF& pos, bool visible) {
            if (!visible) {
                return;
            }
            const QPointF delta = pos - event->pos();
            const qreal distanceSq = delta.x() * delta.x() + delta.y() * delta.y();
            if (distanceSq <= nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                nearest = info;
                found = true;
            }
        };

        for (const auto& item : m_chart->m_detections) {
            testPoint(item.info, detectionPoint(item.info), m_chart->shouldShowDetection(item.info));
        }
        for (const auto& item : m_chart->m_tracks) {
            testPoint(item.info, trackPoint(item.info), m_chart->shouldShowTrack(item.info));
        }

        if (!found) {
            QToolTip::hideText();
            QGraphicsItem::hoverMoveEvent(event);
            return;
        }

        QToolTip::showText(event->screenPos(), tooltipText(nearest));
        QGraphicsItem::hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        QToolTip::hideText();
        QGraphicsItem::hoverLeaveEvent(event);
    }

private:
    QPointF detectionPoint(const PointInfo& info) const
    {
        return m_chart->dataToScene(info.azimuth, info.range / 1000.0);
    }

    QPointF trackPoint(const PointInfo& info) const
    {
        return m_chart->dataToScene(info.azimuth, info.range / 1000.0);
    }

    static QVarLengthArray<QPointF, 2048>& trackBucket(const PointInfo& info,
                                                       QVarLengthArray<QPointF, 2048>& normalTracks,
                                                       QVarLengthArray<QPointF, 2048>& otherTracks,
                                                       QVarLengthArray<QPointF, 2048>& tbdTracks,
                                                       QVarLengthArray<QPointF, 2048>& cooperativeTracks)
    {
        if (info.type == PointType::TBDPointType) return tbdTracks;
        if (info.type == PointType::CooperativeTrackPointType) return cooperativeTracks;
        return (info.targetRecResult == 1) ? normalTracks : otherTracks;
    }

    static void drawPoints(QPainter* painter, const QColor& color, qreal width,
                           const QVarLengthArray<QPointF, 2048>& points)
    {
        if (points.isEmpty()) return;
        QPen pen(color);
        pen.setWidthF(width);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);
        painter->drawPoints(points.constData(), points.size());
    }

    QString tooltipText(const PointInfo& info) const
    {
        if (info.type == PointType::Detection) {
            return QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
                .arg(DET_LABEL)
                .arg(info.range, 0, 'f', 1)
                .arg(info.azimuth, 0, 'f', 1)
                .arg(info.elevation, 0, 'f', 1)
                .arg(info.SNR, 0, 'f', 1)
                .arg(info.speed, 0, 'f', 1)
                .arg(info.altitute, 0, 'f', 1)
                .arg(info.amp);
        }

        return QString("%1\nID:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
            .arg(m_chart->trackTooltipLabel(info.type))
            .arg(info.batch)
            .arg(info.range, 0, 'f', 1)
            .arg(info.azimuth, 0, 'f', 1)
            .arg(info.elevation, 0, 'f', 1)
            .arg(info.SNR, 0, 'f', 1)
            .arg(info.speed, 0, 'f', 1)
            .arg(info.altitute, 0, 'f', 1)
            .arg(info.amp);
    }

    RangeAzimuthChart* m_chart = nullptr;
    QRectF m_bounds = QRectF(0.0, 0.0, 1.0, 1.0);

    void updateBounds(bool prepare)
    {
        QRectF nextBounds(0.0, 0.0, 1.0, 1.0);
        if (m_chart && m_chart->viewport()) {
            nextBounds = QRectF(0.0,
                                0.0,
                                qMax<qreal>(1.0, m_chart->viewport()->width()),
                                qMax<qreal>(1.0, m_chart->viewport()->height()));
        }

        if (nextBounds == m_bounds) {
            return;
        }
        if (prepare) {
            prepareGeometryChange();
        }
        m_bounds = nextBounds;
    }
};

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
    m_minAzimuthEdit->setFixedWidth(60);
    m_minAzimuthEdit->setToolTip(tr("最小方位角 (0-360°)"));
    m_minAzimuthEdit->setText(QString::number(CF_INS.rangeAzimuthAngle("min", 0)));
    layout->addWidget(m_minAzimuthEdit);

    QLabel* separatorLabel = new QLabel("~", this);
    separatorLabel->setObjectName("RangeAzimuthSeparatorLabel");
    layout->addWidget(separatorLabel);

    m_maxAzimuthEdit = new QLineEdit(this);
    m_maxAzimuthEdit->setObjectName("RangeAzimuthMaxEdit");
    m_maxAzimuthEdit->setFixedWidth(60);
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
    m_trackLabelsEnabled = CF_INS.displayFlag("chart_track_labels_enabled", false);
    m_trackLabelRefreshIntervalMs = qMax(0, CF_INS.displayConfig("chart_track_label_refresh_ms", 200));

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
    applyDistanceAxisTicks(yAxis, yAxis.minValue, yAxis.maxValue);
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

    m_batchItem = new RangeAzimuthBatchItem(this);
    scene()->addItem(m_batchItem);
}

RangeAzimuthChart::~RangeAzimuthChart()
{
    clearRadarData();
}

void RangeAzimuthChart::rebuildLatestTrackIndices()
{
    m_latestTrackIndices.clear();
    m_trackCountsByKey.clear();
    for (int i = 0; i < m_tracks.size(); ++i) {
        const PointInfo& info = m_tracks[i].info;
        const quint64 key = makeTrackLabelKey(info.type, info.batch);
        m_latestTrackIndices[key] = i;
        m_trackCountsByKey[key] = m_trackCountsByKey.value(key, 0) + 1;
    }
}

void RangeAzimuthChart::removeTrackAt(int index)
{
    if (index < 0 || index >= m_tracks.size()) {
        return;
    }

    const PointInfo info = m_tracks[index].info;
    const quint64 key = makeTrackLabelKey(info.type, info.batch);
    m_tracks.removeAt(index);

    const int nextCount = m_trackCountsByKey.value(key, 1) - 1;
    if (nextCount <= 0) {
        m_trackCountsByKey.remove(key);
    } else {
        m_trackCountsByKey[key] = nextCount;
    }

    for (auto it = m_latestTrackIndices.begin(); it != m_latestTrackIndices.end(); ) {
        if (it.value() == index) {
            it = m_latestTrackIndices.erase(it);
            continue;
        }
        if (it.value() > index) {
            it.value() = it.value() - 1;
        }
        ++it;
    }
}

const RangeAzimuthChart::TrackItem* RangeAzimuthChart::latestTrackItem(unsigned type, int batch) const
{
    const quint64 key = makeTrackLabelKey(type, batch);
    const auto it = m_latestTrackIndices.constFind(key);
    if (it == m_latestTrackIndices.cend()) {
        return nullptr;
    }

    const int index = it.value();
    if (index < 0 || index >= m_tracks.size()) {
        return nullptr;
    }

    return &m_tracks[index];
}

bool RangeAzimuthChart::shouldShowDetection(const PointInfo& info) const
{
    const ChartAxisConfig yAxis = yAxisConfig();
    const double rangeKm = info.range / 1000.0;
    return m_detectionVisible
        && isAzimuthInRange(info.azimuth)
        && rangeKm >= yAxis.minValue
        && rangeKm <= yAxis.maxValue;
}

bool RangeAzimuthChart::shouldShowTrack(const PointInfo& info) const
{
    const ChartAxisConfig yAxis = yAxisConfig();
    const double rangeKm = info.range / 1000.0;
    return isTrackTypeVisible(info.type)
        && isTrackRecognitionVisible(info)
        && isAzimuthInRange(info.azimuth)
        && rangeKm >= yAxis.minValue
        && rangeKm <= yAxis.maxValue;
}

void RangeAzimuthChart::addDetectionPoint(const PointInfo& info)
{
    // 存储到检测点列表
    DetectionItem detItem;
    detItem.info = info;
    detItem.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_detections.append(detItem);

    // 限制点数
    limitDetectionPoints();
    if (m_batchItem) {
        m_batchItem->update();
    }

    // 更新统计
    updatePointCount();
}

void RangeAzimuthChart::addTrackPoint(const trackInfo& info)
{
    PointInfo pointInfo;
    pointInfo.type = PointType::Track;
    pointInfo.range = info.dis;
    pointInfo.azimuth = info.azi;
    pointInfo.elevation = info.ele;
    pointInfo.SNR = info.SNR;
    pointInfo.speed = info.vel;
    pointInfo.altitute = info.altitute;
    pointInfo.amp = info.amp;
    pointInfo.batch = info.batch;
    pointInfo.statMethod = info.statMethod;
    pointInfo.targetRecResult = info.targetRecResult;
    addPointInfo(pointInfo);
}

void RangeAzimuthChart::addPointInfo(const PointInfo& info)
{
    // statMethod==2 表示消批命令，不添加新点（批次已由 removeBatch 删除）
    if (info.statMethod == 2) {
        for (int i = m_tracks.size() - 1; i >= 0; --i) {
            if (m_tracks[i].info.batch != info.batch || m_tracks[i].info.type != info.type) {
                continue;
            }
            m_tracks.removeAt(i);
        }
        rebuildLatestTrackIndices();
        refreshTrackLabels();
        updatePointCount();
        if (m_batchItem) {
            m_batchItem->update();
        }
        return;
    }

    // 根据 type 字段判断点类型并分发
    if (info.type == Detection) {
        // 检测点
        addDetectionPoint(info);
    } else if (info.type == Track || info.type == TBDPointType || info.type == CooperativeTrackPointType) {
        TrackItem trackItem;
        trackItem.info = info;
        trackItem.timestamp = QDateTime::currentMSecsSinceEpoch();

        m_tracks.append(trackItem);
        const quint64 key = makeTrackLabelKey(info.type, info.batch);
        m_latestTrackIndices[key] = m_tracks.size() - 1;
        m_trackCountsByKey[key] = m_trackCountsByKey.value(key, 0) + 1;

        limitTrackPointsForBatch(info.type, info.batch);
        refreshTrackLabels();
        updatePointCount();
        if (m_batchItem) {
            m_batchItem->update();
        }
    }
}

void RangeAzimuthChart::removeBatch(int batchID)
{
    int removedCount = 0;
    // 从后向前遍历，删除匹配的航迹
    for (int i = m_tracks.size() - 1; i >= 0; --i) {
        if (m_tracks[i].info.batch == batchID) {
            // 从列表中移除
            m_tracks.removeAt(i);
            ++removedCount;
        }
    }

    rebuildLatestTrackIndices();
    refreshTrackLabels();

    // 更新统计显示
    updatePointCount();
    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeAzimuthChart::clearRadarData()
{
    m_detections.clear();

    m_tracks.clear();
    m_latestTrackIndices.clear();
    m_trackCountsByKey.clear();
    m_trackLabelRefreshMs.clear();
    clearTrackLabels();

    // 更新统计
    updatePointCount();
    if (m_batchItem) {
        m_batchItem->update();
    }
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
    applyDistanceAxisTicks(yAxis, minRangeKm, maxRangeKm);

    setYAxisConfig(yAxis);

    refreshAllPoints();
    if (m_batchItem) {
        m_batchItem->refreshGeometry();
    }
}

void RangeAzimuthChart::setDetectionVisible(bool visible)
{
    m_detectionVisible = visible;

    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeAzimuthChart::setTrackVisible(bool visible)
{
    m_trackVisible = visible;

    updateTrackVisibility();
}

void RangeAzimuthChart::setTbdTrackVisible(bool visible)
{
    m_tbdTrackVisible = visible;

    updateTrackVisibility();
}

void RangeAzimuthChart::setCooperativeTrackVisible(bool visible)
{
    m_cooperativeTrackVisible = visible;

    updateTrackVisibility();
}

void RangeAzimuthChart::setOnlyRecognizedDroneTracksVisible(bool enabled)
{
    m_onlyRecognizedDroneTracksVisible = enabled;
    updateTrackVisibility();
}

void RangeAzimuthChart::setDetectionSizeRatio(double ratio)
{
    m_detectionSizeRatio = qBound(0.5, ratio, 3.0);

    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeAzimuthChart::setTrackSizeRatio(double ratio)
{
    m_trackSizeRatio = qBound(0.5, ratio, 3.0);

    if (m_batchItem) {
        m_batchItem->update();
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
    if (maxTracks < 1) {
        maxTracks = 1;
    }
    m_maxTrackPoints = maxTracks;
    limitTrackPoints();
}

void RangeAzimuthChart::limitDetectionPoints()
{
    // FIFO：删除最旧的点（列表头部是最旧的）
    while (m_detections.size() > m_maxDetectionPoints) {
        m_detections.removeFirst();
    }
}

void RangeAzimuthChart::limitTrackPoints()
{
    QMap<quint64, int> batchCounts;

    for (int i = m_tracks.size() - 1; i >= 0; --i) {
        const PointInfo& info = m_tracks[i].info;
        const quint64 key = makeTrackLabelKey(info.type, info.batch);
        const int count = batchCounts.value(key, 0) + 1;
        batchCounts.insert(key, count);

        if (count <= m_maxTrackPoints) {
            continue;
        }

        m_tracks.removeAt(i);
    }

    rebuildLatestTrackIndices();
    refreshTrackLabels();
    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeAzimuthChart::limitTrackPointsForBatch(unsigned type, int batch)
{
    const quint64 key = makeTrackLabelKey(type, batch);
    while (m_trackCountsByKey.value(key, 0) > m_maxTrackPoints) {
        int removeIndex = -1;
        for (int i = 0; i < m_tracks.size(); ++i) {
            const PointInfo& info = m_tracks[i].info;
            if (makeTrackLabelKey(info.type, info.batch) == key) {
                removeIndex = i;
                break;
            }
        }

        if (removeIndex < 0) {
            rebuildLatestTrackIndices();
            return;
        }

        removeTrackAt(removeIndex);
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
    if (m_batchItem) {
        m_batchItem->refreshGeometry();
    }
    refreshTrackLabels();
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

    LOG_DEBUG(QString("[RangeAzimuthChart::setAzimuthRange] Updated to %1~%2")
                  .arg(minAz)
                  .arg(maxAz));
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

QColor RangeAzimuthChart::trackColor(const PointInfo& info) const
{
    if (info.type == PointType::Track) {
        return (info.targetRecResult == 1) ? m_trackColor : m_otherTrackColor;
    }

    switch (static_cast<PointType>(info.type)) {
    case PointType::TBDPointType:
        return m_tbdTrackColor;
    case PointType::CooperativeTrackPointType:
        return m_cooperativeTrackColor;
    case PointType::Track:
    default:
        return m_trackColor;
    }
}

QString RangeAzimuthChart::trackTooltipLabel(unsigned type) const
{
    return trackTypeLabel(type);
}

QString RangeAzimuthChart::trackLabelText(const PointInfo& info) const
{
    return QString("batch : %1").arg(info.batch);
}

bool RangeAzimuthChart::isTrackTypeVisible(unsigned type) const
{
    switch (static_cast<PointType>(type)) {
    case PointType::TBDPointType:
        return m_tbdTrackVisible;
    case PointType::CooperativeTrackPointType:
        return m_cooperativeTrackVisible;
    case PointType::Track:
    default:
        return m_trackVisible;
    }
}

bool RangeAzimuthChart::isTrackBatchRecognitionVisible(unsigned type, int batch) const
{
    if (!m_onlyRecognizedDroneTracksVisible) return true;
    if (type != PointType::Track) return true;

    const TrackItem* trackItem = latestTrackItem(type, batch);
    return trackItem && trackItem->info.targetRecResult == 1;
}

bool RangeAzimuthChart::isTrackRecognitionVisible(const PointInfo& info) const
{
    return isTrackBatchRecognitionVisible(info.type, info.batch);
}

void RangeAzimuthChart::updateTrackVisibility()
{
    if (m_batchItem) {
        m_batchItem->update();
    }
    refreshTrackLabels();
}

void RangeAzimuthChart::updateTrackVisibilityForBatch(unsigned type, int batch)
{
    const quint64 key = makeTrackLabelKey(type, batch);
    for (TrackItem& item : m_tracks) {
        if (makeTrackLabelKey(item.info.type, item.info.batch) != key) {
            continue;
        }
    }

    if (m_batchItem) {
        m_batchItem->update();
    }
    refreshTrackLabels();
}

void RangeAzimuthChart::refreshTrackLabels()
{
    if (!m_trackLabelsEnabled) {
        clearTrackLabels();
        return;
    }

    QSet<quint64> activeKeys;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_latestTrackIndices.cbegin(); it != m_latestTrackIndices.cend(); ++it) {
        const int index = it.value();
        if (index < 0 || index >= m_tracks.size()) {
            continue;
        }

        const TrackItem* trackItem = &m_tracks[index];

        const quint64 key = it.key();
        activeKeys.insert(key);

        QGraphicsSimpleTextItem* labelItem = m_trackLabels.value(key, nullptr);
        if (!labelItem) {
            labelItem = new QGraphicsSimpleTextItem();
            labelItem->setZValue(INFO_Z);
            scene()->addItem(labelItem);
            m_trackLabels.insert(key, labelItem);
        }

        const bool shouldRefresh = !m_trackLabelRefreshMs.contains(key)
                                || m_trackLabelRefreshIntervalMs <= 0
                                || (nowMs - m_trackLabelRefreshMs.value(key)) >= m_trackLabelRefreshIntervalMs;
        if (shouldRefresh) {
            labelItem->setText(trackLabelText(trackItem->info));
            labelItem->setBrush(QBrush(trackColor(trackItem->info)));
            labelItem->setPos(dataToScene(trackItem->info.azimuth, trackItem->info.range / 1000.0)
                              + QPointF(kTrackLabelOffsetX, kTrackLabelOffsetY));
            m_trackLabelRefreshMs.insert(key, nowMs);
        }
        labelItem->setVisible(shouldShowTrack(trackItem->info));
    }

    for (auto it = m_trackLabels.begin(); it != m_trackLabels.end(); ) {
        if (activeKeys.contains(it.key())) {
            ++it;
            continue;
        }

        if (it.value()) {
            scene()->removeItem(it.value());
            delete it.value();
        }
        m_trackLabelRefreshMs.remove(it.key());
        it = m_trackLabels.erase(it);
    }
}

void RangeAzimuthChart::clearTrackLabels()
{
    for (auto it = m_trackLabels.begin(); it != m_trackLabels.end(); ++it) {
        if (it.value()) {
            scene()->removeItem(it.value());
            delete it.value();
        }
    }
    m_trackLabels.clear();
    m_trackLabelRefreshMs.clear();
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
    LOG_DEBUG("[RangeAzimuthChartWidget::onClearRequested] Clear button clicked");
    m_chart->clearRadarData();
}

void RangeAzimuthChartWidget::onResetRequested()
{
    LOG_DEBUG("[RangeAzimuthChartWidget::onResetRequested] Reset button clicked");

    // 重置距离范围到默认状态（0-5km）
    m_chart->setRangeFromMain(0, 5000);

    // 重置方位角范围到默认状态（0-360°）
    m_chart->setAzimuthRange(0, 360);

    // 注意：重置不清除数据，只是恢复默认视图范围
    // 数据会在 setRangeFromMain 和 setAzimuthRange 中被重新排布

    LOG_DEBUG("[RangeAzimuthChartWidget::onResetRequested] Reset completed");
}
