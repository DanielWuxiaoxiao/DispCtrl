#include "track3dwidget.h"
#include "track3dcoordinate.h"

#include "../Basic/DispBasci.h"
#include "../Basic/log.h"
#include "../Basic/offlinerae.h"

#include <QColor>
#include <QDateTime>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtMath>

#include <cmath>

namespace {

constexpr int kWebUpdateIntervalMs = 50;
constexpr qint64 kInvalidPointLogIntervalMs = 5000;
const QUrl kTrack3DPageUrl(QStringLiteral("qrc:///track3d/index.html"));

bool fuzzyEqual(double lhs, double rhs)
{
    return qAbs(lhs - rhs) <= 1e-6;
}

}

Track3DBridge::Track3DBridge(QObject* parent)
    : QObject(parent)
{
}

void Track3DBridge::pageReady()
{
    emit pageReadyReported();
}

void Track3DBridge::reportError(const QString& message)
{
    emit errorReported(message);
}

Track3DWidget::Track3DWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Track3DWidget"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("Track3DToolBar"));
    toolbar->setAttribute(Qt::WA_StyledBackground, true);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(5, 2, 5, 2);
    toolbarLayout->setSpacing(10);

    auto* titleLabel = new QLabel(tr("三维航迹"), toolbar);
    titleLabel->setObjectName(QStringLiteral("Track3DTitleLabel"));
    toolbarLayout->addWidget(titleLabel);

    m_trackCountLabel = new QLabel(tr("航迹: 0"), toolbar);
    m_trackCountLabel->setObjectName(QStringLiteral("Track3DCountLabel"));
    toolbarLayout->addWidget(m_trackCountLabel);
    toolbarLayout->addStretch();

    m_clearButton = new QPushButton(tr("清除"), toolbar);
    m_clearButton->setObjectName(QStringLiteral("Track3DClearButton"));
    toolbarLayout->addWidget(m_clearButton);

    m_resetButton = new QPushButton(tr("视角复位"), toolbar);
    m_resetButton->setObjectName(QStringLiteral("Track3DResetButton"));
    toolbarLayout->addWidget(m_resetButton);

    mainLayout->addWidget(toolbar);

    m_webView = new QWebEngineView(this);
    m_webView->setObjectName(QStringLiteral("Track3DWebView"));
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    m_webView->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    mainLayout->addWidget(m_webView, 1);

    m_bridge = new Track3DBridge(this);
    m_webChannel = new QWebChannel(this);
    m_webChannel->registerObject(QStringLiteral("track3dBridge"), m_bridge);
    m_webView->page()->setWebChannel(m_webChannel);

    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kWebUpdateIntervalMs);

    connect(m_flushTimer, &QTimer::timeout,
            this, &Track3DWidget::flushPendingUpdates);
    connect(m_clearButton, &QPushButton::clicked,
            this, &Track3DWidget::clearRadarData);
    connect(m_resetButton, &QPushButton::clicked,
            this, &Track3DWidget::resetView);
    connect(m_bridge, &Track3DBridge::pageReadyReported,
            this, &Track3DWidget::onPageReady);
    connect(m_bridge, &Track3DBridge::errorReported,
            this, &Track3DWidget::onPageError);
    connect(m_webView, &QWebEngineView::loadFinished,
            this, &Track3DWidget::onLoadFinished);
    connect(m_webView->page(), &QWebEnginePage::renderProcessTerminated,
            this,
            [this](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
        handleRendererFailure(
            QStringLiteral("render process terminated, status=%1 exitCode=%2")
                .arg(static_cast<int>(status))
                .arg(exitCode));
    });

    m_webView->load(kTrack3DPageUrl);
}

Track3DWidget::~Track3DWidget() = default;

quint64 Track3DWidget::makeTrackKey(unsigned type, unsigned batch)
{
    return (static_cast<quint64>(type) << 32) | static_cast<quint64>(batch);
}

QString Track3DWidget::makeTrackKeyString(unsigned type, unsigned batch)
{
    return QStringLiteral("%1:%2").arg(type).arg(batch);
}

bool Track3DWidget::isTrackPoint(const PointInfo& info) const
{
    return info.type == PointType::Track
        || info.type == PointType::TBDPointType
        || info.type == PointType::CooperativeTrackPointType;
}

