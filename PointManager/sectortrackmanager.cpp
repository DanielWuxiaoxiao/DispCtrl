/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:51
 * @Description: 
 */
/**
 * @file sectortrackmanager.cpp
 * @brief 扇形航迹管理器实现文件
 * @details 实现扇形显示区域的航迹管理功能：
 *          - 与RadarDataManager集成的数据接收
 *          - 扇形区域的多航迹并发管理
 *          - 动态标签系统和交互功能
 *          - 扇形范围的航迹连线几何计算
 * @author DispCtrl Team
 * @date 2024
 */

#include "sectortrackmanager.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件
#include <QPen>
#include <QtMath>
#include <QDebug>

namespace {

quint64 makeTrackKey(PointType type, int batch)
{
    return (static_cast<quint64>(type) << 32)
         | static_cast<quint32>(batch);
}

}

// ==================== SectorDraggableLabel 扇形可拖拽标签实现 ====================

/**
 * @brief SectorDraggableLabel构造函数实现
 * @param parent 父图形项指针
 * @details 初始化扇形区域的可拖拽航迹标签：
 *          - 启用移动和几何变化监听功能
 *          - 设置高层级Z值确保标签在最上层显示
 *          - 专门适配扇形显示区域
 */
SectorDraggableLabel::SectorDraggableLabel(QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    setFlag(ItemIsMovable, true);                // 启用拖拽移动
    setFlag(ItemSendsGeometryChanges, true);     // 启用几何变化通知
    setZValue(INFO_Z);                           // 设置高层级，确保在点之上显示
}

/**
 * @brief 设置扇形标签的锚点关联
 * @param anchor 锚点图形项（通常是航迹点）
 * @param tether 连接线图形项
 * @details 建立扇形标签与锚点的动态关联：
 *          - 保存锚点和连线的引用
 *          - 设置连线的层级略低于标签
 */
void SectorDraggableLabel::setAnchorItem(QGraphicsItem* anchor, QGraphicsLineItem* tether)
{
    m_anchor = anchor;
    m_tether = tether;
    if (m_tether) {
        m_tether->setZValue(zValue() - 1);  // 连线层级低于标签
    }
}

/**
 * @brief 扇形标签的图形项变化事件处理
 * @param change 变化类型枚举
 * @param value 变化的值
 * @return 处理后的值
 * @details 监听位置变化事件，自动更新扇形区域内连线几何：
 *          - 当标签位置改变时，重新计算与锚点的连线
 *          - 连线始终连接标签中心和锚点位置
 */
QVariant SectorDraggableLabel::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && m_anchor && m_tether) {
        // 计算标签中心点在场景中的坐标
        QPointF labelCenter = mapToScene(boundingRect().center());
        // 获取锚点在场景中的坐标
        QPointF anchorPos = m_anchor->scenePos();
        // 更新连线几何形状
        m_tether->setLine(QLineF(labelCenter, anchorPos));
    }
    return QGraphicsTextItem::itemChange(change, value);
}

// ==================== SectorTrackManager 扇形航迹管理器实现 ====================

/**
 * @brief SectorTrackManager构造函数实现
 * @param scene 图形场景指针
 * @param axis 极坐标轴指针
 * @param parent 父对象指针
 * @details 完成扇形航迹管理器的初始化：
 *          1. 注册到统一数据管理器接收航迹数据
 *          2. 连接数据信号到对应的处理槽函数
 *          3. 设置默认的扇形角度范围
 */
SectorTrackManager::SectorTrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), m_scene(scene), m_axis(axis)
{
    m_maxPointsPerBatch = CF_INS.displayConfig("max_track_points", 200);

    // 注册到统一数据管理器
    RADAR_DATA_MGR.registerView("SectorTrackManager_" + QString::number((quintptr)this), this);

    // 航迹数据由 MainOverLayOut 通过 Controller 统一分发，避免重复绘制
    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared,
            this, &SectorTrackManager::clear);
}

SectorTrackManager::~SectorTrackManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("SectorTrackManager_" + QString::number((quintptr)this));
    clear();
}

