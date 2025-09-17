#ifndef POINT_H
#define POINT_H

#include <QGraphicsEllipseItem>
#include <QColor>
#include "Basic/Protocol.h"

constexpr int DET_SIZE = 1;
constexpr int DET_BIG_SIZE = 3;
constexpr int TRA_SIZE = 3;
constexpr int TRA_BIG_SIZE = 10;

class Point : public QGraphicsEllipseItem
{
public:
    explicit Point(PointInfo &info);
    virtual ~Point() = default;

    // 供 Manager 统一更新位置（像素坐标）
    void updatePosition(float x, float y);
    // 调整点大小（配合缩放）
    virtual void resize(float ratio) = 0;
    // 设置颜色
    virtual void setColor(QColor color) = 0;

    const PointInfo& infoRef() const { return info; }

protected:
    // 悬停放大/tooltip
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    // 子类用于设置普通/放大矩形
    virtual void setSmallRect();
    virtual void setBigRect();

protected:
    PointInfo info;
    QString text;     // tooltip 文本
    // 小/大尺寸（像素）
    float w = 6.f, h = 6.f;
    float W = 10.f, H = 10.f;

    // 记录像素坐标（中心）
    float mX = 0.f, mY = 0.f;

    // 子类可以覆盖这两个值
    float baseSmallW = 6.f, baseSmallH = 6.f;
    float baseBigW   = 10.f, baseBigH   = 10.f;

    // 当前ratio（避免重复计算）
    float curRatio = 1.f;
};

// 检测点
class DetPoint : public Point
{
public:
    explicit DetPoint(PointInfo &info);
    void resize(float ratio) override;
    void setColor(QColor color) override;

protected:
    void setSmallRect() override;
    void setBigRect() override;
};

// 跟踪点
class TrackPoint : public Point
{
public:
    explicit TrackPoint(PointInfo &info);
    void resize(float ratio) override;
    void setColor(QColor color) override;

protected:
    void setSmallRect() override;
    void setBigRect() override;
};

#endif // POINT_H
