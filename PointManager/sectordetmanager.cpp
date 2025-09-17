// sectordetmanager.cpp
#include "sectordetmanager.h"
#include "Basic/DispBasci.h"
#include <QtMath>
#include <QDebug>

SectorDetManager::SectorDetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), m_scene(scene), m_axis(axis)
{
}

SectorDetManager::~SectorDetManager()
{
    clear();
}

void SectorDetManager::addDetPoint(const PointInfo& info)
{
    // 创建检测点
    PointInfo copy = info;
    copy.type = 1; // 检测点类型
    
    DetPoint* pt = new DetPoint(copy);
    pt->resize(m_pointSizeRatio);
    pt->setColor(DET_COLOR);
    
    // 计算位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    
    // 设置可见性（只有在扇形范围内的点才可见）
    bool visible = m_visible && isPointVisible(copy);
    pt->setVisible(visible);
    
    // 添加到场景
    m_scene->addItem(pt);
    
    // 保存节点
    SectorDetNode node;
    node.point = pt;
    m_nodes.push_back(node);
}

void SectorDetManager::refreshAll()
{
    for (auto& node : m_nodes) {
        if (!node.point) continue;
        
        const PointInfo& info = node.point->infoRef();
        
        // 更新位置
        QPointF pos = polarToPixel(info.range, info.azimuth);
        node.point->updatePosition(pos.x(), pos.y());
        
        // 更新可见性
        bool visible = m_visible && isPointVisible(info);
        node.point->setVisible(visible);
    }
}

void SectorDetManager::setAllVisible(bool visible)
{
    m_visible = visible;
    
    for (auto& node : m_nodes) {
        if (node.point) {
            bool shouldShow = m_visible && isPointVisible(node.point->infoRef());
            node.point->setVisible(shouldShow);
        }
    }
}

void SectorDetManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.0f) ratio = 1.0f;
    
    m_pointSizeRatio = ratio;
    
    for (auto& node : m_nodes) {
        if (node.point) {
            node.point->resize(m_pointSizeRatio);
        }
    }
}

void SectorDetManager::setAngleRange(float minAngle, float maxAngle)
{
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
    
    // 更新所有点的可见性
    refreshAll();
}

void SectorDetManager::clear()
{
    for (auto& node : m_nodes) {
        if (node.point) {
            m_scene->removeItem(node.point);
            delete node.point;
            node.point = nullptr;
        }
    }
    m_nodes.clear();
}

QPointF SectorDetManager::polarToPixel(float range, float azimuthDeg) const
{
    return m_axis->polarToScene(range, azimuthDeg);
}

bool SectorDetManager::inRange(float range) const
{
    return (range >= m_axis->minRange() && range <= m_axis->maxRange());
}

bool SectorDetManager::inAngle(float azimuthDeg) const
{
    // 归一化角度到 [0, 360)
    double angle = fmod(azimuthDeg, 360.0);
    if (angle < 0) angle += 360.0;
    
    double minAngle = fmod(m_minAngle, 360.0);
    double maxAngle = fmod(m_maxAngle, 360.0);
    if (minAngle < 0) minAngle += 360.0;
    if (maxAngle < 0) maxAngle += 360.0;
    
    // 处理跨越0度的情况
    if (minAngle <= maxAngle) {
        return (angle >= minAngle && angle <= maxAngle);
    } else {
        // 跨越0度的扇形
        return (angle >= minAngle || angle <= maxAngle);
    }
}

bool SectorDetManager::isPointVisible(const PointInfo& info) const
{
    return inRange(info.range) && inAngle(info.azimuth);
}
