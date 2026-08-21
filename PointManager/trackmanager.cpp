/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
 * @Description: 
 */
/**
 * @file trackmanager.cpp
 * @brief 航迹管理器实现文件
 * @details 实现雷达航迹数据的统一管理功能：
 *          - 与RadarDataManager集成的数据接收
 *          - 多航迹的并发管理和显示
 *          - 动态标签系统和交互功能
 *          - 航迹连线的几何计算和更新
 * @author DispCtrl Team
 * @date 2024
 */

#include "trackmanager.h"
#include "Basic/log.h"
#include <QBrush>
#include <QDateTime>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QVariant>
#include <QVarLengthArray>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/offlinerae.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件
#include "PolarDisp/tooltip.h"

namespace {

constexpr qreal kFocusedPointZ = INFO_Z + 20;
constexpr qreal kFocusedLineZ = INFO_Z + 19;
constexpr qreal kFocusedLabelZ = INFO_Z + 21;
constexpr qreal kFocusedLabelFontScale = 1.2;
constexpr int kPpiRefreshIntervalMs = 16; // ~60 FPS when the GUI thread keeps up.

quint64 makeTrackKey(PointType type, int batch)
{
    return (static_cast<quint64>(type) << 32)
         | static_cast<quint32>(batch);
}

QColor displayTrackColor(const PointInfo& info)
{
    if (OfflineRae::isOffline(info)) {
        return OfflineRae::colorFor(info, TRA_COLOR);
    }
    if (info.type == PointType::Track) {
        return (info.targetRecResult == 1) ? TRA_COLOR : DRONE_COLOR;
    }
    return trackTypeColor(info.type);
}

QString trackTooltipText(const PointInfo& info)
{
    const QString targetRecStr = (info.targetRecResult == 1) ? "无人机" : "其它";
    return QString("%1\nNum:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9\n识别:%10")
            .arg(trackTypeLabel(info.type))
            .arg(info.batch)
            .arg(info.range)
            .arg(info.azimuth)
            .arg(info.elevation)
            .arg(info.SNR)
            .arg(info.speed)
            .arg(info.altitute)
            .arg(info.amp)
            .arg(targetRecStr);
}

}

