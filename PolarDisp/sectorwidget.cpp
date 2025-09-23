/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:14
 * @Description: 
 */
/**
 * @file sectorwidget.cpp
 * @brief 扇形显示窗口组件实现
 * @details 实现完整的扇形雷达显示窗口，采用垂直工具栏+视图布局
 * 
 * 实现要点：
 * 1. 三类组件：SectorToolBar（工具栏）、SectorView（视图）、SectorWidget（容器）
 * 2. 垂直布局：上方工具栏 + 下方显示视图，类似ZoomView设计
 * 3. 信号通信：工具栏发出信号，SectorWidget响应并更新视图
 * 4. 状态同步：场景变化时同步更新工具栏状态
 * 5. 自适应布局：响应窗口大小变化的动态适配
 * 
 * 设计模式：
 * - 组合模式：SectorWidget组合工具栏和视图
 * - 观察者模式：通过信号槽实现组件间通信
 * - 模板方法：按照ZoomView的成功模式设计
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

#include "sectorwidget.h"
#include "sectorscene.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "Basic/Protocol.h"
#include "Basic/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <QDateTime>

// ===== SectorToolBar 实现 =====

/**
 * @brief 工具栏构造函数
 * @param parent 父窗口组件
 * @details 创建并初始化扇形控制工具栏，采用水平紧凑布局
 */
SectorToolBar::SectorToolBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);

    // 角度范围控制
    QLabel* angleLabel = new QLabel("角度:", this);
    angleLabel->setObjectName("SectorAngleLabel");
    angleLabel->setToolTip("扇形角度范围");
    layout->addWidget(angleLabel);
    
    m_minAngleLineEdit = new QLineEdit(this);
    m_minAngleLineEdit->setText(QString::number(CF_INS.sectorAngle("min", -30)));
    m_minAngleLineEdit->setMinimumWidth(60);
    m_minAngleLineEdit->setObjectName("SectorMinAngleEdit");
    m_minAngleLineEdit->setToolTip("最小角度(-180°~180°)");
    layout->addWidget(m_minAngleLineEdit);
    
    QLabel* angleSeparator = new QLabel("~", this);
    angleSeparator->setObjectName("SectorSeparatorLabel");
    angleSeparator->setToolTip("到");
    layout->addWidget(angleSeparator);
    
    m_maxAngleLineEdit = new QLineEdit(this);
    m_maxAngleLineEdit->setText(QString::number(CF_INS.sectorAngle("max", 30)));
    m_maxAngleLineEdit->setMinimumWidth(60);
    m_maxAngleLineEdit->setObjectName("SectorMaxAngleEdit");
    m_maxAngleLineEdit->setToolTip("最大角度(-180°~180°)");
    layout->addWidget(m_maxAngleLineEdit);

    // 距离范围控制  
    QLabel* rangeLabel = new QLabel("距离(km):", this);
    rangeLabel->setObjectName("SectorRangeLabel");
    rangeLabel->setToolTip("扇形距离范围");
    layout->addWidget(rangeLabel);
    
    m_minRangeLineEdit = new QLineEdit(this);
    m_minRangeLineEdit->setText(QString::number(CF_INS.sectorRange("min", 0)));
    m_minRangeLineEdit->setMinimumWidth(80);
    m_minRangeLineEdit->setObjectName("SectorMinRangeEdit");
    m_minRangeLineEdit->setToolTip("最小距离(公里)");
    layout->addWidget(m_minRangeLineEdit);
    
    QLabel* rangeSeparator = new QLabel("~", this);
    rangeSeparator->setObjectName("SectorSeparatorLabel");
    rangeSeparator->setToolTip("到");
    layout->addWidget(rangeSeparator);
    
    m_maxRangeLineEdit = new QLineEdit(this);
    m_maxRangeLineEdit->setText(QString::number(CF_INS.sectorRange("max", 5)));
    m_maxRangeLineEdit->setMinimumWidth(80);
    m_maxRangeLineEdit->setObjectName("SectorMaxRangeEdit");
    m_maxRangeLineEdit->setToolTip("最大距离(公里)");
    layout->addWidget(m_maxRangeLineEdit);

    // 添加弹性空间，将控件推到左侧
    layout->addStretch();
    
    // 连接回车信号
    connect(m_minAngleLineEdit, &QLineEdit::returnPressed, this, &SectorToolBar::onParameterChanged);
    connect(m_maxAngleLineEdit, &QLineEdit::returnPressed, this, &SectorToolBar::onParameterChanged);
    connect(m_minRangeLineEdit, &QLineEdit::returnPressed, this, &SectorToolBar::onParameterChanged);
    connect(m_maxRangeLineEdit, &QLineEdit::returnPressed, this, &SectorToolBar::onParameterChanged);
}

