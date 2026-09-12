/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-05-18 15:26:15
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:53
 * @Description: 
 */
#include "rangeheightchart.h"
#include "../Basic/DispBasci.h"
#include "../Basic/ConfigManager.h"
#include "../Basic/log.h"
#include "../Basic/offlinerae.h"
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

constexpr double kDefaultMinHeight = 0.0;
constexpr double kDefaultMaxHeight = 500.0;
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

class RangeHeightBatchItem : public QGraphicsItem
{
public:
    explicit RangeHeightBatchItem(RangeHeightChart* chart)
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
        QVarLengthArray<QPointF, 2048> offlineCyanTracks;
        QVarLengthArray<QPointF, 2048> offlineRedTracks;
        QVarLengthArray<QPointF, 2048> offlineBlueTracks;

        for (const auto& item : m_chart->m_detections) {
            if (m_chart->shouldShowDetection(item.info)) {
                detections.append(pointFor(item.info));
            }
        }
        for (const auto& item : m_chart->m_tracks) {
            if (!m_chart->shouldShowTrack(item.info)) {
                continue;
            }
            trackBucket(item.info, normalTracks, otherTracks, tbdTracks, cooperativeTracks,
                        offlineCyanTracks, offlineRedTracks, offlineBlueTracks)
                .append(pointFor(item.info));
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
        drawPoints(painter, QColor(0, 255, 255),
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   offlineCyanTracks);
        drawPoints(painter, QColor(255, 0, 0),
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   offlineRedTracks);
        drawPoints(painter, QColor(0, 0, 255),
                   qMax<qreal>(1.0, m_chart->m_baseTrackSize * m_chart->m_trackSizeRatio),
                   offlineBlueTracks);
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

        auto testPoint = [&](const PointInfo& info, bool visible) {
            if (!visible) {
                return;
            }
            const QPointF delta = pointFor(info) - event->pos();
            const qreal distanceSq = delta.x() * delta.x() + delta.y() * delta.y();
            if (distanceSq <= nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                nearest = info;
                found = true;
            }
        };

        for (const auto& item : m_chart->m_detections) {
            testPoint(item.info, m_chart->shouldShowDetection(item.info));
        }
        for (const auto& item : m_chart->m_tracks) {
            testPoint(item.info, m_chart->shouldShowTrack(item.info));
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
    QPointF pointFor(const PointInfo& info) const
    {
        return m_chart->dataToScene(info.range / 1000.0, info.altitute);
    }

    static QVarLengthArray<QPointF, 2048>& trackBucket(const PointInfo& info,
                                                       QVarLengthArray<QPointF, 2048>& normalTracks,
                                                       QVarLengthArray<QPointF, 2048>& otherTracks,
                                                       QVarLengthArray<QPointF, 2048>& tbdTracks,
                                                       QVarLengthArray<QPointF, 2048>& cooperativeTracks,
                                                       QVarLengthArray<QPointF, 2048>& offlineCyanTracks,
                                                       QVarLengthArray<QPointF, 2048>& offlineRedTracks,
                                                       QVarLengthArray<QPointF, 2048>& offlineBlueTracks)
    {
        if (OfflineRae::isOffline(info)) {
            if (info.targetRecResult == OfflineRae::kColorRed) return offlineRedTracks;
            if (info.targetRecResult == OfflineRae::kColorBlue) return offlineBlueTracks;
            return offlineCyanTracks;
        }
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

    RangeHeightChart* m_chart = nullptr;
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

RangeHeightChartToolBar::RangeHeightChartToolBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RangeAzimuthChartToolBar");
    setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

    QLabel* heightLabel = new QLabel(tr("高度:"), this);
    heightLabel->setObjectName("RangeAzimuthLabel");
    layout->addWidget(heightLabel);

    m_minHeightEdit = new QLineEdit(this);
    m_minHeightEdit->setObjectName("RangeAzimuthMinEdit");
    m_minHeightEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_minHeightEdit->setFixedWidth(ScaleHelper::uiScaled(60));
    m_minHeightEdit->setToolTip(tr("最小高度 (m)"));
    m_minHeightEdit->setText(QString::number(kDefaultMinHeight, 'f', 0));
    layout->addWidget(m_minHeightEdit);

    QLabel* separatorLabel = new QLabel("~", this);
    separatorLabel->setObjectName("RangeAzimuthSeparatorLabel");
    layout->addWidget(separatorLabel);

    m_maxHeightEdit = new QLineEdit(this);
    m_maxHeightEdit->setObjectName("RangeAzimuthMaxEdit");
    m_maxHeightEdit->setFixedWidth(ScaleHelper::uiScaled(60));
    m_maxHeightEdit->setToolTip(tr("最大高度 (m)"));
    m_maxHeightEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_maxHeightEdit->setText(QString::number(kDefaultMaxHeight, 'f', 0));
    layout->addWidget(m_maxHeightEdit);

    connect(m_minHeightEdit, &QLineEdit::returnPressed, this, &RangeHeightChartToolBar::onHeightRangeChanged);
    connect(m_maxHeightEdit, &QLineEdit::returnPressed, this, &RangeHeightChartToolBar::onHeightRangeChanged);

    layout->addStretch();

    m_clearButton = new QPushButton(tr("清除"), this);
    m_clearButton->setObjectName("RangeAzimuthClearButton");
    layout->addWidget(m_clearButton);
    connect(m_clearButton, &QPushButton::clicked, this, &RangeHeightChartToolBar::clearRequested);

    m_resetButton = new QPushButton(tr("重置"), this);
    m_resetButton->setObjectName("RangeAzimuthResetButton");
    layout->addWidget(m_resetButton);
    connect(m_resetButton, &QPushButton::clicked, this, &RangeHeightChartToolBar::resetRequested);
}

double RangeHeightChartToolBar::getMinHeight() const
{
    bool ok;
    double value = m_minHeightEdit->text().toDouble(&ok);
    return ok ? value : kDefaultMinHeight;
}

double RangeHeightChartToolBar::getMaxHeight() const
{
    bool ok;
    double value = m_maxHeightEdit->text().toDouble(&ok);
    return ok ? value : kDefaultMaxHeight;
}

void RangeHeightChartToolBar::setHeightRange(double minHeight, double maxHeight)
{
    m_minHeightEdit->setText(QString::number(minHeight, 'f', 0));
    m_maxHeightEdit->setText(QString::number(maxHeight, 'f', 0));
}

void RangeHeightChartToolBar::updateStatus(int detectionCount, int trackCount)
{
    Q_UNUSED(detectionCount);
    Q_UNUSED(trackCount);
}

void RangeHeightChartToolBar::onHeightRangeChanged()
{
    double minHeight = getMinHeight();
    double maxHeight = getMaxHeight();

    if (minHeight >= maxHeight) {
        minHeight = kDefaultMinHeight;
        maxHeight = kDefaultMaxHeight;
        setHeightRange(minHeight, maxHeight);
    }

    emit heightRangeChanged(minHeight, maxHeight);
}

RangeHeightChart::RangeHeightChart(QWidget* parent)
    : CustomLineChart(parent)
{
    m_trackLabelsEnabled = CF_INS.displayFlag("chart_track_labels_enabled", false);
    m_trackLabelRefreshIntervalMs = qMax(0, CF_INS.displayConfig("chart_track_label_refresh_ms", 200));

    setObjectName("RangeAzimuthChart");
    setFrameShape(QFrame::NoFrame);
    setFrameStyle(QFrame::NoFrame);

    ChartAxisConfig xAxis;
    xAxis.minValue = 0;
    xAxis.maxValue = 5;
    applyDistanceAxisTicks(xAxis, xAxis.minValue, xAxis.maxValue);
    xAxis.label = tr("距离 (km)");
    xAxis.unit = "";
    setXAxisConfig(xAxis);

    ChartAxisConfig yAxis;
    yAxis.minValue = kDefaultMinHeight;
    yAxis.maxValue = kDefaultMaxHeight;
    yAxis.majorTickInterval = 100;
    yAxis.minorTickInterval = 50;
    yAxis.label = tr("高度 (m)");
    yAxis.unit = "";
    setYAxisConfig(yAxis);

    setGridVisible(true);
    setAxisVisible(true);

    setChartBgColor(QColor(10, 16, 16));
    setChartGridMajorColor(QColor(0, 80, 60));
    setChartGridMinorColor(QColor(0, 50, 38));
    setChartAxisColor(QColor(0, 255, 136));
    setChartTextColor(QColor(102, 255, 204));

    m_batchItem = new RangeHeightBatchItem(this);
    scene()->addItem(m_batchItem);
}

RangeHeightChart::~RangeHeightChart()
{
    clearRadarData();
}

void RangeHeightChart::rebuildLatestTrackIndices()
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

void RangeHeightChart::removeTrackAt(int index)
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

const RangeHeightChart::TrackItem* RangeHeightChart::latestTrackItem(unsigned type, int batch) const
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

bool RangeHeightChart::shouldShowDetection(const PointInfo& info) const
{
    const ChartAxisConfig xAxis = xAxisConfig();
    const double distanceKm = info.range / 1000.0;
    return m_detectionVisible
        && distanceKm >= xAxis.minValue
        && distanceKm <= xAxis.maxValue
        && isHeightInRange(info.altitute);
}

bool RangeHeightChart::shouldShowTrack(const PointInfo& info) const
{
    const ChartAxisConfig xAxis = xAxisConfig();
    const double distanceKm = info.range / 1000.0;
    return isTrackTypeVisible(info.type)
        && isTrackRecognitionVisible(info)
        && distanceKm >= xAxis.minValue
        && distanceKm <= xAxis.maxValue
        && isHeightInRange(info.altitute);
}

void RangeHeightChart::addDetectionPoint(const PointInfo& info)
{
    DetectionItem detItem;
    detItem.info = info;
    detItem.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_detections.append(detItem);

    limitDetectionPoints();
    if (m_batchItem) {
        m_batchItem->update();
    }
    updatePointCount();
}

void RangeHeightChart::addTrackPoint(const trackInfo& info)
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

void RangeHeightChart::addPointInfo(const PointInfo& info)
{
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

    if (info.type == Detection) {
        addDetectionPoint(info);
        return;
    }

    if (info.type != Track && info.type != TBDPointType && info.type != CooperativeTrackPointType) {
        return;
    }

    TrackItem trackItem;
    trackItem.info = info;
    trackItem.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_tracks.append(trackItem);
    const quint64 key = makeTrackLabelKey(info.type, info.batch);
    m_latestTrackIndices[key] = m_tracks.size() - 1;
    m_trackCountsByKey[key] = m_trackCountsByKey.value(key, 0) + 1;

    limitTrackPointsForBatch(info.type, info.batch);
    refreshTrackLabels();
    if (m_batchItem) {
        m_batchItem->update();
    }
    updatePointCount();
}

void RangeHeightChart::removeBatch(int batchID)
{
    for (int i = m_tracks.size() - 1; i >= 0; --i) {
        if (m_tracks[i].info.batch != batchID) {
            continue;
        }
        m_tracks.removeAt(i);
    }
    rebuildLatestTrackIndices();
    refreshTrackLabels();
    if (m_batchItem) {
        m_batchItem->update();
    }
    updatePointCount();
}

void RangeHeightChart::clearRadarData()
{
    m_detections.clear();

    m_tracks.clear();
    m_latestTrackIndices.clear();
    m_trackCountsByKey.clear();
    m_trackLabelRefreshMs.clear();
    clearTrackLabels();
    if (m_batchItem) {
        m_batchItem->update();
    }
    updatePointCount();
}

void RangeHeightChart::setRangeFromMain(double minRange, double maxRange)
{
    double minRangeKm = minRange / 1000.0;
    double maxRangeKm = maxRange / 1000.0;

    ChartAxisConfig xAxis = xAxisConfig();
    xAxis.minValue = minRangeKm;
    xAxis.maxValue = maxRangeKm;
    applyDistanceAxisTicks(xAxis, minRangeKm, maxRangeKm);

    setXAxisConfig(xAxis);
    refreshAllPoints();
    if (m_batchItem) {
        m_batchItem->refreshGeometry();
    }
}

void RangeHeightChart::setHeightRange(double minHeight, double maxHeight)
{
    const bool changed = !qFuzzyCompare(m_minHeight + 1.0, minHeight + 1.0)
        || !qFuzzyCompare(m_maxHeight + 1.0, maxHeight + 1.0);
    m_minHeight = minHeight;
    m_maxHeight = maxHeight;

    ChartAxisConfig yAxis = yAxisConfig();
    yAxis.minValue = minHeight;
    yAxis.maxValue = maxHeight;

    double span = maxHeight - minHeight;
    if (span <= 100) {
        yAxis.majorTickInterval = 20;
        yAxis.minorTickInterval = 10;
    } else if (span <= 500) {
        yAxis.majorTickInterval = 100;
        yAxis.minorTickInterval = 50;
    } else if (span <= 1000) {
        yAxis.majorTickInterval = 200;
        yAxis.minorTickInterval = 100;
    } else {
        yAxis.majorTickInterval = 500;
        yAxis.minorTickInterval = 100;
    }

    setYAxisConfig(yAxis);
    refreshAllPoints();
    if (m_batchItem) {
        m_batchItem->refreshGeometry();
    }
    if (changed) {
        emit heightRangeChanged(minHeight, maxHeight);
    }
}

void RangeHeightChart::setDetectionVisible(bool visible)
{
    m_detectionVisible = visible;
    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeHeightChart::setTrackVisible(bool visible)
{
    m_trackVisible = visible;
    updateTrackVisibility();
}

void RangeHeightChart::setTbdTrackVisible(bool visible)
{
    m_tbdTrackVisible = visible;
    updateTrackVisibility();
}

void RangeHeightChart::setCooperativeTrackVisible(bool visible)
{
    m_cooperativeTrackVisible = visible;
    updateTrackVisibility();
}

void RangeHeightChart::setOnlyRecognizedDroneTracksVisible(bool enabled)
{
    m_onlyRecognizedDroneTracksVisible = enabled;
    updateTrackVisibility();
}

void RangeHeightChart::setDetectionSizeRatio(double ratio)
{
    m_detectionSizeRatio = qBound(0.5, ratio, 3.0);
    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeHeightChart::setTrackSizeRatio(double ratio)
{
    m_trackSizeRatio = qBound(0.5, ratio, 3.0);
    if (m_batchItem) {
        m_batchItem->update();
    }
}

void RangeHeightChart::setMaxDetectionPoints(int maxPoints)
{
    m_maxDetectionPoints = maxPoints;
    limitDetectionPoints();
}

void RangeHeightChart::setMaxTrackPoints(int maxTracks)
{
    if (maxTracks < 1) {
        maxTracks = 1;
    }
    m_maxTrackPoints = maxTracks;
    limitTrackPoints();
}

void RangeHeightChart::limitDetectionPoints()
{
    while (m_detections.size() > m_maxDetectionPoints) {
        m_detections.removeFirst();
    }
}

void RangeHeightChart::limitTrackPoints()
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

void RangeHeightChart::limitTrackPointsForBatch(unsigned type, int batch)
{
    const quint64 key = makeTrackLabelKey(type, batch);
    for (const auto& item : m_tracks) {
        if (makeTrackLabelKey(item.info.type, item.info.batch) == key && OfflineRae::isOffline(item.info)) {
            return;
        }
    }
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

void RangeHeightChart::updatePointCount()
{
    emit pointCountChanged(m_detections.size(), m_tracks.size());
}

void RangeHeightChart::resizeEvent(QResizeEvent* event)
{
    CustomLineChart::resizeEvent(event);
    refreshAllPoints();
}

void RangeHeightChart::refreshAllPoints()
{
    if (m_batchItem) {
        m_batchItem->refreshGeometry();
    }
    refreshTrackLabels();
    updatePointCount();
}

QColor RangeHeightChart::trackColor(const PointInfo& info) const
{
    if (OfflineRae::isOffline(info)) {
        return OfflineRae::colorFor(info, m_trackColor);
    }
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

QString RangeHeightChart::trackTooltipLabel(unsigned type) const
{
    return trackTypeLabel(type);
}

QString RangeHeightChart::trackLabelText(const PointInfo& info) const
{
    if (OfflineRae::isOffline(info)) {
        return QString();
    }
    return QString("batch : %1").arg(info.batch);
}

bool RangeHeightChart::isTrackTypeVisible(unsigned type) const
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

bool RangeHeightChart::isTrackBatchRecognitionVisible(unsigned type, int batch) const
{
    if (!m_onlyRecognizedDroneTracksVisible) return true;
    if (type != PointType::Track) return true;

    const TrackItem* trackItem = latestTrackItem(type, batch);
    return trackItem && trackItem->info.targetRecResult == 1;
}

bool RangeHeightChart::isTrackRecognitionVisible(const PointInfo& info) const
{
    return isTrackBatchRecognitionVisible(info.type, info.batch);
}

void RangeHeightChart::updateTrackVisibility()
{
    if (m_batchItem) {
        m_batchItem->update();
    }

    refreshTrackLabels();
}

void RangeHeightChart::updateTrackVisibilityForBatch(unsigned type, int batch)
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

bool RangeHeightChart::isHeightInRange(double height) const
{
    return height >= m_minHeight && height <= m_maxHeight;
}

void RangeHeightChart::refreshTrackLabels()
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
        if (OfflineRae::isOffline(trackItem->info)) {
            continue;
        }

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
            labelItem->setPos(dataToScene(trackItem->info.range / 1000.0, trackItem->info.altitute)
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

void RangeHeightChart::clearTrackLabels()
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

RangeHeightChartWidget::RangeHeightChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RangeHeightChartWidget");
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toolbar = new RangeHeightChartToolBar(this);
    layout->addWidget(m_toolbar);

    m_chart = new RangeHeightChart(this);
    layout->addWidget(m_chart, 1);

    connect(m_toolbar, &RangeHeightChartToolBar::clearRequested,
            this, &RangeHeightChartWidget::onClearRequested);
    connect(m_toolbar, &RangeHeightChartToolBar::resetRequested,
            this, &RangeHeightChartWidget::onResetRequested);
    connect(m_toolbar, &RangeHeightChartToolBar::heightRangeChanged,
            m_chart, &RangeHeightChart::setHeightRange);
    connect(m_chart, &RangeHeightChart::pointCountChanged,
            m_toolbar, &RangeHeightChartToolBar::updateStatus);
}

RangeHeightChartWidget::~RangeHeightChartWidget()
{
}

void RangeHeightChartWidget::onClearRequested()
{
    m_chart->clearRadarData();
}

void RangeHeightChartWidget::onResetRequested()
{
    m_chart->setHeightRange(kDefaultMinHeight, kDefaultMaxHeight);
    m_toolbar->setHeightRange(kDefaultMinHeight, kDefaultMaxHeight);
}
