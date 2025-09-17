#include "cuswindow.h"
#include <QHBoxLayout>
#include <QStyle>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QCursor>

static constexpr int BORDER_WIDTH = 8;

// 用于橡皮筋效果
#include <QRubberBand>

// ...existing code...

// 增加成员变量
// 在 CusWindow 类的 private 区域加：
// QRubberBand* m_rubberBand = nullptr;
// QRect m_rubberBandRect;


CusWindow::CusWindow(const QString& title, const QIcon& icon, QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground); // 支持窗口半透明
    setObjectName("CusWindow");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(3,3,3,3);
    mainLayout->setSpacing(0);

    QWidget* titleBar = new QWidget(this);
    titleBar->setObjectName("TitleBar");
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8,0,8,0);

    QLabel* iconLabel = nullptr;
    if (!icon.isNull()) {
        iconLabel = new QLabel(titleBar);
        iconLabel->setPixmap(icon.pixmap(20, 20));
        iconLabel->setFixedSize(32, 32);
        iconLabel->setScaledContents(true);
    }

    m_titleLabel = new QLabel(title, titleBar);
    m_titleLabel->setObjectName("TitleLabel");
    m_titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    if (iconLabel) {
        titleLayout->addWidget(iconLabel);
    }
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    m_minBtn = new QPushButton(titleBar);
    m_minBtn->setObjectName("cMinButton");
    m_minBtn->setFixedSize(32, 32);
    m_minBtn->setIcon(QIcon(":/resources/icon/min.png"));
    m_minBtn->setIconSize(QSize(20, 20));
    connect(m_minBtn, &QPushButton::clicked, this, [this](){
        showMinimized();
    });

    m_maxBtn = new QPushButton(titleBar);
    m_maxBtn->setObjectName("cMaxButton");
    m_maxBtn->setFixedSize(32, 32);
    m_maxBtn->setIcon(QIcon(":/resources/icon/full.png"));
    m_maxBtn->setIconSize(QSize(20, 20));
    connect(m_maxBtn, &QPushButton::clicked, this, [this](){
        if (!m_maximized) {
            m_restoreRect = geometry();
            setGeometry(QApplication::primaryScreen()->availableGeometry());
            m_maximized = true;
            m_maxBtn->setIcon(QIcon(":/resources/icon/win.png")); // 切换为还原图标
        } else {
            setGeometry(m_restoreRect);
            m_maximized = false;
            m_maxBtn->setIcon(QIcon(":/resources/icon/full.png")); // 切换为最大化图标
        }
    });

    m_closeBtn = new QPushButton(titleBar);
    m_closeBtn->setObjectName("cCloseBtn");
    m_closeBtn->setFixedSize(32, 32);
    m_closeBtn->setIcon(QIcon(":/resources/icon/close.png"));
    m_closeBtn->setIconSize(QSize(20, 20));
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){
        close();
    });

    titleLayout->addWidget(m_minBtn);
    titleLayout->addWidget(m_maxBtn);
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(titleBar);

    // 内容区
    QWidget* contentWrap = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWrap);
    contentLayout->setContentsMargins(8,8,8,8);
    contentLayout->setSpacing(0);
    mainLayout->addWidget(contentWrap, 1);

    // 边框缩放事件过滤
    installEventFilter(this);
    setMouseTracking(true);
    // 递归对子控件也开启鼠标跟踪，保证鼠标在所有区域都能实时变换样式
    auto enableMouseTracking = [](QWidget* w) {
        w->setMouseTracking(true);
        for (QObject* child : w->children()) {
            if (QWidget* cw = qobject_cast<QWidget*>(child)) {
                cw->setMouseTracking(true);
            }
        }
    };
    enableMouseTracking(this);
    m_rubberBand = nullptr;
}

void CusWindow::setContentWidget(QWidget* w) {
    if (m_content) m_content->setParent(nullptr);
    m_content = w;
    if (QVBoxLayout* contentLayout = qobject_cast<QVBoxLayout*>(layout()->itemAt(1)->widget()->layout())) {
        contentLayout->addWidget(w);
    }
}

void CusWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_resizeRegion = hitTest(e->pos());
        // 优先判定四角区域
        if ((m_resizeRegion == TopLeft || m_resizeRegion == TopRight || m_resizeRegion == BottomLeft || m_resizeRegion == BottomRight ||
             m_resizeRegion == Left || m_resizeRegion == Right || m_resizeRegion == Top || m_resizeRegion == Bottom) && !m_maximized) {
            m_resizing = true;
            m_resizeStartRect = geometry();
            m_resizeStartPos = e->globalPos();
            // 橡皮筋效果
            if (!m_rubberBand) {
                m_rubberBand = new QRubberBand(QRubberBand::Rectangle, nullptr);
            }
            m_rubberBandRect = m_resizeStartRect;
            m_rubberBand->setGeometry(m_rubberBandRect);
            m_rubberBand->show();
            e->accept();
            return;
        }
        // 标题栏拖动
        if (e->pos().y() < 40 && !m_maximized) {
            m_dragging = true;
            m_dragPos = e->globalPos() - frameGeometry().topLeft();
            e->accept();
        }
    }
}

void CusWindow::mouseMoveEvent(QMouseEvent* e) {
    if (m_resizing && !m_maximized) {
        QPoint delta = e->globalPos() - m_resizeStartPos;
        QRect r = m_resizeStartRect;
        switch (m_resizeRegion) {
        case Left:      r.setLeft(r.left() + delta.x()); break;
        case Right:     r.setRight(r.right() + delta.x()); break;
        case Top:       r.setTop(r.top() + delta.y()); break;
        case Bottom:    r.setBottom(r.bottom() + delta.y()); break;
        case TopLeft:   r.setTop(r.top() + delta.y()); r.setLeft(r.left() + delta.x()); break;
        case TopRight:  r.setTop(r.top() + delta.y()); r.setRight(r.right() + delta.x()); break;
        case BottomLeft:r.setBottom(r.bottom() + delta.y()); r.setLeft(r.left() + delta.x()); break;
        case BottomRight:r.setBottom(r.bottom() + delta.y()); r.setRight(r.right() + delta.x()); break;
        default: break;
        }
        // 限制最小尺寸
        if (r.width() < 200) r.setWidth(200);
        if (r.height() < 100) r.setHeight(100);
        // 橡皮筋效果
        if (m_rubberBand) {
            m_rubberBand->setGeometry(r);
        }
        e->accept();
        return;
    }
    if (m_dragging && !m_maximized) {
        move(e->globalPos() - m_dragPos);
        e->accept();
        return;
    }
    updateCursor(e->pos());
}

void CusWindow::mouseReleaseEvent(QMouseEvent* e) {
    m_dragging = false;
    if (m_resizing && m_rubberBand) {
        // 拖动结束，应用橡皮筋区域
        setGeometry(m_rubberBand->geometry());
        m_rubberBand->hide();
    }
    m_resizing = false;
    m_resizeRegion = None;
    e->accept();
}

void CusWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->pos().y() < 40) {
        m_maxBtn->click();
    }
}

void CusWindow::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制阴影
    int shadowWidth = 8;
    QRect shadowRect = rect();
    for (int i = shadowWidth; i > 0; --i) {
        QColor shadowColor(0, 0, 0, 10 + 40 * i / shadowWidth); // 渐变透明
        p.setPen(Qt::NoPen);
        p.setBrush(shadowColor);
        p.drawRoundedRect(shadowRect.adjusted(i, i, -i, -i), 15, 15);
    }

    // 2. 绘制窗口主体
    QColor bg(16,24,24,220); // 220/255 约等于 0.85 透明度
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect().adjusted(shadowWidth, shadowWidth, -shadowWidth, -shadowWidth), 15, 15);

    QWidget::paintEvent(e);
}

void CusWindow::leaveEvent(QEvent* e) {
    unsetCursor();
    QWidget::leaveEvent(e);
}

bool CusWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        updateCursor(me->pos());
    }
    return QWidget::eventFilter(obj, event);
}

CusWindow::ResizeRegion CusWindow::hitTest(const QPoint& pos) const {
    int w = width();
    int h = height();
    const int bw = 16;      // 边界判定宽度（更宽）
    const int corner = 28;  // 四角判定正方形区域（更大）

    int x = pos.x(), y = pos.y();

    // 四角优先，判定为正方形区域
    if (x >= 0 && x <= corner && y >= 0 && y <= corner) return TopLeft;
    if (x >= w - corner && x <= w && y >= 0 && y <= corner) return TopRight;
    if (x >= 0 && x <= corner && y >= h - corner && y <= h) return BottomLeft;
    if (x >= w - corner && x <= w && y >= h - corner && y <= h) return BottomRight;

    // 四边
    if (x >= 0 && x <= bw) return Left;
    if (x >= w - bw && x <= w) return Right;
    if (y >= 0 && y <= bw) return Top;
    if (y >= h - bw && y <= h) return Bottom;

    return None;
}

void CusWindow::updateCursor(const QPoint& pos) {
    switch (hitTest(pos)) {
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor); break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor); break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor); break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor); break;
    default:
        unsetCursor(); break;
    }
}
