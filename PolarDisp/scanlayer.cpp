/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:53
 * @Description: 
 */
#include "scanlayer.h"
#include "polaraxis.h"
#include "../Basic/ConfigManager.h"
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

double panelInstallationHeading(int panelId)
{
    return static_cast<double>(panelId) * 90.0;
}

QPainterPath sectorPath(double radius, double startDeg, double spanDeg)
{
    QPainterPath path;
    path.moveTo(0, 0);
    path.arcTo(-radius, -radius, 2.0 * radius, 2.0 * radius,
               90.0 - startDeg, -spanDeg);
    path.closeSubpath();
    return path;
}

} // namespace

ScanLayer::ScanLayer(PolarAxis* axis, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_axis(axis),
      m_angle(0), m_fixedStart(0), m_fixedEnd(360),
      m_direction(1), m_mode(Loop)
{
    const int defaultPanelCount = CF_INS.defaultPanelCount();
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        m_panels[panelId].enabled = panelId < defaultPanelCount;
    }

    LOG_INFO(QString("[ScanLayer] initial enabled panels: count=%1 (panel1..panel%1)")
                 .arg(defaultPanelCount));

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
    const double span = sweepSpan();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    auto drawAfterglow = [this, painter, radius](const QPainterPath& scanAreaPath,
                                                  double currentAngle) {
        constexpr double afterglowAngle = 60.0;
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
    };

    // TAS is a panel-local command. Each enabled array receives the same
    // angular span, rotated by its fixed 0/90/180/270-degree installation
    // direction. Live BIT data is deliberately not used for the sector: its
    // yaw/scan angle only indicates the instantaneous scan line.
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        const PanelState& panel = m_panels[panelId];
        if (!panel.enabled) {
            continue;
        }

        const double rangeStart = normalizeAngle(m_fixedStart + panelInstallationHeading(panelId));
        const QPainterPath scanAreaPath = sectorPath(radius, rangeStart, span);
        const QColor color = panelColor(panelId);
        QColor sectorFill = color;
        sectorFill.setAlpha(30);
        QColor sectorBoundary = color;
        sectorBoundary.setAlpha(120);
        painter->fillPath(scanAreaPath, sectorFill);
        painter->setPen(QPen(sectorBoundary, 3.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(scanAreaPath);

        if (!panel.received) {
            continue;
        }

        const double globalScan = normalizeAngle(panel.scanAngleDeg);
        drawAfterglow(scanAreaPath, globalScan);

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

void ScanLayer::setPanelEnableState(BatteryControlM param)
{
    const std::array<bool, kPanelCount> enabled = {
        param.quadrant1 != 0,
        param.quadrant2 != 0,
        param.quadrant3 != 0,
        param.quadrant4 != 0
    };

    bool changed = false;
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        if (m_panels[panelId].enabled != enabled[panelId]) {
            m_panels[panelId].enabled = enabled[panelId];
            changed = true;
        }
    }

    LOG_INFO(QString("[ScanLayer] enabled scan sectors: panel1=%1 panel2=%2 panel3=%3 panel4=%4; "
                     "localRange=[%5,%6]")
                 .arg(enabled[0] ? "on" : "off")
                 .arg(enabled[1] ? "on" : "off")
                 .arg(enabled[2] ? "on" : "off")
                 .arg(enabled[3] ? "on" : "off")
                 .arg(m_fixedStart, 0, 'f', 2)
                 .arg(m_fixedEnd, 0, 'f', 2));

    if (changed) {
        update();
    }
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

    LOG_INFO(QString("[ScanLayer] TAS local scan range updated: inputStart=%1 inputEnd=%2 "
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
