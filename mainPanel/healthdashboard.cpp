/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:04:32
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:33
 * @Description: 
 */
/**
 * @file healthdashboard.cpp
 * @brief 系统健康大屏实现
 */
#include "healthdashboard.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDateTime>
#include <cmath>

// ─── TempGauge ───────────────────────────────────────────────────────────────

TempGauge::TempGauge(const QString& label, float minT, float maxT, QWidget* parent)
    : QWidget(parent), m_label(label), m_min(minT), m_max(maxT)
{
    setFixedSize(120, 120);
}

void TempGauge::setValue(float celsius)
{
    m_value = celsius;
    update();
}

void TempGauge::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF rect(10, 10, 100, 100);
    const int startAngle = 225;  // 7点方向
    const int spanTotal  = 270;  // 270°覆盖

    // 背景弧
    p.setPen(QPen(QColor(0x33, 0x33, 0x33), 8, Qt::SolidLine, Qt::FlatCap));
    p.drawArc(rect, startAngle * 16, -spanTotal * 16);

    // 值弧
    float ratio = (m_max > m_min) ? qBound(0.f, (m_value - m_min) / (m_max - m_min), 1.f) : 0.f;
    int   span  = static_cast<int>(ratio * spanTotal);

    // 颜色：冷→暖
    QColor arcColor;
    if (ratio < 0.6f)      arcColor = QColor(0x44, 0xff, 0x44);
    else if (ratio < 0.8f) arcColor = QColor(0xff, 0xaa, 0x00);
    else                   arcColor = QColor(0xff, 0x33, 0x33);

    p.setPen(QPen(arcColor, 8, Qt::SolidLine, Qt::FlatCap));
    p.drawArc(rect, startAngle * 16, -span * 16);

    // 数值
    p.setFont(QFont(QStringLiteral("Consolas"), 16, QFont::Bold));
    p.setPen(arcColor);
    p.drawText(rect, Qt::AlignCenter,
               QStringLiteral("%1°").arg(static_cast<int>(m_value)));

    // 标签
    p.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    p.setPen(QColor(0xaa, 0xaa, 0xaa));
    p.drawText(QRectF(0, 95, 120, 20), Qt::AlignCenter, m_label);

    // 范围
    p.setFont(QFont(QStringLiteral("Consolas"), 8));
    p.setPen(QColor(0x55, 0x55, 0x55));
    p.drawText(QRectF(0, 78, 18, 14), Qt::AlignCenter, QString::number(static_cast<int>(m_min)));
    p.drawText(QRectF(102, 78, 18, 14), Qt::AlignCenter, QString::number(static_cast<int>(m_max)));
}

// ─── StatusLed ───────────────────────────────────────────────────────────────

StatusLed::StatusLed(const QString& label, QWidget* parent)
    : QWidget(parent), m_label(label)
{
    setFixedHeight(36);
    setMinimumWidth(140);
}

void StatusLed::setState(State s)
{
    m_state = s;
    update();
}

void StatusLed::setText(const QString& text)
{
    m_extra = text;
    update();
}

void StatusLed::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0x11, 0x11, 0x11));

    // LED圆
    QColor ledColor;
    switch (m_state) {
    case OK:      ledColor = QColor(0x00, 0xdd, 0x44); break;
    case Warning: ledColor = QColor(0xff, 0x88, 0x00); break;
    case Error:   ledColor = QColor(0xff, 0x22, 0x22); break;
    default:      ledColor = QColor(0x44, 0x44, 0x44); break;
    }

    // 光晕
    if (m_state != Unknown) {
        QRadialGradient glow(14, 18, 10);
        glow.setColorAt(0.0, ledColor.lighter(160));
        glow.setColorAt(0.6, ledColor);
        glow.setColorAt(1.0, Qt::transparent);
        p.setBrush(glow);
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 9, 20, 20);
    }

    p.setBrush(ledColor);
    p.setPen(QPen(ledColor.darker(140), 1));
    p.drawEllipse(7, 12, 14, 14);

    // 标签
    p.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10));
    p.setPen(QColor(0xcc, 0xcc, 0xcc));
    p.drawText(28, 0, width() - 30, height(), Qt::AlignVCenter | Qt::AlignLeft, m_label);

    // 额外文字（右侧）
    if (!m_extra.isEmpty()) {
        p.setFont(QFont(QStringLiteral("Consolas"), 9));
        p.setPen(QColor(0x88, 0x88, 0x88));
        p.drawText(28, 0, width() - 32, height(), Qt::AlignVCenter | Qt::AlignRight, m_extra);
    }
}

