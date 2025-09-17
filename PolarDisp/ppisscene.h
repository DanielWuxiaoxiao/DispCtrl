#ifndef PPISSCENE_H
#define PPISSCENE_H

#include <QGraphicsScene>
#include "Basic/Protocol.h"

class PolarAxis;
class PolarGrid;
class TrackManager;
class DetManager;
class Tooltip;
class ScanLayer;

class PPIScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit PPIScene(QObject* parent=nullptr);
    PolarAxis* axis() const { return m_axis; }
    PolarGrid* grid() const { return m_grid; }
    TrackManager* track() const { return m_track; }
    DetManager* det() const { return m_det; }
    Tooltip* tooltip() const { return m_tooltip; }

public slots:
    void setRange(float minR, float maxR);   // 统一改 Range
    // 新增的槽函数，用于响应视图的尺寸变化
    void updateSceneSize(const QSize& newSize);

signals:
    void rangeChanged(float minR, float maxR);

private:
    PolarAxis* m_axis;
    PolarGrid* m_grid;
    TrackManager* m_track;
    DetManager* m_det;
    Tooltip* m_tooltip;
    int pviewMargin = 30;
    void initLayerObjects();
    ScanLayer* m_scan;
};

#endif // PPISSCENE_H
