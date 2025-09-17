#ifndef AZELRANGEWIDGET_H
#define AZELRANGEWIDGET_H

#pragma once
#include <QWidget>
#include <QLineEdit>

class AzElRangeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AzElRangeWidget(QWidget *parent = nullptr);

    void setAzRange(int minDeg, int maxDeg);     // 0..360, 北为0°，顺时针递增
    void setElRange(int minDeg, int maxDeg);     // -45..+45

signals:
    void azRangeChanged(int minDeg, int maxDeg);
    void elRangeChanged(int minDeg, int maxDeg);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override { return {360, 260}; }
    QSize sizeHint() const override { return {420, 300}; }

private:
    // 值
    int mAzMin = 15;          // [0,360)
    int mAzMax = 75;          // [0,360)
    int mElMin = -10;         // [-45,45]
    int mElMax =  30;         // [-45,45]

    // 编辑框
    QLineEdit *edAzMin = nullptr;
    QLineEdit *edAzMax = nullptr;
    QLineEdit *edElMin = nullptr;
    QLineEdit *edElMax = nullptr;

    // 工具
    static int norm360(int d);   // 归一化到[0,360)
    static int clamp(int v, int lo, int hi);
    void syncEditors();
    void connectEditors();
    void drawAzDial(QPainter &p, const QRect &rcDial);
    void drawElBar(QPainter &p, const QRect &rcBar);

    static int cwSpan(int a1, int a2) {  // 以顺时针计算 a1→a2 的角距 [0,360)
        int d = (a2 - a1) % 360;
        if (d < 0) d += 360;
        return d;
    }
    // 将“北为0°, 顺时针增加”的方位角 -> 画布角
    // QPainter::drawArc 从3点钟方向(0°)逆时针为正；我们手工计算点位用数学角：x=cosθ,y=sinθ(上正)。
    // 这里返回数学角(弧度)，满足：north=0° -> θ=+90°；顺时针 +α -> θ = 90° - α
    static double azToThetaRad(int azDeg);
};


#endif // AZELRANGEWIDGET_H