class TrackBatchItem : public QGraphicsItem
{
public:
    explicit TrackBatchItem(const TrackSeries* series)
        : m_series(series)
    {
        setZValue(LINE_Z);
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    QRectF boundingRect() const override
    {
        return m_bounds;
    }

    void includePoint(const QPointF& point)
    {
        const QRectF pointRect(point.x() - m_pointRadius,
                               point.y() - m_pointRadius,
                               m_pointRadius * 2.0,
                               m_pointRadius * 2.0);
        const QRectF nextRect = pointRect.adjusted(-10.0, -10.0, 10.0, 10.0);
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
        bool hasVisibleNode = false;
        if (m_series) {
            for (const TrackNode& node : m_series->nodes) {
                if (!node.pointVisible && !node.lineFromPrevVisible) {
                    continue;
                }
                const QRectF pointRect(node.scenePos.x() - m_pointRadius,
                                       node.scenePos.y() - m_pointRadius,
                                       m_pointRadius * 2.0,
                                       m_pointRadius * 2.0);
                nextBounds = hasVisibleNode ? nextBounds.united(pointRect) : pointRect;
                hasVisibleNode = true;
            }
        }
        m_hasContentBounds = hasVisibleNode;
        m_bounds = hasVisibleNode ? nextBounds.adjusted(-10.0, -10.0, 10.0, 10.0)
                                  : QRectF(-1.0, -1.0, 2.0, 2.0);
    }

    void setPointSizeRatio(float ratio)
    {
        if (ratio <= 0.0f) {
            ratio = 1.0f;
        }
        m_pointRadius = qMax<qreal>(1.0, TRA_SIZE * ratio * 0.5);
        rebuildBounds();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        if (!m_series) {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, m_series->focused);

        if (!m_series->focused) {
            QVarLengthArray<QLineF, 256> trackLines;
            QVarLengthArray<QLineF, 256> otherLines;
            QVarLengthArray<QLineF, 256> tbdLines;
            QVarLengthArray<QLineF, 256> cooperativeLines;
            QVarLengthArray<QPointF, 256> trackPoints;
            QVarLengthArray<QPointF, 256> otherPoints;
            QVarLengthArray<QPointF, 256> tbdPoints;
            QVarLengthArray<QPointF, 256> cooperativePoints;

            QVarLengthArray<QLineF, 256> offlineCyanLines;
            QVarLengthArray<QLineF, 256> offlineRedLines;
            QVarLengthArray<QLineF, 256> offlineBlueLines;
            QVarLengthArray<QPointF, 256> offlineCyanPoints;
            QVarLengthArray<QPointF, 256> offlineRedPoints;
            QVarLengthArray<QPointF, 256> offlineBluePoints;

            auto lineBucket = [&](const PointInfo& info) -> QVarLengthArray<QLineF, 256>& {
                if (OfflineRae::isOffline(info)) {
                    if (info.targetRecResult == OfflineRae::kColorRed) return offlineRedLines;
                    if (info.targetRecResult == OfflineRae::kColorBlue) return offlineBlueLines;
                    return offlineCyanLines;
                }
                if (info.type == PointType::TBDPointType) return tbdLines;
                if (info.type == PointType::CooperativeTrackPointType) return cooperativeLines;
                return (info.targetRecResult == 1) ? trackLines : otherLines;
            };
            auto pointBucket = [&](const PointInfo& info) -> QVarLengthArray<QPointF, 256>& {
                if (OfflineRae::isOffline(info)) {
                    if (info.targetRecResult == OfflineRae::kColorRed) return offlineRedPoints;
                    if (info.targetRecResult == OfflineRae::kColorBlue) return offlineBluePoints;
                    return offlineCyanPoints;
                }
                if (info.type == PointType::TBDPointType) return tbdPoints;
                if (info.type == PointType::CooperativeTrackPointType) return cooperativePoints;
                return (info.targetRecResult == 1) ? trackPoints : otherPoints;
            };

            for (int i = 1; i < m_series->nodes.size(); ++i) {
                const TrackNode& node = m_series->nodes[i];
                if (node.lineFromPrevVisible) {
                    const TrackNode& prevNode = m_series->nodes[i - 1];
                    lineBucket(node.info).append(QLineF(prevNode.scenePos, node.scenePos));
                }
            }

            for (const TrackNode& node : m_series->nodes) {
                if (node.pointVisible) {
                    pointBucket(node.info).append(node.scenePos);
                }
            }

            auto drawLines = [&](const QColor& color, const QVarLengthArray<QLineF, 256>& lines) {
                if (lines.isEmpty()) return;
                QPen pen(color);
                pen.setWidth(1);
                pen.setStyle(Qt::SolidLine);
                painter->setPen(pen);
                painter->drawLines(lines.constData(), lines.size());
            };
            auto drawPoints = [&](const QColor& color, const QVarLengthArray<QPointF, 256>& points) {
                if (points.isEmpty()) return;
                QPen pen(color);
                pen.setWidthF(qMax<qreal>(1.0, m_pointRadius * 2.0));
                pen.setCapStyle(Qt::RoundCap);
                painter->setPen(pen);
                painter->drawPoints(points.constData(), points.size());
            };

            drawLines(TRA_COLOR, trackLines);
            drawLines(DRONE_COLOR, otherLines);
            drawLines(trackTypeColor(PointType::TBDPointType), tbdLines);
            drawLines(trackTypeColor(PointType::CooperativeTrackPointType), cooperativeLines);
            drawLines(QColor(0, 255, 255), offlineCyanLines);
            drawLines(QColor(255, 0, 0), offlineRedLines);
            drawLines(QColor(0, 0, 255), offlineBlueLines);
            drawPoints(TRA_COLOR, trackPoints);
            drawPoints(DRONE_COLOR, otherPoints);
            drawPoints(trackTypeColor(PointType::TBDPointType), tbdPoints);
            drawPoints(trackTypeColor(PointType::CooperativeTrackPointType), cooperativePoints);
            drawPoints(QColor(0, 255, 255), offlineCyanPoints);
            drawPoints(QColor(255, 0, 0), offlineRedPoints);
            drawPoints(QColor(0, 0, 255), offlineBluePoints);
        } else {
            for (int i = 1; i < m_series->nodes.size(); ++i) {
                const TrackNode& node = m_series->nodes[i];
                if (!node.lineFromPrevVisible) {
                    continue;
                }
                const TrackNode& prevNode = m_series->nodes[i - 1];
                QPen linePen(displayTrackColor(node.info));
                linePen.setWidth(2);
                linePen.setStyle(Qt::SolidLine);
                linePen.setCapStyle(Qt::RoundCap);
                linePen.setJoinStyle(Qt::RoundJoin);
                painter->setPen(linePen);
                painter->drawLine(prevNode.scenePos, node.scenePos);
            }

            for (const TrackNode& node : m_series->nodes) {
                if (!node.pointVisible) {
                    continue;
                }

                const QColor color = displayTrackColor(node.info);
                QPen pointPen(color);
                pointPen.setWidth(2);
                painter->setPen(pointPen);

                painter->setBrush(Qt::NoBrush);
                const QRectF pointRect(node.scenePos.x() - m_pointRadius,
                                       node.scenePos.y() - m_pointRadius,
                                       m_pointRadius * 2.0,
                                       m_pointRadius * 2.0);
                const QPointF triangle[3] = {
                    QPointF(pointRect.center().x(), pointRect.top()),
                    QPointF(pointRect.right(), pointRect.bottom()),
                    QPointF(pointRect.left(), pointRect.bottom())
                };
                painter->drawPolygon(triangle, 3);
            }
        }

        painter->restore();
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override
    {
        const TrackNode* node = nearestVisibleNode(event->pos());
        if (!node) {
            TOOL_TIP->setVisible(false);
            QGraphicsItem::hoverMoveEvent(event);
            return;
        }

        TOOL_TIP->showTooltip(event->scenePos() + QPointF(15.0, 15.0),
                              trackTooltipText(node->info));
        QGraphicsItem::hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        TOOL_TIP->setVisible(false);
        QGraphicsItem::hoverLeaveEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            const TrackNode* node = nearestVisibleNode(event->pos());
            if (node && scene()) {
                scene()->setProperty("selectedBatchID", node->info.batch);
                scene()->setProperty("selectedPointInfo", QVariant::fromValue(node->info));
                event->accept();
                return;
            }
        }

        QGraphicsItem::mousePressEvent(event);
    }

private:
    const TrackNode* nearestVisibleNode(const QPointF& scenePos) const
    {
        if (!m_series) {
            return nullptr;
        }

        const qreal pickRadius = qMax<qreal>(8.0, m_pointRadius * 3.0);
        const qreal pickRadiusSq = pickRadius * pickRadius;
        const TrackNode* nearest = nullptr;
        qreal nearestDistanceSq = pickRadiusSq;

        for (const TrackNode& node : m_series->nodes) {
            if (!node.pointVisible) {
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
    const TrackSeries* m_series = nullptr;
    QRectF m_bounds = QRectF(-1.0, -1.0, 2.0, 2.0);
    qreal m_pointRadius = qMax<qreal>(1.0, TRA_SIZE * 0.5);
    bool m_hasContentBounds = false;
};

// ==================== DraggableLabel 可拖拽标签实现 ====================

/**
 * @brief DraggableLabel构造函数实现
 * @param parent 父图形项指针
 * @details 初始化可拖拽的航迹标签：
 *          - 启用移动和几何变化监听功能
 *          - 设置高层级Z值确保标签在最上层显示
 */
DraggableLabel::DraggableLabel(QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    setFlag(ItemIsMovable, true);                // 启用拖拽移动
    setFlag(ItemSendsGeometryChanges, true);     // 启用几何变化通知
    setZValue(INFO_Z);                           // 设置高层级，确保在点之上显示
    m_baseFont = font();
}

/**
 * @brief 设置标签的锚点关联
 * @param a 锚点图形项（通常是航迹点）
 * @param t 连接线图形项
 * @details 建立标签与锚点的动态关联：
 *          - 保存锚点和连线的引用
 *          - 设置连线的层级略低于标签
 */
void DraggableLabel::setAnchorItem(QGraphicsItem* a, QGraphicsLineItem* t)
{
    anchor = a;
    tether = t;
    if (tether) tether->setZValue(zValue()-1);  // 连线层级低于标签
}

void DraggableLabel::setFocused(bool focused)
{
    if (m_focused == focused) {
        return;
    }

    m_focused = focused;

    QFont nextFont = m_baseFont;
    if (focused) {
        const qreal pointSize = nextFont.pointSizeF();
        if (pointSize > 0.0) {
            nextFont.setPointSizeF(pointSize * kFocusedLabelFontScale);
        } else {
            const int pixelSize = nextFont.pixelSize();
            if (pixelSize > 0) {
                nextFont.setPixelSize(qRound(pixelSize * kFocusedLabelFontScale));
            }
        }
        nextFont.setBold(true);
    }
    setFont(nextFont);
    update();
}

/**
 * @brief 图形项变化事件处理
 * @param change 变化类型枚举
 * @param value 变化的值
 * @return 处理后的值
 * @details 监听位置变化事件，自动更新连线几何：
 *          - 当标签位置改变时，重新计算与锚点的连线
 *          - 连线始终连接标签中心和锚点位置
 */
QVariant DraggableLabel::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && anchor && tether) {
        // 计算标签中心点在场景中的坐标
        QPointF p1 = mapToScene(boundingRect().center());
        // 获取锚点在场景中的坐标
        QPointF p2 = anchor->scenePos();
        // 更新连线几何形状
        tether->setLine(QLineF(p1, p2));
    }
    return QGraphicsTextItem::itemChange(change, value);
}

/**
 * @brief 右键菜单事件：发射 rightClicked 信号，由 PPIView 弹出菜单
 * @param event 图形场景右键事件
 */
void DraggableLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    emit rightClicked(m_batchID);
    event->accept();
}

void DraggableLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (m_focused) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRectF backgroundRect = boundingRect().adjusted(-6.0, -4.0, 6.0, 4.0);
        QColor borderColor = defaultTextColor();
        QColor fillColor(8, 18, 18, 230);
        painter->setPen(QPen(borderColor, 1.5));
        painter->setBrush(QBrush(fillColor));
        painter->drawRoundedRect(backgroundRect, 4.0, 4.0);
        painter->restore();
    }

    QGraphicsTextItem::paint(painter, option, widget);
}