bool Track3DWidget::isValidPoint(const PointInfo& info) const
{
    Track3DCoordinate coordinate;
    const bool finite = calculateTrack3DCoordinate(info, coordinate)
        && std::isfinite(info.SNR)
        && std::isfinite(info.speed)
        && std::isfinite(info.amp);
    return finite;
}

QColor Track3DWidget::trackColor(const PointInfo& info) const
{
    if (OfflineRae::isOffline(info)) {
        return OfflineRae::colorFor(info, TRA_COLOR);
    }

    switch (static_cast<PointType>(info.type)) {
    case PointType::TBDPointType:
        return TBD_COLOR;
    case PointType::CooperativeTrackPointType:
        return CO_TRACK_COLOR;
    case PointType::Track:
    default:
        return info.targetRecResult == 1 ? DRONE_COLOR : TRA_COLOR;
    }
}

QVariantMap Track3DWidget::pointPayload(const PointInfo& info) const
{
    Track3DCoordinate coordinate;
    calculateTrack3DCoordinate(info, coordinate);

    QVariantMap payload;
    payload.insert(QStringLiteral("key"), makeTrackKeyString(info.type, info.batch));
    payload.insert(QStringLiteral("type"), info.type);
    payload.insert(QStringLiteral("batch"), info.batch);
    payload.insert(QStringLiteral("eastM"), coordinate.eastM);
    payload.insert(QStringLiteral("northM"), coordinate.northM);
    payload.insert(QStringLiteral("heightM"), coordinate.heightM);
    payload.insert(QStringLiteral("rangeM"), static_cast<double>(info.range));
    payload.insert(QStringLiteral("azimuthDeg"), coordinate.azimuthDeg);
    payload.insert(QStringLiteral("elevationDeg"), static_cast<double>(info.elevation));
    payload.insert(QStringLiteral("snrDb"), static_cast<double>(info.SNR));
    payload.insert(QStringLiteral("speedMps"), static_cast<double>(info.speed));
    payload.insert(QStringLiteral("amp"), static_cast<double>(info.amp));
    payload.insert(QStringLiteral("targetRecResult"), info.targetRecResult);
    payload.insert(QStringLiteral("offline"), OfflineRae::isOffline(info));
    payload.insert(QStringLiteral("color"), trackColor(info).name(QColor::HexRgb));
    return payload;
}

QVariantMap Track3DWidget::sceneState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("minRangeM"), m_minRangeM);
    state.insert(QStringLiteral("maxRangeM"), m_maxRangeM);
    state.insert(QStringLiteral("minHeightM"), m_minHeightM);
    state.insert(QStringLiteral("maxHeightM"), m_maxHeightM);
    state.insert(QStringLiteral("trackVisible"), m_trackVisible);
    state.insert(QStringLiteral("tbdTrackVisible"), m_tbdTrackVisible);
    state.insert(QStringLiteral("cooperativeTrackVisible"), m_cooperativeTrackVisible);
    state.insert(QStringLiteral("onlyRecognizedDroneTracksVisible"),
                 m_onlyRecognizedDroneTracksVisible);
    state.insert(QStringLiteral("pointSizeRatio"), m_trackSizeRatio);
    return state;
}

void Track3DWidget::addPointInfo(const PointInfo& info)
{
    if (!isTrackPoint(info)) {
        return;
    }

    const quint64 key = makeTrackKey(info.type, info.batch);
    if (info.statMethod == 2) {
        m_latestTracks.remove(key);
        m_pendingUpserts.remove(key);
        m_pendingRemovals.insert(key);
        updateTrackCountLabel();
        scheduleFlush();
        return;
    }

    if (!isValidPoint(info)) {
        logInvalidPoint(info);
        return;
    }

    m_latestTracks.insert(key, info);
    m_pendingRemovals.remove(key);
    m_pendingUpserts.insert(key, pointPayload(info));
    updateTrackCountLabel();
    scheduleFlush();
}

void Track3DWidget::clearRadarData()
{
    m_latestTracks.clear();
    m_pendingUpserts.clear();
    m_pendingRemovals.clear();
    m_snapshotRequired = false;
    updateTrackCountLabel();

    if (m_pageReady && m_rendererActive) {
        emit m_bridge->clearRequested();
    } else {
        m_snapshotRequired = true;
    }
}

