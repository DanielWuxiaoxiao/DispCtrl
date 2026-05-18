/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
 * @Description: 
 */
/**
 * @file detmanager.cpp
 * @brief 检测点管理器实现文件
 * @details 实现雷达检测点的统一管理功能：
 *          - 与RadarDataManager集成的数据接收
 *          - 高效的批量点对象管理
 *          - 实时坐标变换和显示更新
 *          - 角度和距离范围的动态过滤
 * @author DispCtrl Team
 * @date 2024
 */

#include "Basic/log.h"

#include "detmanager.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件
#include <QDebug>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVarLengthArray>
#include "PolarDisp/tooltip.h"

namespace {

constexpr int kPpiRefreshIntervalMs = 16; // ~60 FPS when the GUI thread keeps up.

QString detectionTooltipText(const PointInfo& info)
{
    return QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
            .arg(QString::fromUtf8(DET_LABEL))
            .arg(info.range)
            .arg(info.azimuth)
            .arg(info.elevation)
            .arg(info.SNR)
            .arg(info.speed)
            .arg(info.altitute)
            .arg(info.amp);
}

} // namespace

class DetBatchItem : public QGraphicsItem
{
public:
    explicit DetBatchItem(const QVector<DetNode>* nodes)
        : m_nodes(nodes)
    {
        setZValue(POINT_Z - 1);
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::NoButton);
    }

    QRectF boundingRect() const override
    {
        return m_bounds;
    }

    void includePoint(const QPointF& point)
    {
        const QRectF pointRect(point.x() - m_radius,
                               point.y() - m_radius,
                               m_radius * 2.0,
                               m_radius * 2.0);
        const QRectF nextRect = pointRect.adjusted(-8.0, -8.0, 8.0, 8.0);
        if (m_hasContentBounds && m_bounds.contains(nextRect)) {
            return;
        }

        prepareGeometryChange();
        m_bounds = m_hasContentBounds ? m_bounds.united(nextRect) : nextRect;
        m_hasContentBounds = true;
    }

    void rebuildBounds()
    {
        prepareGeometryChange();
        QRectF nextBounds;
        bool hasVisiblePoint = false;
        if (m_nodes) {
            for (const DetNode& node : *m_nodes) {
                if (!node.visible) {
                    continue;
                }
                const QRectF pointRect(node.scenePos.x() - m_radius,
                                       node.scenePos.y() - m_radius,
                                       m_radius * 2.0,
                                       m_radius * 2.0);
                nextBounds = hasVisiblePoint ? nextBounds.united(pointRect) : pointRect;
                hasVisiblePoint = true;
            }
        }

        m_hasContentBounds = hasVisiblePoint;
        m_bounds = hasVisiblePoint ? nextBounds.adjusted(-8.0, -8.0, 8.0, 8.0)
                                   : QRectF(-1.0, -1.0, 2.0, 2.0);
    }

    void setPointSizeRatio(float ratio)
    {
        if (ratio <= 0.0f) {
            ratio = 1.0f;
        }
        m_radius = qMax<qreal>(1.0, DET_SIZE * ratio * 0.5);
        rebuildBounds();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        if (!m_nodes) {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        QPen pen(DET_COLOR);
        pen.setWidthF(qMax<qreal>(1.0, m_radius * 2.0));
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        QVarLengthArray<QPointF, 2048> visiblePoints;
        for (const DetNode& node : *m_nodes) {
            if (node.visible) {
                visiblePoints.append(node.scenePos);
            }
        }

        if (!visiblePoints.isEmpty()) {
            painter->drawPoints(visiblePoints.constData(), visiblePoints.size());
        }

        painter->restore();
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override
    {
        const DetNode* nearest = nearestVisibleNode(event->pos());
        if (!nearest) {
            TOOL_TIP->setVisible(false);
            QGraphicsItem::hoverMoveEvent(event);
            return;
        }

        TOOL_TIP->showTooltip(event->scenePos() + QPointF(15.0, 15.0),
                              detectionTooltipText(nearest->info));
        QGraphicsItem::hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        TOOL_TIP->setVisible(false);
        QGraphicsItem::hoverLeaveEvent(event);
    }

private:
    const DetNode* nearestVisibleNode(const QPointF& scenePos) const
    {
        if (!m_nodes) {
            return nullptr;
        }

        const qreal pickRadius = qMax<qreal>(6.0, m_radius * 3.0);
        const qreal pickRadiusSq = pickRadius * pickRadius;
        const DetNode* nearest = nullptr;
        qreal nearestDistanceSq = pickRadiusSq;

        for (const DetNode& node : *m_nodes) {
            if (!node.visible) {
                continue;
            }
            const QPointF delta = node.scenePos - scenePos;
            const qreal distanceSq = delta.x() * delta.x() + delta.y() * delta.y();
            if (distanceSq <= nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                nearest = &node;
            }
        }

        return nearest;
    }

private:
    const QVector<DetNode>* m_nodes = nullptr;
    QRectF m_bounds = QRectF(-1.0, -1.0, 2.0, 2.0);
    qreal m_radius = qMax<qreal>(1.0, DET_SIZE * 0.5);
    bool m_hasContentBounds = false;
};

/**
 * @brief DetManager构造函数实现
 * @param scene 图形场景指针
 * @param axis 极坐标轴指针
 * @param parent 父对象指针
 * @details 完成检测点管理器的初始化：
 *          1. 注册到统一数据管理器接收检测点数据
 *          2. 连接数据信号到对应的处理槽函数
 *          3. 设置默认的显示参数
 */

DetManager::DetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    m_maxPoints = qMax(100, CF_INS.displayConfig("max_points", 1000));

    mBatchItem = new DetBatchItem(&mNodes);
    mScene->addItem(mBatchItem);

    mRepaintTimer.setSingleShot(true);
    mRepaintTimer.setInterval(kPpiRefreshIntervalMs);
    connect(&mRepaintTimer, &QTimer::timeout, this, [this]() {
        if (!mRepaintPending || !mBatchItem) {
            return;
        }
        mRepaintPending = false;
        mBatchItem->update();
    });

    // 注册到统一数据管理器，使用唯一标识符
    RADAR_DATA_MGR.registerView("DetManager_" + QString::number((quintptr)this), this);

        // 检测点数据由 PPIScene 通过 Controller::detInfoProcess 统一分发，
        // 这里仅保留清空通知，避免同一批点被重复添加两次。
        connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared,
            this, &DetManager::clear);             // 响应数据清理
}

