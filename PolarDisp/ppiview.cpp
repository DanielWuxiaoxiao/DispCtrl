#include "ppiview.h"

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "PolarDisp/polaraxis.h"
#include "PolarDisp/ppisscene.h"

PPIView::PPIView(QWidget* parent)
    : QGraphicsView(parent)
{
    setBackgroundBrush(Qt::black);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setMouseTracking(true);
}

void PPIView::setPPIScene(PPIScene* scene)
{
    m_scene = scene;
    setScene(scene);
    if (m_scene) {
        m_scene->updateSceneSize(viewport()->size());
    }
}

void PPIView::enableRubberBandZoom(bool enabled)
{
    m_rubberBandZoom = enabled;
    setDragMode(enabled ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
}

void PPIView::mousePressEvent(QMouseEvent* event)
{
    if (m_rubberBandZoom && event->button() == Qt::LeftButton) {
        m_dragOrigin = event->pos();
        if (!m_band) {
            m_band = new QRubberBand(QRubberBand::Rectangle, viewport());
        }
        m_band->setGeometry(QRect(m_dragOrigin, QSize()));
        m_band->show();
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void PPIView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_scene && m_scene->axis()) {
        const PolarAxis::PolarCoord polar = m_scene->axis()->sceneToPolar(mapToScene(event->pos()));
        emit cursorPositionChanged(polar.distance / 1000.0, polar.azimuthDeg);
    }

    if (m_rubberBandZoom && m_band && m_band->isVisible()) {
        m_band->setGeometry(QRect(m_dragOrigin, event->pos()).normalized());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void PPIView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_rubberBandZoom && m_band && m_band->isVisible() && event->button() == Qt::LeftButton) {
        const QRect selection = m_band->geometry();
        m_band->hide();
        if (selection.width() >= 10 && selection.height() >= 10) {
            emit areaSelected(mapToScene(selection).boundingRect());
        }
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void PPIView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_scene) {
        m_scene->updateSceneSize(event->size());
    }
    emit viewResized(event->size());
}
