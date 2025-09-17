// sectortrackmanager.h
#ifndef SECTORTRACKMANAGER_H
#define SECTORTRACKMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMap>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

// 可拖拽的标签类（与原版相同）
class SectorDraggableLabel : public QGraphicsTextItem
{
public:
    SectorDraggableLabel(QGraphicsItem* parent = nullptr);
    void setAnchorItem(QGraphicsItem* anchor, QGraphicsLineItem* tether);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QGraphicsItem* m_anchor = nullptr;
    QGraphicsLineItem* m_tether = nullptr;
};

struct SectorTrackNode {
    TrackPoint* point = nullptr;
    QGraphicsLineItem* lineFromPrev = nullptr;
};

struct SectorTrackSeries {
    QVector<SectorTrackNode> nodes;
    SectorDraggableLabel* label = nullptr;
    QGraphicsLineItem* labelLine = nullptr;
    bool visible = true;
    QColor color;
};

class SectorTrackManager : public QObject
{
    Q_OBJECT

public:
    explicit SectorTrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    virtual ~SectorTrackManager();

    // 添加航迹点
    void addTrackPoint(const PointInfo& info);
    
    // 刷新所有航迹
    void refreshAll();
    
    // 显隐控制
    void setBatchVisible(int batchID, bool visible);
    void setAllVisible(bool visible);
    
    // 样式控制
    void setPointSizeRatio(float ratio);
    void setBatchColor(int batchID, const QColor& color);
    
    // 扇形角度范围
    void setAngleRange(float minAngle, float maxAngle);
    
    // 删除和清理
    void removeBatch(int batchID);
    void clear();
    
    // 获取信息
    int batchCount() const { return m_series.size(); }
    QList<int> batchIDs() const { return m_series.keys(); }

private:
    void ensureSeries(int batchID);
    void updateLatestLabel(int batchID);
    void updateBatchVisibility(int batchID);
    void updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b);
    
    QPointF polarToPixel(float range, float azimuthDeg) const;
    bool inRange(float range) const;
    bool inAngle(float azimuthDeg) const;
    bool isPointVisible(const PointInfo& info) const;

private:
    QGraphicsScene* m_scene;
    PolarAxis* m_axis;
    QMap<int, SectorTrackSeries> m_series;
    
    float m_pointSizeRatio = 1.0f;
    float m_minAngle = -30.0f;
    float m_maxAngle = 30.0f;
};

#endif