// ─── HealthDashboard ────────────────────────────────────────────────────────

HealthDashboard::HealthDashboard(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    applyStyle();
}

void HealthDashboard::buildUi()
{
    // === 软件状态区 ===
    m_ledDataPro   = new StatusLed(QStringLiteral("数据处理"), this);
    m_ledBeamCon   = new StatusLed(QStringLiteral("波束控制"), this);
    m_ledSigPro    = new StatusLed(QStringLiteral("信号处理"), this);
    m_ledTargetRec = new StatusLed(QStringLiteral("目标识别"), this);

    auto* softGroup = new QVBoxLayout;
    auto mkSection = [](const QString& title) {
        auto* l = new QLabel(title);
        l->setStyleSheet("color:#ff8800; font-size:13px; font-weight:bold; "
                         "border-bottom: 1px solid #ff8800; padding-bottom:3px;");
        return l;
    };
    softGroup->addWidget(mkSection(QStringLiteral("▌ 软件状态")));
    softGroup->addWidget(m_ledDataPro);
    softGroup->addWidget(m_ledBeamCon);
    softGroup->addWidget(m_ledSigPro);
    softGroup->addWidget(m_ledTargetRec);
    softGroup->addStretch();

    // === 硬件状态区 ===
    m_ledTxEnable  = new StatusLed(QStringLiteral("发射通道"), this);
    m_ledRxEnable  = new StatusLed(QStringLiteral("接收通道"), this);
    m_ledFreqSrc   = new StatusLed(QStringLiteral("频率源"), this);
    m_ledDigiLink  = new StatusLed(QStringLiteral("数字收发板"), this);
    m_ledServo     = new StatusLed(QStringLiteral("伺服系统"), this);
    m_ledBeidou    = new StatusLed(QStringLiteral("北斗授时"), this);
    m_ledWaveCtrl  = new StatusLed(QStringLiteral("波控板电源"), this);

    auto* hwGroup = new QVBoxLayout;
    hwGroup->addWidget(mkSection(QStringLiteral("▌ 硬件BIT")));
    hwGroup->addWidget(m_ledTxEnable);
    hwGroup->addWidget(m_ledRxEnable);
    hwGroup->addWidget(m_ledFreqSrc);
    hwGroup->addWidget(m_ledDigiLink);
    hwGroup->addWidget(m_ledServo);
    hwGroup->addWidget(m_ledBeidou);
    hwGroup->addWidget(m_ledWaveCtrl);
    hwGroup->addStretch();

    // === 温度仪表区 ===
    m_gaugeFpga  = new TempGauge(QStringLiteral("FPGA温度"), 0, 100, this);
    m_gaugePanel = new TempGauge(QStringLiteral("阵面温度"), -20, 80, this);
    m_lblFpgaTempNum  = new QLabel(QStringLiteral("-- °C"), this);
    m_lblPanelTempNum = new QLabel(QStringLiteral("-- °C"), this);

    auto* tempGroup = new QVBoxLayout;
    tempGroup->addWidget(mkSection(QStringLiteral("▌ 温度监控")));
    auto* gaugeRow = new QHBoxLayout;
    auto* fpgaV = new QVBoxLayout;
    fpgaV->addWidget(m_gaugeFpga, 0, Qt::AlignCenter);
    fpgaV->addWidget(m_lblFpgaTempNum, 0, Qt::AlignCenter);
    auto* panelV = new QVBoxLayout;
    panelV->addWidget(m_gaugePanel, 0, Qt::AlignCenter);
    panelV->addWidget(m_lblPanelTempNum, 0, Qt::AlignCenter);
    gaugeRow->addLayout(fpgaV);
    gaugeRow->addLayout(panelV);
    tempGroup->addLayout(gaugeRow);
    tempGroup->addStretch();

    // === 伺服信息区 ===
    m_lblServoSpeed = new QLabel(QStringLiteral("转速: -- rpm"),   this);
    m_lblServoAz    = new QLabel(QStringLiteral("方位: -- °"),  this);
    m_lblScanAngle  = new QLabel(QStringLiteral("扫描角: -- °"),  this);
    m_lblLastUpdate = new QLabel(QStringLiteral("最后更新: --"),    this);

    auto mkDataLbl = [](QLabel* l) { l->setStyleSheet("color:#ccc; font-family:Consolas; font-size:14px;"); };
    mkDataLbl(m_lblServoSpeed);
    mkDataLbl(m_lblServoAz);
    mkDataLbl(m_lblScanAngle);
    m_lblLastUpdate->setStyleSheet("color:#555; font-size:11px;");

    auto* servoGroup = new QVBoxLayout;
    servoGroup->addWidget(mkSection(QStringLiteral("▌ 伺服状态")));
    servoGroup->addWidget(m_lblServoSpeed);
    servoGroup->addWidget(m_lblServoAz);
    servoGroup->addWidget(m_lblScanAngle);
    servoGroup->addStretch();
    servoGroup->addWidget(m_lblLastUpdate);

    // === 主布局（水平排列四列）===
    auto* mainRow = new QHBoxLayout(this);
    mainRow->setContentsMargins(12, 12, 12, 12);
    mainRow->setSpacing(16);

    auto wrapGroup = [](QLayout* lay) {
        auto* frame = new QFrame;
        frame->setObjectName("dashCard");
        frame->setLayout(lay);
        lay->setContentsMargins(10, 10, 10, 10);
        return frame;
    };

    mainRow->addWidget(wrapGroup(softGroup), 1);
    mainRow->addWidget(wrapGroup(hwGroup),   1);
    mainRow->addWidget(wrapGroup(tempGroup), 1);
    mainRow->addWidget(wrapGroup(servoGroup),1);
    setLayout(mainRow);
}

