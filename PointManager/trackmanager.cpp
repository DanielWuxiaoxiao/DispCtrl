/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:07
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
#include <QPen>
#include "Basic/DispBasci.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件

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
    // 注册到统一数据管理器，使用唯一标识符
    RADAR_DATA_MGR.registerView("TrackManager_" + QString::number((quintptr)this), this);
    
    // 连接统一数据管理器的信号到本地处理函数
    connect(&RADAR_DATA_MGR, &RadarDataManager::trackReceived, 
            this, &TrackManager::addTrackPoint);        // 接收航迹点数据
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
        // 对每个航迹的所有节点应用新比例
        for (auto& n : it->nodes) {
            if (n.point) n.point->resize(mPointSizeRatio);
        }
        // 更新该航迹的显示状态
        updateBatchVisibility(it.key());
    }
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
    ensureSeries(batchID);      // 确保序列存在
    auto& s = mSeries[batchID]; // 获取航迹序列引用
    
    // 更新所有节点的点和连线颜色
    for (auto& n : s.nodes) {
        if (n.point) n.point->setColor(c);      // 航迹点颜色
        if (n.lineFromPrev) {
            QPen pen(c);
            pen.setWidth(1);
            n.lineFromPrev->setPen(pen);        // 连线颜色
        }
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
void TrackManager::ensureSeries(int batchID)
{
    if (!mSeries.contains(batchID)) {
        TrackSeries s;
        s.color = TRA_COLOR;    // 默认航迹颜色
        mSeries.insert(batchID, s);
    }
}

/**
 * @brief 极坐标转屏幕坐标
 * @param range 距离值(公里)
 * @param azimuthDeg 方位角(度)
 * @return 屏幕像素坐标
 * @details 通过PolarAxis进行坐标变换，确保与显示系统一致
 */
QPointF TrackManager::polarToPixel(float range, float azimuthDeg) const
{
    // 使用统一的 PolarAxis 进行坐标变换
    return mAxis->polarToScene(range, azimuthDeg);
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
    // 确保指定批次的航迹序列存在
    ensureSeries(info.batch);
    auto& s = mSeries[info.batch];  // 获取航迹序列引用

    // 创建航迹点对象并设置基本属性
    PointInfo copy = info;
    copy.type = 2;                  // 标记为航迹点类型
    auto* pt = new TrackPoint(copy);
    pt->setColor(s.color);          // 应用航迹序列的颜色
    pt->resize(mPointSizeRatio);    // 应用当前缩放比例

    // 计算并设置屏幕坐标位置
    const QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    
    // 应用可见性过滤：序列可见性 && 距离范围 && 角度范围
    bool vis = s.visible && inRange(copy.range) && inAngle(copy.azimuth);
    pt->setVisible(vis);
    
    // 添加到图形场景
    mScene->addItem(pt);

    // 创建航迹节点
    TrackNode node;
    node.point = pt;

    // 与前一节点建立连线关系
    if (!s.nodes.isEmpty()) {
        auto* prev = s.nodes.last().point;  // 获取前一个航迹点
        if (prev) {
            // 创建连线对象
            auto* line = new QGraphicsLineItem();
            QPen pen(s.color);
            pen.setWidth(1);
            line->setPen(pen);
            
            // 设置连线几何形状
            updateLineGeometry(line,
                               prev->scenePos(),    // 前一点位置
                               pt->scenePos());     // 当前点位置
            
            // 连线可见性：两个点都在范围内且航迹序列可见时显示
            bool lineVis = s.visible && inRange(s.nodes.last().point->infoRef().range) && inRange(copy.range)
                       && inAngle(s.nodes.last().point->infoRef().azimuth) && inAngle(copy.azimuth);
            line->setVisible(lineVis);
            mScene->addItem(line);
            node.lineFromPrev = line;  // 保存连线引用
        }
    }

    // 将节点添加到航迹序列
    s.nodes.push_back(node);

    // 更新最新点的动态标签显示
    updateLatestLabel(info.batch);
}

void TrackManager::updateLatestLabel(int batchID)
{
    auto it = mSeries.find(batchID);
    if (it == mSeries.end()) return;

    auto& s = it.value();
    if (s.nodes.isEmpty()) return;

    TrackNode& latest = s.nodes.last();
    if (!latest.point) return;

    // 创建/更新 label 与连线
    if (!s.label) {
        s.label = new DraggableLabel();
        s.label->setDefaultTextColor(Qt::white);
        s.label->setZValue(INFO_Z);
        s.labelLine = new QGraphicsLineItem();
        QPen pen(s.color);
        pen.setStyle(Qt::DashLine);
        s.labelLine->setPen(pen);
        s.labelLine->setZValue(INFO_Z);
        mScene->addItem(s.label);
        mScene->addItem(s.labelLine);
        s.label->setAnchorItem(latest.point, s.labelLine);
    } else {
        s.label->setAnchorItem(latest.point, s.labelLine);
    }

    // 标签内容：根据你的需求自由定制
    const auto& pi = latest.point->infoRef();
    QString labelText = QString("Num:%1")
                        .arg(pi.batch);
    s.label->setPlainText(labelText);

    // 初始放在最新点的右上方
    QPointF anchor = latest.point->scenePos();
    QPointF labelPos = anchor + QPointF(30, -20);
    if (s.label->scene() == nullptr) {
        mScene->addItem(s.label);
    }
    s.label->setPos(labelPos);

    // 更新标签连线
    updateLineGeometry(s.labelLine, s.label->mapToScene(s.label->boundingRect().center()), anchor);

    // 可见性跟随最新点 & series
    bool vis = s.visible && inRange(pi.range);
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
            if (!n.point) continue;

            const auto& pi = n.point->infoRef();
            QPointF pos = polarToPixel(pi.range, pi.azimuth);
            n.point->updatePosition(pos.x(), pos.y());
            updateNodeVisibility(n);

            // 更新与前一个点的连线
            if (n.lineFromPrev) {
                auto* prevPt = s.nodes[i-1].point;
                if (prevPt) {
                    updateLineGeometry(n.lineFromPrev, prevPt->scenePos(), n.point->scenePos());
                    bool vis = s.visible && inRange(prevPt->infoRef().range) && inRange(pi.range)
                               && inAngle(prevPt->infoRef().azimuth) && inAngle(pi.azimuth);
                    n.lineFromPrev->setVisible(vis);
                }
            }
        }

        // 最新点标签与其连线
        if (s.nodes.size() > 0) {
            auto& latest = s.nodes.last();
            if (latest.point) {
                // 如果用户曾经拖动过 label，我们不改它位置，只更新 tether 线
                if (s.label && s.labelLine) {
                    QPointF anchor = latest.point->scenePos();
                    updateLineGeometry(s.labelLine, s.label->mapToScene(s.label->boundingRect().center()), anchor);
                    bool vis = s.visible && inRange(latest.point->infoRef().range);
                    s.label->setVisible(vis);
                    s.labelLine->setVisible(vis);
                } else {
                    updateLatestLabel(it.key());
                }
            }
        }
    }
}