// ==================== TrackManager 航迹管理器实现 ====================

/**
 * @brief TrackManager构造函数实现
 * @param scene 图形场景指针
 * @param axis 极坐标轴指针
 * @param parent 父对象指针
 * @details 完成航迹管理器的初始化：
 *          1. 注册到统一数据管理器接收航迹数据
 *          2. 连接数据信号到对应的处理槽函数
 *          3. 设置默认的显示参数
 */

TrackManager::TrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    m_maxPointsPerBatch = qMax(1, CF_INS.displayConfig("max_track_points", 200));
    m_labelRefreshIntervalMs = qMax(0, CF_INS.displayConfig("track_label_refresh_ms", 200));

    m_batchRepaintTimer.setSingleShot(true);
    m_batchRepaintTimer.setInterval(kPpiRefreshIntervalMs);
    connect(&m_batchRepaintTimer, &QTimer::timeout, this, [this]() {
        const QSet<quint64> pending = m_pendingBatchRepaints;
        m_pendingBatchRepaints.clear();
        for (quint64 trackKey : pending) {
            auto it = mSeries.find(trackKey);
            if (it == mSeries.end() || !it->batchItem) {
                continue;
            }
            it->batchItem->update();
        }
    });

    // 注册到统一数据管理器，使用唯一标识符
    RADAR_DATA_MGR.registerView("TrackManager_" + QString::number((quintptr)this), this);

    // 注意：不在这里连接 RadarDataManager::trackReceived 信号
    // 数据流由 PPIScene 通过 Controller::traInfoProcess/tbdInfoProcess 统一管理
    // 避免双重连接导致每个航迹点被处理两次

    // 只连接数据清理信号
    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared,
            this, &TrackManager::clear);                // 响应数据清理
}

