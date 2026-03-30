/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 11:49:01
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:10
 * @Description: 
 */
#include "colorbarwidget.h"
#include <QPainter>
#include <QLinearGradient>

ColorBarWidget::ColorBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("ColorBarWidget");
    setAttribute(Qt::WA_TranslucentBackground);
    m_colorLUT.fill(qRgba(0, 0, 0, 0));
}

void ColorBarWidget::setColorLUT(const std::array<QRgb, 256>& lut)
{
    m_colorLUT = lut;
    m_hasLUT = true;
    update();
}

void ColorBarWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int barWidth = qMin(28, w / 3);
    int barLeft = 6;
    int barTop = 24;
    int barBottom = h - 24;
    int barHeight = barBottom - barTop;

    if (barHeight < 10) return;

    // 背景
    p.fillRect(rect(), QColor(0, 0, 0, 180));

    // 标题
    p.setPen(QColor(200, 200, 200));
    QFont f("Consolas", 0, QFont::Bold);
    f.setPixelSize(13);
    p.setFont(f);
    p.drawText(QRect(0, 3, w, 20), Qt::AlignCenter, "dB");

    // 画色阶条 (从上到下: 255 → 16)
    for (int y = 0; y < barHeight; ++y) {
        // 映射 y→amplitude: top=255, bottom=16
        int amp = 255 - static_cast<int>((y * 239.0) / barHeight);
        amp = qBound(16, amp, 255);

        QRgb c = m_colorLUT[amp];
        p.setPen(QColor(qRed(c), qGreen(c), qBlue(c), 255));
        p.drawLine(barLeft, barTop + y, barLeft + barWidth, barTop + y);
    }

    // 边框
    p.setPen(QColor(120, 120, 120));
    p.drawRect(barLeft, barTop, barWidth, barHeight);

    // 刻度标注
    p.setPen(QColor(200, 200, 200));
    f.setPixelSize(12);
    f.setBold(false);
    p.setFont(f);

    int textX = barLeft + barWidth + 6;
    struct { double frac; QString label; } ticks[] = {
        {0.0,  "255"},
        {0.2,  "207"},
        {0.4,  "159"},
        {0.6,  "111"},
        {0.8,  "63"},
        {1.0,  "16"}
    };
    for (const auto& tk : ticks) {
        int y = barTop + static_cast<int>(tk.frac * barHeight);
        p.drawLine(barLeft + barWidth, y, barLeft + barWidth + 4, y);
        p.drawText(textX, y + 4, tk.label);
    }
}
