/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:32
 * @Description: 
 */
#ifndef SCANLAYER_H
#define SCANLAYER_H

#include <QGraphicsItem>
#include <QTimer>
#include <QObject>
#include <array>
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

public slots:
    // 外部航向角更新（单位：度，极坐标方位）
    void setHeadingAngle(double deg);

    // 接收BIT上报信息，更新扫描角度（实时波束指向）
    void onBITReport(BITReport report);

    // 阵面开启控制决定哪些阵面的 TAS 扫描范围参与 P 显绘制。
    void setPanelEnableState(BatteryControlM param);

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
    // BIT yaw + scan angle remains the authoritative live scan-line position.
    std::array<PanelState, kPanelCount> m_panels{};
};

#endif // SCANLAYER_H
