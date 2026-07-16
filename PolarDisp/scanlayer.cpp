/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-07-14 16:30:00
 * @Description:
 */
#include "scanlayer.h"
#include "polaraxis.h"
#include "../Basic/log.h"
#include <QColor>
#include <QConicalGradient>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QtMath>
#include <cmath>

namespace {

QColor panelColor(int panelId)
{
    static const std::array<QColor, 4> colors = {
        QColor(255, 107, 107),
        QColor(255, 209, 102),
        QColor(77, 171, 247),
        QColor(199, 125, 255)
    };
    return colors[panelId % static_cast<int>(colors.size())];
}

QPointF polarPoint(double radius, double azimuthDeg)
{
    const double rad = qDegreesToRadians(azimuthDeg);
    return QPointF(radius * qSin(rad), -radius * qCos(rad));
}

} // namespace

ScanLayer::ScanLayer(PolarAxis* axis, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_axis(axis),
      m_angle(0), m_fixedStart(0), m_fixedEnd(360),
      m_direction(1), m_mode(Loop)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScanLayer::advanceSweep);
}

QRectF ScanLayer::boundingRect() const
{
    const double r = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0.0;
    return QRectF(-r - 50, -r - 50, (r + 50) * 2, (r + 50) * 2);
}

void ScanLayer::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem*,
                      QWidget*)
{
    if (!m_axis) {
        return;
    }

    const double radius = m_axis->rangeToPixel(m_axis->maxRange());
    const double currentAngle = m_angle;
    const double span = sweepSpan();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Restore the original shared translucent scan-range sector.
    QPainterPath scanAreaPath;
    scanAreaPath.moveTo(0, 0);
    const double qtStartAngle = 90.0 - m_fixedStart;
    scanAreaPath.arcTo(-radius, -radius, 2.0 * radius, 2.0 * radius,
                       qtStartAngle, -span);
    scanAreaPath.closeSubpath();

    painter->fillPath(scanAreaPath, QColor(251, 159, 147, 30));

    // Keep the original scan afterglow inside the shared range. The colored
    // per-array lines below are the authoritative multi-array indicators.
    const double afterglowAngle = 60.0;
    const double qtCurrentAngle = 90.0 - currentAngle;
    QPainterPath afterglowPath;
    afterglowPath.moveTo(0, 0);
    QConicalGradient gradient(0, 0, 0);
    if (m_direction > 0) {
        afterglowPath.arcTo(-radius, -radius, 2.0 * radius, 2.0 * radius,
                            qtCurrentAngle, afterglowAngle);
        gradient.setAngle(qtCurrentAngle);
        gradient.setColorAt(0.0, QColor(0, 255, 0, 180));
        gradient.setColorAt(0.02, QColor(0, 255, 0, 150));
        gradient.setColorAt(0.05, QColor(0, 255, 0, 100));
        gradient.setColorAt(0.10, QColor(0, 255, 0, 60));
        gradient.setColorAt(0.15, QColor(0, 255, 0, 30));
        gradient.setColorAt(0.20, QColor(0, 255, 0, 10));
        gradient.setColorAt(0.25, QColor(0, 255, 0, 0));
    } else {
        afterglowPath.arcTo(-radius, -radius, 2.0 * radius, 2.0 * radius,
                            qtCurrentAngle - afterglowAngle, afterglowAngle);
        gradient.setAngle(qtCurrentAngle - afterglowAngle);
        gradient.setColorAt(0.0, QColor(0, 255, 0, 0));
        gradient.setColorAt(0.05, QColor(0, 255, 0, 10));
        gradient.setColorAt(0.10, QColor(0, 255, 0, 30));
        gradient.setColorAt(0.15, QColor(0, 255, 0, 60));
        gradient.setColorAt(0.18, QColor(0, 255, 0, 100));
        gradient.setColorAt(0.20, QColor(0, 255, 0, 150));
        gradient.setColorAt(0.22, QColor(0, 255, 0, 180));
    }
    afterglowPath.closeSubpath();
    painter->save();
    painter->setClipPath(scanAreaPath);
    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawPath(afterglowPath);
    painter->restore();

    // Draw one line per received array. yaw is the installation heading and
    // scanAngle is the panel-local live angle, both in degrees.
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        const PanelState& panel = m_panels[panelId];
        if (!panel.received) {
            continue;
        }

        const double globalScan = normalizeAngle(panel.yawDeg + panel.scanAngleDeg);
        const QColor color = panelColor(panelId);
        QPen scanPen(color, 4.0, Qt::SolidLine);
        painter->setPen(scanPen);
        painter->drawLine(QPointF(0, 0), polarPoint(radius, globalScan));

        QFont labelFont = painter->font();
        labelFont.setPointSizeF(qMax(8.0, labelFont.pointSizeF()));
        painter->setFont(labelFont);
        painter->setPen(color);
        painter->drawText(
            polarPoint(radius * 0.78, globalScan) + QPointF(5.0, -5.0),
            QStringLiteral("阵面%1 扫描%2° / 北%3°")
                .arg(panelId + 1)
                .arg(panel.scanAngleDeg, 0, 'f', 1)
                .arg(globalScan, 0, 'f', 1));
    }

    // Keep the original subtle range boundary; no per-array dashed sectors.
    painter->setPen(QPen(QColor(251, 159, 147, 100), 3.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(scanAreaPath);

    painter->restore();
}

void ScanLayer::advanceSweep()
{
    if (m_useRealTimeAngle) {
        return;
    }

    const bool crossZero = m_fixedStart > m_fixedEnd;
    m_angle += m_direction * 2;
    if (m_angle >= 360) m_angle -= 360;
    if (m_angle < 0) m_angle += 360;

    if (m_mode == Loop) {
        if (crossZero) {
            if (m_direction > 0 && m_angle > m_fixedEnd && m_angle < m_fixedStart) {
                m_angle = m_fixedStart;
            } else if (m_direction < 0 && m_angle < m_fixedStart && m_angle > m_fixedEnd) {
                m_angle = m_fixedEnd;
            }
        } else if ((m_direction > 0 && m_angle >= m_fixedEnd) ||
                   (m_direction < 0 && m_angle <= m_fixedStart)) {
            m_angle = m_direction > 0 ? m_fixedStart : m_fixedEnd;
        }
    } else {
        if (m_angle >= m_fixedEnd) {
            m_angle = m_fixedEnd;
            m_direction = -1;
        } else if (m_angle <= m_fixedStart) {
            m_angle = m_fixedStart;
            m_direction = 1;
        }
    }

    update();
}

void ScanLayer::setHeadingAngle(double deg)
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }

    m_useRealTimeAngle = true;
    m_angle = deg;
    m_panels[0].received = true;
    m_panels[0].yawDeg = 0.0;
    m_panels[0].scanAngleDeg = deg;
    update();
}

