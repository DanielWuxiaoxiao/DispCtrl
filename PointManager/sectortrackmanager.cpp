// sectortrackmanager.cpp
#include "sectortrackmanager.h"
#include "Basic/DispBasci.h"
#include <QPen>
#include <QtMath>
#include <QDebug>

// SectorDraggableLabel 实现
SectorDraggableLabel::SectorDraggableLabel(QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);
    setZValue(INFO_Z);
}

void SectorDraggableLabel::setAnchorItem(QGraphicsItem* anchor, QGraphicsLineItem* tether)
{
    m_anchor = anchor;
    m_tether = tether;
    if (m_tether) {
        m_tether->setZValue(zValue() - 1);
    }
}

QVariant SectorDraggableLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && m_anchor && m_tether) {
        // 更新标签与锚点的连线
        QPointF labelCenter = mapToScene(boundingRect().center());
        QPointF anchorPos = m_anchor->scenePos();
        m_tether->setLine(QLineF(labelCenter, anchorPos));
    }
    return QGraphicsTextItem::itemChange(change, value);
}

// SectorTrackManager 实现
SectorTrackManager::SectorTrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), m_scene(scene), m_axis(axis)
{
}

SectorTrackManager::~SectorTrackManager()
{
    clear();
}

void SectorTrackManager::addTrackPoint(const PointInfo& info)
{
    ensureSeries(info.batch);
    SectorTrackSeries& series = m_series[info.batch];
    
    // 创建航迹点
    PointInfo copy = info;
    copy.type = 2; // 航迹点类型
    
    TrackPoint* pt = new TrackPoint(copy);
    pt->setColor(series.color);
    pt->resize(m_pointSizeRatio);
    
    // 设置位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    
    // 设置可见性
    bool visible = series.visible && isPointVisible(copy);
    pt->setVisible(visible);
    
    m_scene->addItem(pt);
    
    SectorTrackNode node;
    node.point = pt;
    
    // 与前一个点连线
    if (!series.nodes.isEmpty()) {
        TrackPoint* prevPoint = series.nodes.last().point;
        if (prevPoint) {
            QGraphicsLineItem* line = new QGraphicsLineItem();
            QPen pen(series.color);
            pen.setWidth(1);
            line->setPen(pen);
            
            updateLineGeometry(line, prevPoint->scenePos(), pt->scenePos());
            
            // 连线只有当两个点都在扇形内时才可见
            bool lineVisible = series.visible && 
                             isPointVisible(prevPoint->infoRef()) && 
                             isPointVisible(copy);
            line->setVisible(lineVisible);
            
            m_scene->addItem(line);
            node.lineFromPrev = line;
        }
    }
    
    series.nodes.append(node);
    
    // 更新最新点标签
    updateLatestLabel(info.batch);
}

void SectorTrackManager::refreshAll()
{
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        SectorTrackSeries& series = it.value();
        
        // 更新所有节点
        for (int i = 0; i < series.nodes.size(); ++i) {
            SectorTrackNode& node = series.nodes[i];
            if (!node.point) continue;
            
            const PointInfo& info = node.point->infoRef();
            
            // 更新位置
            QPointF pos = polarToPixel(info.range, info.azimuth);
            node.point->updatePosition(pos.x(), pos.y());
            
            // 更新可见性
            bool visible = series.visible && isPointVisible(info);
            node.point->setVisible(visible);
            
            // 更新连线
            if (node.lineFromPrev && i > 0) {
                TrackPoint* prevPoint = series.nodes[i-1].point;
                if (prevPoint) {
                    updateLineGeometry(node.lineFromPrev, 
                                     prevPoint->scenePos(), 
                                     node.point->scenePos());
                    
                    bool lineVisible = series.visible && 
                                     isPointVisible(prevPoint->infoRef()) && 
                                     isPointVisible(info);
                    node.lineFromPrev->setVisible(lineVisible);
                }
            }
        }
        
        // 更新最新点标签
        if (!series.nodes.isEmpty()) {
            SectorTrackNode& latestNode = series.nodes.last();
            if (latestNode.point && series.label && series.labelLine) {
                QPointF anchorPos = latestNode.point->scenePos();
                updateLineGeometry(series.labelLine, 
                                 series.label->mapToScene(series.label->boundingRect().center()),
                                 anchorPos);
                
                bool labelVisible = series.visible && isPointVisible(latestNode.point->infoRef());
                series.label->setVisible(labelVisible);
                series.labelLine->setVisible(labelVisible);
            }
        }
    }
}

void SectorTrackManager::setBatchVisible(int batchID, bool visible)
{
    auto it = m_series.find(batchID);
    if (it == m_series.end()) return;
    
    it->visible = visible;
    updateBatchVisibility(batchID);
}

void SectorTrackManager::setAllVisible(bool visible)
{
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        it->visible = visible;
        updateBatchVisibility(it.key());
    }
}

void SectorTrackManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.0f) ratio = 1.0f;
    
    m_pointSizeRatio = ratio;
    
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        for (SectorTrackNode& node : it->nodes) {
            if (node.point) {
                node.point->resize(m_pointSizeRatio);
            }
        }
    }
}

