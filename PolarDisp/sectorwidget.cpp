// sectorwidget.cpp
#include "sectorwidget.h"
#include "sectorscene.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "Basic/Protocol.h"
#include <QGridLayout>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <QDateTime>

SectorWidget::SectorWidget(QWidget* parent)
    : QWidget(parent),
      m_view(nullptr),
      m_scene(nullptr),
      m_autoAddTimer(new QTimer(this))
{
    setupUI();
    setupControls();
    
    // 设置自动添加点迹的定时器
    m_autoAddTimer->setSingleShot(false);
    m_autoAddTimer->setInterval(2000); // 2秒间隔
    connect(m_autoAddTimer, &QTimer::timeout, this, &SectorWidget::addRandomTrackPoint);
}

void SectorWidget::setupUI()
{
    // 创建扇形场景
    m_scene = new SectorScene(this);
    
    // 创建视图
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setMinimumSize(600, 400);
    
    // 连接场景信号
    connect(m_scene, &SectorScene::rangeChanged, 
            this, &SectorWidget::onSceneRangeChanged);
    
    // 主布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_view, 1); // 视图占主要空间
    
    // 右侧控制面板
    QVBoxLayout* controlLayout = new QVBoxLayout();
    
    // 扇形范围控制组
    QGroupBox* rangeGroup = new QGroupBox("扇形范围", this);
    QGridLayout* rangeLayout = new QGridLayout(rangeGroup);
    
    // 角度范围控制
    rangeLayout->addWidget(new QLabel("最小角度(°):"), 0, 0);
    m_minAngleSpinBox = new QSpinBox(this);
    m_minAngleSpinBox->setRange(-180, 180);
    m_minAngleSpinBox->setValue(-30);
    rangeLayout->addWidget(m_minAngleSpinBox, 0, 1);
    
    rangeLayout->addWidget(new QLabel("最大角度(°):"), 1, 0);
    m_maxAngleSpinBox = new QSpinBox(this);
    m_maxAngleSpinBox->setRange(-180, 180);
    m_maxAngleSpinBox->setValue(30);
    rangeLayout->addWidget(m_maxAngleSpinBox, 1, 1);
    
    // 距离范围控制
    rangeLayout->addWidget(new QLabel("最小距离:"), 2, 0);
    m_minRangeSpinBox = new QDoubleSpinBox(this);
    m_minRangeSpinBox->setRange(0, 10000);
    m_minRangeSpinBox->setValue(0);
    rangeLayout->addWidget(m_minRangeSpinBox, 2, 1);
    
    rangeLayout->addWidget(new QLabel("最大距离:"), 3, 0);
    m_maxRangeSpinBox = new QDoubleSpinBox(this);
    m_maxRangeSpinBox->setRange(1, 10000);
    m_maxRangeSpinBox->setValue(500);
    rangeLayout->addWidget(m_maxRangeSpinBox, 3, 1);
    
    m_updateButton = new QPushButton("更新范围", this);
    rangeLayout->addWidget(m_updateButton, 4, 0, 1, 2);
    
    controlLayout->addWidget(rangeGroup);
    
    // 点迹控制组
    QGroupBox* pointGroup = new QGroupBox("点迹操作", this);
    QVBoxLayout* pointLayout = new QVBoxLayout(pointGroup);
    
    m_addDetButton = new QPushButton("添加随机检测点", this);
    m_addTrackButton = new QPushButton("添加随机航迹点", this);
    m_clearButton = new QPushButton("清除所有点", this);
    
    pointLayout->addWidget(m_addDetButton);
    pointLayout->addWidget(m_addTrackButton);
    pointLayout->addWidget(m_clearButton);
    
    controlLayout->addWidget(pointGroup);
    
    // 状态标签
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    controlLayout->addWidget(m_statusLabel);
    
    controlLayout->addStretch(); // 添加弹性空间
    
    // 设置控制面板宽度
    QWidget* controlWidget = new QWidget(this);
    controlWidget->setLayout(controlLayout);
    controlWidget->setMaximumWidth(250);
    controlWidget->setMinimumWidth(200);
    
    mainLayout->addWidget(controlWidget);
}

void SectorWidget::setupControls()
{
    // 连接控制信号
    connect(m_updateButton, &QPushButton::clicked, 
            this, &SectorWidget::updateSectorRange);
    connect(m_addDetButton, &QPushButton::clicked, 
            this, &SectorWidget::addRandomDetPoint);
    connect(m_addTrackButton, &QPushButton::clicked, 
            this, &SectorWidget::addRandomTrackPoint);
    connect(m_clearButton, &QPushButton::clicked, 
            this, &SectorWidget::clearAllPoints);
    
    // 初始化扇形范围
    updateSectorRange();
}

