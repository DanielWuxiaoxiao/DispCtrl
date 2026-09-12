/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-15 14:23:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:55
 * @Description: 
 */
//
// Custom spin box style to draw thin triangle arrows instead of default squares.
//
#pragma once

#include <QProxyStyle>
#include <QStyleOption>

class CustomSpinBoxStyle : public QProxyStyle {
public:
    explicit CustomSpinBoxStyle(QStyle *baseStyle = nullptr);

    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;
};