double SectorToolBar::getMinAngle() const {
    return m_minAngleLineEdit->text().toDouble();
}

double SectorToolBar::getMaxAngle() const {
    return m_maxAngleLineEdit->text().toDouble();
}

double SectorToolBar::getMinRange() const {
    return m_minRangeLineEdit->text().toDouble();
}

double SectorToolBar::getMaxRange() const {
    return m_maxRangeLineEdit->text().toDouble();
}

void SectorToolBar::updateRangeDisplay(double minAngle, double maxAngle, double minRange, double maxRange) {
    // 移除自动更新输入框显示值的功能
    // 保持方法以维持接口兼容性，但不再更新显示
}

void SectorToolBar::onParameterChanged() {
    // 收集所有参数值并发出更新信号（距离单位：公里）
    emit sectorRangeUpdateRequested(getMinAngle(), getMaxAngle(), getMinRange(), getMaxRange());
}

// ===== SectorView 实现 =====

/**
 * @brief 扇形视图构造函数
 * @param parent 父窗口组件
 * @details 创建专门用于扇形显示的图形视图
 */
SectorView::SectorView(QWidget* parent)
    : QGraphicsView(parent), m_scene(nullptr)
{
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::NoDrag);
    setObjectName("sectorView");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏滚动条
    
    // 设置size policy以避免过度伸展
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void SectorView::setSectorScene(SectorScene* scene)
{
    m_scene = scene;
    setScene(scene);
    // 添加初始的fitInView调用，确保场景内容完整显示
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void SectorView::resetView()
{
    if (m_scene) {
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

// ===== SectorWidget 实现 =====

/**
 * @brief 扇形窗口构造函数
 * @param parent 父窗口组件
 * @details 初始化完整的扇形显示窗口，采用垂直布局（类似ZoomViewWidget）
 */
SectorWidget::SectorWidget(QWidget* parent)
    : QWidget(parent),
      m_toolBar(nullptr),
      m_view(nullptr),
      m_scene(nullptr),
      m_autoAddTimer(new QTimer(this))
{
    setupUI();
    
    // 连接工具栏信号
    connect(m_toolBar, &SectorToolBar::sectorRangeUpdateRequested,
            this, &SectorWidget::updateSectorRange);
    
    // 连接场景信号
    connect(m_scene, &SectorScene::rangeChanged, 
            this, &SectorWidget::onSceneRangeChanged);
    
    // 设置自动添加点迹的定时器（已禁用）
    m_autoAddTimer->setSingleShot(false);
    m_autoAddTimer->setInterval(2000); // 2秒间隔
    
    // 设置size policy以避免过度伸展
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setObjectName("SectorWidget");
    // 初始化默认扇形范围
    updateSectorRange(m_toolBar->getMinAngle(), m_toolBar->getMaxAngle(),
                     m_toolBar->getMinRange(), m_toolBar->getMaxRange());
    
    // 确保初始显示时场景内容完整显示
    if (m_view && m_scene) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void SectorWidget::setupUI()
{
    // 创建扇形场景
    m_scene = new SectorScene(this);
    
    // 创建工具栏
    m_toolBar = new SectorToolBar(this);
    
    // 创建视图
    m_view = new SectorView(this);
    m_view->setSectorScene(m_scene);
    
    // 主布局：垂直布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // 添加组件：上方工具栏，下方视图
    layout->addWidget(m_toolBar);
    layout->addWidget(m_view, 1); // 视图占主要空间
}

void SectorWidget::updateSectorRange(double minAngle, double maxAngle, double minRangeKm, double maxRangeKm)
{
    // 参数验证
    if (minAngle >= maxAngle) {
        return;
    }
    
    if (minRangeKm >= maxRangeKm) {
        return;
    }
    
    // 将公里转换为米（因为polaraxis和点迹使用米单位）
    double minRange = minRangeKm * 1000.0;
    double maxRange = maxRangeKm * 1000.0;
    
    // 更新扇形场景范围（使用米单位）
    m_scene->setSectorRange(minAngle, maxAngle, minRange, maxRange);
    
    // 重新计算场景大小以适应新的扇形范围
    if (m_view) {
        QSize viewSize = m_view->size();
        m_scene->updateSceneSize(viewSize);
        
        // 立即应用 fitInView 以确保新的范围正确显示
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void SectorWidget::onSceneRangeChanged(float minRange, float maxRange)
{
    // 移除范围显示的同步更新，场景范围变化时不再同步显示
    // 保留方法以维持信号连接的完整性
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

void SectorWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    
    // 确保首次显示时场景内容完整显示
    if (m_scene && m_view) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}