/**
 * @brief TrackManager析构函数实现
 * @details 确保资源的正确清理：
 *          - 从统一数据管理器注销当前视图
 *          - 清理所有航迹对象和连线
 *          - 断开信号连接
 */
TrackManager::~TrackManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("TrackManager_" + QString::number((quintptr)this));
    clear();  // 清理所有航迹
}

/**
 * @brief 设置点尺寸缩放比例
 * @param ratio 缩放比例，1.0为默认大小
 * @details 批量调整所有航迹点的显示尺寸：
 *          - 防护性检查确保比例值有效
 *          - 遍历所有航迹序列和节点
 *          - 应用新的缩放比例到每个航迹点
 *          - 更新显示状态确保变化生效
 */
void TrackManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.f) ratio = 1.f;  // 防护性检查
    mPointSizeRatio = ratio;

    // 遍历所有航迹序列
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        if (it->batchItem) {
            it->batchItem->setPointSizeRatio(mPointSizeRatio);
        }
        if (it->latestPoint) {
            it->latestPoint->resize(mPointSizeRatio);
        }
    }
    // 刷新所有显示以确保尺寸更新生效
    refreshAll();
}

/**
 * @brief 设置指定批次的颜色
 * @param batchID 批次ID
 * @param c 新的颜色值
 * @details 定制单条航迹的完整外观：
 *          1. 确保航迹序列存在
 *          2. 更新所有航迹点的颜色
 *          3. 更新所有连线的颜色
 *          4. 更新标签连线的颜色样式
 */
void TrackManager::setBatchColor(int batchID, const QColor& c)
{
    const quint64 trackKey = makeTrackKey(PointType::Track, batchID);
    if (!mSeries.contains(trackKey)) {
        ensureSeries(batchID);  // 默认使用DBT颜色
    }
    auto& s = mSeries[trackKey]; // 获取航迹序列引用
    s.color = c;

    if (s.latestPoint) {
        s.latestPoint->setColor(c);
    }
    if (s.batchItem) {
        s.batchItem->update();
    }

    // 更新标签连线颜色
    if (s.labelLine) {
        QPen pen(c);
        pen.setStyle(Qt::DashLine);             // 虚线样式
        s.labelLine->setPen(pen);
    }
}

/**
 * @brief 确保航迹序列存在
 * @param batchID 批次ID
 * @details 惰性创建航迹序列：
 *          - 检查指定批次是否已存在
 *          - 如不存在则创建新的航迹序列
 *          - 设置默认的颜色和可见性参数
 */
void TrackManager::ensureSeries(int batchID, PointType type)
{
    const quint64 trackKey = makeTrackKey(type, batchID);
    if (!mSeries.contains(trackKey)) {
        TrackSeries s;
        s.type = type;
        s.color = trackTypeColor(type);
        // s.visible 默认值是 true（在结构体定义中）
        mSeries.insert(trackKey, s);

        LOG_DEBUG(QString("[TrackManager] Created %1 display series: batch=%2 visible=%3 totalSeries=%4")
                  .arg(trackTypeLabel(type))
                  .arg(batchID)
                  .arg(s.visible)
                  .arg(mSeries.size()));
        return;
    }

}

void TrackManager::ensureBatchGraphics(quint64 trackKey)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) {
        return;
    }

    TrackSeries& s = it.value();
    if (!s.batchItem) {
        s.batchItem = new TrackBatchItem(&s);
        s.batchItem->setPointSizeRatio(mPointSizeRatio);
        mScene->addItem(s.batchItem);
    }
}

void TrackManager::scheduleBatchRepaint(TrackSeries& series)
{
    if (!series.batchItem) {
        return;
    }

    if (!series.nodes.isEmpty()) {
        const PointInfo& info = series.nodes.last().info;
        m_pendingBatchRepaints.insert(makeTrackKey(static_cast<PointType>(info.type), info.batch));
    }
    if (!m_batchRepaintTimer.isActive()) {
        m_batchRepaintTimer.start();
    }
}

void TrackManager::scheduleBatchRepaintAll()
{
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        if (!it->batchItem) {
            continue;
        }
        it->batchItem->rebuildBounds();
        m_pendingBatchRepaints.insert(it.key());
    }
    if (!m_pendingBatchRepaints.isEmpty() && !m_batchRepaintTimer.isActive()) {
        m_batchRepaintTimer.start();
    }
}

