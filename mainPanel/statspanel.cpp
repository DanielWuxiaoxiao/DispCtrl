/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 11:35:14
 * @Description: 
 */
/**
 * @file statspanel.cpp
 * @brief 多目标统计面板实现
 */
#include "statspanel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <algorithm>

// ─── StatsChart ───────────────────────────────────────────────────────────────

StatsChart::StatsChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setStyleSheet("background: #0d0d0d;");
}

void StatsChart::setWindowSec(int seconds)
{
    m_windowSec = qMax(10, seconds);
}

void StatsChart::setMaxY(int maxY)
{
    m_maxY = maxY;
}

void StatsChart::pushSample(int dets, int tracks, int tbds)
{
    m_samples.append({dets, tracks, tbds});
    if (m_samples.size() > m_windowSec)
        m_samples.removeFirst();
    update();
}

void StatsChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width();
    const int H = height();
    const int marginL = 40, marginR = 8, marginT = 8, marginB = 24;
    const int chartW = W - marginL - marginR;
    const int chartH = H - marginT - marginB;

    p.fillRect(rect(), QColor(0x0d, 0x0d, 0x0d));

    if (m_samples.isEmpty()) {
        p.setPen(QColor(0x44, 0x44, 0x44));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("等待数据..."));
        return;
    }

    // 确定Y轴上限
    int yMax = m_maxY;
    if (yMax == 0) {
        for (const auto& s : m_samples)
            yMax = qMax(qMax(yMax, s.dets), qMax(s.tracks, s.tbds));
        yMax = qMax(yMax + 2, 10);
    }

    // 网格
    p.setPen(QPen(QColor(0x22, 0x22, 0x22)));
    for (int i = 0; i <= 4; ++i) {
        int y = marginT + chartH - i * chartH / 4;
        p.drawLine(marginL, y, W - marginR, y);
        p.setPen(QColor(0x44, 0x44, 0x44));
        p.setFont(QFont("Consolas", 9));
        p.drawText(0, y - 6, marginL - 2, 14, Qt::AlignRight, QString::number(i * yMax / 4));
        p.setPen(QPen(QColor(0x22, 0x22, 0x22)));
    }

    // X轴标签（每10s一格）
    p.setPen(QColor(0x55, 0x55, 0x55));
    p.setFont(QFont("Consolas", 8));
    p.drawText(marginL, H - marginB + 4, chartW, 20, Qt::AlignLeft,
               QStringLiteral("-%1s").arg(m_samples.size()));
    p.drawText(marginL, H - marginB + 4, chartW, 20, Qt::AlignRight,
               QStringLiteral("0s"));

    const int N = m_samples.size();
    auto toX = [&](int i) { return marginL + (i * chartW) / qMax(1, m_windowSec - 1); };
    auto toY = [&](int v) { return marginT + chartH - (v * chartH) / yMax; };

    // 三条折线
    struct LineSpec { QColor color; QString label; std::function<int(const Sample&)> val; };
    const QList<LineSpec> lines = {
        { QColor(0x44, 0x88, 0xff), QStringLiteral("检测"), [](const Sample& s){ return s.dets;   } },
        { QColor(0xff, 0xaa, 0x00), QStringLiteral("航迹"), [](const Sample& s){ return s.tracks; } },
        { QColor(0x44, 0xff, 0x88), QStringLiteral("TBD"),  [](const Sample& s){ return s.tbds;   } },
    };

    for (const auto& spec : lines) {
        p.setPen(QPen(spec.color, 1.5));
        for (int i = 1; i < N; ++i) {
            p.drawLine(toX(i - 1), toY(spec.val(m_samples[i - 1])),
                       toX(i),     toY(spec.val(m_samples[i])));
        }
    }

    // 图例
    p.setFont(QFont("Consolas", 9));
    int lx = marginL + 4;
    for (const auto& spec : lines) {
        p.setPen(spec.color);
        p.fillRect(lx, marginT + 2, 14, 2, spec.color);
        p.drawText(lx + 17, marginT + 12, spec.label);
        lx += 50;
    }
}

// ─── StatsPanel ───────────────────────────────────────────────────────────────

