/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:12
 * @Description: 
 */
#ifndef SCANLAYER_H
#define SCANLAYER_H

#include <QGraphicsItem>
#include <QTimer>
#include <QObject>

class PolarAxis;
//object必须在前面
class ScanLayer : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    enum ScanMode { Loop, PingPong };

    ScanLayer(PolarAxis* axis, QGraphicsItem* parent=nullptr);
    ~ScanLayer();

    QRectF boundingRect() const override;
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem*,
               QWidget*) override;

    void setSweepSpeed(int msPerStep);
    void setSweepRange(double startDeg, double endDeg);
    void setScanMode(ScanMode mode);

private slots:
    void advanceSweep();

private:
    PolarAxis* m_axis;
    QTimer* m_timer;
    double m_angle;
    double m_fixedStart, m_fixedEnd;

    ScanMode m_mode = Loop;
    int m_direction = +1; // 1=顺时针，-1=逆时针  每次转动度数
};

#endif // SCANLAYER_H
