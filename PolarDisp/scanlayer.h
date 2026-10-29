/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:53
 * @Description: 
 */
#ifndef SCANLAYER_H
#define SCANLAYER_H

#include <QGraphicsItem>
#include <QTimer>
#include <QObject>
#include <array>
#include <QLineF>
#include <QPainterPath>
#include "Basic/Protocol.h"

class PolarAxis;
//object必须在前面
class ScanLayer : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    enum ScanMode { Loop, PingPong };

    ScanLayer(PolarAxis* axis, QGraphicsItem* parent=nullptr);
    ~ScanLayer();

    QRectF boundingRect() const override;
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem*,
               QWidget*) override;

    void setSweepSpeed(int msPerStep);
    void setSweepRange(double startDeg, double endDeg);
    void setScanMode(ScanMode mode);
    void setMultiPanelMode(bool enabled);
    QPainterPath coveragePath() const;
    std::array<QLineF, 2> tasBoundaryLines() const;
    static constexpr double MarkerLength = 5.0;
    static constexpr double MarkerWidth = 5.0;
    static constexpr double MarkerAngleWidth = 5.0; // Degrees, centred on the reported scan angle.
    std::array<QLineF, 4> markerLines() const;
    std::array<QPainterPath, 4> markerPaths() const;
    static void paintMarker(QPainter* painter, const QPainterPath& path);

signals:
    void markersChanged();

public slots:
    // 外部航向角更新（单位：度，极坐标方位）
    void setHeadingAngle(double deg);

    // 接收BIT上报信息，更新扫描角度（实时波束指向）
    void onBITReport(BITReport report);

    // 阵面开启控制决定哪些阵面的 TAS 扫描范围参与 P 显绘制。
    void setPanelEnableState(BatteryControlM param);
    void setReferenceYaw(bool manual, double yawDeg);

private slots:
    void advanceSweep();

private:
    struct PanelState {
        bool enabled = true;
        bool received = false;
        double yawDeg = 0.0;
        double scanAngleDeg = 0.0;
    };

    static constexpr int kPanelCount = 4;

    static double normalizeAngle(double deg);
    double sweepSpan() const;

    /**
     * @brief 判断角度是否在扫描范围内
     * @param angle 待检查的角度
     * @return true 如果在范围内
     */
    bool isAngleInRange(double angle) const;

    PolarAxis* m_axis;
    QTimer* m_timer;
    double m_angle;
    double m_fixedStart, m_fixedEnd;

    ScanMode m_mode = Loop;
    int m_direction = +1; // 1=顺时针，-1=逆时针  每次转动度数

    bool m_useRealTimeAngle = false; // 是否使用实时角度（来自BIT上报）

    // TAS supplies one panel-local range. Enabled panels rotate that same local
    // range by their fixed installation headings (0/90/180/270 degrees).
    // Manual platform yaw instead fixes sector centres at yaw + panelId*90.
    // BIT yaw is retained independently; scan-line coordinate semantics remain
    // an explicit renderer policy rather than being inferred here.
    std::array<PanelState, kPanelCount> m_panels{};
    bool m_manualReferenceYaw = false;
    double m_referenceYawDeg = 0.0;
    bool m_multiPanelMode = false;
    bool m_hasTasRange = false;
    double m_tasStartDeg = 0, m_tasEndDeg = 0;
};

#endif // SCANLAYER_H