/**
 * @brief 极坐标转屏幕像素坐标
 * @param range 距离(米)
 * @param azimuthDeg 方位角(度)
 * @return 屏幕像素坐标
 * @details 通过PolarAxis进行坐标变换，确保与显示系统一致
 */
QPointF TrackManager::polarToPixel(float range, float azimuthDeg) const
{
    // 使用统一的 PolarAxis 进行坐标变换
    QPointF result = mAxis->polarToScene(range, azimuthDeg);

    // 添加调试打印
    // qCritical() << "[TrackManager::polarToPixel] Input: range=" << range
    //             << "m, azimuth=" << azimuthDeg << "°";
    // qCritical() << "    -> Output: x=" << result.x() << "y=" << result.y();

    return result;
}

/**
 * @brief 检查距离是否在显示范围内
 * @param range 距离值(公里)
 * @return true表示在显示范围内
 * @details 根据PolarAxis的当前最小/最大距离设置判断
 */
bool TrackManager::inRange(float range) const
{
    return (range >= mAxis->minRange() && range <= mAxis->maxRange());
}

/**
 * @brief 添加航迹点到管理器
 * @param info 航迹点信息结构体
 * @details 完整的航迹点添加流程：
 *          1. 确保指定批次的航迹序列存在
 *          2. 创建TrackPoint对象并配置外观
 *          3. 计算屏幕坐标位置
 *          4. 与前一点建立连线关系
 *          5. 应用可见性过滤规则
 *          6. 更新动态标签显示
 */
void TrackManager::addTrackPoint(const PointInfo& info)
{
    // 检查 statMethod==2，表示需要删除该批号的航迹
    if (info.statMethod == 2) {
        LOG_INFO(QString("[TrackManager::addTrackPoint] statMethod==2: removing batch=%1, series count before=%2")
                 .arg(info.batch).arg(mSeries.size()));
        removeSeries(makeTrackKey(static_cast<PointType>(info.type), info.batch));
        LOG_INFO(QString("[TrackManager::addTrackPoint] statMethod==2: batch=%1 removed, series count after=%2, emitting trackRemoved")
                 .arg(info.batch).arg(mSeries.size()));
        // 发出信号通知其他组件删除对应航迹
        emit trackRemoved(info.batch);
        return;
    }

    // 确保指定批次的航迹序列存在，并根据类型选择颜色
    PointType type = PointType::Track;
    if (info.type == PointType::TBDPointType) {
        type = PointType::TBDPointType;
    } else if (info.type == PointType::CooperativeTrackPointType) {
        type = PointType::CooperativeTrackPointType;
    }
    const quint64 trackKey = makeTrackKey(type, info.batch);
    ensureSeries(info.batch, type);
    ensureBatchGraphics(trackKey);
    auto& s = mSeries[trackKey];  // 获取航迹序列引用
    const QColor color = displayTrackColor(info);
    s.color = color;
    const bool firstPointInBatch = s.nodes.isEmpty();

    PointInfo copy = info;
    copy.type = info.type;          // 保持传入类型，用于标识DBT/TBD

    // 调试输出：查看原始数据和坐标转换
    // 使用 qCritical 确保在 Release 模式下也能看到（输出到 stderr）
    // qCritical() << "[TrackManager::addTrackPoint]"
    //             << "Batch:" << info.batch
    //             << "Range:" << copy.range
    //             << "Azimuth:" << copy.azimuth
    //             << "Elevation:" << copy.elevation;

    // 计算并设置屏幕坐标位置
    const QPointF pos = polarToPixel(copy.range, copy.azimuth);
    // qCritical() << "    -> Screen pos: x=" << pos.x() << "y=" << pos.y()
    //             << "pixelsPerMeter=" << mAxis->pixelsPerMeter()
    //             << "maxRange=" << mAxis->maxRange();
    // 应用可见性过滤：序列可见性 && 距离范围
    // 注意：移除角度过滤，因为主PPI界面应该显示所有方向的航迹（与检测点保持一致）
    // 角度过滤仅在扇区窗口（SectorWidget）中使用
    bool vis = s.visible && inRange(copy.range) && isTrackRecognitionVisible(copy);

    if (firstPointInBatch && info.type != PointType::Track) {
        LOG_INFO(QString("[TrackManager] First %1 point reached PPI: batch=%2 range=%3 azimuth=%4 visible=%5 batchVisible=%6 inRange=%7 recogVisible=%8")
                 .arg(trackTypeLabel(info.type))
                 .arg(info.batch)
                 .arg(copy.range, 0, 'f', 1)
                 .arg(copy.azimuth, 0, 'f', 2)
                 .arg(vis)
                 .arg(s.visible)
                 .arg(inRange(copy.range))
                 .arg(isTrackRecognitionVisible(copy)));
    }

    // 创建航迹节点
    TrackNode node;
    node.info = copy;
    node.scenePos = pos;
    node.pointVisible = vis;

    // 将节点添加到航迹序列
    s.nodes.push_back(node);
    limitBatchPoints(s);
    updateNodeLineVisibility(s);
    if (s.batchItem && s.nodes.last().pointVisible) {
        s.batchItem->includePoint(s.nodes.last().scenePos);
    }

    // 更新最新点的动态标签显示
    updateLatestInteractivePoint(trackKey);
    updateLatestLabel(trackKey, firstPointInBatch);
    updateBatchFocusStyle(trackKey);

    scheduleBatchRepaint(s);

    // 发出航迹点添加信号，用于更新选中航迹的信息显示
    emit trackPointAdded(info);
}