void Track3DWidget::setRangeFromMain(double minRange, double maxRange)
{
    if (!std::isfinite(minRange) || !std::isfinite(maxRange) || maxRange <= minRange) {
        return;
    }
    if (fuzzyEqual(m_minRangeM, minRange) && fuzzyEqual(m_maxRangeM, maxRange)) {
        return;
    }

    m_minRangeM = minRange;
    m_maxRangeM = maxRange;
    sendSceneState();
}

void Track3DWidget::setHeightRange(double minHeight, double maxHeight)
{
    if (!std::isfinite(minHeight) || !std::isfinite(maxHeight) || maxHeight <= minHeight) {
        return;
    }
    if (fuzzyEqual(m_minHeightM, minHeight) && fuzzyEqual(m_maxHeightM, maxHeight)) {
        return;
    }

    m_minHeightM = minHeight;
    m_maxHeightM = maxHeight;
    sendSceneState();
}

void Track3DWidget::setTrackVisible(bool visible)
{
    if (m_trackVisible == visible) return;
    m_trackVisible = visible;
    sendSceneState();
}

void Track3DWidget::setTbdTrackVisible(bool visible)
{
    if (m_tbdTrackVisible == visible) return;
    m_tbdTrackVisible = visible;
    sendSceneState();
}

void Track3DWidget::setCooperativeTrackVisible(bool visible)
{
    if (m_cooperativeTrackVisible == visible) return;
    m_cooperativeTrackVisible = visible;
    sendSceneState();
}

void Track3DWidget::setOnlyRecognizedDroneTracksVisible(bool enabled)
{
    if (m_onlyRecognizedDroneTracksVisible == enabled) return;
    m_onlyRecognizedDroneTracksVisible = enabled;
    sendSceneState();
}

void Track3DWidget::setTrackSizeRatio(double ratio)
{
    const double boundedRatio = qBound(0.5, ratio, 3.0);
    if (fuzzyEqual(m_trackSizeRatio, boundedRatio)) return;
    m_trackSizeRatio = boundedRatio;
    sendSceneState();
}

void Track3DWidget::setDisplayActive(bool active)
{
    setRendererActive(active);
    if (active && m_pageReady) {
        sendSnapshot();
    }
}

void Track3DWidget::resetView()
{
    if (m_pageReady && m_rendererActive) {
        emit m_bridge->resetViewRequested();
    }
}

void Track3DWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    setDisplayActive(true);
}

void Track3DWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    setDisplayActive(false);
    m_snapshotRequired = true;
    m_pendingUpserts.clear();
    m_pendingRemovals.clear();
    m_flushTimer->stop();
}

void Track3DWidget::scheduleFlush()
{
    if (!m_pageReady || !m_rendererActive) {
        m_snapshotRequired = true;
        m_pendingUpserts.clear();
        m_pendingRemovals.clear();
        return;
    }

    if (!m_flushTimer->isActive()) {
        m_flushTimer->start();
    }
}

void Track3DWidget::flushPendingUpdates()
{
    if (!m_pageReady || !m_rendererActive) {
        m_snapshotRequired = true;
        return;
    }
    if (m_snapshotRequired) {
        sendSnapshot();
        return;
    }
    if (m_pendingUpserts.isEmpty() && m_pendingRemovals.isEmpty()) {
        return;
    }

    QVariantList upserts;
    upserts.reserve(m_pendingUpserts.size());
    for (auto it = m_pendingUpserts.cbegin(); it != m_pendingUpserts.cend(); ++it) {
        upserts.append(it.value());
    }

    QStringList removals;
    removals.reserve(m_pendingRemovals.size());
    for (quint64 key : m_pendingRemovals) {
        const unsigned type = static_cast<unsigned>(key >> 32);
        const unsigned batch = static_cast<unsigned>(key & 0xffffffffULL);
        removals.append(makeTrackKeyString(type, batch));
    }

    m_pendingUpserts.clear();
    m_pendingRemovals.clear();
    emit m_bridge->trackDeltaChanged(upserts, removals);
}

