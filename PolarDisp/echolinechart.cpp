/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-07 11:18:01
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-19 10:10:44
 * @Description: 
 */
/**
 * @file echolinechart.cpp
 * @brief SIMRAD风格回波幅值折线图控件实现
 */
#include "echolinechart.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

EchoLineChart::EchoLineChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_updateTimer.start();
}

void EchoLineChart::setRangeMeters(double meters)
{
    m_rangeMeters = meters;
    update();
}

void EchoLineChart::updateEchoLine(const MarineEchoLine& line)
{
    if (!isVisible())
        return;

    if (m_updateTimer.isValid() && m_updateTimer.elapsed() < m_minUpdateIntervalMs)
        return;
    m_updateTimer.restart();

    m_amplitudes  = line.amplitudes;
    m_azimuthDeg  = line.azimuthDeg;
    m_packetNum   = line.packetNum;
    update();
}

void EchoLineChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // ---- 背景 ----
    p.fillRect(rect(), QColor::fromRgba(CLR_BG));

    // ---- 绘图区域 ----
    const int plotL = m_marginLeft;
    const int plotR = w - m_marginRight;
    const int plotT = m_marginTop;
    const int plotB = h - m_marginBottom;
    const int plotW = plotR - plotL;
    const int plotH = plotB - plotT;

    if (plotW < 10 || plotH < 10) return;

    // ---- 网格 ----
    p.setPen(QPen(QColor::fromRgba(CLR_GRID), 1, Qt::DotLine));
    // 水平网格 (幅值 0, 50, 100, 150, 200, 255)
    const int yTicks[] = {0, 50, 100, 150, 200, 255};
    for (int v : yTicks) {
        int y = plotB - static_cast<int>(v / 255.0 * plotH);
        p.drawLine(plotL, y, plotR, y);
    }
    // 垂直网格 (距离方向5等分)
    for (int i = 1; i < 5; ++i) {
        int x = plotL + plotW * i / 5;
        p.drawLine(x, plotT, x, plotB);
    }

    // ---- 坐标轴 ----
    p.setPen(QPen(QColor::fromRgba(CLR_AXIS), 1));
    p.drawRect(plotL, plotT, plotW, plotH);

    // ---- 刻度文字 ----
    p.setPen(QColor::fromRgba(CLR_TEXT));
    QFont tickFont("Consolas", 9);
    p.setFont(tickFont);
    // Y轴刻度
    for (int v : yTicks) {
        int y = plotB - static_cast<int>(v / 255.0 * plotH);
        p.drawText(0, y - 8, m_marginLeft - 6, 16, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(v));
    }
    // X轴刻度 (距离)
    int n = m_amplitudes.isEmpty() ? 512 : m_amplitudes.size();
    for (int i = 0; i <= 5; ++i) {
        int x = plotL + plotW * i / 5;
        double dist = m_rangeMeters * i / 5.0;
        QString label;
        if (dist >= 1000.0)
            label = QString("%1km").arg(dist / 1000.0, 0, 'f', 1);
        else
            label = QString("%1m").arg(static_cast<int>(dist));
        p.drawText(x - 30, plotB + 4, 60, 20, Qt::AlignHCenter | Qt::AlignTop, label);
    }

    // ---- 数据折线 ----
    if (!m_amplitudes.isEmpty()) {
        int count = m_amplitudes.size();
        // 构建折线路径
        QPainterPath linePath;
        QPainterPath fillPath;
        double xScale = static_cast<double>(plotW) / (count - 1);

        auto ptAt = [&](int i) -> QPointF {
            double x = plotL + i * xScale;
            double y = plotB - (m_amplitudes[i] / 255.0) * plotH;
            return QPointF(x, y);
        };

        QPointF first = ptAt(0);
        linePath.moveTo(first);
        fillPath.moveTo(QPointF(first.x(), plotB));
        fillPath.lineTo(first);

        // 降采样绘制，避免过多点导致卡顿
        int step = qMax(1, count / plotW);
        for (int i = step; i < count; i += step) {
            // 在step区间内取峰值
            int maxVal = 0;
            int maxIdx = i;
            for (int j = i - step + 1; j <= i && j < count; ++j) {
                if (m_amplitudes[j] > maxVal) {
                    maxVal = m_amplitudes[j];
                    maxIdx = j;
                }
            }
            QPointF pt = ptAt(maxIdx);
            linePath.lineTo(pt);
            fillPath.lineTo(pt);
        }
        // 确保绘制到最后一个点
        QPointF last = ptAt(count - 1);
        linePath.lineTo(last);
        fillPath.lineTo(last);
        fillPath.lineTo(QPointF(last.x(), plotB));
        fillPath.closeSubpath();

        // 半透明填充
        QLinearGradient grad(0, plotT, 0, plotB);
        grad.setColorAt(0.0, QColor::fromRgba(CLR_FILL_TOP));
        grad.setColorAt(1.0, QColor::fromRgba(CLR_FILL_BOT));
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawPath(fillPath);

        // 折线
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor::fromRgba(CLR_LINE), 1.5));
        p.drawPath(linePath);
    }

    // ---- 标题 ----
    p.setPen(QColor::fromRgba(CLR_TITLE));
    QFont titleFont("Microsoft YaHei", 11, QFont::Bold);
    p.setFont(titleFont);
    QString title = QString::fromUtf8("A\u663e  \u65b9\u4f4d %1\u00B0  #%2")
                        .arg(m_azimuthDeg, 0, 'f', 1)
                        .arg(m_packetNum);
    p.drawText(plotL, 2, plotW, m_marginTop - 2, Qt::AlignLeft | Qt::AlignVCenter, title);

    // ---- SIMRAD橙色边框 ----
    p.setPen(QPen(QColor::fromRgba(CLR_BORDER), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(1, 1, w - 2, h - 2);
}
