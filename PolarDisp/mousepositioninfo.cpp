/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-18 16:12:31
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:07
 * @Description: 
 */
#include "mousepositioninfo.h"
#include "ui_mousepositioninfo.h"
#include <QStyleOption>
#include <QPainter>

MousePositionInfo::MousePositionInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MousePositionInfo)
{
    ui->setupUi(this);
    
    // 设置简化的tooltip
    ui->label_distance->setToolTip("鼠标距离");
    ui->distanceValue->setToolTip("鼠标到雷达中心的距离(公里)");
    ui->label_azimuth->setToolTip("方位角");
    ui->azimuthValue->setToolTip("鼠标位置的方位角(度)");
    
    // 设置对象名称以便样式表应用
    setObjectName("MousePositionInfo");
    ui->distanceValue->setObjectName("MouseDistanceValue");
    ui->azimuthValue->setObjectName("MouseAzimuthValue");
}

void MousePositionInfo::paintEvent(QPaintEvent* event) {
    QStyleOption o;
    o.initFrom(this);
    o.rect = rect();
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);
}

void MousePositionInfo::updatePosition(double distance, double azimuth)
{
    // 距离显示为公里，保留1位小数，添加单位
    ui->distanceValue->setText(QString::number(distance, 'f', 1) + " km");
    
    // 方位角显示为度，保留1位小数，添加单位
    ui->azimuthValue->setText(QString::number(azimuth, 'f', 1) + "°");
}

MousePositionInfo::~MousePositionInfo()
{
    delete ui;
}