/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:23:31
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 11:43:47
 * @Description: 
 */
/**
 * @file geofencemanager.cpp
 * @brief 电子围栏管理器实现
 */
#include "geofencemanager.h"
#include <algorithm>

GeoFenceManager::GeoFenceManager(QObject* parent)
    : QObject(parent)
{}

// ============================================================================
// 围栏管理
// ============================================================================

int GeoFenceManager::addFence(const GeoFenceDef& fence)
{
    GeoFenceDef f = fence;
    f.id = m_nextId++;
    m_fences.append(f);
    return f.id;
}

void GeoFenceManager::removeFence(int id)
{
    m_fences.erase(
        std::remove_if(m_fences.begin(), m_fences.end(),
                       [id](const GeoFenceDef& f){ return f.id == id; }),
        m_fences.end());
}

void GeoFenceManager::updateFence(int id, const QVector<QPointF>& vertices)
{
    for (auto& f : m_fences) {
        if (f.id == id) {
            f.vertices = vertices;
            return;
        }
    }
}

void GeoFenceManager::setFenceEnabled(int id, bool enabled)
{
    for (auto& f : m_fences) {
        if (f.id == id) {
            f.enabled = enabled;
            return;
        }
    }
}

void GeoFenceManager::clearAll()
{
    m_fences.clear();
}

// ============================================================================
// 目标检测
// ============================================================================

void GeoFenceManager::checkPoint(const PointInfo& pt)
{
    for (const auto& fence : m_fences) {
        if (!fence.enabled || fence.vertices.size() < 3)
            continue;

        if (pointInPolygon(static_cast<double>(pt.azimuth),
                           static_cast<double>(pt.range),
                           fence.vertices)) {
            const QString msg = QStringLiteral(
                "\u26a0 \u76ee\u6807[\u6279\u53f7%1]\u8fdb\u5165\u56f4\u680f[%2]  "
                "\u8ddd\u79bb%.0fm @\u65b9\u4f4d%.1f\u00b0")
                    .arg(pt.batch)
                    .arg(fence.name)
                    .arg(static_cast<double>(pt.range))
                    .arg(static_cast<double>(pt.azimuth));
            emit fenceAlert(msg, fence.id, pt);
        }
    }
}

// ============================================================================
// 射线法 点-多边形检测
// (azimuth, range) 视为 2D 笛卡尔坐标 (x=az, y=range)
// ============================================================================

bool GeoFenceManager::pointInPolygon(double azDeg, double rangM,
                                     const QVector<QPointF>& verts)
{
    const double px = azDeg;
    const double py = rangM;
    const int n = verts.size();
    bool inside = false;

    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double xi = verts[i].x(), yi = verts[i].y();
        const double xj = verts[j].x(), yj = verts[j].y();

        const bool intersect = ((yi > py) != (yj > py)) &&
                               (px < (xj - xi) * (py - yi) / (yj - yi) + xi);
        if (intersect)
            inside = !inside;
    }

    return inside;
}
