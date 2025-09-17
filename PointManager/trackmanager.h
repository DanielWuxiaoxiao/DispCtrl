#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMap>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

// 可拖拽文本标签，拖动时自动更新连线
class DraggableLabel : public QGraphicsTextItem
{
public:
    DraggableLabel(QGraphicsItem* parent = nullptr);
    void setAnchorItem(QGraphicsItem* anchor, QGraphicsLineItem* tether);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QGraphicsItem* anchor = nullptr;
    QGraphicsLineItem* tether = nullptr; // 连接 label 与 anchor 的线
};

struct TrackNode {
    TrackPoint* point = nullptr;
    QGraphicsLineItem* lineFromPrev = nullptr; // 与前一节点的连线
    // 如需时间戳/序列号，可在此处再加字段
};

struct TrackSeries {
    QVector<TrackNode> nodes;  //  points
    DraggableLabel* label = nullptr;          // 最新点标签
    QGraphicsLineItem* labelLine = nullptr;   // 标签 ↔ 最新点 的连线
    bool visible = true;
    QColor color;
};

class TrackManager : public QObject
{
    Q_OBJECT
public:
    explicit TrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    ~TrackManager();  // 添加析构函数声明

    // 添加一个航迹点（按 batchID 拆分不同航迹）
    void addTrackPoint(const PointInfo& info);

    // 范围/像素变化后调用（例如 PolarGrid 调整了 pixelRange 或 min/max）
    void refreshAll();

    // 显隐控制
    void setBatchVisible(int batchID, bool vis);
    void setAllVisible(bool vis);

    // 删除/清理
    void removeBatch(int batchID);
    void clear();

    // 样式/尺寸
    void setPointSizeRatio(float ratio);
    void setBatchColor(int batchID, const QColor& c);

    // 扇区角度过滤
    void setAngleRange(double startDeg, double endDeg);

private:
    void ensureSeries(int batchID);
    void updateLatestLabel(int batchID);
    void updateNodeVisibility(TrackNode& node);
    void updateBatchVisibility(int batchID);
    void updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b);

    // 根据 PolarAxis 计算像素坐标
    QPointF polarToPixel(float range, float azimuthDeg) const;
    bool inRange(float range) const;

private:
    QGraphicsScene* mScene = nullptr;
    PolarAxis* mAxis = nullptr;
    QMap<int, TrackSeries> mSeries; // batchID → Series
    float mPointSizeRatio = 1.f;
    double m_angleStart = 0.0;
    double m_angleEnd = 360.0;
    bool inAngle(float azimuthDeg) const;
};