void SectorTrackManager::addTrackPoint(const PointInfo& info)
{
    // 检查 statMethod==2，表示需要删除该批号的航迹
    if (info.statMethod == 2) {
        LOG_DEBUG(QString("[SectorTrackManager::addTrackPoint] statMethod==2, removing batch %1")
                  .arg(info.batch));
        removeSeries(makeTrackKey(static_cast<PointType>(info.type), info.batch));
        // 发出信号通知其他组件删除对应航迹
        emit trackRemoved(info.batch);
        return;
    }

    PointType type = PointType::Track;
    if (info.type == PointType::TBDPointType) {
        type = PointType::TBDPointType;
    } else if (info.type == PointType::CooperativeTrackPointType) {
        type = PointType::CooperativeTrackPointType;
    }
    const quint64 trackKey = makeTrackKey(type, info.batch);
    ensureSeries(info.batch, type);
    SectorTrackSeries& series = m_series[trackKey];
    const bool firstPointInBatch = series.nodes.isEmpty();

    // 创建航迹点
    PointInfo copy = info;

    TrackPoint* pt = new TrackPoint(copy);
    pt->setColor(series.color);
    pt->resize(m_pointSizeRatio);

    // 设置位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());

    // 设置可见性
    bool visible = series.visible && isPointVisible(copy);
    setItemSceneVisible(pt, visible);

    if (firstPointInBatch && info.type != PointType::Track) {
        LOG_INFO(QString("[SectorTrackManager] First %1 point reached sector view: batch=%2 range=%3 azimuth=%4 visible=%5 batchVisible=%6 inSector=%7")
                 .arg(trackTypeLabel(info.type))
                 .arg(info.batch)
                 .arg(copy.range, 0, 'f', 1)
                 .arg(copy.azimuth, 0, 'f', 2)
                 .arg(visible)
                 .arg(series.visible)
                 .arg(isPointVisible(copy)));
    }

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
            setItemSceneVisible(line, lineVisible);
            node.lineFromPrev = line;
        }
    }

    series.nodes.append(node);
    limitBatchPoints(series);

    // 更新最新点标签
    updateLatestLabel(trackKey);
}

void SectorTrackManager::limitBatchPoints(SectorTrackSeries& series)
{
    while (series.nodes.size() > m_maxPointsPerBatch) {
        SectorTrackNode oldNode = series.nodes.takeFirst();
        if (oldNode.lineFromPrev) {
            if (oldNode.lineFromPrev->scene() == m_scene) {
                m_scene->removeItem(oldNode.lineFromPrev);
            }
            delete oldNode.lineFromPrev;
        }
        if (oldNode.point) {
            if (oldNode.point->scene() == m_scene) {
                m_scene->removeItem(oldNode.point);
            }
            delete oldNode.point;
        }

        if (!series.nodes.isEmpty()) {
            SectorTrackNode& firstNode = series.nodes.first();
            if (firstNode.lineFromPrev) {
                if (firstNode.lineFromPrev->scene() == m_scene) {
                    m_scene->removeItem(firstNode.lineFromPrev);
                }
                delete firstNode.lineFromPrev;
                firstNode.lineFromPrev = nullptr;
            }
        }
    }
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
            setItemSceneVisible(node.point, visible);

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
                    setItemSceneVisible(node.lineFromPrev, lineVisible);
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
                setItemSceneVisible(series.label, labelVisible);
                setItemSceneVisible(series.labelLine, labelVisible);
            }
        }
    }
}

void SectorTrackManager::setBatchVisible(int batchID, bool visible)
{
    const quint64 trackKey = makeTrackKey(PointType::Track, batchID);
    auto it = m_series.find(trackKey);
    if (it == m_series.end()) return;

    it->visible = visible;
    updateBatchVisibility(trackKey);
}

void SectorTrackManager::setAllVisible(bool visible)
{
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        it->visible = visible;
        updateBatchVisibility(it.key());
    }
}

void SectorTrackManager::setTypeVisible(PointType type, bool visible)
{
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        if (it->type != type) continue;
        it->visible = visible;
        updateBatchVisibility(it.key());
    }
}

void SectorTrackManager::setMaxPointsPerBatch(int maxPoints)
{
    if (maxPoints < 1) {
        maxPoints = 1;
    }

    m_maxPointsPerBatch = maxPoints;
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        limitBatchPoints(it.value());
        updateLatestLabel(it.key());
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
    const quint64 trackKey = makeTrackKey(PointType::Track, batchID);
    if (!m_series.contains(trackKey)) {
        ensureSeries(batchID);
    }
    SectorTrackSeries& series = m_series[trackKey];
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
    removeSeries(makeTrackKey(PointType::Track, batchID));
}

