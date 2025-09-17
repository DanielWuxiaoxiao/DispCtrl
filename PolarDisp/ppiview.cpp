#include "ppiview.h"
#include "ppisscene.h"
#include "pviewtopleft.h"
#include "pointinfow.h"
#include <QMouseEvent>

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
}

void PPIView::setupOverlay() {
    // 顶角信息使用 QLabel 作为 overlay，不随缩放
    radarInfoW = new mainviewTopLeft(this);
    pointInfo = new PointInfoW(this);
    layoutOverlay();
}

void PPIView::layoutOverlay() {
    if (radarInfoW) {
        radarInfoW->move(0, 5);
    }
    if (pointInfo) {
        QSize s = pointInfo->size();
        pointInfo->move(width()-s.width()-8, 5);
    }
}

void PPIView::setPPIScene(PPIScene* scene) {
    m_scene = scene;
    QGraphicsView::setScene(scene);
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void PPIView::enableRubberBandZoom(bool on) {
    m_rubberBandZoom = on;
    setDragMode(on ? QGraphicsView::RubberBandDrag : QGraphicsView::ScrollHandDrag);
}

void PPIView::mousePressEvent(QMouseEvent* e) {
    if (m_rubberBandZoom && e->button()==Qt::LeftButton) {
        m_origin = e->pos();
        if (!m_band) m_band = new QRubberBand(QRubberBand::Rectangle, this);
        m_band->setGeometry(QRect(m_origin, QSize()));
        m_band->show();
    }
    QGraphicsView::mousePressEvent(e);
}

void PPIView::mouseMoveEvent(QMouseEvent* e) {
    if (m_rubberBandZoom && m_band) {
        m_band->setGeometry(QRect(m_origin, e->pos()).normalized());
    }
    QGraphicsView::mouseMoveEvent(e);
}

void PPIView::mouseReleaseEvent(QMouseEvent* e) {
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

void PPIView::resizeEvent(QResizeEvent* e) {
    // 当尺寸改变时，发出信号
    //  qDebug() << e->size();
    QGraphicsView::resizeEvent(e);
    emit viewResized(e->size());

    layoutOverlay();
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}
