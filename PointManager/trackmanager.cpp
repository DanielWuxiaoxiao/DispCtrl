#include "trackmanager.h"
#include <QPen>
#include "Basic/DispBasci.h"

//trainfo的实现
DraggableLabel::DraggableLabel(QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);
    setZValue(INFO_Z); // 高于点
}

void DraggableLabel::setAnchorItem(QGraphicsItem* a, QGraphicsLineItem* t)
{
    anchor = a;
    tether = t;
    if (tether) tether->setZValue(zValue()-1);
}

QVariant DraggableLabel::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && anchor && tether) {
        // 更新标签与锚点的连线
        QPointF p1 = mapToScene(boundingRect().center());
        QPointF p2 = anchor->scenePos();
        tether->setLine(QLineF(p1, p2));
    }
    return QGraphicsTextItem::itemChange(change, value);
}

// ===== TrackManager =====

TrackManager::TrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
}

void TrackManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.f) ratio = 1.f;
    mPointSizeRatio = ratio;
    for (auto it = mSeries.begin(); it != mSeries.end(); ++it) {
        for (auto& n : it->nodes) {
            if (n.point) n.point->resize(mPointSizeRatio);
        }
        updateBatchVisibility(it.key());
    }
}

void TrackManager::setBatchColor(int batchID, const QColor& c)
{
    ensureSeries(batchID);
    auto& s = mSeries[batchID];
    for (auto& n : s.nodes) {
        if (n.point) n.point->setColor(c);
        if (n.lineFromPrev) {
            QPen pen(c);
            pen.setWidth(1);
            n.lineFromPrev->setPen(pen);
        }
    }
    if (s.labelLine) {
        QPen pen(c);
        pen.setStyle(Qt::DashLine);
        s.labelLine->setPen(pen);
    }
}

void TrackManager::ensureSeries(int batchID)
{
    if (!mSeries.contains(batchID)) {
        TrackSeries s;
        s.color = TRA_COLOR;
        mSeries.insert(batchID, s);
    }
}

QPointF TrackManager::polarToPixel(float range, float azimuthDeg) const
{
    // 使用统一的 PolarAxis
    return mAxis->polarToScene(range, azimuthDeg);
}

bool TrackManager::inRange(float range) const
{
    return (range >= mAxis->minRange() && range <= mAxis->maxRange());
}

void TrackManager::addTrackPoint(const PointInfo& info)
{
    // 新点也要考虑 range 决定显隐
    ensureSeries(info.batch);
    auto& s = mSeries[info.batch];

    // 创建点
    PointInfo copy = info;
    copy.type = 2; // 航迹点
    auto* pt = new TrackPoint(copy);
    pt->setColor(s.color);
    pt->resize(mPointSizeRatio);

    const QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    bool vis = s.visible && inRange(copy.range) && inAngle(copy.azimuth);
    pt->setVisible(vis);
    mScene->addItem(pt);

    TrackNode node;
    node.point = pt;

    // 与前一节点连线
    if (!s.nodes.isEmpty()) {
        auto* prev = s.nodes.last().point;
        if (prev) {
            auto* line = new QGraphicsLineItem();
            QPen pen(s.color);
            pen.setWidth(1);
            line->setPen(pen);
            updateLineGeometry(line,
                               prev->scenePos(),
                               pt->scenePos());
            // 连线仅当两个点都在范围内且 series 可见时显示
            bool vis = s.visible && inRange(s.nodes.last().point->infoRef().range) && inRange(copy.range)
                       && inAngle(s.nodes.last().point->infoRef().azimuth) && inAngle(copy.azimuth);
            line->setVisible(vis);
            mScene->addItem(line);
            node.lineFromPrev = line;
        }
    }

    s.nodes.push_back(node);

    // 更新最新点标签
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
