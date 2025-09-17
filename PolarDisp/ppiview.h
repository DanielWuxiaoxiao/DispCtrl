#ifndef PPIVIEW_H
#define PPIVIEW_H
#pragma once
#include <QGraphicsView>
#include <QRubberBand>
class PPIScene;
class mainviewTopLeft;
class PointInfoW;

class PPIView : public QGraphicsView {
    Q_OBJECT
public:
    explicit PPIView(QWidget* parent=nullptr);
    void setPPIScene(PPIScene* scene);
    void enableRubberBandZoom(bool on);

signals:
    // 当视图大小改变时，发出此信号
    void viewResized(const QSize& newSize);
    void areaSelected(const QRectF& sceneRect); // 新增：当rubberband选择区域时发出

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    PPIScene* m_scene;
    bool m_rubberBandZoom = true;
    QRubberBand* m_band = nullptr;
    QPoint m_origin;

    // 叠加 UI：不随缩放
    mainviewTopLeft* radarInfoW = nullptr;
    PointInfoW* pointInfo = nullptr;

    void setupOverlay();
    void layoutOverlay();
};
#endif //PPIVIEW_H
