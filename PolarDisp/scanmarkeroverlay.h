#ifndef SCANMARKEROVERLAY_H
#define SCANMARKEROVERLAY_H
// Keep this QWidget's ABI independent of Protocol.h's legacy global pack(1).
#pragma pack(push, 8)
#include <QWidget>
#include <QPointer>
#include <QRegion>
#include <QLineF>
#include <QPainterPath>
#include <QElapsedTimer>
#include <QTransform>
#include <array>
class QGraphicsView;
class ScanLayer;

// Widget-level overlay: scene z-values cannot place a scan arc over QWidget controls.
class ScanMarkerOverlay final : public QWidget {
public:
    explicit ScanMarkerOverlay(QGraphicsView* view);
    void setSource(ScanLayer* layer);
protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void requestRefresh();
    void refreshFrame();
    std::array<QPainterPath, 4> mappedPaths() const;
    QGraphicsView* m_view;
    QPointer<ScanLayer> m_layer;
    QMetaObject::Connection m_markerConnection, m_axisConnection, m_sceneConnection;
    QRegion m_previousRegion;
    bool m_pending = false;
    struct Fade {
        QPainterPath current, previous;
        qint64 startedMs = 0;
    };
    std::array<Fade, 4> m_fades{};
    QElapsedTimer m_clock;
    bool m_resetFades = true;
    QTransform m_lastTransform;
    double m_lastRadius = -1;
};
#pragma pack(pop)
#endif
