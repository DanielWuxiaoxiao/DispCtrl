// sectorpolargrid.h - 专门为扇形显示设计的网格类
#ifndef SECTORPOLARGRID_H
#define SECTORPOLARGRID_H

#include <QObject>
#include <QGraphicsItem>
#include <QPen>

class PolarAxis;

class SectorPolarGrid : public QObject, public QGraphicsItem {
    Q_OBJECT
public:
    explicit SectorPolarGrid(PolarAxis* axis, QGraphicsItem* parent = nullptr);

    // 设置扇形显示范围
    void setSectorRange(float minAngle, float maxAngle);

    // QGraphicsItem 接口
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    void updateGrid();

private:
    PolarAxis* m_axis;
    float m_minAngle = -30.0f;
    float m_maxAngle = 30.0f;

    QPen m_rangePen;     // 距离圈画笔
    QPen m_anglePen;     // 角度线画笔
    QPen m_textPen;      // 文本画笔

    void drawRangeCircles(QPainter* painter);
    void drawAngleLines(QPainter* painter);
    void drawLabels(QPainter* painter);

    // 检查角度是否在扇形范围内
    bool isAngleInSector(float angle) const;
};

#endif // SECTORPOLARGRID_H
