/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-19 11:01:16
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:16
 * @Description: 
 */
/**
 * @file customcombobox.cpp
 * @brief 自定义ComboBox组件实现文件
 * @details 实现带有自定义绿色箭头的ComboBox组件
 * 
 * 实现要点：
 * 1. 保持原有功能：完全继承QComboBox的所有行为
 * 2. 自定义绘制：重写paintEvent绘制自定义箭头
 * 3. 状态管理：根据hover状态调整箭头颜色
 * 4. 性能优化：高效的绘制实现，避免不必要的重绘
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

#include "customcombobox.h"
#include <QStylePainter>
#include <QPolygon>

/**
 * @brief 构造函数实现
 * @param parent 父窗口组件
 * @details 初始化自定义ComboBox，保持默认QComboBox行为
 */
CustomComboBox::CustomComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // 保持默认设置，不需要特殊初始化
}

/**
 * @brief 重写paintEvent实现自定义绘制
 * @param event 绘制事件对象
 * @details 先绘制标准ComboBox，然后在其上绘制自定义箭头
 * 
 * 绘制流程：
 * 1. 使用QStylePainter绘制标准ComboBox外观（背景、边框、文本）
 * 2. 隐藏标准箭头区域
 * 3. 绘制自定义的绿色三角形箭头
 * 4. 根据hover状态调整箭头颜色
 */
void CustomComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // 绘制ComboBox的基础部分（背景、边框、文本等）
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    
    // 隐藏标准箭头，我们将绘制自定义箭头
    opt.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField;
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);
    
    // 绘制ComboBox文本
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
    
    // 绘制自定义箭头
    drawCustomArrow(&painter, rect());
}

/**
 * @brief 绘制自定义下拉箭头
 * @param painter 绘制器对象
 * @param rect ComboBox整体区域
 * @details 在ComboBox右侧绘制绿色三角形箭头
 * 
 * 箭头特性：
 * - 位置：ComboBox右侧边缘内侧
 * - 形状：向下的等腰三角形
 * - 颜色：默认 #66ffcc，hover #99ffdd
 * - 尺寸：8x6像素，适中大小
 * - 抗锯齿：平滑边缘渲染
 */
void CustomComboBox::drawCustomArrow(QPainter *painter, const QRect &rect)
{
    // 箭头尺寸设置 - 简单线条式
    const int arrowSize = 4;    // 箭头大小
    
    // 计算箭头位置：右侧边缘内侧，垂直居中
    int arrowX = rect.right() - 15;  // 距离右边缘15像素
    int arrowY = rect.center().y();  // 垂直居中
    
    // 根据hover状态设置箭头颜色
    bool isHover = underMouse();
    QColor arrowColor = isHover ? 
                       QColor(153, 255, 221) :  // #99ffdd - hover状态亮绿色
                       QColor(102, 255, 204);   // #66ffcc - 默认状态青绿色
    
    // 设置绘制参数 - 线条式绘制
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);  // 无填充，只绘制线条
    
    // 绘制简单的线条式箭头（倒V字形）
    QPoint leftPoint(arrowX - arrowSize, arrowY - arrowSize/2);   // 左上点
    QPoint centerPoint(arrowX, arrowY + arrowSize/2);             // 中心下点
    QPoint rightPoint(arrowX + arrowSize, arrowY - arrowSize/2);  // 右上点
    
    // 绘制两条线组成的箭头
    painter->drawLine(leftPoint, centerPoint);   // 左边线
    painter->drawLine(centerPoint, rightPoint);  // 右边线
}