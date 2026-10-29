#ifndef GPSPOSITIONFILTER_H
#define GPSPOSITIONFILTER_H
#pragma pack(push, 8)
#include <array>
#include <algorithm>
#include <cmath>

class GpsPositionFilter {
public:
    static constexpr std::size_t SampleCount = 100;
    struct Fix { double latitude = 0, longitude = 0, altitude = 0; };
    enum class Result { Accepted, Invalid, AllZero, Deviation };
    void setLimits(double horizontalM, double altitudeM) {
        if (std::isfinite(horizontalM) && horizontalM > 0) m_horizontalM = horizontalM;
        if (std::isfinite(altitudeM) && altitudeM > 0) m_altitudeM = altitudeM;
    }
    Result push(Fix fix, Fix& average) {
        if (!std::isfinite(fix.latitude) || !std::isfinite(fix.longitude) || !std::isfinite(fix.altitude)
            || fix.latitude < -90 || fix.latitude > 90 || fix.longitude < -180 || fix.longitude > 180
            || fix.altitude < -10000 || fix.altitude > 100000) return Result::Invalid;
        if (fix.latitude == 0 && fix.longitude == 0 && fix.altitude == 0) return Result::AllZero;
        if (m_count) {
            const Fix centre = median();
            if (horizontalDistance(centre, fix) > m_horizontalM || std::abs(fix.altitude - centre.altitude) > m_altitudeM)
                return Result::Deviation;
        }
        m_fixes[m_next] = fix;
        m_next = (m_next + 1) % m_fixes.size();
        if (m_count < m_fixes.size()) ++m_count;
        average = {};
        const double anchor = m_fixes[0].longitude;
        for (std::size_t i = 0; i < m_count; ++i) {
            average.latitude += m_fixes[i].latitude / m_count;
            average.longitude += (anchor + longitudeDelta(m_fixes[i].longitude - anchor)) / m_count;
            average.altitude += m_fixes[i].altitude / m_count;
        }
        average.longitude = longitudeDelta(average.longitude);
        m_average = average;
        return Result::Accepted;
    }
    std::size_t count() const { return m_count; }
    Fix average() const { return m_average; }
    std::array<Fix, SampleCount> samples() const {
        std::array<Fix, SampleCount> result{};
        for (std::size_t i = 0; i < m_count; ++i)
            result[i] = m_fixes[(m_next + m_fixes.size() - m_count + i) % m_fixes.size()];
        return result;
    }
private:
    static double longitudeDelta(double degrees) {
        double d = std::fmod(degrees + 180.0, 360.0);
        if (d < 0) d += 360.0;
        return d - 180.0;
    }
    static double horizontalDistance(const Fix& a, const Fix& b) {
        constexpr double radians = 3.14159265358979323846 / 180.0;
        const double dLat = (b.latitude - a.latitude) * radians;
        const double dLon = longitudeDelta(b.longitude - a.longitude) * radians;
        const double h = std::pow(std::sin(dLat / 2), 2)
            + std::cos(a.latitude * radians) * std::cos(b.latitude * radians) * std::pow(std::sin(dLon / 2), 2);
        return 6378137.0 * 2 * std::asin(std::sqrt(std::clamp(h, 0.0, 1.0)));
    }
    Fix median() const {
        std::array<double, SampleCount> lat{}, lon{}, alt{};
        const double anchor = m_fixes[0].longitude;
        for (std::size_t i = 0; i < m_count; ++i) {
            lat[i] = m_fixes[i].latitude;
            lon[i] = anchor + longitudeDelta(m_fixes[i].longitude - anchor);
            alt[i] = m_fixes[i].altitude;
        }
        std::sort(lat.begin(), lat.begin() + m_count);
        std::sort(lon.begin(), lon.begin() + m_count);
        std::sort(alt.begin(), alt.begin() + m_count);
        return {lat[m_count / 2], longitudeDelta(lon[m_count / 2]), alt[m_count / 2]};
    }
    std::array<Fix, SampleCount> m_fixes{};
    std::size_t m_count = 0, m_next = 0;
    double m_horizontalM = 100, m_altitudeM = 50;
    Fix m_average{};
};
#pragma pack(pop)
#endif