void HealthDashboard::applyStyle()
{
    setStyleSheet(
        "HealthDashboard { background: #0a0a0a; }"
        "QFrame#dashCard { background: #111; border: 1px solid #333; border-radius:6px; }"
        "QLabel { background: transparent; color: #ccc; }"
    );
    m_lblFpgaTempNum->setStyleSheet("color:#ff8800; font-family:Consolas; font-size:13px;");
    m_lblPanelTempNum->setStyleSheet("color:#ff8800; font-family:Consolas; font-size:13px;");
}

void HealthDashboard::onMonitorParam(const MonitorParam& p)
{
    auto mapSta = [](unsigned char sta) -> StatusLed::State {
        if (sta == 0 || sta == 2) return StatusLed::OK;
        if (sta == 1 || sta == 3) return StatusLed::Error;
        return StatusLed::Unknown;
    };
    m_ledDataPro->setState(  mapSta(p.dataProSta));
    m_ledBeamCon->setState(  mapSta(p.beamConSta));
    m_ledSigPro->setState(   mapSta(p.sigProSta));
    m_ledTargetRec->setState(mapSta(p.targetRecSta));

    m_lblLastUpdate->setText(
        QStringLiteral("最后更新: ") + QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void HealthDashboard::onBITReport(const BITReport& r)
{
    auto bit = [&](int b) { return (r.bitGroup >> b) & 0x01; };

    m_ledTxEnable->setState( bit(7) ? StatusLed::OK : StatusLed::Warning);
    m_ledRxEnable->setState( bit(4) ? StatusLed::OK : StatusLed::Warning);
    m_ledFreqSrc->setState(  bit(3) ? StatusLed::OK : StatusLed::Error);
    m_ledDigiLink->setState( bit(2) ? StatusLed::OK : StatusLed::Error);
    m_ledServo->setState(    bit(1) ? StatusLed::OK : StatusLed::Error);
    m_ledBeidou->setState(   bit(0) ? StatusLed::OK : StatusLed::Warning);
    m_ledWaveCtrl->setState( (r.powerState & 0x01) ? StatusLed::OK : StatusLed::Error);

    float fpgaC  = r.fpgaTemp  * 0.1f;
    float panelC = r.panelTemp * 0.1f;
    m_gaugeFpga->setValue(fpgaC);
    m_gaugePanel->setValue(panelC);
    m_lblFpgaTempNum->setText( QStringLiteral("FPGA: %1 °C").arg(fpgaC, 0, 'f', 1));
    m_lblPanelTempNum->setText(QStringLiteral("阵面: %1 °C").arg(panelC, 0, 'f', 1));

    float scanDeg = r.scanAngle * 0.01f;
    m_lblScanAngle->setText(QStringLiteral("扫描角: %1 °").arg(scanDeg, 0, 'f', 1));

    m_lblLastUpdate->setText(
        QStringLiteral("最后更新: ") + QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void HealthDashboard::onServoCtrlRet(const ServoCtrlRet& ret)
{
    m_lblServoSpeed->setText(
        QStringLiteral("转速: %1 秒/转").arg(ret.speed));
    float azDeg = ret.azCur * 0.01f;
    m_lblServoAz->setText(
        QStringLiteral("方位: %1 °").arg(azDeg, 0, 'f', 1));
    m_ledServo->setState(
        (ret.result == 1 || ret.result == 2) ? StatusLed::OK : StatusLed::Error);
}