void TrackManager::limitBatchPoints(TrackSeries& series)
{
    if (!series.nodes.isEmpty() && OfflineRae::isOffline(series.nodes.first().info)) {
        return;
    }
    while (series.nodes.size() > m_maxPointsPerBatch) {
        series.nodes.removeFirst();
    }
}

void TrackManager::updateLatestInteractivePoint(quint64 trackKey)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) {
        return;
    }

    TrackSeries& s = it.value();
    if (s.nodes.isEmpty()) {
        if (s.latestPoint) {
            s.latestPoint->setVisible(false);
        }
        return;
    }

    TrackNode& latest = s.nodes.last();
    PointInfo copy = latest.info;
    const QColor color = displayTrackColor(copy);
    if (!s.latestPoint) {
        s.latestPoint = new TrackPoint(copy);
        mScene->addItem(s.latestPoint);
    } else {
        s.latestPoint->setInfo(copy);
    }

    s.latestPoint->setColor(color);
    s.latestPoint->resize(mPointSizeRatio);
    s.latestPoint->setFocused(m_focusedBatches.contains(trackKey));
    s.latestPoint->updatePosition(latest.scenePos.x(), latest.scenePos.y());
    s.latestPoint->setVisible(latest.pointVisible);
}

void TrackManager::updateNodeLineVisibility(TrackSeries& series)
{
    const bool recognitionVisible = isSeriesRecognitionVisible(series);
    for (int i = 0; i < series.nodes.size(); ++i) {
        TrackNode& node = series.nodes[i];
        node.pointVisible = series.visible
                         && recognitionVisible
                         && inRange(node.info.range);
        if (i == 0) {
            node.lineFromPrevVisible = false;
            continue;
        }

        const TrackNode& prevNode = series.nodes[i - 1];
        node.lineFromPrevVisible = series.visible
                                && recognitionVisible
                                && inRange(prevNode.info.range)
                                && inRange(node.info.range)
                                && inAngle(prevNode.info.azimuth)
                                && inAngle(node.info.azimuth);
    }
}

void TrackManager::updateLatestLabel(quint64 trackKey, bool force)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) return;

    auto& s = it.value();
    if (s.nodes.isEmpty()) return;

    TrackNode& latest = s.nodes.last();
    if (!s.latestPoint) {
        updateLatestInteractivePoint(trackKey);
    }
    if (!s.latestPoint) return;
    const auto& pi = latest.info;
    if (OfflineRae::isOffline(pi)) {
        if (s.label) {
            s.label->setVisible(false);
        }
        if (s.labelLine) {
            s.labelLine->setVisible(false);
        }
        return;
    }
    const QColor labelColor = displayTrackColor(pi);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool labelExists = (s.label != nullptr);

    // 创建/更新 label 与连线
    if (!s.label) {
        s.label = new DraggableLabel();
        s.label->setDefaultTextColor(labelColor);
        s.label->setZValue(INFO_Z);
        s.label->setBatchID(pi.batch);
        connect(s.label, &DraggableLabel::rightClicked,
                this,    &TrackManager::labelRightClicked);
        s.labelLine = new QGraphicsLineItem();
        QPen pen(labelColor);
        pen.setStyle(Qt::DashLine);
        s.labelLine->setPen(pen);
        s.labelLine->setZValue(INFO_Z);
        mScene->addItem(s.label);
        mScene->addItem(s.labelLine);
        s.label->setAnchorItem(s.latestPoint, s.labelLine);
    } else {
        s.label->setAnchorItem(s.latestPoint, s.labelLine);
    }

    const bool shouldThrottle = labelExists
                             && !force
                             && m_labelRefreshIntervalMs > 0
                             && !m_focusedBatches.contains(trackKey)
                             && (nowMs - s.lastLabelRefreshMs) < m_labelRefreshIntervalMs;
    if (shouldThrottle) {
        bool vis = s.visible && isSeriesRecognitionVisible(s) && inRange(pi.range);
        if (s.label) {
            s.label->setVisible(vis);
        }
        if (s.labelLine) {
            updateLineGeometry(s.labelLine, s.label->mapToScene(s.label->boundingRect().center()), latest.scenePos);
            s.labelLine->setVisible(vis);
        }
        return;
    }

    s.label->setDefaultTextColor(labelColor);
    QPen pen(labelColor);
    pen.setStyle(Qt::DashLine);
    s.labelLine->setPen(pen);

    // 标签内容：根据你的需求自由定制
    QString labelText;
    if (s.type == PointType::Track) {
        labelText = QString("batch : %1").arg(pi.batch);
    } else {
        QString typeText = trackTypeLabel(s.type);
        labelText = QString("%1:%2").arg(typeText).arg(pi.batch);
    }
    s.label->setPlainText(labelText);

    // 初始放在最新点的右上方
    QPointF anchor = latest.scenePos;
    QPointF labelPos = anchor + QPointF(30, -20);
    if (s.label->scene() == nullptr) {
        mScene->addItem(s.label);
    }
    s.label->setPos(labelPos);

    // 更新标签连线
    updateLineGeometry(s.labelLine, s.label->mapToScene(s.label->boundingRect().center()), anchor);
    updateBatchFocusStyle(trackKey);
    s.lastLabelRefreshMs = nowMs;

    // 可见性跟随最新点 & series
    bool vis = s.visible && isSeriesRecognitionVisible(s) && inRange(pi.range);
    if (s.label)     s.label->setVisible(vis);
    if (s.labelLine) s.labelLine->setVisible(vis);
}

