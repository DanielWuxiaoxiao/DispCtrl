#include "point.h"
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include "Basic/DispBasci.h"
#include "PolarDisp/tooltip.h"

Point::Point(PointInfo &pi) : info(pi)
{
    QString typeStr;
    switch (info.type) {
    case 1:
    {
        typeStr = DET_LABEL;
        break;
    }
    case 2:
    {
        typeStr = TRA_LABEL;
        break;
    }
    }

    if (info.type == 1) {
        text = QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
                .arg(typeStr).arg(info.range).arg(info.azimuth).arg(info.elevation)
                .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp);
    } else {
        text = QString("%1\nNum:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
                .arg(typeStr).arg(info.batch).arg(info.range).arg(info.azimuth).arg(info.elevation)
                .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp);
    }

    setAcceptHoverEvents(true);
    setZValue(POINT_Z); // 让点在网格上层
}

void Point::updatePosition(float x, float y)
{
    mX = x; mY = y;
    setSmallRect();
}

void Point::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    TOOL_TIP->showTooltip(event->screenPos(), text);
    setBigRect();
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void Point::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    TOOL_TIP->showTooltip(event->screenPos(), text);
    setBigRect();
    QGraphicsEllipseItem::hoverMoveEvent(event);
}

void Point::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
#ifdef Q_OS_LINUX
    TOOL_TIP->setHoldingState(false);
#else
    TOOL_TIP->setVisible(false);
#endif
    setSmallRect();
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

void Point::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

void Point::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}

// ===== DetPoint =====

DetPoint::DetPoint  (PointInfo &info) : Point(info)
{
    baseSmallW = baseSmallH = DET_SIZE;    // 也可用你原先的 DET_SIZE
    baseBigW   = baseBigH   = DET_BIG_SIZE;   // 也可用你原先的 DET_BIG_SIZE
    w = baseSmallW; h = baseSmallH;
    W = baseBigW;   H = baseBigH;
    setColor(DET_COLOR);
}

void DetPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;
    curRatio = ratio;
    w = baseSmallW / ratio;
    h = baseSmallH / ratio;
    W = baseBigW   / ratio;
    H = baseBigH   / ratio;
    setSmallRect();
}

void DetPoint::setColor(QColor color)
{
    QPen pen(color);
    pen.setWidth(1);
    setPen(pen);
    setBrush(QBrush(color));
}

void DetPoint::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

void DetPoint::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}

// ===== TrackPoint =====

TrackPoint::TrackPoint(PointInfo &info) : Point(info)
{
    baseSmallW = baseSmallH = TRA_SIZE;   // 航迹点略大些
    baseBigW   = baseBigH   = TRA_BIG_SIZE;
    w = baseSmallW; h = baseSmallH;
    W = baseBigW;   H = baseBigH;
    setColor(TRA_COLOR);
}

void TrackPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;
    curRatio = ratio;
    // 航迹点也做等比缩放，避免你之前出现 drawline int 限制导致的 BUG
    w = baseSmallW / ratio;
    h = baseSmallH / ratio;
    W = baseBigW   / ratio;
    H = baseBigH   / ratio;
    setSmallRect();
}

void TrackPoint::setColor(QColor color)
{
    QPen pen(color);
    pen.setWidth(1);
    setPen(pen);
    setBrush(QBrush(color));
}

void TrackPoint::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

void TrackPoint::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}
