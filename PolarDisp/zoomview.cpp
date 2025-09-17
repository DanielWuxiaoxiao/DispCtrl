#include "zoomview.h"
#include "ppisscene.h"
#include "polaraxis.h"
#include <QWheelEvent>
#include <QtMath>
#include <QDebug>
#include <QButtonGroup>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QFont>

// ===== 工具栏实现 =====
ZoomViewToolBar::ZoomViewToolBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 放大按钮（icon）
    m_zoomInBtn = new QPushButton(this);
    m_zoomInBtn->setIcon(QIcon(":/resources/icon/plus.png"));
    m_zoomInBtn->setToolTip("放大视图");
    m_zoomInBtn->setObjectName("ZoomInBtn");

    // 缩小按钮（icon）
    m_zoomOutBtn = new QPushButton(this);
    m_zoomOutBtn->setIcon(QIcon(":/resources/icon/min.png"));
    m_zoomOutBtn->setToolTip("缩小视图");
    m_zoomOutBtn->setObjectName("ZoomOutBtn");

    // 重置视图按钮（icon）
    m_resetBtn = new QPushButton(this);
    m_resetBtn->setIcon(QIcon(":/resources/icon/reset.png"));
    m_resetBtn->setToolTip("重置视图");
    m_resetBtn->setObjectName("ResetBtn");

    // 缩放比例显示
    m_zoomLabel = new QLabel("缩放比例:100%", this);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setObjectName("ZoomLabel");

    // 拖动/测距模式按钮（icon + 互斥按钮组）
    m_dragBtn = new QPushButton(this);
    m_dragBtn->setCheckable(true);
    m_dragBtn->setIcon(QIcon(":/resources/icon/hand.png"));
    m_dragBtn->setToolTip("拖动视图");
    m_dragBtn->setObjectName("DragBtn");

    m_measureBtn = new QPushButton(this);
    m_measureBtn->setCheckable(true);
    m_measureBtn->setIcon(QIcon(":/resources/icon/meas.png"));
    m_measureBtn->setToolTip("测量距离");
    m_measureBtn->setObjectName("MeasureBtn");

    // 互斥逻辑使用 QButtonGroup
    QButtonGroup* modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(m_dragBtn);
    modeGroup->addButton(m_measureBtn);
    m_dragBtn->setChecked(true);

    // 添加到布局
    layout->addWidget(m_zoomInBtn);
    layout->addWidget(m_zoomOutBtn);
    layout->addWidget(m_resetBtn);
    layout->addWidget(m_zoomLabel);
    layout->addStretch();
    layout->addWidget(m_dragBtn);
    layout->addWidget(m_measureBtn);

    // 连接信号
    connect(m_zoomInBtn, &QPushButton::clicked, this, &ZoomViewToolBar::zoomIn);
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &ZoomViewToolBar::zoomOut);
    connect(m_resetBtn, &QPushButton::clicked, this, &ZoomViewToolBar::resetViewRequested);

    // 互斥按钮逻辑：选中时发出对应信号
    connect(m_dragBtn, &QPushButton::toggled, [this](bool checked) {
        if (checked) emit dragModeChanged(true);
    });
    connect(m_measureBtn, &QPushButton::toggled, [this](bool checked) {
        if (checked) emit measureModeChanged(true);
    });
}

void ZoomViewToolBar::setZoomLevel(double level) {
    m_zoomLabel->setText(QString("缩放比例:%1%").arg(qRound(level * 100)));
}

// ===== ZoomView 实现 =====
ZoomView::ZoomView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_mode(DragMode)
    , m_zoomFactor(1.0)
    , m_measuring(false)
    , m_measurePath(nullptr)
    , m_startMarker(nullptr)
    , m_endMarker(nullptr)
    , m_distanceText(nullptr)
{
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    QGraphicsView::setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //隐藏滚动条
    // 设置背景
    setObjectName("ZoomView");
}

ZoomView::~ZoomView() {
    clearMeasureLine();
}