void TrackManager::refreshAll()
{
    // Axis 像素比例或 min/max 变了：重算每个点的位置与显隐；连线也随之更新
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        auto& s = it.value();

        // 逐点更新位置与显隐
        for (int i = 0; i < s.nodes.size(); ++i) {
            auto& n = s.nodes[i];

            const auto& pi = n.info;
            QPointF pos = polarToPixel(pi.range, pi.azimuth);
            n.scenePos = pos;
            updateNodeVisibility(s, n);
        }
        updateNodeLineVisibility(s);
        if (s.batchItem) {
            s.batchItem->rebuildBounds();
        }
        updateLatestInteractivePoint(it.key());
        scheduleBatchRepaint(s);

        // 最新点标签与其连线
        if (s.nodes.size() > 0) {
            auto& latest = s.nodes.last();
            // 如果用户曾经拖动过 label，我们不改它位置，只更新 tether 线
            if (s.label && s.labelLine) {
                QPointF anchor = latest.scenePos;
                updateLineGeometry(s.labelLine, s.label->mapToScene(s.label->boundingRect().center()), anchor);
                bool vis = s.visible && isSeriesRecognitionVisible(s)
                           && inRange(latest.info.range);
                s.label->setVisible(vis);
                s.labelLine->setVisible(vis);
            } else {
                updateLatestLabel(it.key(), true);
            }
        }
    }
}

void TrackManager::updateNodeVisibility(const TrackSeries& series, TrackNode& node)
{
    // 移除角度过滤，与 addTrackPoint 保持一致
    bool vis = series.visible && isSeriesRecognitionVisible(series)
            && inRange(node.info.range);
    node.pointVisible = vis;
}

void TrackManager::setAngleRange(double startDeg, double endDeg)
{
    LOG_DEBUG(QString("[TrackManager::setAngleRange] Setting angle range: start %1 end %2")
                  .arg(startDeg)
                  .arg(endDeg));
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    refreshAll();
}

bool TrackManager::inAngle(float azimuthDeg) const
{
    double a = fmod(azimuthDeg, 360.0);
    if (a < 0) a += 360.0;
    double s = fmod(m_angleStart, 360.0);
    double e = fmod(m_angleEnd, 360.0);
    if (s < 0) s += 360.0;
    if (e < 0) e += 360.0;

    bool result;
    if (s <= e) {
        result = (a >= s && a <= e);
    } else {
        result = (a >= s || a <= e);
    }

    // 添加调试日志（每100次只打印一次以减少日志量）
    static int callCount = 0;
    if (++callCount % 100 == 0) {
        LOG_DEBUG(QString("[TrackManager::inAngle] azimuth %1 normalized %2 range [%3-%4] result %5")
                  .arg(azimuthDeg)
                  .arg(a)
                  .arg(s)
                  .arg(e)
                  .arg(result));
    }

    return result;
}

void TrackManager::setBatchVisible(int batchID, bool vis)
{
    const quint64 trackKey = makeTrackKey(PointType::Track, batchID);
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) return;
    it->visible = vis;
    updateBatchVisibility(trackKey);
}

void TrackManager::setBatchFocused(int batchID, bool focused)
{
    if (focused) {
        m_focusedBatches.insert(makeTrackKey(PointType::Track, batchID));
    } else {
        m_focusedBatches.remove(makeTrackKey(PointType::Track, batchID));
    }

    updateBatchFocusStyle(makeTrackKey(PointType::Track, batchID));
}

bool TrackManager::isBatchFocused(int batchID) const
{
    return m_focusedBatches.contains(makeTrackKey(PointType::Track, batchID));
}

void TrackManager::setAllVisible(bool vis)
{
    LOG_DEBUG(QString("[TrackManager::setAllVisible] %1 - Series count: %2")
                  .arg(vis)
                  .arg(mSeries.size()));

    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        LOG_DEBUG(QString("  Setting batch %1 visible to %2").arg(it.key()).arg(vis));
        it->visible = vis;
        updateBatchVisibility(it.key());
    }
}

void TrackManager::setTypeVisible(PointType type, bool vis)
{
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        if (it->type != type) continue;
        it->visible = vis;
        updateBatchVisibility(it.key());
    }
}

void TrackManager::setMaxPointsPerBatch(int maxPoints)
{
    if (maxPoints < 1) {
        maxPoints = 1;
    }

    m_maxPointsPerBatch = maxPoints;
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        limitBatchPoints(it.value());
        updateLatestLabel(it.key(), true);
        updateBatchVisibility(it.key());
        updateBatchFocusStyle(it.key());
    }
}