void ScanLayer::onBITReport(BITReport report)
{
    if (report.radarId < RADAR_ID_MIN || report.radarId > RADAR_ID_MAX) {
        return;
    }

    const int panelId = static_cast<int>(report.radarId);
    PanelState& panel = m_panels[panelId];
    panel.received = true;
    panel.yawDeg = normalizeAngle(report.yaw * 0.01);
    panel.scanAngleDeg = normalizeAngle(report.scanAngle * 0.01);

    // Keep the legacy single-scan fields synchronized for callers that still
    // use the old API; the renderer uses the per-panel state above.
    if (panelId == 0) {
        m_angle = panel.scanAngleDeg;
        m_useRealTimeAngle = true;
    }
    update();
}

void ScanLayer::setSweepSpeed(int msPerStep)
{
    if (msPerStep > 0) {
        m_timer->setInterval(msPerStep);
    }
}

void ScanLayer::setSweepRange(double startDeg, double endDeg)
{
    auto normalizeForRange = [](double deg) {
        while (deg < 0) deg += 360;
        while (deg > 360) deg -= 360;
        return deg;
    };

    const double normStart = normalizeForRange(startDeg);
    double normEnd = normalizeForRange(endDeg);
    double span = normEnd - normStart;
    if (span < 0) span += 360;

    LOG_INFO(QString("[ScanLayer] shared scan range updated: inputStart=%1 inputEnd=%2 "
                     "normalizedStart=%3 normalizedEnd=%4 span=%5")
                 .arg(startDeg, 0, 'f', 2)
                 .arg(endDeg, 0, 'f', 2)
                 .arg(normStart, 0, 'f', 2)
                 .arg(normEnd, 0, 'f', 2)
                 .arg(span, 0, 'f', 2));

    if ((qAbs(normStart) < 0.01 && qAbs(normEnd - 360.0) < 0.01) ||
        qAbs(span - 360.0) < 0.01) {
        m_fixedStart = 0;
        m_fixedEnd = 360;
        update();
        return;
    }

    if (qAbs(normEnd - 360.0) < 0.01) normEnd = 0;
    if (qAbs(normStart - 360.0) < 0.01) {
        m_fixedStart = 0;
    } else {
        m_fixedStart = normStart;
    }
    m_fixedEnd = normEnd;

    if (!isAngleInRange(m_angle)) {
        m_angle = m_fixedStart;
        if (m_mode == PingPong) m_direction = 1;
    }
    update();
}

bool ScanLayer::isAngleInRange(double angle) const
{
    angle = normalizeAngle(angle);
    if (m_fixedStart <= m_fixedEnd) {
        return angle >= m_fixedStart && angle <= m_fixedEnd;
    }
    return angle >= m_fixedStart || angle <= m_fixedEnd;
}

void ScanLayer::setScanMode(ScanMode mode)
{
    m_mode = mode;
    if (m_mode == Loop) {
        m_direction = 1;
    } else {
        m_direction = m_angle >= m_fixedEnd ? -1 : 1;
    }
    update();
}

double ScanLayer::normalizeAngle(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

double ScanLayer::sweepSpan() const
{
    if (qAbs(m_fixedStart) < 0.01 && qAbs(m_fixedEnd - 360.0) < 0.01) {
        return 360.0;
    }

    double span = m_fixedEnd - m_fixedStart;
    if (span < 0.0) span += 360.0;
    return span;
}

ScanLayer::~ScanLayer()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}
