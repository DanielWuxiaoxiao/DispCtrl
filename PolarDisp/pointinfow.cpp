/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:08
 * @Description: 
 */
#include "pointinfow.h"
#include "ui_pointinfow.h"
#include <QStyleOption>
#include <QPainter>

PointInfoW::PointInfoW(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointInfoW)
{
    ui->setupUi(this);
    
    // 设置简化的tooltip
    ui->label_8->setToolTip("批号");
    ui->batch->setToolTip("目标批次编号");
    ui->label->setToolTip("距离");
    ui->range->setToolTip("目标距离(公里)");
    ui->label_3->setToolTip("方位");
    ui->azi->setToolTip("方位角(度)");
    ui->label_5->setToolTip("俯仰");
    ui->ele->setToolTip("俯仰角(度)");
    ui->label_6->setToolTip("速度");
    ui->speed->setToolTip("径向速度(m/s)");
    ui->label_7->setToolTip("信噪比");
    ui->SNR->setToolTip("信噪比(dB)");
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
