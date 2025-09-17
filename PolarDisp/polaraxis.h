#ifndef POLARAXIS_H
#define POLARAXIS_H
#pragma once
#include <cmath>
#include <QPointF>
#include <QObject>

class PolarAxis : public QObject {
    Q_OBJECT
public:
    explicit PolarAxis(QObject* parent = nullptr);

    void setRange(double minR, double maxR);
    double minRange() const { return m_minRange; }
    double maxRange() const { return m_maxRange; }
    void setPixelsPerMeter(double pixelsPerMeter);

    // 物理距离转像素
    double rangeToPixel(double distance) const;
    // 像素转物理距离
    double pixelToRange(double pixel) const;

    // 极坐标 -> 场景坐标
    QPointF polarToScene(double distance, double azimuthDeg) const;
    // 场景坐标 -> 极坐标 (返回 {距离, 方位角°})
    struct PolarCoord {
        double distance;
        double azimuthDeg;
    };
    PolarCoord sceneToPolar(const QPointF& scenePos) const;

signals:
    void rangeChanged(double minR, double maxR);

private:
    double m_minRange = 0.0;
    double m_maxRange = 5000.0; // 默认 1km
    double m_pixelsPerMeter = 1.0;
};
#endif // POLARAXIS_H