void SectorTrackManager::removeSeries(quint64 trackKey)
{
    const int batchID = static_cast<int>(trackKey & 0xffffffffULL);
    LOG_DEBUG(QString("[SectorTrackManager::removeBatch] ===== CALLED with batchID: %1 =====")
                  .arg(batchID));

    auto it = m_series.find(trackKey);
    if (it == m_series.end()) {
        LOG_DEBUG(QString("[SectorTrackManager::removeBatch] Batch %1 not found in series")
                      .arg(batchID));
        return;
    }

    LOG_DEBUG(QString("[SectorTrackManager::removeBatch] Found batch %1, removing...").arg(batchID));
    SectorTrackSeries& series = it.value();

    // 删除所有节点
    for (SectorTrackNode& node : series.nodes) {
        if (node.lineFromPrev) {
            if (node.lineFromPrev->scene() == m_scene) {
                m_scene->removeItem(node.lineFromPrev);
            }
            delete node.lineFromPrev;
        }
        if (node.point) {
            if (node.point->scene() == m_scene) {
                m_scene->removeItem(node.point);
            }
            delete node.point;
        }
    }

    // 删除标签
    if (series.labelLine) {
        if (series.labelLine->scene() == m_scene) {
            m_scene->removeItem(series.labelLine);
        }
        delete series.labelLine;
    }
    if (series.label) {
        if (series.label->scene() == m_scene) {
            m_scene->removeItem(series.label);
        }
        delete series.label;
    }

    m_series.erase(it);
}

void SectorTrackManager::clear()
{
    QList<quint64> keys = m_series.keys();
    for (quint64 trackKey : keys) {
        removeSeries(trackKey);
    }
}

void SectorTrackManager::ensureSeries(int batchID, PointType type)
{
    const quint64 trackKey = makeTrackKey(type, batchID);
    if (!m_series.contains(trackKey)) {
        SectorTrackSeries series;
        series.type = type;
        series.color = trackTypeColor(type);
        series.visible = true;
        m_series.insert(trackKey, series);

        LOG_DEBUG(QString("[SectorTrackManager] Created %1 display series: batch=%2 totalSeries=%3")
              .arg(trackTypeLabel(type))
              .arg(batchID)
              .arg(m_series.size()));
        return;
    }

}

void SectorTrackManager::updateLatestLabel(quint64 trackKey)
{
    auto it = m_series.find(trackKey);
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

        series.label->setAnchorItem(latestNode.point, series.labelLine);
    } else {
        series.label->setAnchorItem(latestNode.point, series.labelLine);
    }

    // 设置标签内容
    const PointInfo& info = latestNode.point->infoRef();
    QString typeText = trackTypeLabel(series.type);
    QString labelText = QString("%1:%2").arg(typeText).arg(info.batch);
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
    setItemSceneVisible(series.label, visible);
    setItemSceneVisible(series.labelLine, visible);
}

void SectorTrackManager::updateBatchVisibility(quint64 trackKey)
{
    auto it = m_series.find(trackKey);
    if (it == m_series.end()) return;

    SectorTrackSeries& series = it.value();

    for (int i = 0; i < series.nodes.size(); ++i) {
        SectorTrackNode& node = series.nodes[i];
        if (node.point) {
            bool visible = series.visible && isPointVisible(node.point->infoRef());
            setItemSceneVisible(node.point, visible);
        }

        if (node.lineFromPrev && i > 0) {
            TrackPoint* prevPoint = series.nodes[i-1].point;
            if (prevPoint) {
                bool lineVisible = series.visible &&
                                 isPointVisible(prevPoint->infoRef()) &&
                                 isPointVisible(node.point->infoRef());
                setItemSceneVisible(node.lineFromPrev, lineVisible);
            }
        }
    }

    // 更新标签可见性
    if (!series.nodes.isEmpty()) {
        SectorTrackNode& latestNode = series.nodes.last();
        if (series.label && series.labelLine) {
            bool visible = series.visible && isPointVisible(latestNode.point->infoRef());
            setItemSceneVisible(series.label, visible);
            setItemSceneVisible(series.labelLine, visible);
        }
    }
}

void SectorTrackManager::updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b)
{
    if (!line) return;
    line->setLine(QLineF(a, b));
    line->setZValue(LINE_Z);
}

void SectorTrackManager::setItemSceneVisible(QGraphicsItem* item, bool visible)
{
    if (!item || !m_scene) {
        return;
    }

    item->setVisible(visible);
    if (visible) {
        if (!item->scene()) {
            m_scene->addItem(item);
        }
        return;
    }

    if (item->scene() == m_scene) {
        m_scene->removeItem(item);
    }
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