/**
 * @brief DetManager析构函数实现
 * @details 确保资源的正确清理：
 *          - 从统一数据管理器注销当前视图
 *          - 清理所有检测点对象
 *          - 断开信号连接
 */
DetManager::~DetManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("DetManager_" + QString::number((quintptr)this));
    mNodes.clear();
    if (mBatchItem) {
        mScene->removeItem(mBatchItem);
        delete mBatchItem;
        mBatchItem = nullptr;
    }
}

/**
 * @brief 添加检测点到管理器
 * @param info 检测点信息结构体
 * @details 完整的检测点创建流程：
 *          1. 复制并标记为检测点类型
 *          2. 创建DetPoint对象并配置外观
 *          3. 计算屏幕坐标位置
 *          4. 应用可见性过滤规则
 *          5. 添加到场景和内部容器
 */

void DetManager::addDetPoint(const PointInfo& info)
{
    // 创建检测点信息副本并设置类型
    PointInfo copy = info;
    copy.type = PointType::Detection; // 标记为检测点类型

    // 计算屏幕坐标位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);

    // 应用可见性过滤：全局可见性 && 距离范围 && 角度范围
    bool vis = mVisible && inRange(copy.range) && inAngle(copy.azimuth);

    // 创建节点并加入内部容器
    DetNode node;
    node.info = copy;
    node.scenePos = pos;
    node.visible = vis;
    mNodes.push_back(node);

    // 检查是否超出最大数量限制
    while (mNodes.size() > m_maxPoints) {
        mNodes.removeFirst();
    }

    if (mBatchItem && vis) {
        mBatchItem->includePoint(pos);
    }
    scheduleRepaint();
}

/**
 * @brief 刷新所有检测点显示
 * @details 当坐标轴参数或显示设置变化时的批量更新：
 *          1. 重新计算每个检测点的屏幕坐标
 *          2. 更新点的显示位置
 *          3. 重新应用可见性过滤规则
 *          4. 确保显示状态与当前设置一致
 */
void DetManager::refreshAll()
{
    for (auto& n : mNodes) {
        // 获取检测点信息并重新计算位置
        const auto& pi = n.info;
        QPointF pos = polarToPixel(pi.range, pi.azimuth);
        n.scenePos = pos;

        // 重新应用可见性过滤
        n.visible = mVisible && inRange(pi.range) && inAngle(pi.azimuth);
    }

    if (mBatchItem) {
        mBatchItem->rebuildBounds();
    }
    scheduleRepaint();
}

/**
 * @brief 设置全局可见性状态
 * @param vis 可见性标志
 * @details 批量控制所有检测点的显示/隐藏：
 *          - 更新全局可见性标志
 *          - 遍历所有检测点应用新状态
 *          - 结合距离和角度过滤条件
 */
void DetManager::setAllVisible(bool vis)
{
    mVisible = vis;
    for (auto& n : mNodes) {
        const bool in = inRange(n.info.range) && inAngle(n.info.azimuth);
        n.visible = mVisible && in;
    }
    if (mBatchItem) {
        mBatchItem->rebuildBounds();
    }
    scheduleRepaint();
}

/**
 * @brief 设置角度显示范围
 * @param startDeg 起始角度(度)
 * @param endDeg 结束角度(度)
 * @details 设置检测点的角度过滤扇区：
 *          - 保存新的角度范围参数
 *          - 刷新所有检测点的显示状态
 *          - 支持跨越0度的角度范围
 */
