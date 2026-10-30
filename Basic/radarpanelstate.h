#ifndef RADARPANELSTATE_H
#define RADARPANELSTATE_H
#include "Basic/radarpositioncontrol.h"
#include "Basic/gpspositionfilter.h"
#pragma pack(push, 8)

// GUI-thread owned, fixed-size per-panel state. Dialog inputs are separate from live BIT.
class RadarPanelState {
public:
    using Pose = RadarReference::Pose;
    struct YawSample { quint64 frame = 0; double yaw = 0; int id = 0; double average = 0; };
    struct Snapshot {
        RadarPanelValues values{};
        std::array<bool, 4> locked{};
        std::array<qint64, 4> receivedMs{};
        std::array<YawSample, 200> history{};
        std::size_t historyCount = 0;
    };
    struct Update {
        GpsPositionFilter::Result position = GpsPositionFilter::Result::Invalid;
        bool attitudeValid = false;
        bool positionChanged = false;
        bool changed = false;
    };
    struct DebugSnapshot {
        std::size_t gpsCount = 0, attitudeCount = 0, curveCount = 0;
        std::array<GpsPositionFilter::Fix, GpsPositionFilter::SampleCount> gps{};
        std::array<std::array<double, 3>, GpsPositionFilter::SampleCount> attitude{}, curve{};
        GpsPositionFilter::Fix gpsMean;
        std::array<double, 3> attitudeMean{}, curveMean{};
    };
    DebugSnapshot debugSnapshot(int id) const {
        DebugSnapshot s;
        if (id < 0 || id >= 4) return s;
        s.gpsCount = m_filters[id].count(); s.gps = m_filters[id].samples(); s.gpsMean = m_filters[id].average();
        const auto copy = [](const auto& filter, auto& output) {
            for (std::size_t i = 0; i < filter.count; ++i)
                output[i] = filter.samples[(filter.next + filter.samples.size() - filter.count + i) % filter.samples.size()];
        };
        s.attitudeCount = m_attitudes[id].count; s.attitudeMean = m_attitudes[id].lastMean;
        copy(m_attitudes[id], s.attitude);
        s.curveCount = m_curve.filter.count; s.curveMean = m_curve.filter.lastMean;
        copy(m_curve.filter, s.curve);
        return s;
    }
    struct YawPlotData {
        std::array<double, 200> raw{}, average{};
        double lower = -5, upper = 5;
    };
    static YawPlotData yawPlotData(const Snapshot& s) {
        YawPlotData data;
        for (std::size_t i = 0; i < s.historyCount; ++i) {
            // Plot physical values verbatim: unwrapping can turn an actual 40 degrees into 400.
            data.raw[i] = s.history[i].yaw;
            data.average[i] = s.history[i].average;
            const double low = std::min(data.raw[i], data.average[i]);
            const double high = std::max(data.raw[i], data.average[i]);
            data.lower = i ? std::min(data.lower, low - 5) : low - 5;
            data.upper = i ? std::max(data.upper, high + 5) : high + 5;
        }
        return data;
    }
    void initialize(const Pose& first, double horizontalLimit, double altitudeLimit, bool gateReports = false) {
        m_gateReports = gateReports;
        for (int id = 0; id < 4; ++id) {
            Pose defaults = first;
            defaults[RadarReference::Yaw] = RadarReference::panelHeading(first[RadarReference::Yaw], id);
            m_references[id] = RadarReference(defaults);
            m_filters[id].setLimits(horizontalLimit, altitudeLimit);
            if (m_gateReports) m_references[id].freezeCurrent();
        }
    }
    // Accessors require a validated physical ID (0..3); mutating entrypoints check IDs.
    RadarReference& reference(int id) { return m_references[id]; }
    const RadarReference& reference(int id) const { return m_references[id]; }
    Update ingest(const BITReportV2& report, qint64 receivedMs = 0) {
        Update update;
        const int id = report.radarId;
        if (report.mesID != 0xDE02 || id >= 4) return update;
        m_receivedMs[id] = receivedMs;
        auto& reference = m_references[id];
        if (m_gateReports && reference.locked()) return update;
        GpsPositionFilter::Fix average;
        update.position = m_filters[id].push({report.latitudeE9 / 1e9, report.longitudeE9 / 1e9,
                                             report.altitudeCm * 0.01}, average);
        if (update.position == GpsPositionFilter::Result::Accepted) {
            update.positionChanged = reference.updateReport(RadarReference::Longitude, average.longitude);
            update.positionChanged = reference.updateReport(RadarReference::Latitude, average.latitude) || update.positionChanged;
            update.positionChanged = reference.updateReport(RadarReference::Altitude, average.altitude) || update.positionChanged;
            update.changed = update.positionChanged;
        }
        update.attitudeValid = update.position == GpsPositionFilter::Result::Accepted && report.headingMilliDeg <= 360000
            && report.pitchMilliDeg >= -90000 && report.pitchMilliDeg <= 90000
            && report.rollCentiDeg >= -9000 && report.rollCentiDeg <= 9000;
        if (update.attitudeValid) {
            auto& filter = m_attitudes[id];
            const auto pose = filter.push(report.headingMilliDeg * 0.001, report.pitchMilliDeg * 0.001, report.rollCentiDeg * 0.01);
            update.changed = reference.updateReport(RadarReference::Yaw, pose[0]) || update.changed;
            update.changed = reference.updateReport(RadarReference::Pitch, pose[1]) || update.changed;
            update.changed = reference.updateReport(RadarReference::Roll, pose[2]) || update.changed;
        }
        {
            auto& curve = m_curve;
            const double yaw = m_gateReports ? report.headingMilliDeg * 0.001 : report.yaw * 0.01;
            curve.history[curve.next] = {++curve.frame, yaw, id, curve.filter.push(yaw, 0, 0)[0]};
            curve.next = (curve.next + 1) % curve.history.size();
            curve.count = std::min(curve.count + 1, curve.history.size());
        }
        return update;
    }
    bool lock(int id, const Pose& pose) {
        return id >= 0 && id < 4 && m_references[id].lock(pose);
    }
    bool unlock(int id) {
        if (id < 0 || id >= 4 || !m_references[id].locked()) return false;
        if (!m_gateReports) return m_references[id].unlock();
        for (int other = 0; other < 4; ++other)
            if (other != id && !m_references[other].locked()) m_references[other].freezeCurrent();
        return m_references[id].resumeFromFrozen();
    }
    bool updateLegacyHeading(double heading) {
        if (m_gateReports) return false; // Multi-panel reference is exclusively updated via unlocked DE02.
        if (!RadarReference::valid(RadarReference::Yaw, heading)) return false;
        return m_references[0].updateReport(RadarReference::Yaw, m_legacyAttitude.push(heading, 0, 0)[0]);
    }
    RadarPanelValues dialogValues() const {
        RadarPanelValues result{};
        for (int id = 0; id < 4; ++id) result[id] = m_references[id].pose();
        return result;
    }
    void restoreLocked(const RadarPanelValues& values) {
        for (int id = 0; id < 4; ++id) {
            m_references[id] = RadarReference(values[id]);
            m_references[id].freezeCurrent();
        }
    }
    Snapshot snapshot() const {
        Snapshot s;
        s.values = dialogValues(); s.receivedMs = m_receivedMs;
        for (int id = 0; id < 4; ++id) s.locked[id] = m_references[id].locked();
        {
            const auto& curve = m_curve;
            s.historyCount = curve.count;
            for (std::size_t i = 0; i < curve.count; ++i)
                s.history[i] = curve.history[(curve.next + curve.history.size() - curve.count + i) % curve.history.size()];
        }
        return s;
    }
    bool buildCommand(RadarPanelValues values, RadarPositionControl& output) const {
        for (int id = 0; id < 4; ++id) if (!m_references[id].locked()) values[id] = m_references[id].pose();
        return buildRadarPositionControl(values, output);
    }
    static RadarPanelValues submittedValues(const RadarPositionControl& command) {
        RadarPanelValues values{};
        for (int id = 0; id < 4; ++id) {
            const auto& panel = command.panels[id];
            values[id] = {panel.longitudeE9 / 1e9, panel.latitudeE9 / 1e9, panel.altitudeCm * 0.01,
                          panel.headingMilliDeg * 0.001, panel.pitchMilliDeg * 0.001, panel.rollCentiDeg * 0.01};
        }
        return values;
    }
    void rememberSubmitted(const RadarPositionControl& command) {
        const auto values = submittedValues(command);
        for (int id = 0; id < 4; ++id)
            if (m_references[id].locked()) m_references[id].editLocked(values[id]);
    }
private:
    struct AttitudeFilter {
        std::array<std::array<double, 3>, GpsPositionFilter::SampleCount> samples{};
        std::size_t count = 0, next = 0;
        std::array<double, 3> lastMean{};
        std::array<double, 3> push(double yaw, double pitch, double roll) {
            samples[next] = {yaw, pitch, roll}; next = (next + 1) % samples.size();
            count = std::min(count + 1, samples.size());
            std::array<double, 3> mean{};
            double sinSum = 0, cosSum = 0;
            constexpr double rad = 3.14159265358979323846 / 180.0;
            for (std::size_t i = 0; i < count; ++i) {
                sinSum += std::sin(samples[i][0] * rad); cosSum += std::cos(samples[i][0] * rad);
                mean[1] += samples[i][1] / count; mean[2] += samples[i][2] / count;
            }
            // Opposite directions have no circular mean: retain the latest valid direction.
            mean[0] = std::hypot(sinSum, cosSum) < 1e-9 ? yaw : std::atan2(sinSum, cosSum) / rad;
            mean[0] = RadarReference::panelHeading(mean[0], 0);
            if (mean[0] >= 360.0) mean[0] = 0.0; // roundoff at the north crossing
            lastMean = mean;
            return mean;
        }
    };
    std::array<RadarReference, 4> m_references{RadarReference{}, RadarReference{}, RadarReference{}, RadarReference{}};
    std::array<GpsPositionFilter, 4> m_filters{};
    std::array<AttitudeFilter, 4> m_attitudes{};
    AttitudeFilter m_legacyAttitude;
    struct Curve {
        AttitudeFilter filter;
        std::array<YawSample, 200> history{};
        std::size_t count = 0, next = 0;
        quint64 frame = 0;
    };
    Curve m_curve; // Continuous selected-panel history and 100-sample mean across manual switches.
    bool m_gateReports = false;
    std::array<qint64, 4> m_receivedMs{};
};
#pragma pack(pop)
#endif
