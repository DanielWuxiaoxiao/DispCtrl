// sectordetmanager.h
#ifndef SECTORDETMANAGER_H
#define SECTORDETMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

struct SectorDetNode {
    DetPoint* point = nullptr;
};

class SectorDetManager : public QObject
{
    Q_OBJECT

public:
    explicit SectorDetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    virtual ~SectorDetManager();

    // 添加检测点
    void addDetPoint(const PointInfo& info);
    
    // 刷新所有点的位置和可见性
    void refreshAll();
    
    // 显隐控制
    void setAllVisible(bool visible);
    
    // 设置点大小比例
    void setPointSizeRatio(float ratio);
    
    // 设置扇形角度范围
    void setAngleRange(float minAngle, float maxAngle);
    
    // 清理所有点
    void clear();
    
    // 获取点数量
    int pointCount() const { return m_nodes.size(); }

private:
    QPointF polarToPixel(float range, float azimuthDeg) const;
    bool inRange(float range) const;
    bool inAngle(float azimuthDeg) const;
    bool isPointVisible(const PointInfo& info) const;

private:
    QGraphicsScene* m_scene;
    PolarAxis* m_axis;
    QVector<SectorDetNode> m_nodes;
    
    bool m_visible = true;
    float m_pointSizeRatio = 1.0f;
    float m_minAngle = -30.0f;
    float m_maxAngle = 30.0f;
};

#endif // SECTORDETMANAGER_H