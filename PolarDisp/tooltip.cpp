/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:54
 * @Description: 
 */
#include "Basic/uitheme.h"
#include "tooltip.h"
#include <QBrush>
#include <QPen>
#include <QtMath>
#include "Basic/DispBasci.h"
#include <QFont>
#include "Basic/uifontscale.h"

Q_GLOBAL_STATIC(Tooltip, ToolTipInstance)
Tooltip::Tooltip(QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
{
    m_background = new QGraphicsRectItem();
    // 深色半透明背景，与系统风格一致
    m_background->setBrush(QBrush((UiTheme::classicEnabled ? QColor(16, 24, 24, 230) : QColor(27, 34, 40, 235))));
    // 冷青细边框；正文用浅色，避免整块文字抢占目标颜色。
    QPen borderPen(UiTheme::classicEnabled ? QColor(102, 255, 204) : PPI_ACCENT_COLOR, 1);
    m_background->setPen(borderPen);

    m_text = new QGraphicsTextItem();
    m_text->setDefaultTextColor(UiTheme::classicEnabled ? QColor(102, 255, 204) : PPI_TEXT_COLOR);

    // 设置字体
    QFont font = m_text->font();
    font.setPixelSize(UiFontScale::pointPixels(8));
    font.setWeight(QFont::Bold);
    m_text->setFont(font);

    addToGroup(m_background);
    addToGroup(m_text);

    setVisible(false);
    setZValue(TOOL_TIP_Z); // 保证在最上层

    // 忽略视图变换，保证tooltip在放大视图中不会跟随缩放而变大
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

void Tooltip::showTooltip(const QPointF& scenePos,const QString& text)
{
    m_text->setPlainText(text);

    // 更新背景矩形大小以适应文字
    QRectF textRect = m_text->boundingRect();
    m_background->setRect(textRect.adjusted(-6, -3, 6, 3)); // 添加内边距

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
