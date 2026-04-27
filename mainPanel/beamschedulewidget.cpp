/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:21:35
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:33
 * @Description: 
 */
/**
 * @file beamschedulewidget.cpp
 * @brief 波束调度甘特图显示控件实现
 */
#include "beamschedulewidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>

// ============================================================================
// BeamScheduleChart
// ============================================================================

BeamScheduleChart::BeamScheduleChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(300, 100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("background:#0a0a0a;");
}

void BeamScheduleChart::setSchedule(const BeamScheduleReport& hdr,
                                    const QVector<BeamSlot>& slots)
{
    m_header  = hdr;
    m_slots   = slots;
    m_hasData = true;
    update();
}

QColor BeamScheduleChart::slotColor(quint8 taskType)
{
    switch (taskType) {
        case 0:  return QColor(0x50, 0x8c, 0xdc);   // 搜索 — 蓝
        case 1:  return QColor(0xff, 0x8c, 0x28);   // 跟踪 — 橙
        case 2:  return QColor(0x50, 0x50, 0x50);   // 空闲 — 深灰
        default: return QColor(0xa0, 0x50, 0xc8);   // 其他 — 紫
    }
}

QString BeamScheduleChart::taskLabel(quint8 taskType)
{
    switch (taskType) {
        case 0:  return QStringLiteral("搜索");
        case 1:  return QStringLiteral("跟踪");
        case 2:  return QStringLiteral("空闲");
        default: return QStringLiteral("其他");
    }
}

void BeamScheduleChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(0x0a, 0x0a, 0x0a));

    if (!m_hasData || m_header.frameTimeUs == 0) {
        p.setPen(QColor(0x78, 0x78, 0x78));
        p.drawText(rect(), Qt::AlignCenter,
                   QStringLiteral("等待波束调度数据\u2026"));
        return;
    }

    constexpr int kRows       = 3;
    constexpr int kLeftMargin = 52;
    constexpr int kRightMargin= 10;
    constexpr int kTopMargin  = 8;
    constexpr int kRowH       = 22;
    constexpr int kTickCount  = 5;

    const int W = width();

    // ---- 行标签 ----
    static const char* kRowLabels[kRows] = { "搜索", "跟踪", "空闲" };
    p.setFont(QFont("Microsoft YaHei", 8));

    for (int r = 0; r < kRows; ++r) {
        const int y = kTopMargin + r * kRowH;
        p.setPen(QColor(0xcc, 0xcc, 0xcc));
        p.drawText(0, y, kLeftMargin - 4, kRowH,
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::fromUtf8(kRowLabels[r]));
        // 水平分隔线
        p.setPen(QColor(0x30, 0x30, 0x30));
        p.drawLine(kLeftMargin, y + kRowH, W - kRightMargin, y + kRowH);
    }

    // ---- X 轴刻度 ----
    const double totalUs = static_cast<double>(m_header.frameTimeUs);
    const double pxPerUs = static_cast<double>(W - kLeftMargin - kRightMargin) / totalUs;
    const int chartH = kRowH * kRows;

    p.setFont(QFont("Consolas", 7));
    for (int i = 0; i <= kTickCount; ++i) {
        const int x = kLeftMargin +
                      static_cast<int>(i * (W - kLeftMargin - kRightMargin) / kTickCount);
        p.setPen(QColor(0x50, 0x50, 0x50));
        p.drawLine(x, kTopMargin, x, kTopMargin + chartH);

        p.setPen(QColor(0x90, 0x90, 0x90));
        const int usVal = static_cast<int>(i * totalUs / kTickCount);
        const QString label = QStringLiteral("%1µs").arg(usVal);
        p.drawText(x - 20, kTopMargin + chartH + 2, 40, 14,
                   Qt::AlignCenter, label);
    }

    // ---- 槽块 ----
    for (const auto& slot : m_slots) {
        const int row = qMin(static_cast<int>(slot.taskType), kRows - 1);
        const int y   = kTopMargin + row * kRowH + 2;
        const int x0  = kLeftMargin + static_cast<int>(slot.startUs * pxPerUs);
        const int x1  = kLeftMargin +
                        static_cast<int>((slot.startUs + slot.durationUs) * pxPerUs);
        const int slotW = qMax(2, x1 - x0);

        p.fillRect(x0, y, slotW, kRowH - 4, slotColor(slot.taskType));

        // 跟踪槽批号标注（仅在宽度足够时）
        if (slot.taskType == 1 && slotW > 20) {
            p.setPen(Qt::white);
            p.setFont(QFont("Consolas", 7));
            p.drawText(x0 + 2, y, slotW - 4, kRowH - 4,
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral("#%1").arg(slot.batchID));
        }
    }
}

// ============================================================================
// BeamScheduleWidget
// ============================================================================

BeamScheduleWidget::BeamScheduleWidget(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background:#0a0a0a; color:#cccccc;");

    // ---- 顶部状态栏 ----
    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(6, 4, 6, 2);
    topBar->setSpacing(12);

    auto* lblTitle = new QLabel(QStringLiteral("波束调度"));
    lblTitle->setStyleSheet(
        "color:#ff8800; font:bold 11px 'Microsoft YaHei';");
    topBar->addWidget(lblTitle);
    topBar->addStretch();

    const QString statStyle = "color:#cccccc; font:10px Consolas;";

    m_lblSeq = new QLabel(QStringLiteral("帧序: —"));
    m_lblSeq->setStyleSheet(statStyle);

    m_lblSlotNum = new QLabel(QStringLiteral("槽数: —"));
    m_lblSlotNum->setStyleSheet(statStyle);

    m_lblFrameUs = new QLabel(QStringLiteral("帧长: —"));
    m_lblFrameUs->setStyleSheet(statStyle);

    topBar->addWidget(m_lblSeq);
    topBar->addWidget(m_lblSlotNum);
    topBar->addWidget(m_lblFrameUs);

    // ---- 甘特图 ----
    m_chart = new BeamScheduleChart(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    layout->addLayout(topBar);
    layout->addWidget(m_chart, 1);
}

void BeamScheduleWidget::onBeamSchedule(const BeamScheduleReport& hdr,
                                        const QVector<BeamSlot>& slots)
{
    m_lblSeq->setText(QStringLiteral("帧序: %1").arg(hdr.frameSeq));
    m_lblSlotNum->setText(QStringLiteral("槽数: %1").arg(hdr.slotNum));
    m_lblFrameUs->setText(QStringLiteral("帧长: %1µs").arg(hdr.frameTimeUs));
    m_chart->setSchedule(hdr, slots);
}
