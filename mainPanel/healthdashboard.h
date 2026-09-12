/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:56
 * @Description: 
 */
/**
 * @file healthdashboard.h
 * @brief 系统健康大屏
 * @details 以全屏仪表盘形式展示雷达系统各分系统运行状态（MonitorParam）
 *          和硬件BIT上报（BITReport），包括：
 *          - 各分系统软件运行状态（绿=正常/红=异常/橙=未知）
 *          - FPGA温度 / 阵面温度仪表盘
 *          - 伺服扫描角实时指针
 *          - 频率源/收发/北斗/伺服等硬件状态指示灯
 *          - 伺服控制回送（ServoCtrlRet）：当前转速/方位
 *
 * 数据来源：Controller::monitorParamSend / bitReport / servoCtrlRet
 */
#ifndef HEALTHDASHBOARD_H
#define HEALTHDASHBOARD_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include "Basic/Protocol.h"

// ─── 温度仪表盘控件 ──────────────────────────────────────────────────────────

/**
 * @class TempGauge
 * @brief 圆弧温度仪表盘（QPainter 绘制）
 */
class TempGauge : public QWidget
{
    Q_OBJECT
public:
    explicit TempGauge(const QString& label, float minT, float maxT,
                       QWidget* parent = nullptr);
    void setValue(float celsius);

protected:
    void paintEvent(QPaintEvent* e) override;
    QSize sizeHint() const override { return {120, 120}; }

private:
    QString m_label;
    float   m_min, m_max, m_value = 0.f;
};

// ─── StatusLed 状态指示灯 ────────────────────────────────────────────────────

/**
 * @class StatusLed
 * @brief 简单圆形状态灯（绿=OK / 红=错误 / 橙=警告 / 灰=未知）
 */
class StatusLed : public QWidget
{
    Q_OBJECT
public:
    enum State { Unknown, OK, Warning, Error };
    explicit StatusLed(const QString& label, QWidget* parent = nullptr);
    void setState(State s);
    void setText(const QString& text);

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return {140, 36}; }

private:
    QString m_label;
    QString m_extra; // 额外文字（如转速值）
    State   m_state = Unknown;
};

// ─── HealthDashboard ────────────────────────────────────────────────────────

/**
 * @class HealthDashboard
 * @brief 系统健康大屏
 *
 * 使用方式：
 * @code
 * m_healthDash = new HealthDashboard(this);
 * connect(CON_INS, &Controller::monitorParamSend,
 *         m_healthDash, &HealthDashboard::onMonitorParam);
 * connect(CON_INS, &Controller::bitReport,
 *         m_healthDash, &HealthDashboard::onBITReport);
 * connect(CON_INS, &Controller::servoCtrlRet,
 *         m_healthDash, &HealthDashboard::onServoCtrlRet);
 * @endcode
 */
class HealthDashboard : public QWidget
{
    Q_OBJECT
public:
    explicit HealthDashboard(QWidget* parent = nullptr);
    ~HealthDashboard() override = default;

public slots:
    void onMonitorParam(const MonitorParam& param);
    void onBITReport(const BITReport& report);
    void onServoCtrlRet(const ServoCtrlRet& ret);

private:
    void buildUi();
    void applyStyle();

    // 软件状态 LED
    StatusLed* m_ledDataPro    = nullptr;
    StatusLed* m_ledBeamCon    = nullptr;
    StatusLed* m_ledSigPro     = nullptr;
    StatusLed* m_ledTargetRec  = nullptr;

    // 硬件状态 LED（来自 BIT 的 bitGroup 字段）
    StatusLed* m_ledTxEnable   = nullptr;
    StatusLed* m_ledRxEnable   = nullptr;
    StatusLed* m_ledFreqSrc    = nullptr;
    StatusLed* m_ledDigiLink   = nullptr;
    StatusLed* m_ledServo      = nullptr;
    StatusLed* m_ledBeidou     = nullptr;
    StatusLed* m_ledWaveCtrl   = nullptr;

    // 温度仪表
    TempGauge* m_gaugeFpga     = nullptr;
    TempGauge* m_gaugePanel    = nullptr;

    // 伺服信息
    QLabel*    m_lblServoSpeed = nullptr;
    QLabel*    m_lblServoAz    = nullptr;
    QLabel*    m_lblScanAngle  = nullptr;
    QLabel*    m_lblLastUpdate = nullptr;

    // 数值摘要（FPGA温度等）
    QLabel*    m_lblFpgaTempNum  = nullptr;
    QLabel*    m_lblPanelTempNum = nullptr;
};

#endif // HEALTHDASHBOARD_H