void ZoomView::setPPIScene(PPIScene* scene) {
    m_scene = scene;
    setScene(scene);
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void ZoomView::showArea(const QRectF& sceneRect) {
    if (!m_scene) return;

    // 显示指定的场景区域
    fitInView(sceneRect, Qt::KeepAspectRatio);
    updateZoomLevel();
}

double ZoomView::zoomLevel() const {
    // 返回相对于整个 scene 的放大倍数：
    // 当视图显示整个 scene 时返回 1.0（100%），当视图只显示 scene 的一半宽度时返回 2.0（200%）
    if (!scene()) return transform().m11();
    QRectF full = scene()->sceneRect();
    if (full.isEmpty()) return transform().m11();

    // 可视区域对应的 scene 矩形
    QRectF visible = mapToScene(viewport()->rect()).boundingRect();
    if (visible.isEmpty()) return transform().m11();

    // 使用宽度比作为缩放倍率（也可以改为高度或面积平均）
    if (visible.width() <= 0.0) return transform().m11();
    double factor = full.width() / visible.width();
    if (!qIsFinite(factor) || factor <= 0.0) return transform().m11();
    return factor;
}

void ZoomView::zoomIn() {
    scale(1.2, 1.2);
    updateZoomLevel();
}

void ZoomView::zoomOut() {
    scale(0.8, 0.8);
    updateZoomLevel();
}

void ZoomView::setCustomDragMode(bool drag) {
    if (drag) {
        m_mode = DragMode;
        QGraphicsView::setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(Qt::OpenHandCursor);
        clearMeasureLine();
    }
}

void ZoomView::setMeasureMode(bool measure) {
    if (measure) {
        m_mode = MeasureMode;
    QGraphicsView::setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
    }
}

void ZoomView::resetView() {
    if (!m_scene) return;
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    emit zoomLevelChanged(1.0f);
}

void ZoomView::mousePressEvent(QMouseEvent* event) {
    if (m_mode == MeasureMode && event->button() == Qt::LeftButton) {
        m_measuring = true;
        m_measureStart = mapToScene(event->pos());

        // 清除之前的测量显示
        clearMeasureLine();

        // 创建用于显示尺子的 path item
        QPen pen(Qt::yellow);
        pen.setWidthF(2.0);
        pen.setStyle(Qt::DashLine);

        m_measurePath = new QGraphicsPathItem();
        m_measurePath->setPen(pen);
        m_measurePath->setZValue(1000);
        m_scene->addItem(m_measurePath);

    // 起止圆点标记（空心圆环，更像尺子的端点）
    QPen markerPen(Qt::yellow);
    markerPen.setWidthF(1.5);
    QBrush noBrush(Qt::transparent);
    m_startMarker = new QGraphicsEllipseItem(-6, -6, 12, 12);
    m_startMarker->setBrush(noBrush);
    m_startMarker->setPen(markerPen);
    m_startMarker->setZValue(1001);
    m_scene->addItem(m_startMarker);

    m_endMarker = new QGraphicsEllipseItem(-6, -6, 12, 12);
    m_endMarker->setBrush(noBrush);
    m_endMarker->setPen(markerPen);
    m_endMarker->setZValue(1001);
    m_scene->addItem(m_endMarker);

        // 距离文本
        m_distanceText = m_scene->addText("");
        QFont f = m_distanceText->font();
        f.setPointSize(10);
        f.setBold(true);
        m_distanceText->setFont(f);
        m_distanceText->setDefaultTextColor(Qt::yellow);
        m_distanceText->setZValue(1002);

        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void ZoomView::mouseMoveEvent(QMouseEvent* event) {
    if (m_mode == MeasureMode && m_measuring && m_measurePath) {
        QPointF currentPos = mapToScene(event->pos());

        // 计算主线和 ticks
        QLineF baseLine(m_measureStart, currentPos);
        QPainterPath path;
        path.moveTo(m_measureStart);
        path.lineTo(currentPos);

        // 计算距离与刻度（以真实单位：m 为基础）
        double totalLen = baseLine.length();
        double distanceMeters = totalLen; // fallback: pixels
        if (m_scene && m_scene->axis()) {
            distanceMeters = m_scene->axis()->pixelToRange(totalLen);
        }

        // 计算像素每米比例用于将刻度间隔从米转换为像素
        double pixelsPerMeter = 1.0;
        if (distanceMeters > 0.0) pixelsPerMeter = totalLen / distanceMeters;

        // 希望刻度数在 6..12 之间，选择一个“好看”的刻度间隔（1,2,5 x 10^n）
        const int desiredTicks = 8;
        double rawSpacingMeters = qMax(1.0, distanceMeters / double(desiredTicks));
        // nice step: 1,2,5 * 10^n
        double exp = floor(log10(rawSpacingMeters));
        double base = pow(10.0, exp);
        double f = rawSpacingMeters / base;
        double niceMant;
        if (f <= 1.0) niceMant = 1.0;
        else if (f <= 2.0) niceMant = 2.0;
        else if (f <= 5.0) niceMant = 5.0;
        else niceMant = 10.0;
        double spacingMeters = niceMant * base;
        double spacingPixels = spacingMeters * pixelsPerMeter;

        const double minorTickLen = 6.0;
        const double majorTickLen = 12.0;
        int tickCount = int(totalLen / spacingPixels);
        for (int i = 1; i <= tickCount; ++i) {
            double distPx = i * spacingPixels;
            if (distPx >= totalLen) break;
            double ratio = distPx / totalLen;
            QPointF pt = baseLine.pointAt(ratio);
            double angle = baseLine.angle() * M_PI / 180.0;
            double nx = -sin(angle);
            double ny = cos(angle);
            bool major = (i % 5 == 0);
            double tlen = major ? majorTickLen : minorTickLen;
            QPointF a = pt + QPointF(nx * tlen * 0.5, ny * tlen * 0.5);
            QPointF b = pt - QPointF(nx * tlen * 0.5, ny * tlen * 0.5);
            path.moveTo(a);
            path.lineTo(b);
            // 在主刻度处添加小标签（仅在较大刻度显示数值）
            Q_UNUSED(major);
        }

        m_measurePath->setPath(path);

        // 更新起止点标记位置
        if (m_startMarker) m_startMarker->setPos(m_measureStart);
        if (m_endMarker) m_endMarker->setPos(currentPos);

        // 更新距离文本，带单位显示
        if (m_distanceText) {
            QString text;
            if (distanceMeters >= 1000.0) text = QString("距离: %1 km").arg(distanceMeters/1000.0, 0, 'f', 1);
            else text = QString("距离: %1 m").arg(int(distanceMeters));
            m_distanceText->setPlainText(text);
            QPointF midPoint = (m_measureStart + currentPos) / 2;
            m_distanceText->setPos(midPoint + QPointF(8, -12));
        }

        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void ZoomView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_mode == MeasureMode && m_measuring && event->button() == Qt::LeftButton) {
        m_measuring = false;

        QPointF endPos = mapToScene(event->pos());
        double distancePixels = QLineF(m_measureStart, endPos).length();
        double distanceVal = distancePixels;
        if (m_scene && m_scene->axis()) {
            distanceVal = m_scene->axis()->pixelToRange(distancePixels);
        }
        emit distanceMeasured(distanceVal);
        // 保持显示，直到下次测量或切换模式

        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void ZoomView::wheelEvent(QWheelEvent* event) {
    // 使用鼠标滚轮缩放
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    updateZoomLevel();
    event->accept();
}

void ZoomView::updateZoomLevel() {
    emit zoomLevelChanged(zoomLevel());
}

void ZoomView::clearMeasureLine() {
    if (!m_scene) return;
    if (m_measurePath) {
        m_scene->removeItem(m_measurePath);
        delete m_measurePath;
        m_measurePath = nullptr;
    }
    if (m_startMarker) {
        m_scene->removeItem(m_startMarker);
        delete m_startMarker;
        m_startMarker = nullptr;
    }
    if (m_endMarker) {
        m_scene->removeItem(m_endMarker);
        delete m_endMarker;
        m_endMarker = nullptr;
    }
    if (m_distanceText) {
        m_scene->removeItem(m_distanceText);
        delete m_distanceText;
        m_distanceText = nullptr;
    }
}

// ===== ZoomViewWidget 实现 =====
ZoomViewWidget::ZoomViewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    // 设置窗口属性
    setWindowTitle("P显");
    //setMinimumSize(400, 300);
}

ZoomViewWidget::~ZoomViewWidget() {
}

void ZoomViewWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建工具栏
    m_toolBar = new ZoomViewToolBar();
    // 创建视图
    m_view = new ZoomView();

    // 添加到布局
    mainLayout->addWidget(m_toolBar);
    mainLayout->addWidget(m_view);
}

void ZoomViewWidget::connectSignals() {
    // 连接工具栏信号到视图
    connect(m_toolBar, &ZoomViewToolBar::zoomIn, m_view, &ZoomView::zoomIn);
    connect(m_toolBar, &ZoomViewToolBar::zoomOut, m_view, &ZoomView::zoomOut);
    connect(m_toolBar, &ZoomViewToolBar::dragModeChanged, m_view, &ZoomView::setCustomDragMode);
    connect(m_toolBar, &ZoomViewToolBar::measureModeChanged, m_view, &ZoomView::setMeasureMode);
    connect(m_toolBar, &ZoomViewToolBar::resetViewRequested, m_view, &ZoomView::resetView);

    // 连接视图信号到工具栏
    connect(m_view, &ZoomView::zoomLevelChanged, m_toolBar, &ZoomViewToolBar::setZoomLevel);

    // 转发测距信号
    connect(m_view, &ZoomView::distanceMeasured, this, &ZoomViewWidget::distanceMeasured);
}

void ZoomViewWidget::setPPIScene(PPIScene* scene) {
    m_view->setPPIScene(scene);
    connect(scene, &PPIScene::rangeChanged, m_view, &ZoomView::resetView);
}

void ZoomViewWidget::showArea(const QRectF& sceneRect) {
    m_view->showArea(sceneRect);
}
