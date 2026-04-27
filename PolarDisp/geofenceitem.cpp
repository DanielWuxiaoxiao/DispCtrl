/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:19:26
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:32
 * @Description: 
 */
/**
 * @file geofenceitem.cpp
 * @brief PPI 电子围栏多边形图形项实现
 */
#include "geofenceitem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtMath>

GeoFenceItem::GeoFenceItem(int fenceId, QGraphicsItem* parent)
    : QGraphicsPolygonItem(parent)
    , m_id(fenceId)
{
    setZValue(50);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    // 默认外观：半透明黄色
    setPen(QPen(QColor(255, 200, 0, 200), 2));
    setBrush(QBrush(QColor(255, 200, 0, 30)));
}

void GeoFenceItem::setFenceName(const QString& name)
{
    m_name = name;
    update();
}

void GeoFenceItem::setVerticesPolar(const QVector<QPointF>& vertices, double pxPerMeter)
{
    QPolygonF poly;
    poly.reserve(vertices.size());

    for (const auto& v : vertices) {
        const double azRad   = qDegreesToRadians(v.x());   // x = azimuthDeg
        const double rangePx = v.y() * pxPerMeter;         // y = rangeM

        // PPI 坐标：北方为 -Y，东方为 +X
        poly << QPointF(rangePx * qSin(azRad),
                       -rangePx * qCos(azRad));
    }

    setPolygon(poly);
}

void GeoFenceItem::setAlert(bool alert)
{
    m_alert = alert;
    if (alert) {
        setPen(QPen(QColor(255, 60, 60, 220), 2, Qt::DashLine));
        setBrush(QBrush(QColor(255, 60, 60, 55)));
    } else {
        setPen(QPen(QColor(255, 200, 0, 200), 2));
        setBrush(QBrush(QColor(255, 200, 0, 30)));
    }
    update();
}

void GeoFenceItem::setEditMode(bool edit)
{
    m_edit = edit;
    setFlag(QGraphicsItem::ItemIsMovable, edit);
    update();
}

void GeoFenceItem::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget)
{
    QGraphicsPolygonItem::paint(painter, option, widget);

    // 绘制围栏名称
    if (!m_name.isEmpty()) {
        const QRectF br = boundingRect();
        painter->setPen(QColor(255, 230, 80));
        painter->setFont(QFont("Microsoft YaHei", 8));
        painter->drawText(br.topLeft() + QPointF(4, 14), m_name);
    }

    // 编辑模式：绘制顶点手柄
    if (m_edit) {
        painter->setBrush(QColor(255, 220, 0, 200));
        painter->setPen(Qt::NoPen);
        for (const auto& pt : polygon()) {
            painter->drawEllipse(pt, 5, 5);
        }
    }
}
