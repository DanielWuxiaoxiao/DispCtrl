/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-15 14:23:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:55
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-01-12
 * @Description: Custom SpinBox implementation with styled arrows
 */
/**
 * @file customspinbox.cpp
 * @brief 自定义SpinBox组件实现文件
 * @details 实现带有自定义绿色箭头的SpinBox组件
 *
 * 实现要点：
 * 1. 保持原有功能：完全继承QSpinBox/QDoubleSpinBox的所有行为
 * 2. 自定义绘制：重写paintEvent绘制自定义上下箭头
 * 3. 状态管理：根据hover状态调整箭头颜色
 * 4. 按钮透明：使按钮背景透明，箭头清晰显示
 *
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2025
 */

#include "customspinbox.h"
#include <QStylePainter>
#include <QStyleOptionSpinBox>
#include <QStyle>

// ============================================================================
// CustomSpinBox Implementation
// ============================================================================

/**
 * @brief CustomSpinBox构造函数
 * @param parent 父窗口组件
 */
CustomSpinBox::CustomSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    // 保持默认SpinBox行为，尺寸由QSS控制
}

/**
 * @brief 重写paintEvent实现自定义绘制
 * @param event 绘制事件对象
 * @details 先绘制标准SpinBox，然后在其上绘制自定义箭头
 *
 * 绘制流程：
 * 1. 使用QStylePainter绘制标准SpinBox外观（背景、边框、文本）
 * 2. 隐藏标准箭头区域
 * 3. 获取上下按钮区域
 * 4. 在按钮区域绘制自定义的绿色三角形箭头
 */
void CustomSpinBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    // 绘制SpinBox基础部分（背景、边框、编辑框）
    // 隐藏标准箭头，我们将绘制自定义箭头
    opt.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
    painter.drawComplexControl(QStyle::CC_SpinBox, opt);

    // 获取上下按钮的区域
    QRect upRect = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this);
    QRect downRect = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this);

    // 绘制自定义箭头
    drawUpArrow(&painter, upRect);
    drawDownArrow(&painter, downRect);
}

/**
 * @brief 绘制自定义上箭头
 * @param painter 绘制器对象
 * @param rect 上按钮区域
 * @details 在上按钮区域绘制绿色向上三角形箭头
 *
 * 箭头特性：
 * - 形状：向上的倒V字形
 * - 颜色：默认 #66ffcc，hover #99ffdd
 * - 尺寸：4像素
 * - 抗锯齿：平滑边缘渲染
 */
void CustomSpinBox::drawUpArrow(QPainter *painter, const QRect &rect)
{
    const int arrowSize = 4;

    int centerX = rect.center().x();
    int centerY = rect.center().y();

    // 根据hover状态设置箭头颜色
    bool isHover = underMouse() && rect.contains(mapFromGlobal(QCursor::pos()));
    QColor arrowColor = isHover ?
                       QColor(153, 255, 221) :  // #99ffdd - hover状态
                       QColor(102, 255, 204);   // #66ffcc - 默认状态

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);

    // 绘制向上箭头（倒V字形）
    QPoint topPoint(centerX, centerY - arrowSize/2);        // 顶点
    QPoint leftPoint(centerX - arrowSize, centerY + arrowSize/2);   // 左下点
    QPoint rightPoint(centerX + arrowSize, centerY + arrowSize/2);  // 右下点

    painter->drawLine(leftPoint, topPoint);
    painter->drawLine(topPoint, rightPoint);
}

/**
 * @brief 绘制自定义下箭头
 * @param painter 绘制器对象
 * @param rect 下按钮区域
 * @details 在下按钮区域绘制绿色向下三角形箭头
 *
 * 箭头特性：
 * - 形状：向下的V字形
 * - 颜色：默认 #66ffcc，hover #99ffdd
 * - 尺寸：4像素
 * - 抗锯齿：平滑边缘渲染
 */
void CustomSpinBox::drawDownArrow(QPainter *painter, const QRect &rect)
{
    const int arrowSize = 4;

    int centerX = rect.center().x();
    int centerY = rect.center().y();

    // 根据hover状态设置箭头颜色
    bool isHover = underMouse() && rect.contains(mapFromGlobal(QCursor::pos()));
    QColor arrowColor = isHover ?
                       QColor(153, 255, 221) :  // #99ffdd - hover状态
                       QColor(102, 255, 204);   // #66ffcc - 默认状态

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);

    // 绘制向下箭头（V字形）
    QPoint bottomPoint(centerX, centerY + arrowSize/2);     // 底点
    QPoint leftPoint(centerX - arrowSize, centerY - arrowSize/2);   // 左上点
    QPoint rightPoint(centerX + arrowSize, centerY - arrowSize/2);  // 右上点

    painter->drawLine(leftPoint, bottomPoint);
    painter->drawLine(bottomPoint, rightPoint);
}

// ============================================================================
// CustomDoubleSpinBox Implementation
// ============================================================================

/**
 * @brief CustomDoubleSpinBox构造函数
 * @param parent 父窗口组件
 */
CustomDoubleSpinBox::CustomDoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    // 保持默认DoubleSpinBox行为，尺寸由QSS控制
}

/**
 * @brief 重写paintEvent实现自定义绘制
 * @param event 绘制事件对象
 * @details 与CustomSpinBox实现相同的绘制逻辑
 */
void CustomDoubleSpinBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    // 绘制DoubleSpinBox基础部分
    opt.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
    painter.drawComplexControl(QStyle::CC_SpinBox, opt);

    // 获取上下按钮的区域
    QRect upRect = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this);
    QRect downRect = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this);

    // 绘制自定义箭头
    drawUpArrow(&painter, upRect);
    drawDownArrow(&painter, downRect);
}

/**
 * @brief 绘制自定义上箭头
 * @param painter 绘制器对象
 * @param rect 上按钮区域
 */
void CustomDoubleSpinBox::drawUpArrow(QPainter *painter, const QRect &rect)
{
    const int arrowSize = 4;

    int centerX = rect.center().x();
    int centerY = rect.center().y();

    bool isHover = underMouse() && rect.contains(mapFromGlobal(QCursor::pos()));
    QColor arrowColor = isHover ?
                       QColor(153, 255, 221) :
                       QColor(102, 255, 204);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);

    QPoint topPoint(centerX, centerY - arrowSize/2);
    QPoint leftPoint(centerX - arrowSize, centerY + arrowSize/2);
    QPoint rightPoint(centerX + arrowSize, centerY + arrowSize/2);

    painter->drawLine(leftPoint, topPoint);
    painter->drawLine(topPoint, rightPoint);
}

/**
 * @brief 绘制自定义下箭头
 * @param painter 绘制器对象
 * @param rect 下按钮区域
 */
void CustomDoubleSpinBox::drawDownArrow(QPainter *painter, const QRect &rect)
{
    const int arrowSize = 4;

    int centerX = rect.center().x();
    int centerY = rect.center().y();

    bool isHover = underMouse() && rect.contains(mapFromGlobal(QCursor::pos()));
    QColor arrowColor = isHover ?
                       QColor(153, 255, 221) :
                       QColor(102, 255, 204);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);

    QPoint bottomPoint(centerX, centerY + arrowSize/2);
    QPoint leftPoint(centerX - arrowSize, centerY - arrowSize/2);
    QPoint rightPoint(centerX + arrowSize, centerY - arrowSize/2);

    painter->drawLine(leftPoint, bottomPoint);
    painter->drawLine(bottomPoint, rightPoint);
}
