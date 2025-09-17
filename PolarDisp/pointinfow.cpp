#include "pointinfow.h"
#include "ui_pointinfow.h"
#include <QStyleOption>
#include <QPainter>

PointInfoW::PointInfoW(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointInfoW)
{
    ui->setupUi(this);
}

void PointInfoW::paintEvent(QPaintEvent* event) {
    QStyleOption o;
    o.initFrom(this); // 初始化 QStyleOption
    o.rect = rect();  // 设置绘制区域
    // 绘制背景和边框
    // QWidget::paintEvent(event); // 如果要继承父类的绘制，可以调用，但对于纯背景绘制，可能不需要
    QPainter painter(this);
    // 绘制背景色和圆角
    // 注意：这里直接使用 QStyle 绘制，可以更好地处理样式表中的属性
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);
}

PointInfoW::~PointInfoW()
{
    delete ui;
}
