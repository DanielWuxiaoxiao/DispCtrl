// zoomview.h
#ifndef ZOOMVIEW_H
#define ZOOMVIEW_H

#include <QGraphicsView>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLineF>

class PPIScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsTextItem;

// 工具栏控件
class ZoomViewToolBar : public QWidget {
    Q_OBJECT
public:
    explicit ZoomViewToolBar(QWidget* parent = nullptr);

signals:
    void zoomIn();
    void zoomOut();
    void dragModeChanged(bool drag);
    void measureModeChanged(bool measure);
    void resetViewRequested();

public slots:
    void setZoomLevel(double level);

private:
    QPushButton* m_zoomInBtn;
    QPushButton* m_zoomOutBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_dragBtn;
    QPushButton* m_measureBtn;
    QLabel* m_zoomLabel;
};

// 局部放大视图
class ZoomView : public QGraphicsView {
    Q_OBJECT

public:
    enum Mode {
        DragMode,      // 拖动模式
        MeasureMode    // 测距模式
    };

    explicit ZoomView(QWidget* parent = nullptr);
    ~ZoomView();

    // 设置场景（与主视图共享同一个场景）
    void setPPIScene(PPIScene* scene);

    // 显示指定区域（从主视图的rubberband传入）
    void showArea(const QRectF& sceneRect);

    // 获取当前缩放比例
    double zoomLevel() const;

signals:
    void zoomLevelChanged(double level);
    void distanceMeasured(double distance);

public slots:
    void zoomIn();
    void zoomOut();
    void setCustomDragMode(bool drag);
    void setMeasureMode(bool measure);
    void resetView();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void updateZoomLevel();
    void clearMeasureLine();

private:
    PPIScene* m_scene;
    Mode m_mode;
    double m_zoomFactor;

    // 测距相关（面向更具象的尺子表现）
    bool m_measuring;
    QPointF m_measureStart;
    QGraphicsPathItem* m_measurePath; // dashed ruler path
    QGraphicsEllipseItem* m_startMarker;
    QGraphicsEllipseItem* m_endMarker;
    QGraphicsTextItem* m_distanceText;
};

// 完整的局部放大窗口（包含工具栏和视图）
class ZoomViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZoomViewWidget(QWidget* parent = nullptr);
    ~ZoomViewWidget();

    // 设置场景
    void setPPIScene(PPIScene* scene);

    // 显示指定区域
    void showArea(const QRectF& sceneRect);

    ZoomView* view() const { return m_view; }

signals:
    void distanceMeasured(double distance);

private:
    void setupUI();
    void connectSignals();

private:
    ZoomView* m_view;
    ZoomViewToolBar* m_toolBar;
};

#endif // ZOOMVIEW_H
