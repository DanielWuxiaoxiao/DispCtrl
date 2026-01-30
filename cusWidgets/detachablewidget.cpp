/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:47
 * @Description: 
 */
#include "detachablewidget.h"
#include "cuswindow.h"
#include <QHBoxLayout>
#include <QCloseEvent>
DetachableWidget::DetachableWidget(QString name , QWidget* childWidget, QIcon icon, QWidget* parent)
    : QWidget(parent), m_child(childWidget) , m_name(name) , m_icon(icon)
{
    m_btn = new QPushButton(); // 小按钮
    m_btn->setIcon(QIcon(":/resources/icon/full.png"));
    m_btn->setToolTip("分离窗口");
    m_btn->setObjectName("DetachBtn");
    connect(m_btn, &QPushButton::clicked, this, &DetachableWidget::detach);

    QLabel* iconLabel = nullptr;
    if (!icon.isNull()) {
        iconLabel = new QLabel(this);
        iconLabel->setPixmap(icon.pixmap(20, 20));
        iconLabel->setFixedSize(32, 32);
        iconLabel->setScaledContents(true);
        iconLabel->setToolTip("组件图标");
    }
    m_titleLabel = new QLabel(m_name, this);
    m_titleLabel->setObjectName("dTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_titleLabel->setToolTip("组件名称");

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(3,6,3,3);
    m_layout->setSpacing(0);
    QHBoxLayout* titleBar = new QHBoxLayout();
    titleBar->setContentsMargins(8,0,8,0);
    if (iconLabel) {
        titleBar->addWidget(iconLabel);
    }
    titleBar->addWidget(m_titleLabel);
    titleBar->addStretch();
    titleBar->addWidget(m_btn);
    m_layout->addLayout(titleBar);
    m_layout->addWidget(m_child);
    setLayout(m_layout);
}

DetachableWidget::~DetachableWidget()
{
    m_destroying = true;
    if (m_floatWindow) {
        // 防止浮动窗关闭时的reattach在析构阶段操作已销毁的this
        disconnect(m_floatWindow, nullptr, this, nullptr);
        m_floatWindow->close();
        m_child->setParent(this);
        m_floatWindow = nullptr;
    }
    // m_child 会作为普通子控件随父窗口一起析构
}

void DetachableWidget::detach() {
    if (m_floatWindow) return; // 已经在外面了

    CusWindow* win = new CusWindow(m_name, m_icon);
    m_floatWindow = win;
    m_child->setParent(win);
    win->setContentWidget(m_child);

    connect(win, &CusWindow::windowClosed, this, &DetachableWidget::reattach);
    connect(win, &QWidget::destroyed, this, &DetachableWidget::reattach);

    win->resize(400, 300);
    win->show();
}

void DetachableWidget::reattach() {
    if (!m_floatWindow || m_destroying) return;

    m_child->setParent(this);
    m_layout->addWidget(m_child); // 加回布局
    m_floatWindow = nullptr;
}