void SectorWidget::updateSectorRange()
{
    float minAngle = m_minAngleSpinBox->value();
    float maxAngle = m_maxAngleSpinBox->value();
    float minRange = m_minRangeSpinBox->value();
    float maxRange = m_maxRangeSpinBox->value();
    
    // 参数验证
    if (minAngle >= maxAngle) {
        m_statusLabel->setText("错误：最小角度应小于最大角度");
        m_statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        return;
    }
    
    if (minRange >= maxRange) {
        m_statusLabel->setText("错误：最小距离应小于最大距离");
        m_statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        return;
    }
    
    // 更新扇形范围
    m_scene->setSectorRange(minAngle, maxAngle, minRange, maxRange);
    
    QString status = QString("扇形范围：%1°~%2°, %3~%4")
                    .arg(minAngle).arg(maxAngle)
                    .arg(minRange, 0, 'f', 1).arg(maxRange, 0, 'f', 1);
    m_statusLabel->setText(status);
    m_statusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
}

void SectorWidget::addRandomDetPoint()
{
    if (!m_scene) return;
    
    // 在扇形范围内生成随机检测点
    float minAngle = m_scene->minAngle();
    float maxAngle = m_scene->maxAngle();
    float minRange = m_scene->minRange();
    float maxRange = m_scene->maxRange();
    
    // 生成随机参数
    QRandomGenerator* rng = QRandomGenerator::global();
    float angle = minAngle + rng->generateDouble() * (maxAngle - minAngle);
    float range = minRange + rng->generateDouble() * (maxRange - minRange);
    
    // 创建点迹信息
    PointInfo info;
    info.batch = 0; // 检测点不分批
    info.azimuth = angle;
    info.range = range;
    info.type = 1;
    
    // 添加到场景
    m_scene->detManager()->addDetPoint(info);
    
    m_statusLabel->setText(QString("添加检测点：角度=%1°, 距离=%2")
                          .arg(angle, 0, 'f', 1).arg(range, 0, 'f', 1));
}

void SectorWidget::addRandomTrackPoint()
{
    if (!m_scene) return;
    
    // 在扇形范围内生成随机航迹点
    float minAngle = m_scene->minAngle();
    float maxAngle = m_scene->maxAngle();
    float minRange = m_scene->minRange();
    float maxRange = m_scene->maxRange();
    
    // 生成随机参数
    QRandomGenerator* rng = QRandomGenerator::global();
    float angle = minAngle + rng->generateDouble() * (maxAngle - minAngle);
    float range = minRange + rng->generateDouble() * (maxRange - minRange);
    
    // 创建点迹信息
    PointInfo info;
    info.batch = m_trackCounter; // 航迹编号
    info.azimuth = angle;
    info.range = range;
    info.type = 2;
    
    // 添加到场景
    m_scene->trackManager()->addTrackPoint(info);
    
    // 每10个点换一个新航迹
    static int pointsInCurrentTrack = 0;
    pointsInCurrentTrack++;
    if (pointsInCurrentTrack >= 10) {
        m_trackCounter++;
        pointsInCurrentTrack = 0;
    }
    
    m_statusLabel->setText(QString("添加航迹点：批次=%1, 角度=%2°, 距离=%3")
                          .arg(info.batch).arg(angle, 0, 'f', 1).arg(range, 0, 'f', 1));
}

void SectorWidget::clearAllPoints()
{
    if (!m_scene) return;
    
    m_scene->detManager()->clear();
    m_scene->trackManager()->clear();
    m_trackCounter = 1;
    
    m_statusLabel->setText("已清除所有点迹");
}

void SectorWidget::onSceneRangeChanged(float minRange, float maxRange)
{
    // 同步控件状态
    m_minRangeSpinBox->blockSignals(true);
    m_maxRangeSpinBox->blockSignals(true);
    
    m_minRangeSpinBox->setValue(minRange);
    m_maxRangeSpinBox->setValue(maxRange);
    
    m_minRangeSpinBox->blockSignals(false);
    m_maxRangeSpinBox->blockSignals(false);
}

void SectorWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    if (m_scene && m_view) {
        // 更新场景大小以适应视图
        QSize viewSize = m_view->size();
        m_scene->updateSceneSize(viewSize);
        
        // 自动缩放以适应内容
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}