void Track3DWidget::sendSnapshot()
{
    if (!m_pageReady || !m_rendererActive) {
        m_snapshotRequired = true;
        return;
    }

    QVariantList tracks;
    tracks.reserve(m_latestTracks.size());
    for (auto it = m_latestTracks.cbegin(); it != m_latestTracks.cend(); ++it) {
        tracks.append(pointPayload(it.value()));
    }

    m_pendingUpserts.clear();
    m_pendingRemovals.clear();
    m_snapshotRequired = false;
    emit m_bridge->snapshotChanged(tracks, sceneState());
}

void Track3DWidget::sendSceneState()
{
    if (m_pageReady && m_rendererActive) {
        emit m_bridge->sceneStateChanged(sceneState());
    } else {
        m_snapshotRequired = true;
    }
}

void Track3DWidget::setRendererActive(bool active)
{
    if (m_rendererActive == active) {
        return;
    }
    m_rendererActive = active;
    LOG_INFO(QString("[Track3DWidget] Renderer active=%1 pageReady=%2 cachedTracks=%3")
                 .arg(active)
                 .arg(m_pageReady)
                 .arg(m_latestTracks.size()));
    if (m_pageReady) {
        emit m_bridge->activeChanged(active);
    }
}

void Track3DWidget::updateTrackCountLabel()
{
    if (m_trackCountLabel) {
        m_trackCountLabel->setText(tr("航迹: %1").arg(m_latestTracks.size()));
    }
}

void Track3DWidget::logInvalidPoint(const PointInfo& info)
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastInvalidPointLogMs != 0
        && nowMs - m_lastInvalidPointLogMs < kInvalidPointLogIntervalMs) {
        return;
    }
    m_lastInvalidPointLogMs = nowMs;
    LOG_WARNING(QString("[Track3DWidget] Invalid track ignored: type=%1 batch=%2 range=%3 az=%4 el=%5 height=%6")
                    .arg(info.type)
                    .arg(info.batch)
                    .arg(info.range)
                    .arg(info.azimuth)
                    .arg(info.elevation)
                    .arg(info.altitute));
}

void Track3DWidget::onPageReady()
{
    if (m_showingFailurePage) {
        return;
    }
    m_pageReady = true;
    emit m_bridge->activeChanged(m_rendererActive);
    if (m_rendererActive) {
        sendSnapshot();
    }
    LOG_INFO("[Track3DWidget] WebGL page and WebChannel ready");
}

void Track3DWidget::onPageError(const QString& message)
{
    LOG_ERROR(QString("[Track3DWidget] JavaScript error: %1").arg(message));
}

void Track3DWidget::onLoadFinished(bool ok)
{
    if (m_showingFailurePage) {
        return;
    }
    if (!ok) {
        handleRendererFailure(QStringLiteral("failed to load qrc 3D page"));
    }
}

void Track3DWidget::handleRendererFailure(const QString& reason)
{
    m_pageReady = false;
    m_snapshotRequired = true;
    m_flushTimer->stop();
    LOG_ERROR(QString("[Track3DWidget] %1").arg(reason));

    if (!m_reloadAttempted) {
        m_reloadAttempted = true;
        QTimer::singleShot(500, this, [this]() {
            if (!m_webView || m_showingFailurePage) {
                return;
            }
            LOG_WARNING("[Track3DWidget] Reloading 3D page after renderer failure");
            m_webView->load(kTrack3DPageUrl);
        });
        return;
    }

    showRendererFailurePage(reason);
}

void Track3DWidget::showRendererFailurePage(const QString& reason)
{
    m_showingFailurePage = true;
    const QString html = QStringLiteral(
        "<!doctype html><html><head><meta charset='utf-8'></head>"
        "<body style='margin:0;background:#0a1010;color:#66ffcc;font-family:Microsoft YaHei,sans-serif;"
        "display:flex;align-items:center;justify-content:center;height:100vh;'>"
        "<div style='border:1px solid #00ff88;padding:18px 28px;background:#101818;'>"
        "<div style='font-size:18px;margin-bottom:8px;'>3D渲染不可用</div>"
        "<div style='font-size:12px;color:#9ab8ad;'>%1</div></div></body></html>")
        .arg(reason.toHtmlEscaped());
    m_webView->setHtml(html);
}
