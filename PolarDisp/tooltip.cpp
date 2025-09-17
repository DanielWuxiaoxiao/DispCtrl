#include "tooltip.h"
#include <QBrush>
#include <QPen>
#include <QtMath>
#include "Basic/DispBasci.h"

Q_GLOBAL_STATIC(Tooltip, ToolTipInstance)
Tooltip::Tooltip(QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
{
    m_background = new QGraphicsRectItem();
    m_background->setBrush(QBrush(QColor(50, 50, 50, 180)));
    m_background->setPen(Qt::NoPen);

    m_text = new QGraphicsTextItem();
    m_text->setDefaultTextColor(Qt::white);

    addToGroup(m_background);
    addToGroup(m_text);

    setVisible(false);
    setZValue(TOOL_TIP_Z); // 保证在最上层
}

void Tooltip::showTooltip(const QPointF& scenePos,const QString& text)
{
    m_text->setPlainText(text);

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
