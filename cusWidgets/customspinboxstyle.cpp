/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-15 14:23:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:55
 * @Description: 
 */
//
// Custom spin box style to render slim triangle arrows matching combobox look.
//

#include "customspinboxstyle.h"

#include <QPainter>
#include <QStyleOptionSpinBox>

CustomSpinBoxStyle::CustomSpinBoxStyle(QStyle *baseStyle)
    : QProxyStyle(baseStyle) {}

void CustomSpinBoxStyle::drawComplexControl(ComplexControl control,
                                            const QStyleOptionComplex *option,
                                            QPainter *painter,
                                            const QWidget *widget) const {
    if (control == CC_SpinBox) {
        const QStyleOptionSpinBox *spinOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        if (!spinOpt || !painter) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }

        // Draw only the frame + edit field via base style; skip default up/down background.
        QStyleOptionSpinBox frameOpt(*spinOpt);
        frameOpt.subControls = SC_SpinBoxFrame | SC_SpinBoxEditField;
        QProxyStyle::drawComplexControl(control, &frameOpt, painter, widget);

        // Now draw custom arrows in their subcontrol rects (no button backgrounds).
        QStyleOption arrowOpt;
        arrowOpt.palette = spinOpt->palette;
        arrowOpt.state = spinOpt->state;

        // Up arrow
        QRect upRect = subControlRect(control, spinOpt, SC_SpinBoxUp, widget);
        arrowOpt.rect = upRect;
        drawPrimitive(PE_IndicatorSpinUp, &arrowOpt, painter, widget);

        // Down arrow
        QRect downRect = subControlRect(control, spinOpt, SC_SpinBoxDown, widget);
        arrowOpt.rect = downRect;
        drawPrimitive(PE_IndicatorSpinDown, &arrowOpt, painter, widget);
        return;
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void CustomSpinBoxStyle::drawPrimitive(PrimitiveElement element,
                                       const QStyleOption *option,
                                       QPainter *painter,
                                       const QWidget *widget) const {
    if (element == PE_IndicatorSpinUp || element == PE_IndicatorSpinDown) {
        if (!option || !painter)
            return;

        painter->save();

        // Use the button text color from the palette to keep in sync with theme/QSS.
        QColor arrowColor = option->palette.buttonText().color();
        if (option->state & State_MouseOver)
            arrowColor = arrowColor.lighter(110);
        if (!(option->state & State_Enabled))
            arrowColor.setAlpha(120);

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(arrowColor);

        const QRect r = option->rect;
        // Triangle size similar to combobox arrow: width 10, height 6.
        const int triW = 10;
        const int triH = 6;
        const int centerX = r.center().x();
        const int centerY = r.center().y();

        QPolygon poly;
        if (element == PE_IndicatorSpinUp) {
            poly << QPoint(centerX - triW / 2, centerY + triH / 2)
                 << QPoint(centerX + triW / 2, centerY + triH / 2)
                 << QPoint(centerX, centerY - triH / 2);
        } else { // PE_IndicatorSpinDown
            poly << QPoint(centerX - triW / 2, centerY - triH / 2)
                 << QPoint(centerX + triW / 2, centerY - triH / 2)
                 << QPoint(centerX, centerY + triH / 2);
        }

        painter->drawPolygon(poly);
        painter->restore();
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}