void SectorTrackManager::setBatchColor(int batchID, const QColor& color)
{
    ensureSeries(batchID);
    SectorTrackSeries& series = m_series[batchID];
    series.color = color;
    
    // 更新所有点和线的颜色
    for (SectorTrackNode& node : series.nodes) {
        if (node.point) {
            node.point->setColor(color);
        }
        if (node.lineFromPrev) {
            QPen pen(color);
            pen.setWidth(1);
            node.lineFromPrev->setPen(pen);
        }
    }
    
    if (series.labelLine) {
        QPen pen(color);
        pen.setStyle(Qt::DashLine);
        series.labelLine->setPen(pen);
    }
}

void SectorTrackManager::setAngleRange(float minAngle, float maxAngle)
{
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
    
    // 刷新所有航迹的显示
    refreshAll();
}

void SectorTrackManager::removeBatch(int batchID)
{
    auto it = m_series.find(batchID);
    if (it == m_series.end()) return;
    
    SectorTrackSeries& series = it.value();
    
    // 删除所有节点
    for (SectorTrackNode& node : series.nodes) {
        if (node.lineFromPrev) {
            m_scene->removeItem(node.lineFromPrev);
            delete node.lineFromPrev;
        }
        if (node.point) {
            m_scene->removeItem(node.point);
            delete node.point;
        }
    }
    
    // 删除标签
    if (series.labelLine) {
        m_scene->removeItem(series.labelLine);
        delete series.labelLine;
    }
    if (series.label) {
        m_scene->removeItem(series.label);
        delete series.label;
    }
    
    m_series.erase(it);
}

void SectorTrackManager::clear()
{
    QList<int> keys = m_series.keys();
    for (int batchID : keys) {
        removeBatch(batchID);
    }
}

void SectorTrackManager::ensureSeries(int batchID)
{
    if (!m_series.contains(batchID)) {
        SectorTrackSeries series;
        series.color = TRA_COLOR;
        series.visible = true;
        m_series.insert(batchID, series);
    }
}

void SectorTrackManager::updateLatestLabel(int batchID)
{
    auto it = m_series.find(batchID);
    if (it == m_series.end() || it->nodes.isEmpty()) return;
    
    SectorTrackSeries& series = it.value();
    SectorTrackNode& latestNode = series.nodes.last();
    if (!latestNode.point) return;
    
    // 创建或更新标签
    if (!series.label) {
        series.label = new SectorDraggableLabel();
        series.label->setDefaultTextColor(Qt::white);
        series.label->setZValue(INFO_Z);
        
        series.labelLine = new QGraphicsLineItem();
        QPen pen(series.color);
        pen.setStyle(Qt::DashLine);
        series.labelLine->setPen(pen);
        series.labelLine->setZValue(INFO_Z);
        
        m_scene->addItem(series.label);
        m_scene->addItem(series.labelLine);
        
        series.label->setAnchorItem(latestNode.point, series.labelLine);
    } else {
        series.label->setAnchorItem(latestNode.point, series.labelLine);
    }
    
    // 设置标签内容
    const PointInfo& info = latestNode.point->infoRef();
    QString labelText = QString("Track:%1").arg(info.batch);
    series.label->setPlainText(labelText);
    
    // 设置初始位置
    QPointF anchorPos = latestNode.point->scenePos();
    QPointF labelPos = anchorPos + QPointF(30, -20);
    series.label->setPos(labelPos);
    
    // 更新连线
    updateLineGeometry(series.labelLine, 
                     series.label->mapToScene(series.label->boundingRect().center()),
                     anchorPos);
    
    // 设置可见性
    bool visible = series.visible && isPointVisible(info);
    series.label->setVisible(visible);
    series.labelLine->setVisible(visible);
}

void SectorTrackManager::updateBatchVisibility(int batchID)
{
    auto it = m_series.find(batchID);
    if (it == m_series.end()) return;
    
    SectorTrackSeries& series = it.value();
    
    for (int i = 0; i < series.nodes.size(); ++i) {
        SectorTrackNode& node = series.nodes[i];
        if (node.point) {
            bool visible = series.visible && isPointVisible(node.point->infoRef());
            node.point->setVisible(visible);
        }
        
        if (node.lineFromPrev && i > 0) {
            TrackPoint* prevPoint = series.nodes[i-1].point;
            if (prevPoint) {
                bool lineVisible = series.visible && 
                                 isPointVisible(prevPoint->infoRef()) && 
                                 isPointVisible(node.point->infoRef());
                node.lineFromPrev->setVisible(lineVisible);
            }
        }
    }
    
    // 更新标签可见性
    if (!series.nodes.isEmpty()) {
        SectorTrackNode& latestNode = series.nodes.last();
        if (series.label && series.labelLine) {
            bool visible = series.visible && isPointVisible(latestNode.point->infoRef());
            series.label->setVisible(visible);
            series.labelLine->setVisible(visible);
        }
    }
}

void SectorTrackManager::updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b)
{
    if (!line) return;
    line->setLine(QLineF(a, b));
    line->setZValue(LINE_Z);
}

QPointF SectorTrackManager::polarToPixel(float range, float azimuthDeg) const
{
    return m_axis->polarToScene(range, azimuthDeg);
}

bool SectorTrackManager::inRange(float range) const
{
    return (range >= m_axis->minRange() && range <= m_axis->maxRange());
}

bool SectorTrackManager::inAngle(float azimuthDeg) const
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

bool SectorTrackManager::isPointVisible(const PointInfo& info) const
{
    return inRange(info.range) && inAngle(info.azimuth);
}