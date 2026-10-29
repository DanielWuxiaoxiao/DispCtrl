#include "scanmarkeroverlay.h"
#include <QEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include "scanlayer.h"

ScanMarkerOverlay::ScanMarkerOverlay(QGraphicsView* view) : QWidget(view), m_view(view)
{
    setObjectName(QStringLiteral("ppiScanMarkerOverlay"));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    m_clock.start();
    view->viewport()->setProperty("scanMarkerOverlayActive", true);
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] { m_resetFades = true; requestRefresh(); });
    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { m_resetFades = true; requestRefresh(); });
    setGeometry(view->rect());
    show();
    raise();
}

void ScanMarkerOverlay::setSource(ScanLayer* layer)
{
    disconnect(m_markerConnection);
    disconnect(m_axisConnection);
    disconnect(m_sceneConnection);
    m_layer = layer;
    m_resetFades = true;
    if (layer) {
        m_markerConnection = connect(layer, &ScanLayer::markersChanged, this, [this] { requestRefresh(); });
        m_axisConnection = connect(layer, &QObject::destroyed, this, [this] { requestRefresh(); });
        if (layer->scene()) m_sceneConnection = connect(layer->scene(), &QGraphicsScene::sceneRectChanged,
                                                       this, [this] { requestRefresh(); });
    }
    requestRefresh();
}

std::array<QPainterPath, 4> ScanMarkerOverlay::mappedPaths() const
{
    std::array<QPainterPath, 4> paths;
    if (!m_layer || m_layer->scene() != m_view->scene()) return paths;
    const QTransform transform = m_layer->deviceTransform(m_view->viewportTransform());
    paths = m_layer->markerPaths();
    for (auto& path : paths) {
        if (path.isEmpty()) continue;
        path = transform.map(path);
        path.translate(m_view->viewport()->pos());
    }
    return paths;
}

void ScanMarkerOverlay::requestRefresh()
{
    if (m_pending) return;
    m_pending = true;
    QTimer::singleShot(16, this, [this] {
        m_pending = false;
        refreshFrame();
    });
}

void ScanMarkerOverlay::refreshFrame()
{
    setGeometry(m_view->rect());
    raise();
    if (m_layer) {
        const auto transform = m_layer->deviceTransform(m_view->viewportTransform());
        double radius = 0;
        for (const auto& line : m_layer->markerLines())
            if (!line.isNull()) { radius = QLineF(QPointF(), line.p2()).length(); break; }
        if (transform != m_lastTransform || qAbs(radius - m_lastRadius) > 0.001) m_resetFades = true;
        m_lastTransform = transform; m_lastRadius = radius;
    }
    const auto paths = mappedPaths();
    const qint64 now = m_clock.elapsed();
    QRegion region;
    bool animating = false;
    for (std::size_t id = 0; id < paths.size(); ++id) {
        auto& fade = m_fades[id];
        if (m_resetFades || paths[id].isEmpty()) {
            fade.current = paths[id]; fade.previous = {};
        } else if (paths[id] != fade.current) {
            fade.previous = fade.current; // At most one old position per panel; no growing trail.
            fade.current = paths[id]; fade.startedMs = now;
        }
        if (!fade.previous.isEmpty() && now - fade.startedMs >= 120) fade.previous = {};
        animating = animating || !fade.previous.isEmpty();
        for (const auto* path : {&fade.current, &fade.previous})
            if (!path->isEmpty()) region |= path->boundingRect().adjusted(-4, -4, 4, 4).toAlignedRect();
    }
    m_resetFades = false;
    update(region | m_previousRegion);
    m_previousRegion = region;
    if (animating && isVisible()) requestRefresh(); // Stop frame callbacks once the 120ms fade ends.
}

bool ScanMarkerOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Resize || event->type() == QEvent::Move || event->type() == QEvent::Show
        || event->type() == QEvent::Wheel || event->type() == QEvent::ChildAdded) {
        m_resetFades = true; // Geometry changes must not leave an old arc at the previous screen position.
        requestRefresh();
    }
    return QWidget::eventFilter(watched, event);
}

void ScanMarkerOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setClipRect(m_view->viewport()->geometry());
    painter.setRenderHint(QPainter::Antialiasing);
    const qint64 now = m_clock.elapsed();
    for (const auto& fade : m_fades) {
        const double t = fade.previous.isEmpty() ? 1.0 : qBound(0.0, (now - fade.startedMs) / 120.0, 1.0);
        painter.setOpacity(1.0 - t);
        ScanLayer::paintMarker(&painter, fade.previous);
        painter.setOpacity(t);
        ScanLayer::paintMarker(&painter, fade.current);
    }
}