void TrackManager::updateNodeVisibility(TrackNode& node)
{
    if (!node.point) return;
    bool vis = inRange(node.point->infoRef().range) && inAngle(node.point->infoRef().azimuth);
    node.point->setVisible(vis);
    if (node.lineFromPrev) {
        // 线的显隐会在 refreshAll / addTrackPoint 中同时考虑前一点
        // 这里不单独处理，以免缺 prev 状态
    }
}

void TrackManager::setAngleRange(double startDeg, double endDeg)
{
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
    if (s <= e) return (a >= s && a <= e);
    return (a >= s || a <= e);
}

void TrackManager::setBatchVisible(int batchID, bool vis)
{
    auto it = mSeries.find(batchID);
    if (it == mSeries.end()) return;
    it->visible = vis;
    updateBatchVisibility(batchID);
}

void TrackManager::setAllVisible(bool vis)
{
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        it->visible = vis;
        updateBatchVisibility(it.key());
    }
}

//更新航迹批的可见性
void TrackManager::updateBatchVisibility(int batchID)
{
    auto it = mSeries.find(batchID);
    if (it == mSeries.end()) return;

    auto& s = it.value();
    for (int i = 0; i < s.nodes.size(); ++i) {
        auto& n = s.nodes[i];
        if (n.point) {
            bool in = inRange(n.point->infoRef().range);
            n.point->setVisible(s.visible && in);
        }
        if (n.lineFromPrev) {
            auto* prev = s.nodes[i-1].point;
            if (prev) {
                bool inCur = n.point ? inRange(n.point->infoRef().range) : false;
                bool inPrev = inRange(prev->infoRef().range);
                n.lineFromPrev->setVisible(s.visible && inCur && inPrev);
            } else {
                n.lineFromPrev->setVisible(false);
            }
        }
    }

    // 最新点的标签与连线
    if (!s.nodes.isEmpty()) {
        auto& latest = s.nodes.last();
        if (s.label)     s.label->setVisible(s.visible && inRange(latest.point->infoRef().range));
        if (s.labelLine) s.labelLine->setVisible(s.visible && inRange(latest.point->infoRef().range));
    }
}

void TrackManager::removeBatch(int batchID)
{
    auto it = mSeries.find(batchID);
    if (it == mSeries.end()) return;

    auto& s = it.value();
    for (auto& n : s.nodes) {
        if (n.lineFromPrev) { mScene->removeItem(n.lineFromPrev); delete n.lineFromPrev; n.lineFromPrev = nullptr; }
        if (n.point)        { mScene->removeItem(n.point);        delete n.point;        n.point = nullptr; }
    }
    s.nodes.clear();

    if (s.labelLine) { mScene->removeItem(s.labelLine); delete s.labelLine; s.labelLine = nullptr; }
    if (s.label)     { mScene->removeItem(s.label);     delete s.label;     s.label = nullptr; }

    mSeries.erase(it);
}

void TrackManager::clear()
{
    QList<int> keys = mSeries.keys();
    for (int id : keys) removeBatch(id);
    mSeries.clear();
}

void TrackManager::updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b)
{
    if (!line) return;
    line->setLine(QLineF(a, b));
    line->setZValue(LINE_Z); // 在点之下、网格之上
}
