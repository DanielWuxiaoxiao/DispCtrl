/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:11
 * @Description: 
 */
#include "pviewtopleft.h"
#include "ui_pviewtopleft.h"
#include <QStyleOption>
#include <QPainter>
#include "Basic/ConfigManager.h"

mainviewTopLeft::mainviewTopLeft(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainviewTopLeft)
{
    ui->setupUi(this);
    
    // 设置简化的tooltip
    ui->label->setToolTip("雷达经度");
    ui->lineEdit->setToolTip("经度(-180°~180°)");
    ui->label_2->setToolTip("雷达纬度"); 
    ui->lat->setToolTip("纬度(-90°~90°)");
    ui->label_3->setToolTip("海拔高度");
    ui->height->setToolTip("高度(米)");
    ui->label_4->setToolTip("阵面指北角");
    ui->dir->setToolTip("指北角(0°~360°)");
    ui->label_6->setToolTip("倾角");
    ui->yaw->setToolTip("俯仰倾角(度)");
    ui->label_7->setToolTip("横滚");
    ui->roll->setToolTip("横滚角(度)");
    
    // 从配置文件初始化雷达位置信息
    ui->lineEdit->setText(QString::number(CF_INS.longitude(), 'f', 6));   // 经度
    ui->lat->setText(QString::number(CF_INS.latitude(), 'f', 6));         // 纬度
    ui->height->setText(QString::number(CF_INS.altitude(), 'f', 1));      // 高度
    ui->dir->setText("0.0");      // 阵面指北角（暂时固定）
    ui->yaw->setText("0.0");      // 倾角（暂时固定）
    ui->roll->setText("0.0");     // 横滚（暂时固定）
}

void mainviewTopLeft::paintEvent(QPaintEvent* event) {
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

mainviewTopLeft::~mainviewTopLeft()
{
    delete ui;
}
