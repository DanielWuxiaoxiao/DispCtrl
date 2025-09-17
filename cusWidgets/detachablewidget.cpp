#include "detachablewidget.h"
#include "cuswindow.h"
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QLabel>

DetachableWidget::DetachableWidget(QString name , QWidget* childWidget, QIcon icon, QWidget* parent)
    : QWidget(parent), m_child(childWidget) , m_name(name) , m_icon(icon)
{
    m_btn = new QPushButton(QIcon(":/resources/icon/maxi.png"),"最大化"); // 小按钮
    connect(m_btn, &QPushButton::clicked, this, &DetachableWidget::detach);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(3,6,3,3);
    m_layout->setSpacing(0);
    QHBoxLayout* titleBar = new QHBoxLayout();
    titleBar->addStretch();
    titleBar->addWidget(m_btn);
    titleBar->addStretch();
    m_layout->addLayout(titleBar);
    m_layout->addWidget(m_child);
    setLayout(m_layout);
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
    if (!m_floatWindow) return;

    m_child->setParent(this);
    m_layout->addWidget(m_child); // 加回布局
    m_floatWindow = nullptr;
}
