/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:56
 * @Description: 
 */
/**
 * @file ascanwidget.cpp
 * @brief A显（距离–幅度）波形显示控件实现
 */
#include "ascanwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

// ─── AScanPlot ───────────────────────────────────────────────────────────────

AScanPlot::AScanPlot(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(160);
    setStyleSheet("background: #0d0d0d;");
}

void AScanPlot::setData(int /*pointNum*/, float rangeResM, float cfar,
                        const QVector<float>& pcAmps,
                        const QVector<float>& mtdAmps)
{
    m_rangeRes = rangeResM;
    m_cfar     = cfar;
    m_pcAmps   = pcAmps;
    m_mtdAmps  = mtdAmps;
    update();
}

void AScanPlot::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width();
    const int H = height();
    const int mL = 46, mR = 10, mT = 8, mB = 28;
    const int cW = W - mL - mR;
    const int cH = H - mT - mB;

    p.fillRect(rect(), QColor(0x0d, 0x0d, 0x0d));

    int N = m_pcAmps.isEmpty() ? m_mtdAmps.size() : m_pcAmps.size();
    if (N == 0) {
        p.setPen(QColor(0x44, 0x44, 0x44));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("等待A显数据..."));
        return;
    }

    float yRange = m_yHi - m_yLo;
    if (yRange <= 0) yRange = 1;

    auto toX = [&](int i)   { return mL + i * cW / qMax(1, N - 1); };
    auto toY = [&](float v) { return mT + cH - static_cast<int>((v - m_yLo) / yRange * cH); };

    // Y轴网格 & 刻度
    p.setFont(QFont("Consolas", 8));
    for (int tick = 0; tick <= 4; ++tick) {
        float v = m_yLo + tick * yRange / 4;
        int   y = toY(v);
        p.setPen(QPen(QColor(0x22, 0x22, 0x22)));
        p.drawLine(mL, y, W - mR, y);
        p.setPen(QColor(0x55, 0x55, 0x55));
        p.drawText(0, y - 7, mL - 2, 14, Qt::AlignRight,
                   QString::number(static_cast<int>(v)));
    }

    // X轴刻度（距离，km）
    p.setPen(QColor(0x55, 0x55, 0x55));
    for (int i = 0; i <= 5; ++i) {
        int   idx = i * (N - 1) / 5;
        int   x   = toX(idx);
        float km  = idx * m_rangeRes / 1000.f;
        p.drawLine(x, mT + cH, x, mT + cH + 4);
        p.drawText(x - 16, mT + cH + 6, 32, 18, Qt::AlignCenter,
                   QString::number(km, 'f', 1));
    }
    p.setPen(QColor(0x55, 0x55, 0x55));
    p.setFont(QFont("Consolas", 9));
    p.drawText(W - mR - 22, H - 4, QStringLiteral("km"));

    // CFAR 门限（橙色虚线）
    int yCfar = toY(m_cfar);
    p.setPen(QPen(QColor(0xff, 0x88, 0x00), 1.5, Qt::DashLine));
    p.drawLine(mL, yCfar, W - mR, yCfar);
    p.setPen(QColor(0xff, 0x88, 0x00));
    p.setFont(QFont("Consolas", 8));
    p.drawText(mL + 2, yCfar - 12, 60, 12, Qt::AlignLeft,
               QStringLiteral("CFAR %1dB").arg(static_cast<int>(m_cfar)));

    // PC后幅度（蓝色）
    if (m_showPC && !m_pcAmps.isEmpty()) {
        p.setPen(QPen(QColor(0x44, 0x88, 0xff), 1.5));
        for (int i = 1; i < N; ++i) {
            p.drawLine(toX(i-1), toY(m_pcAmps[i-1]),
                       toX(i),   toY(m_pcAmps[i]));
        }
    }

    // MTD后幅度（绿色）
    if (m_showMTD && !m_mtdAmps.isEmpty() && m_mtdAmps.size() >= N) {
        p.setPen(QPen(QColor(0x44, 0xff, 0x88), 1.5));
        for (int i = 1; i < N; ++i) {
            p.drawLine(toX(i-1), toY(m_mtdAmps[i-1]),
                       toX(i),   toY(m_mtdAmps[i]));
        }
    }

    // 图例
    int lx = mL + 4, ly = mT + 4;
    if (m_showPC) {
        p.fillRect(lx, ly + 4, 16, 2, QColor(0x44, 0x88, 0xff));
        p.setPen(QColor(0x44, 0x88, 0xff));
        p.setFont(QFont("Consolas", 9));
        p.drawText(lx + 20, ly, 60, 14, Qt::AlignLeft, QStringLiteral("PC后"));
        lx += 75;
    }
    if (m_showMTD && !m_mtdAmps.isEmpty()) {
        p.fillRect(lx, ly + 4, 16, 2, QColor(0x44, 0xff, 0x88));
        p.setPen(QColor(0x44, 0xff, 0x88));
        p.drawText(lx + 20, ly, 60, 14, Qt::AlignLeft, QStringLiteral("MTD后"));
    }
}

// ─── AScanWidget ─────────────────────────────────────────────────────────────

AScanWidget::AScanWidget(QWidget* parent)
    : QWidget(parent)
{
    // 工具栏
    m_lblInfo = new QLabel(QStringLiteral("A显  方位: --°  量程: --"), this);
    m_lblInfo->setStyleSheet("color:#ff8800; font-family:Consolas; font-size:12px;");

    m_chkPC  = new QCheckBox(QStringLiteral("PC后"), this);
    m_chkMTD = new QCheckBox(QStringLiteral("MTD后"), this);
    m_chkPC->setChecked(true);
    m_chkMTD->setChecked(true);

    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(4, 2, 4, 2);
    toolbar->addWidget(m_lblInfo);
    toolbar->addStretch();
    toolbar->addWidget(m_chkPC);
    toolbar->addWidget(m_chkMTD);

    // 波形区
    m_plot = new AScanPlot(this);

    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(4, 4, 4, 4);
    vlay->setSpacing(4);
    vlay->addLayout(toolbar);
    vlay->addWidget(m_plot, 1);
    setLayout(vlay);

    connect(m_chkPC,  &QCheckBox::toggled, m_plot, &AScanPlot::setShowPC);
    connect(m_chkMTD, &QCheckBox::toggled, m_plot, &AScanPlot::setShowMTD);

    applyStyle();
}

void AScanWidget::onAScanData(const AScanFrame& frame,
                              const QVector<float>& pcAmps,
                              const QVector<float>& mtdAmps)
{
    float aziDeg = frame.azimuth  * 0.01f;
    float eleDeg = frame.elevation* 0.01f;
    float maxKm  = frame.pointNum * frame.rangeResM / 1000.f;

    m_lblInfo->setText(
        QStringLiteral("A显  方位: %1°  俯仰: %2°  量程: %3 km  CFAR: %4 dB")
        .arg(aziDeg, 0, 'f', 1)
        .arg(eleDeg, 0, 'f', 1)
        .arg(maxKm,  0, 'f', 1)
        .arg(frame.cfar, 0, 'f', 1)
    );

    m_plot->setData(frame.pointNum, frame.rangeResM, frame.cfar, pcAmps, mtdAmps);
}

void AScanWidget::applyStyle()
{
    setStyleSheet(
        "AScanWidget { background: #0a0a0a; }"
        "QCheckBox { color:#aaa; font-size:12px; }"
        "QCheckBox::indicator:checked { background:#44ff88; border:1px solid #44ff88; }"
    );
}
