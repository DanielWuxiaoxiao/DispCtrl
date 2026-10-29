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
#include "../Basic/radarreference.h"
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QWidget>
#include <QtMath>
#include <cmath>

namespace {

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
    m_multiPanelMode = defaultPanelCount > 1;
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        m_panels[panelId].enabled = panelId < defaultPanelCount;
    }

    LOG_INFO(QString("[ScanLayer] initial enabled panels: count=%1 (panel1..panel%1)")
                 .arg(defaultPanelCount));

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScanLayer::advanceSweep);
    if (m_axis) connect(m_axis, &PolarAxis::rangeChanged, this, [this] {
        update();
        emit markersChanged();
    });
}

QRectF ScanLayer::boundingRect() const
{
    const double r = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0.0;
    return QRectF(-r - 50, -r - 50, (r + 50) * 2, (r + 50) * 2);
}

void ScanLayer::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem*,
                      QWidget* widget)
{
    if (!m_axis) {
        return;
    }

    const double radius = m_axis->rangeToPixel(m_axis->maxRange());
    if (!std::isfinite(radius) || radius <= 0.0) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QPainterPath scanAreas = coveragePath();
    painter->fillPath(scanAreas, QColor(0, 255, 136, 30));
    painter->setPen(QPen(QColor(0, 255, 136, 120), 3.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(scanAreas);

    QPen boundaryPen(QColor(255, 70, 70), 2.0, Qt::DashLine);
    boundaryPen.setCosmetic(true);
    painter->setPen(boundaryPen);
    for (const auto& line : tasBoundaryLines()) if (!line.isNull()) painter->drawLine(line);

    // Main-view markers are above its fixed QWidget controls; other scene views draw here.
    if (!widget || !widget->property("scanMarkerOverlayActive").toBool()) {
        for (const auto& path : markerPaths()) paintMarker(painter, path);
    }

    painter->restore();
}

QPainterPath ScanLayer::coveragePath() const
{
    QPainterPath scanAreas;
    const double radius = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0;
    if (!std::isfinite(radius) || radius <= 0) return scanAreas;
    const double span = m_multiPanelMode ? 90.0 : sweepSpan();
    // Fixed physical IDs: disabled panels must not renumber the remaining normals.
    scanAreas.setFillRule(Qt::WindingFill);
    for (int panelId = 0; panelId < kPanelCount; ++panelId) {
        const PanelState& panel = m_panels[panelId];
        if (!panel.enabled) {
            continue;
        }

        // A manual installation yaw defines the panel normal (sector centre), not its edge.
        const double rangeStart = (m_multiPanelMode || m_manualReferenceYaw)
            ? normalizeAngle(RadarReference::panelHeading(m_referenceYawDeg, panelId) - span * 0.5)
            : normalizeAngle(m_fixedStart + panelInstallationHeading(panelId));
        scanAreas.addPath(sectorPath(radius, rangeStart, span));
    }
    return scanAreas;
}

std::array<QLineF, 2> ScanLayer::tasBoundaryLines() const
{
    std::array<QLineF, 2> lines{};
    if (!m_multiPanelMode || !m_hasTasRange) return lines;
    const double radius = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0;
    if (!std::isfinite(radius) || radius <= 0) return lines;
    lines[0] = QLineF(QPointF(), polarPoint(radius, m_tasStartDeg));
    lines[1] = QLineF(QPointF(), polarPoint(radius, m_tasEndDeg));
    return lines;
}

void ScanLayer::setMultiPanelMode(bool enabled)
{
    if (m_multiPanelMode == enabled) return;
    m_multiPanelMode = enabled;
    update();
}

std::array<QLineF, 4> ScanLayer::markerLines() const
{
    std::array<QLineF, 4> lines{};
    const double radius = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0.0;
    if (!std::isfinite(radius) || radius <= 0.0) return lines;
    const double innerRadius = qMax(0.0, radius - MarkerLength);
    for (std::size_t i = 0; i < m_panels.size(); ++i) {
        const auto& panel = m_panels[i];
        if (!panel.enabled || !panel.received) continue;
        const double angle = normalizeAngle(panel.scanAngleDeg);
        lines[i] = QLineF(polarPoint(innerRadius, angle), polarPoint(radius, angle));
    }
    return lines;
}

std::array<QPainterPath, 4> ScanLayer::markerPaths() const
{
    std::array<QPainterPath, 4> paths;
    const double radius = m_axis ? m_axis->rangeToPixel(m_axis->maxRange()) : 0.0;
    if (!std::isfinite(radius) || radius <= 0) return paths;
    const double centreRadius = qMax(radius / 2, radius - MarkerWidth / 2);
    const QRectF arcRect(-centreRadius, -centreRadius, 2 * centreRadius, 2 * centreRadius);
    for (std::size_t id = 0; id < m_panels.size(); ++id) {
        const auto& panel = m_panels[id];
        if (!panel.enabled || !panel.received) continue;
        const double angle = normalizeAngle(panel.scanAngleDeg);
        const double start = 90.0 - angle + MarkerAngleWidth / 2; // Qt arcs: east=0, CCW positive.
        auto& path = paths[id];
        path.arcMoveTo(arcRect, start);
        path.arcTo(arcRect, start, -MarkerAngleWidth);
    }
    return paths;
}

void ScanLayer::paintMarker(QPainter* painter, const QPainterPath& path)
{
    if (path.isEmpty()) return;
    QLinearGradient gradient(path.pointAtPercent(0), path.pointAtPercent(1));
    gradient.setColorAt(0, QColor(255, 0, 0, 0));
    gradient.setColorAt(0.25, QColor(255, 0, 0, 180));
    gradient.setColorAt(0.5, QColor(255, 0, 0, 255));
    gradient.setColorAt(0.75, QColor(255, 0, 0, 180));
    gradient.setColorAt(1, QColor(255, 0, 0, 0));
    QPen pen(QBrush(gradient), MarkerWidth, Qt::SolidLine, Qt::RoundCap);
    pen.setCosmetic(true); // Five logical display pixels, independent of view zoom.
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
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
    emit markersChanged();
}

void ScanLayer::setReferenceYaw(bool manual, double yawDeg)
{
    if (!std::isfinite(yawDeg)) return;
    const double yaw = normalizeAngle(yawDeg);
    if (m_manualReferenceYaw == manual && m_referenceYawDeg == yaw) return;
    m_manualReferenceYaw = manual;
    m_referenceYawDeg = yaw;
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
    emit markersChanged();
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
        emit markersChanged();
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
    if (!std::isfinite(startDeg) || !std::isfinite(endDeg)) return;
    m_tasStartDeg = startDeg; m_tasEndDeg = endDeg; m_hasTasRange = true;
    auto normalizeForRange = [](double deg) {
        while (deg < 0) deg += 360;
        while (deg > 360) deg -= 360;
        return deg;
    };

    const double normStart = normalizeForRange(startDeg);
    double normEnd = normalizeForRange(endDeg);
    double span = normEnd - normStart;
    if (span < 0) span += 360;

    LOG_INFO(QString("[ScanLayer] TAS display boundary updated: inputStart=%1 inputEnd=%2 "
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
