/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:32
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-28
 * @Description: 距离-方位显示组件实现
 */

#include "rangeazimuthwidget.h"
#include "sectorscene.h"
#include "polaraxis.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "Basic/ConfigManager.h"
#include "Controller/controller.h"
#include "Controller/RadarDataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QDebug>

// ===== RangeAzimuthToolBar 实现 =====

RangeAzimuthToolBar::RangeAzimuthToolBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);

    // 方位角范围控制
    QLabel* azimuthLabel = new QLabel("方位角:", this);
    azimuthLabel->setObjectName("RangeAzimuthLabel");
    azimuthLabel->setToolTip("显示方位角范围");
    layout->addWidget(azimuthLabel);

    m_minAzimuthLineEdit = new QLineEdit(this);
    m_minAzimuthLineEdit->setText(QString::number(CF_INS.rangeAzimuthAngle("min", 0)));
    m_minAzimuthLineEdit->setMaximumWidth(60);
    m_minAzimuthLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_minAzimuthLineEdit->setObjectName("RangeAzimuthMinEdit");
    m_minAzimuthLineEdit->setToolTip("最小方位角(0°~360°)");
    //layout->addWidget(m_minAzimuthLineEdit);

    QLabel* separator = new QLabel("~", this);
    separator->setObjectName("RangeAzimuthSeparatorLabel");
    separator->setToolTip("到");
    layout->addWidget(separator);

    m_maxAzimuthLineEdit = new QLineEdit(this);
    m_maxAzimuthLineEdit->setText(QString::number(CF_INS.rangeAzimuthAngle("max", 360)));
    m_maxAzimuthLineEdit->setMinimumWidth(60);
    m_maxAzimuthLineEdit->setObjectName("RangeAzimuthMaxEdit");
    m_maxAzimuthLineEdit->setToolTip("最大方位角(0°~360°)");
    layout->addWidget(m_maxAzimuthLineEdit);

    // QLabel* noteLabel = new QLabel("(距离范围自动同步主视图)", this);
    // noteLabel->setObjectName("RangeAzimuthNoteLabel");
    // noteLabel->setStyleSheet("color: #888; font-size: 11px;");
    // layout->addWidget(noteLabel);

    // 添加弹性空间
    layout->addStretch();

    // 连接回车信号
    connect(m_minAzimuthLineEdit, &QLineEdit::returnPressed, this, &RangeAzimuthToolBar::onParameterChanged);
    connect(m_maxAzimuthLineEdit, &QLineEdit::returnPressed, this, &RangeAzimuthToolBar::onParameterChanged);
}

double RangeAzimuthToolBar::getMinAzimuth() const {
    return m_minAzimuthLineEdit->text().toDouble();
}

double RangeAzimuthToolBar::getMaxAzimuth() const {
    return m_maxAzimuthLineEdit->text().toDouble();
}

void RangeAzimuthToolBar::onParameterChanged() {
    emit azimuthRangeUpdateRequested(getMinAzimuth(), getMaxAzimuth());
}

// ===== RangeAzimuthView 实现 =====

RangeAzimuthView::RangeAzimuthView(SectorScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setObjectName("RangeAzimuthView");

    // 设置背景
    setStyleSheet("background: #1a1a1a;");
}

void RangeAzimuthView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if (scene()) {
        QRectF sceneRect = scene()->sceneRect();
        fitInView(sceneRect, Qt::KeepAspectRatio);
    }
}

// ===== RangeAzimuthWidget 实现 =====

RangeAzimuthWidget::RangeAzimuthWidget(QWidget* parent)
    : QWidget(parent)
    , m_toolbar(nullptr)
    , m_view(nullptr)
    , m_scene(nullptr)
    , m_axis(nullptr)
    , m_detManager(nullptr)
    , m_trackManager(nullptr)
    , m_currentMinRange(0.0)
    , m_currentMaxRange(5.0)
{
    setupUI();
    connectSignals();

    qDebug() << "[RangeAzimuthWidget] Initialized with range:" << m_currentMinRange << "~" << m_currentMaxRange << "km";
}

RangeAzimuthWidget::~RangeAzimuthWidget()
{
    // 从雷达数据管理器注销
    if (m_detManager) {
        RADAR_DATA_MGR.unregisterView("RangeAzimuthWidget_Det_" + QString::number((quintptr)this));
    }
    if (m_trackManager) {
        RADAR_DATA_MGR.unregisterView("RangeAzimuthWidget_Track_" + QString::number((quintptr)this));
    }

    qDebug() << "[RangeAzimuthWidget] Destroyed";
}

void RangeAzimuthWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建工具栏
    m_toolbar = new RangeAzimuthToolBar(this);
    mainLayout->addWidget(m_toolbar);

    // 创建极坐标轴（无参数构造）
    m_axis = new PolarAxis(this);
    m_axis->setRange(m_currentMinRange, m_currentMaxRange);

    // 创建扇区场景（无参数构造）
    m_scene = new SectorScene(this);

    // 初始化方位角范围为0~360度（全圆）
    double minAzimuth = CF_INS.rangeAzimuthAngle("min", 0);
    double maxAzimuth = CF_INS.rangeAzimuthAngle("max", 360);
    m_scene->setSectorRange(minAzimuth, maxAzimuth, m_currentMinRange, m_currentMaxRange);

    // 创建视图
    m_view = new RangeAzimuthView(m_scene, this);
    mainLayout->addWidget(m_view);

    // 创建点迹管理器
    m_detManager = new SectorDetManager(m_scene, m_axis, this);
    m_trackManager = new SectorTrackManager(m_scene, m_axis, this);

    // 注册到雷达数据管理器
    QString detViewID = "RangeAzimuthWidget_Det_" + QString::number((quintptr)this);
    QString trackViewID = "RangeAzimuthWidget_Track_" + QString::number((quintptr)this);

    RADAR_DATA_MGR.registerView(detViewID, m_detManager);
    RADAR_DATA_MGR.registerView(trackViewID, m_trackManager);

    // 注意：SectorDetManager和SectorTrackManager已经在各自构造函数中连接了信号
    // 因此这里不需要重复连接dataCleared、detectionReceived、trackReceived信号
    // 避免重复触发导致的问题

    qDebug() << "[RangeAzimuthWidget] UI setup complete, azimuth range:" << minAzimuth << "~" << maxAzimuth;
}

void RangeAzimuthWidget::connectSignals()
{
    // 连接工具栏信号
    connect(m_toolbar, &RangeAzimuthToolBar::azimuthRangeUpdateRequested,
            this, &RangeAzimuthWidget::onAzimuthRangeUpdateRequested);
}

void RangeAzimuthWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // 窗口显示时调整视图
    if (m_view && m_scene) {
        QRectF sceneRect = m_scene->sceneRect();
        m_view->fitInView(sceneRect, Qt::KeepAspectRatio);
    }
}

void RangeAzimuthWidget::onAzimuthRangeUpdateRequested(double minAzimuth, double maxAzimuth)
{
    if (m_scene && m_axis) {
        // 更新场景的方位角范围（距离范围保持不变）
        m_scene->setSectorRange(minAzimuth, maxAzimuth, m_currentMinRange, m_currentMaxRange);

        // 刷新管理器
        if (m_detManager) {
            m_detManager->setAngleRange(minAzimuth, maxAzimuth);
        }
        if (m_trackManager) {
            m_trackManager->setAngleRange(minAzimuth, maxAzimuth);
        }

        // 保存到配置
        CF_INS.setRangeAzimuthAngle("min", minAzimuth);
        CF_INS.setRangeAzimuthAngle("max", maxAzimuth);

        qDebug() << "[RangeAzimuthWidget] Azimuth range updated:" << minAzimuth << "~" << maxAzimuth;
    }
}

void RangeAzimuthWidget::setRangeFromMain(double minRange, double maxRange)
{
    m_currentMinRange = minRange;
    m_currentMaxRange = maxRange;

    if (m_axis) {
        m_axis->setRange(minRange, maxRange);
    }

    if (m_scene) {
        // 获取当前方位角范围
        double minAzimuth = m_toolbar->getMinAzimuth();
        double maxAzimuth = m_toolbar->getMaxAzimuth();

        // 更新场景（保持方位角范围，更新距离范围）
        m_scene->setSectorRange(minAzimuth, maxAzimuth, minRange, maxRange);
    }

    // 刷新管理器
    if (m_detManager) {
        m_detManager->refreshAll();
    }
    if (m_trackManager) {
        m_trackManager->refreshAll();
    }

    qDebug() << "[RangeAzimuthWidget] Range synced from main view:" << minRange << "~" << maxRange << "km";
}

void RangeAzimuthWidget::setDetectionVisible(bool visible)
{
    if (m_detManager) {
        m_detManager->setAllVisible(visible);
        qDebug() << "[RangeAzimuthWidget] Detection visibility:" << visible;
    }
}

void RangeAzimuthWidget::setTrackVisible(bool visible)
{
    if (m_trackManager) {
        m_trackManager->setAllVisible(visible);
        qDebug() << "[RangeAzimuthWidget] Track visibility:" << visible;
    }
}

void RangeAzimuthWidget::setMaxDetectionPoints(int maxPoints)
{
    // SectorDetManager 不支持 setMaxPoints 方法
    // 扇区管理器使用自己的内部机制管理点数
    qDebug() << "[RangeAzimuthWidget] Max detection points setting not supported by SectorDetManager";
    Q_UNUSED(maxPoints);
}

void RangeAzimuthWidget::setMaxTrackPoints(int maxTracks)
{
    // SectorTrackManager 不支持 setMaxPoints 方法
    // 扇区管理器使用自己的内部机制管理点数
    qDebug() << "[RangeAzimuthWidget] Max track points setting not supported by SectorTrackManager";
    Q_UNUSED(maxTracks);
}

void RangeAzimuthWidget::setDetectionSizeRatio(float ratio)
{
    if (m_detManager) {
        m_detManager->setPointSizeRatio(ratio);
        qDebug() << "[RangeAzimuthWidget] Detection size ratio:" << ratio;
    }
}

void RangeAzimuthWidget::setTrackSizeRatio(float ratio)
{
    if (m_trackManager) {
        m_trackManager->setPointSizeRatio(ratio);
        qDebug() << "[RangeAzimuthWidget] Track size ratio:" << ratio;
    }
}
