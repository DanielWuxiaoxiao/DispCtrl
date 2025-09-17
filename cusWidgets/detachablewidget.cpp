#include "detachablewidget.h"
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QLabel>

DetachableWidget::DetachableWidget(QWidget* childWidget, QWidget* parent)
    : QWidget(parent), m_child(childWidget)
{
    m_btn = new QPushButton("↗", this); // 小按钮
    m_btn->setFixedSize(20, 20);

    connect(m_btn, &QPushButton::clicked, this, &DetachableWidget::detach);

    m_layout = new QVBoxLayout(this);
    QHBoxLayout* titleBar = new QHBoxLayout();
    titleBar->addWidget(new QLabel("面板", this));
    titleBar->addStretch();
    titleBar->addWidget(m_btn);

    m_layout->addLayout(titleBar);
    m_layout->addWidget(m_child);
    setLayout(m_layout);
}

void DetachableWidget::detach() {
    if (m_floatWindow) return; // 已经在外面了

    m_floatWindow = new QDialog(nullptr, Qt::Window);
    m_floatWindow->setAttribute(Qt::WA_DeleteOnClose);
    m_floatWindow->setWindowTitle("独立面板");

    QVBoxLayout* floatLayout = new QVBoxLayout(m_floatWindow);
    m_child->setParent(m_floatWindow);  // 把内容转移出去
    floatLayout->addWidget(m_child);

    // 当窗口关闭时，把内容放回原来的地方
    connect(m_floatWindow, &QDialog::finished, this, &DetachableWidget::reattach);

    m_floatWindow->resize(400, 300);
    m_floatWindow->show();
}

void DetachableWidget::reattach() {
    if (!m_floatWindow) return;

    m_child->setParent(this);
    m_layout->addWidget(m_child); // 加回布局
    m_floatWindow = nullptr;
}