StatsPanel::StatsPanel(QWidget* parent)
    : QWidget(parent)
{
    // 折线图
    m_chart = new StatsChart(this);

    // 数字摘要区
    auto mkTitleLbl = [&](const QString& text, const QString& color) {
        auto* l = new QLabel(text, this);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(QStringLiteral("color:%1; font-size:11px; font-weight:bold;").arg(color));
        return l;
    };
    auto mkValLbl = [&](const QString& color) {
        auto* l = new QLabel(QStringLiteral("0"), this);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(QStringLiteral("color:%1; font-size:28px; font-family:Consolas; font-weight:bold;").arg(color));
        return l;
    };
    auto mkRateLbl = [&]() {
        auto* l = new QLabel(QStringLiteral("0/s"), this);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(QStringLiteral("color:#888; font-size:11px;"));
        return l;
    };

    m_lblDetVal    = mkValLbl("#4488ff");
    m_lblTrackVal  = mkValLbl("#ffaa00");
    m_lblTbdVal    = mkValLbl("#44ff88");
    m_lblDetRate   = mkRateLbl();
    m_lblTrackRate = mkRateLbl();
    m_lblTbdRate   = mkRateLbl();

    auto* summaryGrid = new QGridLayout;
    summaryGrid->setSpacing(2);
    // 行0: 标题
    summaryGrid->addWidget(mkTitleLbl(QStringLiteral("检测点"), "#4488ff"),  0, 0);
    summaryGrid->addWidget(mkTitleLbl(QStringLiteral("DBT航迹"), "#ffaa00"), 0, 1);
    summaryGrid->addWidget(mkTitleLbl(QStringLiteral("TBD航迹"), "#44ff88"), 0, 2);
    // 行1: 数值
    summaryGrid->addWidget(m_lblDetVal,   1, 0);
    summaryGrid->addWidget(m_lblTrackVal, 1, 1);
    summaryGrid->addWidget(m_lblTbdVal,   1, 2);
    // 行2: 速率
    summaryGrid->addWidget(m_lblDetRate,   2, 0);
    summaryGrid->addWidget(m_lblTrackRate, 2, 1);
    summaryGrid->addWidget(m_lblTbdRate,   2, 2);

    auto* summaryW = new QWidget(this);
    summaryW->setLayout(summaryGrid);
    summaryW->setFixedHeight(90);

    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(4, 4, 4, 4);
    vlay->setSpacing(4);
    vlay->addWidget(summaryW);
    vlay->addWidget(m_chart, 1);
    setLayout(vlay);

    applyStyle();

    // 1秒定时器推采样
    m_secTimer = new QTimer(this);
    m_secTimer->setInterval(1000);
    connect(m_secTimer, &QTimer::timeout, this, &StatsPanel::onSecondTick);
    m_secTimer->start();
}

void StatsPanel::onDetPoint(const PointInfo& /*info*/)
{
    ++m_detCount;
    ++m_totalDet;
    m_lblDetVal->setText(QString::number(m_totalDet));
}

void StatsPanel::onTrackPoint(const PointInfo& /*info*/)
{
    ++m_trackCount;
    ++m_totalTrack;
    m_lblTrackVal->setText(QString::number(m_totalTrack));
}

void StatsPanel::onTbdPoint(const PointInfo& /*info*/)
{
    ++m_tbdCount;
    ++m_totalTbd;
    m_lblTbdVal->setText(QString::number(m_totalTbd));
}

void StatsPanel::onSecondTick()
{
    // 推入当前秒的速率样本
    m_chart->pushSample(m_detCount, m_trackCount, m_tbdCount);

    // 更新速率标签
    m_lblDetRate->setText(  QString::number(m_detCount)   + QStringLiteral("/s"));
    m_lblTrackRate->setText(QString::number(m_trackCount) + QStringLiteral("/s"));
    m_lblTbdRate->setText(  QString::number(m_tbdCount)   + QStringLiteral("/s"));

    // 重置帧内计数
    m_detCount = m_trackCount = m_tbdCount = 0;
}

void StatsPanel::applyStyle()
{
    setStyleSheet(
        "StatsPanel { background: #0a0a0a; }"
        "QWidget { background: #0a0a0a; }"
        "QLabel  { background: transparent; }"
    );
}
