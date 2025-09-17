#ifndef DET_MANAGER_H
#define DET_MANAGER_H

#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

struct DetNode {
    DetPoint* point = nullptr;
};

class DetManager : public QObject
{
    Q_OBJECT
public:
    explicit DetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);

    // 添加一个检测点
    void addDetPoint(const PointInfo& info);

    // PolarAxis min/max/pixelRange 变化后调用
    void refreshAll();

    // 设置全局显隐
    void setAllVisible(bool vis);

    // 设置大小缩放比（随 viewRatio 调整）
    void setPointSizeRatio(float ratio);

    // 清理
    void clear();

    // 扇区角度过滤
    void setAngleRange(double startDeg, double endDeg);

private:
    QPointF polarToPixel(float range, float azimuthDeg) const;
    bool inRange(float range) const;

private:
    QGraphicsScene* mScene = nullptr;
    PolarAxis* mAxis = nullptr;
    QVector<DetNode> mNodes;
    float mPointSizeRatio = 1.f;
    bool mVisible = true;
    double m_angleStart = 0.0;
    double m_angleEnd = 360.0;
    bool inAngle(float azimuthDeg) const;
};

#endif
