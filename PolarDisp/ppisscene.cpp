#include "ppisscene.h"
#include "polaraxis.h"
#include <QGraphicsTextItem>
#include "polargrid.h"
#include "PointManager/trackmanager.h"
#include "PointManager/detmanager.h"
#include "tooltip.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/ConfigManager.h"
#include "scanlayer.h"

PPIScene::PPIScene(QObject *parent)
    : QGraphicsScene(parent),
      m_axis(new PolarAxis())
{
    initLayerObjects();
    setRange(CF_INS.range("min", MIN_RANGE), CF_INS.range("max", MAX_RANGE));

    m_scan = new ScanLayer(m_axis);
    addItem(m_scan);
    m_scan->setSweepRange(-30, 30);    // 固定粉色扇区（30°~60°）
    m_scan->setScanMode(ScanLayer::Loop);

    // ensure axis->rangeChanged is forwarded
    connect(m_axis, &PolarAxis::rangeChanged, this, &PPIScene::rangeChanged);
}

void PPIScene::updateSceneSize(const QSize &newSize) {
    setSceneRect(QRectF(-newSize.width()/2, -newSize.height()/2, newSize.width(), newSize.height()));

    double radius = qMin(newSize.width(), newSize.height())/2.0 - pviewMargin; // margin=20
    m_axis->setPixelsPerMeter(radius / m_axis->maxRange());

    emit rangeChanged(m_axis->minRange(), m_axis->maxRange());
}


void PPIScene::initLayerObjects()
{
    m_grid = new PolarGrid(this, m_axis);
    m_det = new DetManager(this, m_axis);
    m_track = new TrackManager(this, m_axis);
    m_tooltip = new Tooltip(); //
    // 联动：有 range 改变时，网格重绘，点迹重定位/隐藏
    connect(this, &PPIScene::rangeChanged, m_grid, &PolarGrid::updateGrid);
    connect(this, &PPIScene::rangeChanged, m_det, &DetManager::refreshAll);
    connect(this, &PPIScene::rangeChanged, m_track, &TrackManager::refreshAll);
}

void PPIScene::setRange(float minR, float maxR)
{
    if (minR < 0)
        minR = 0;
    if (maxR <= minR)
        maxR = minR + 1;
    m_axis->setRange(minR, maxR);
    emit rangeChanged(minR, maxR);
}
