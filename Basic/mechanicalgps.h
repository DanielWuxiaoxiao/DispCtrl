#ifndef MECHANICALGPS_H
#define MECHANICALGPS_H

#include "Basic/Protocol.h"
#pragma pack(push, 8)
#include <array>
#include <optional>

class MechanicalGps {
public:
    struct Angles {
        std::optional<double> heading, encoder, deviation;
    };

    void ingest(const BITReportV2& report) {
        const bool headingValid = (report.bitGroup & 0x01) && report.headingMilliDeg <= 360000;
        const bool encoderValid = (report.bitGroup & 0x02) && report.servoEncoderMilliDeg <= 360000;
        m_latest.heading = headingValid ? std::optional<double>(report.headingMilliDeg / 1000.0) : std::nullopt;
        m_latest.encoder = encoderValid ? std::optional<double>(report.servoEncoderMilliDeg / 1000.0) : std::nullopt;
        m_latest.deviation.reset();
        m_deviationMilliDeg.reset();
        if (headingValid && encoderValid) {
            // Keep the signed difference in the requested convention, without wrapping at north.
            const qint32 delta = qint32(report.servoEncoderMilliDeg) - qint32(report.headingMilliDeg);
            m_deviationMilliDeg = delta;
            m_latest.deviation = delta / 1000.0;
        }
        m_samples[m_next] = m_latest.deviation;
        m_next = (m_next + 1) % m_samples.size();
        if (m_count < m_samples.size()) ++m_count;
    }

    const Angles& latest() const { return m_latest; }
    bool fillNorthDeviation(ServoControlParam& command, bool manual) const {
        if (!manual && !m_deviationMilliDeg) return false;
        command.northDeviationMilliDeg = manual ? 0 : *m_deviationMilliDeg;
        return true;
    }
    std::size_t count() const { return m_count; }
    std::optional<double> sample(std::size_t index) const {
        return m_samples[(m_next + m_samples.size() - m_count + index) % m_samples.size()];
    }

private:
    Angles m_latest;
    std::optional<qint32> m_deviationMilliDeg;
    std::array<std::optional<double>, 200> m_samples{};
    std::size_t m_next = 0;
    std::size_t m_count = 0;
};

#pragma pack(pop)
#endif
