#include "ppisscene.h"

#include "Basic/ConfigManager.h"
#include "Controller/controller.h"
#include "PolarDisp/echorenderer.h"
#include "PolarDisp/polaraxis.h"
#include "PolarDisp/polargrid.h"

PPIScene::PPIScene(QObject* parent)
    : QGraphicsScene(parent)
    , m_axis(new PolarAxis(this))
{
    m_grid = new PolarGrid(this, m_axis, this);
    m_echo = new EchoRenderer(this, m_axis, 2048, this);
    m_echo->setSweepHistoryRounds(CF_INS.marineDisplayInt("sweep_history_rounds", 1));

    const float minRangeMeters = static_cast<float>(CF_INS.range("min", 0.0) * 1000.0);
    const float maxRangeMeters = static_cast<float>(CF_INS.range("max", 5.0) * 1000.0);
    setRange(minRangeMeters, maxRangeMeters);

    connect(this, &PPIScene::rangeChanged, m_grid, &PolarGrid::updateGrid);
    connect(CON_INS, &Controller::marineEchoLine,
            m_echo, &EchoRenderer::updateEchoLine);
}

void PPIScene::setRange(float minRangeMeters, float maxRangeMeters)
{
    if (minRangeMeters < 0.0F) {
        minRangeMeters = 0.0F;
    }
    if (maxRangeMeters <= minRangeMeters) {
        maxRangeMeters = minRangeMeters + 1.0F;
    }

    m_axis->setRange(minRangeMeters, maxRangeMeters);
    updateAxisScaleFromScene();
    emit rangeChanged(minRangeMeters, maxRangeMeters);
}

void PPIScene::updateSceneSize(const QSize& newSize)
{
    const int width = qMax(1, newSize.width());
    const int height = qMax(1, newSize.height());
    setSceneRect(-width / 2.0, -height / 2.0, width, height);
    updateAxisScaleFromScene();
    emit rangeChanged(static_cast<float>(m_axis->minRange()),
                      static_cast<float>(m_axis->maxRange()));
}

void PPIScene::updateAxisScaleFromScene()
{
    if (m_axis->maxRange() <= 0.0 || sceneRect().isEmpty()) {
        return;
    }

    const double radius = qMax(1.0, qMin(sceneRect().width(), sceneRect().height()) / 2.0 - m_viewMargin);
    m_axis->setPixelsPerMeter(radius / m_axis->maxRange());
}