void DetManager::setAngleRange(double startDeg, double endDeg)
{
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    // 更新所有点的显隐状态
    refreshAll();
}

/**
 * @brief 设置最大监测点数量限制
 * @param maxPoints 最大监测点数量
 * @details 设置监测点数量上限，超出时自动删除最旧的监测点：
 *          - 更新最大数量限制
 *          - 立即清理超出限制的旧监测点
 *          - 采用FIFO策略，删除最先添加的点
 */
void DetManager::setMaxPoints(int maxPoints)
{
    if (maxPoints < 100) maxPoints = 100;  // 最小值保护
    m_maxPoints = maxPoints;

    // 立即清理超出限制的旧监测点
    while (mNodes.size() > m_maxPoints) {
        mNodes.removeFirst();
    }
    if (mBatchItem) {
        mBatchItem->rebuildBounds();
    }
    scheduleRepaint();

    LOG_DEBUG(QString("DetManager: Max points limit set to %1, current count: %2")
                  .arg(m_maxPoints)
                  .arg(mNodes.size()));
}

/**
 * @brief 检查角度是否在显示扇区内
 * @param azimuthDeg 方位角(度)
 * @return true表示在扇区内
 * @details 角度范围判断逻辑：
 *          1. 将角度归一化到[0,360)范围
 *          2. 处理起始和结束角度的归一化
 *          3. 支持跨越0度的扇区(如350-010度)
 */
bool DetManager::inAngle(float azimuthDeg) const
{
    // 归一化角度到 [0,360) 范围
    double a = fmod(azimuthDeg, 360.0);
    if (a < 0) a += 360.0;

    // 归一化起始和结束角度
    // 注意：如果角度正好是360，应保持为360而不是0（表示全圆）
    double s = m_angleStart;
    double e = m_angleEnd;

    // 只对非360的角度进行归一化
    if (s != 360.0) {
        s = fmod(s, 360.0);
        if (s < 0) s += 360.0;
    }
    if (e != 360.0) {
        e = fmod(e, 360.0);
        if (e < 0) e += 360.0;
    }

    // 处理两种情况：普通扇区和跨越0度的扇区
    if (s <= e) {
        return (a >= s && a <= e);      // 普通扇区：如90-270度
    } else {
        return (a >= s || a <= e);      // 跨越0度：如350-010度
    }
}

/**
 * @brief 设置点尺寸缩放比例
 * @param ratio 缩放比例，1.0为默认大小
 * @details 批量调整所有检测点的显示尺寸：
 *          - 防护性检查确保比例值有效
 *          - 遍历所有检测点应用新比例
 *          - 刷新显示确保尺寸更新
 */
void DetManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.f) ratio = 1.f;  // 防护性检查
    mPointSizeRatio = ratio;

    if (mBatchItem) {
        mBatchItem->setPointSizeRatio(mPointSizeRatio);
    }
    scheduleRepaint();
}

/**
 * @brief 清理所有检测点
 * @details 完整的资源清理流程：
 *          1. 遍历所有检测点节点
 *          2. 从图形场景移除检测点
 *          3. 删除检测点对象释放内存
 *          4. 清空内部容器
 */
void DetManager::clear()
{
    mNodes.clear();  // 清空容器
    if (mBatchItem) {
        mBatchItem->rebuildBounds();
    }
    scheduleRepaint();
}

void DetManager::scheduleRepaint()
{
    mRepaintPending = true;
    if (!mRepaintTimer.isActive()) {
        mRepaintTimer.start();
    }
}

bool DetManager::pointInfoAt(const QPointF& scenePos, PointInfo& out, qreal pickRadius) const
{
    const qreal pickRadiusSq = pickRadius * pickRadius;
    const DetNode* nearest = nullptr;
    qreal nearestDistanceSq = pickRadiusSq;

    for (const DetNode& node : mNodes) {
        if (!node.visible) {
            continue;
        }

        const QPointF delta = node.scenePos - scenePos;
        const qreal distanceSq = delta.x() * delta.x() + delta.y() * delta.y();
        if (distanceSq <= nearestDistanceSq) {
            nearestDistanceSq = distanceSq;
            nearest = &node;
        }
    }

    if (!nearest) {
        return false;
    }

    out = nearest->info;
    return true;
}

/**
 * @brief 极坐标转屏幕坐标
 * @param range 距离值(公里)
 * @param azimuthDeg 方位角(度)
 * @return 屏幕像素坐标
 * @details 通过PolarAxis进行坐标变换，考虑当前的缩放和偏移
 */
QPointF DetManager::polarToPixel(float range, float azimuthDeg) const
{
    return mAxis->polarToScene(range, azimuthDeg);
}

/**
 * @brief 检查距离是否在显示范围内
 * @param range 距离值(公里)
 * @return true表示在显示范围内
 * @details 根据PolarAxis的当前最小/最大距离设置判断
 */
bool DetManager::inRange(float range) const
{
    return (range >= mAxis->minRange() && range <= mAxis->maxRange());
}
