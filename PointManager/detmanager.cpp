#include "detmanager.h"
#include "Basic/DispBasci.h"
#include "Controller/CentralDataManager.h"  // 添加新的头文件

DetManager::DetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    // 注册到统一数据管理器
    RADAR_DATA_MGR.registerView("DetManager_" + QString::number((quintptr)this), this);
    
    // 连接统一数据管理器的信号
    connect(&RADAR_DATA_MGR, &RadarDataManager::detectionReceived, 
            this, &DetManager::addDetPoint);
    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared, 
            this, &DetManager::clear);
}

DetManager::~DetManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("DetManager_" + QString::number((quintptr)this));
    clear();
}

void DetManager::addDetPoint(const PointInfo& info)
{
    PointInfo copy = info;
    copy.type = 1; // 检测点
    auto* pt = new DetPoint(copy);
    pt->resize(mPointSizeRatio);
    pt->setColor(DET_COLOR);

    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    // 仅当点在距离范围且角度范围内时可见
    bool vis = mVisible && inRange(copy.range) && inAngle(copy.azimuth);
    pt->setVisible(vis);
    mScene->addItem(pt);

    DetNode node;
    node.point = pt;
    mNodes.push_back(node);
}

void DetManager::refreshAll()
{
    for (auto& n : mNodes) {
        if (!n.point) continue;
        const auto& pi = n.point->infoRef();
        QPointF pos = polarToPixel(pi.range, pi.azimuth);
        n.point->updatePosition(pos.x(), pos.y());
    bool vis = mVisible && inRange(pi.range) && inAngle(pi.azimuth);
        n.point->setVisible(vis);
    }
}

void DetManager::setAllVisible(bool vis)
{
    mVisible = vis;
    for (auto& n : mNodes) {
        if (n.point) {
            bool in = inRange(n.point->infoRef().range) && inAngle(n.point->infoRef().azimuth);
            n.point->setVisible(mVisible && in);
        }
    }
}

void DetManager::setAngleRange(double startDeg, double endDeg)
{
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    // 更新所有点的显隐
    refreshAll();
}

bool DetManager::inAngle(float azimuthDeg) const
{
    // 归一到 [0,360)
    double a = fmod(azimuthDeg, 360.0);
    if (a < 0) a += 360.0;
    double s = fmod(m_angleStart, 360.0);
    double e = fmod(m_angleEnd, 360.0);
    if (s < 0) s += 360.0;
    if (e < 0) e += 360.0;
    if (s <= e) return (a >= s && a <= e);
    // wrap-around
    return (a >= s || a <= e);
}

void DetManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.f) ratio = 1.f;
    mPointSizeRatio = ratio;
    for (auto& n : mNodes) {
        if (n.point) n.point->resize(mPointSizeRatio);
    }
    refreshAll();
}

void DetManager::clear()
{
    for (auto& n : mNodes) {
        if (n.point) { mScene->removeItem(n.point); delete n.point; n.point = nullptr; }
    }
    mNodes.clear();
}

QPointF DetManager::polarToPixel(float range, float azimuthDeg) const
{
    return mAxis->polarToScene(range, azimuthDeg);
}

bool DetManager::inRange(float range) const
{
    return (range >= mAxis->minRange() && range <= mAxis->maxRange());
}
