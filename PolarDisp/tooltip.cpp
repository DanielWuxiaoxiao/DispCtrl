/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:12
 * @Description: 
 */
#include "tooltip.h"
#include <QBrush>
#include <QPen>
#include <QtMath>
#include "Basic/DispBasci.h"
#include <QFont>

Q_GLOBAL_STATIC(Tooltip, ToolTipInstance)
Tooltip::Tooltip(QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
{
    m_background = new QGraphicsRectItem();
    // 深色半透明背景，与系统风格一致
    m_background->setBrush(QBrush(QColor(16, 24, 24, 230)));
    // 青绿色边框，与主题色一致
    QPen borderPen(QColor(102, 255, 204), 1);
    m_background->setPen(borderPen);

    m_text = new QGraphicsTextItem();
    m_text->setDefaultTextColor(QColor(102, 255, 204)); // 青绿色文字

    // 设置字体
    QFont font = m_text->font();
    font.setPointSize(10);
    font.setWeight(QFont::Bold);
    m_text->setFont(font);

    addToGroup(m_background);
    addToGroup(m_text);

    setVisible(false);
    setZValue(TOOL_TIP_Z); // 保证在最上层
}

void Tooltip::showTooltip(const QPointF& scenePos,const QString& text)
{
    m_text->setPlainText(text);

    // 更新背景矩形大小以适应文字
    QRectF textRect = m_text->boundingRect();
    m_background->setRect(textRect.adjusted(-8, -4, 8, 4)); // 添加内边距

    setPos(scenePos);
    setVisible(true);
}

void Tooltip::hideTooltip() {
    setVisible(false);
}
Tooltip *Tooltip::getInstance()
{
    return ToolTipInstance();
}