void TrackManager::setOnlyRecognizedDroneTracksVisible(bool enabled)
{
    if (m_onlyRecognizedDroneTracksVisible == enabled) return;
    m_onlyRecognizedDroneTracksVisible = enabled;
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        updateBatchVisibility(it.key());
    }
}

//更新航迹批的可见性
void TrackManager::updateBatchVisibility(quint64 trackKey)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) return;

    auto& s = it.value();
    updateNodeLineVisibility(s);
    if (s.batchItem) {
        s.batchItem->rebuildBounds();
    }
    updateLatestInteractivePoint(trackKey);
    scheduleBatchRepaint(s);

    // 最新点的标签与连线
    if (!s.nodes.isEmpty()) {
        auto& latest = s.nodes.last();
        if (s.label) {
            s.label->setVisible(s.visible && isSeriesRecognitionVisible(s) && inRange(latest.info.range));
        }
        if (s.labelLine) {
            s.labelLine->setVisible(s.visible && isSeriesRecognitionVisible(s) && inRange(latest.info.range));
        }
    }
}

void TrackManager::updateBatchFocusStyle(quint64 trackKey)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) return;

    const bool focused = m_focusedBatches.contains(trackKey);
    auto& s = it.value();
    s.focused = focused;

    if (s.latestPoint) {
        s.latestPoint->setFocused(focused);
        s.latestPoint->setZValue(focused ? kFocusedPointZ : POINT_Z);
    }
    if (s.batchItem) {
        s.batchItem->setZValue(focused ? kFocusedLineZ : LINE_Z);
        s.batchItem->update();
    }

    if (s.label) {
        s.label->setFocused(focused);
        s.label->setZValue(focused ? kFocusedLabelZ : INFO_Z);
    }
    if (s.labelLine) {
        s.labelLine->setZValue(focused ? kFocusedLineZ : INFO_Z);
    }
}

bool TrackManager::isTrackRecognitionVisible(const PointInfo& info) const
{
    if (!m_onlyRecognizedDroneTracksVisible) return true;
    if (info.type != PointType::Track) return true;
    return info.targetRecResult == 1;
}

bool TrackManager::isSeriesRecognitionVisible(const TrackSeries& series) const
{
    if (!m_onlyRecognizedDroneTracksVisible) return true;
    if (series.type != PointType::Track) return true;
    if (series.nodes.isEmpty()) return false;
    const TrackNode& latest = series.nodes.last();
    return isTrackRecognitionVisible(latest.info);
}

void TrackManager::removeBatch(int batchID)
{
    removeSeries(makeTrackKey(PointType::Track, batchID));
}

void TrackManager::removeSeries(quint64 trackKey)
{
    auto it = mSeries.find(trackKey);
    if (it == mSeries.end()) return;

    m_focusedBatches.remove(trackKey);
    m_pendingBatchRepaints.remove(trackKey);

    auto& s = it.value();
    s.nodes.clear();

    if (s.batchItem) { mScene->removeItem(s.batchItem); delete s.batchItem; s.batchItem = nullptr; }
    if (s.latestPoint) { mScene->removeItem(s.latestPoint); delete s.latestPoint; s.latestPoint = nullptr; }
    if (s.labelLine) { mScene->removeItem(s.labelLine); delete s.labelLine; s.labelLine = nullptr; }
    if (s.label)     { mScene->removeItem(s.label);     delete s.label;     s.label = nullptr; }

    mSeries.erase(it);
}

void TrackManager::clear()
{
    QList<quint64> keys = mSeries.keys();
    for (quint64 trackKey : keys) removeSeries(trackKey);
    mSeries.clear();
    m_focusedBatches.clear();
}

void TrackManager::updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b)
{
    if (!line) return;
    line->setLine(QLineF(a, b));
    line->setZValue(LINE_Z); // 在点之下、网格之上
}

bool TrackManager::latestPointInfo(int batchID, PointInfo& out) const
{
    auto it = mSeries.constFind(makeTrackKey(PointType::Track, batchID));
    if (it == mSeries.constEnd()) return false;
    const TrackSeries& series = it.value();
    if (series.nodes.isEmpty()) return false;
    const TrackNode& last = series.nodes.last();
    out = last.info;
    return true;
}

bool TrackManager::pointInfoAt(const QPointF& scenePos, PointInfo& out, qreal pickRadius) const
{
    const qreal pickRadiusSq = pickRadius * pickRadius;
    const TrackNode* nearest = nullptr;
    qreal nearestDistanceSq = pickRadiusSq;

    for (auto it = mSeries.cbegin(); it != mSeries.cend(); ++it) {
        const TrackSeries& series = it.value();
        for (const TrackNode& node : series.nodes) {
            if (!node.pointVisible) {
                continue;
            }
            const QPointF delta = node.scenePos - scenePos;
            const qreal distanceSq = delta.x() * delta.x() + delta.y() * delta.y();
            if (distanceSq <= nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                nearest = &node;
            }
        }
    }

    if (!nearest) {
        return false;
    }

    out = nearest->info;
    return true;
}
