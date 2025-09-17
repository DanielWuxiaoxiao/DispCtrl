// sectorscene.cpp
#include "sectorscene.h"
#include "sectorpolargrid.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "PolarDisp/polaraxis.h"
#include <QtMath>
#include <QDebug>

SectorScene::SectorScene(QObject* parent)
    : QGraphicsScene(parent),
      m_axis(new PolarAxis()),
      m_grid(nullptr),
      m_det(nullptr),
      m_track(nullptr)
{
    initLayerObjects();
    
    // 默认扇形范围：-30° 到 +30°，距离 0-500
    setSectorRange(-30.0f, 30.0f, 0.0f, 500.0f);
    
    // 确保轴的范围变化信号被转发
    connect(m_axis, &PolarAxis::rangeChanged, this, &SectorScene::rangeChanged);
}

SectorScene::~SectorScene()
{
    // Qt的父子关系会自动删除子对象，但为了清晰还是显式删除
    delete m_axis;
}

void SectorScene::initLayerObjects()
{
    // 创建网格
    m_grid = new SectorPolarGrid(m_axis);
    addItem(m_grid);
    
    // 创建点迹管理器
    m_det = new SectorDetManager(this, m_axis);
    m_track = new SectorTrackManager(this, m_axis);
    
    // 连接信号
    connect(this, &SectorScene::rangeChanged, m_grid, &SectorPolarGrid::updateGrid);
    connect(this, &SectorScene::rangeChanged, m_det, &SectorDetManager::refreshAll);
    connect(this, &SectorScene::rangeChanged, m_track, &SectorTrackManager::refreshAll);
    
    // 扇形范围改变时更新显示
    connect(this, &SectorScene::sectorChanged, this, &SectorScene::updateSectorDisplay);
}

void SectorScene::setSectorRange(float minAngle, float maxAngle, float minRange, float maxRange)
{
    // 参数验证
    if (minAngle >= maxAngle) {
        qWarning() << "Invalid angle range: minAngle should be less than maxAngle";
        return;
    }
    if (minRange < 0) minRange = 0;
    if (maxRange <= minRange) maxRange = minRange + 1;
    
    bool ifangleChanged = (m_minAngle != minAngle || m_maxAngle != maxAngle);
    bool ifrangeChanged = (m_minRange != minRange || m_maxRange != maxRange);
    
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
    m_minRange = minRange;
    m_maxRange = maxRange;
    
    // 更新轴的距离范围
    m_axis->setRange(minRange, maxRange);
    
    if (ifangleChanged || ifrangeChanged) {
        emit sectorChanged(minAngle, maxAngle, minRange, maxRange);
    }
    
    if (ifangleChanged) {
        emit rangeChanged(minRange, maxRange);
    }
}

void SectorScene::updateSceneSize(const QSize& newSize)
{
    // 计算扇形显示所需的场景矩形
    double radius = qMin(newSize.width(), newSize.height()) / 2.0 - 20; // margin=20
    m_axis->setPixelsPerMeter(radius / m_axis->maxRange());
    
    // 根据扇形角度范围计算场景矩形
    double angleSpan = qAbs(m_maxAngle - m_minAngle);
    double centerAngle = (m_minAngle + m_maxAngle) / 2.0;
    
    if (angleSpan >= 180.0) {
        // 如果扇形跨度大于等于180度，使用完整的矩形
        setSceneRect(QRectF(-newSize.width()/2, -newSize.height()/2,
                           newSize.width(), newSize.height()));
    } else {
        // 根据扇形角度优化场景大小
        double radCenterAngle = qDegreesToRadians(centerAngle);
        double radHalfSpan = qDegreesToRadians(angleSpan / 2.0);
        
        // 计算扇形边界点
        double maxR = radius;
        double x1 = maxR * qCos(radCenterAngle - radHalfSpan);
        double y1 = maxR * qSin(radCenterAngle - radHalfSpan);
        double x2 = maxR * qCos(radCenterAngle + radHalfSpan);
        double y2 = maxR * qSin(radCenterAngle + radHalfSpan);
        
        // 包含原点和扇形边界的矩形
        double left = qMin(0.0, qMin(x1, x2)) - 50;
        double right = qMax(0.0, qMax(x1, x2)) + 50;
        double top = qMin(0.0, qMin(y1, y2)) - 50;
        double bottom = qMax(0.0, qMax(y1, y2)) + 50;
        
        setSceneRect(QRectF(left, top, right - left, bottom - top));
    }
    
    emit rangeChanged(m_axis->minRange(), m_axis->maxRange());
}

void SectorScene::updateSectorDisplay()
{
    // 更新网格的扇形范围
    if (m_grid) {
        m_grid->setSectorRange(m_minAngle, m_maxAngle);
        m_grid->updateGrid();
    }
    
    // 更新点迹管理器的角度过滤范围
    if (m_det) {
        m_det->setAngleRange(m_minAngle, m_maxAngle);
        m_det->refreshAll();
    }
    
    if (m_track) {
        m_track->setAngleRange(m_minAngle, m_maxAngle);
        m_track->refreshAll();
    }
}