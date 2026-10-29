#ifndef RADARREFERENCE_H
#define RADARREFERENCE_H

// Protocol.h uses legacy global pack(1); never let it change STL/state layouts.
#pragma pack(push, 8)
#include <array>
#include <cmath>
#include <optional>

// A panel lock freezes all six effective values; reports continue updating a bounded shadow cache.
// Otherwise LLA uses the latest valid UI/report event; manual attitude stays authoritative.
class RadarReference {
public:
    enum Field { Longitude, Latitude, Altitude, Yaw, Pitch, Roll, Count };
    enum class Source { Boot, Report, Manual, Locked };
    using Pose = std::array<double, Count>;
    using Values = std::array<std::optional<double>, Count>;

    explicit RadarReference(std::array<double, Count> defaults = {}) : m_defaults(defaults) {
        for (int i = 0; i < Count; ++i) {
            if (!valid(static_cast<Field>(i), m_defaults[i])) m_defaults[i] = 0.0;
        }
    }
    static bool valid(Field field, double value) {
        if (!std::isfinite(value)) return false;
        switch (field) {
        case Longitude: return value >= -180 && value <= 180;
        case Latitude: return value >= -90 && value <= 90;
        case Altitude: return value >= -10000 && value <= 100000;
        case Yaw: return value >= 0 && value <= 360;
        case Pitch: case Roll: return value >= -90 && value <= 90;
        default: return false;
        }
    }
    Source source(Field field) const {
        if (m_locked) return Source::Locked;
        if (field >= Yaw && m_manual[field]) return Source::Manual;
        return m_sources[field];
    }
    static bool validUi(Field field, double value) {
        return valid(field, value) && (field != Altitude || (value >= -1000 && value <= 5000));
    }
    double value(Field field) const {
        if (m_locked) return (*m_locked)[field];
        const auto selected = source(field);
        if (selected == Source::Manual) return m_manual[field].value_or(m_defaults[field]);
        if (selected == Source::Report) return m_report[field].value_or(m_defaults[field]);
        return m_defaults[field];
    }
    const Values& manual() const { return m_manual; }
    bool setManual(const Values& values) {
        if (m_locked) return false;
        for (int i = 0; i < Count; ++i) {
            if (values[i] && !valid(static_cast<Field>(i), *values[i])) return false;
        }
        m_manual = values;
        for (int i = 0; i < Count; ++i) {
            if (values[i]) m_sources[i] = Source::Manual;
            else if (m_sources[i] == Source::Manual) m_sources[i] = m_report[i] ? Source::Report : Source::Boot;
        }
        return true;
    }
    bool updateReport(Field field, double reported) {
        if (!valid(field, reported)) return false;
        const double oldValue = value(field);
        const Source oldSource = source(field);
        m_report[field] = reported;
        m_sources[field] = Source::Report;
        return oldValue != value(field) || oldSource != source(field);
    }
    Pose pose() const {
        Pose result{};
        for (int i = 0; i < Count; ++i) result[i] = value(static_cast<Field>(i));
        return result;
    }
    bool locked() const { return m_locked.has_value(); }
    bool lock(const Pose& values) {
        for (int i = 0; i < Count; ++i) if (!validUi(static_cast<Field>(i), values[i])) return false;
        if (m_locked) return false;
        m_locked = values;
        return true;
    }
    bool unlock() {
        if (!m_locked) return false;
        // Explicit unlock means resume accepted reports, including attitude, not old manual overrides.
        m_locked.reset();
        m_manual = {};
        for (int i = 0; i < Count; ++i) m_sources[i] = m_report[i] ? Source::Report : Source::Boot;
        return true;
    }
    bool editLocked(const Pose& values) {
        if (!m_locked) return false;
        for (int i = 0; i < Count; ++i) if (!validUi(static_cast<Field>(i), values[i])) return false;
        m_locked = values;
        return true;
    }
    void freezeCurrent() { m_locked = pose(); }
    bool resumeFromFrozen() {
        if (!m_locked) return false;
        for (int i = 0; i < Count; ++i) { m_report[i] = (*m_locked)[i]; m_sources[i] = Source::Report; }
        m_manual = {};
        m_locked.reset();
        return true;
    }
    // Panel IDs are physical installation order; disabling one must not renumber the others.
    static double panelHeading(double firstYaw, int panelId) {
        double result = std::fmod(firstYaw + panelId * 90.0, 360.0);
        return result < 0 ? result + 360.0 : result;
    }
private:
    std::array<double, Count> m_defaults;
    Values m_report{};
    Values m_manual{};
    std::optional<Pose> m_locked;
    std::array<Source, Count> m_sources{};
};

#pragma pack(pop)
#endif
