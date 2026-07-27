#ifndef PPIVIEW_H
#define PPIVIEW_H

#include <QGraphicsView>
#include <QRubberBand>

class PPIScene;

class PPIView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PPIView(QWidget* parent = nullptr);
    void setPPIScene(PPIScene* scene);
    void enableRubberBandZoom(bool enabled);

signals:
    void viewResized(const QSize& newSize);
    void cursorPositionChanged(double distanceKm, double bearingDeg);
    void areaSelected(const QRectF& sceneRect);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    PPIScene* m_scene = nullptr;
    bool m_rubberBandZoom = false;
    QRubberBand* m_band = nullptr;
    QPoint m_dragOrigin;
};

#endif // PPIVIEW_H
