/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:14
 * @Description: 
 */
#ifndef TOOLTIP_H
#define TOOLTIP_H

#pragma once
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include "polaraxis.h"

#define TOOL_TIP Tooltip::getInstance() // tooltip为单例模式
class Tooltip : public QGraphicsItemGroup {
public:
    Tooltip(QGraphicsItem* parent = nullptr);
    static Tooltip *getInstance();
    void showTooltip(const QPointF& scenePos,const QString& text);
    void hideTooltip();

private:
    QGraphicsRectItem* m_background;
    QGraphicsTextItem* m_text;
};

#endif // TOOLTIP_H
