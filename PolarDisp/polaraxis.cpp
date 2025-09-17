#include "polaraxis.h"
#include <QtMath>

PolarAxis::PolarAxis(QObject* parent)
    : QObject(parent) {

}

void PolarAxis::setRange(double minR, double maxR) {
    m_minRange = minR;
    m_maxRange = maxR;
    emit rangeChanged(m_minRange, m_maxRange);
}

void PolarAxis::setPixelsPerMeter(double pixelsPerMeter)
{
    m_pixelsPerMeter = pixelsPerMeter;
}
double PolarAxis::rangeToPixel(double distance) const {
    return distance * m_pixelsPerMeter;
}

double PolarAxis::pixelToRange(double pixel) const {
    return pixel / m_pixelsPerMeter;
}

QPointF PolarAxis::polarToScene(double distance, double azimuthDeg) const {
    double r = rangeToPixel(distance);
    double rad = qDegreesToRadians(azimuthDeg);
    return QPointF(r * qSin(rad), -r * qCos(rad));
}

PolarAxis::PolarCoord PolarAxis::sceneToPolar(const QPointF& scenePos) const {
    double r = qSqrt(scenePos.x() * scenePos.x() + scenePos.y() * scenePos.y());
    double distance = pixelToRange(r);
    double azimuthDeg = qRadiansToDegrees(qAtan2(scenePos.x(), -scenePos.y()));
    if (azimuthDeg < 0) azimuthDeg += 360;
    return { distance, azimuthDeg };
}

