#ifndef POLARGRID_H
#define POLARGRID_H
#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <vector>
#include <QPen>
#include "PolarDisp/polaraxis.h"

class PolarGrid : public QObject {
    Q_OBJECT
public:
    explicit PolarGrid(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);

public slots:
    void updateGrid();
    void setAngleRange(double startDeg, double endDeg);

private:
    QGraphicsScene* mScene;
    PolarAxis* mAxis;
    double m_angleStart = 0.0;
    double m_angleEnd = 360.0;
    QList<QGraphicsItem*> mCircleItems;
    QList<QGraphicsItem*> mTickItems;
    QList<QGraphicsItem*> mTextItems;
    QGraphicsPixmapItem* mRadarIcon = nullptr;
};

#endif // POLARGRID_H
