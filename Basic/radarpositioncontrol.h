#ifndef RADARPOSITIONCONTROL_H
#define RADARPOSITIONCONTROL_H
#include "Basic/Protocol.h"
#include "Basic/radarreference.h"
#pragma pack(push, 8)
using RadarPanelValues = std::array<RadarReference::Pose, 4>;

inline bool buildRadarPositionControl(const RadarPanelValues& values, RadarPositionControl& output)
{
    RadarPositionControl result;
    for (int id = 0; id < 4; ++id) {
        const auto& v = values[id];
        for (int f = 0; f < RadarReference::Count; ++f)
            if (!RadarReference::validUi(static_cast<RadarReference::Field>(f), v[f])) return false;
        auto& panel = result.panels[id];
        panel.longitudeE9 = static_cast<qint64>(std::llround(v[RadarReference::Longitude] * 1e9));
        panel.latitudeE9 = static_cast<qint64>(std::llround(v[RadarReference::Latitude] * 1e9));
        panel.altitudeCm = static_cast<qint32>(std::llround(v[RadarReference::Altitude] * 100));
        panel.headingMilliDeg = static_cast<quint32>(std::llround(v[RadarReference::Yaw] * 1000)) % 360000;
        panel.pitchMilliDeg = static_cast<qint32>(std::llround(v[RadarReference::Pitch] * 1000));
        panel.rollCentiDeg = static_cast<qint16>(std::llround(v[RadarReference::Roll] * 100));
    }
    output = result;
    return true;
}

inline bool buildRadarPositionControl(const RadarReference& reference, RadarPositionControl& output)
{
    for (int i = 0; i < RadarReference::Count; ++i)
        if (!RadarReference::valid(static_cast<RadarReference::Field>(i), reference.value(static_cast<RadarReference::Field>(i)))) return false;
    RadarPositionControl result;
    for (int id = 0; id < 4; ++id) {
        auto& panel = result.panels[id];
        panel.latitudeE9 = static_cast<qint64>(std::llround(reference.value(RadarReference::Latitude) * 1e9));
        panel.longitudeE9 = static_cast<qint64>(std::llround(reference.value(RadarReference::Longitude) * 1e9));
        panel.altitudeCm = static_cast<qint32>(std::llround(reference.value(RadarReference::Altitude) * 100));
        panel.headingMilliDeg = static_cast<quint32>(std::llround(RadarReference::panelHeading(reference.value(RadarReference::Yaw), id) * 1000)) % 360000;
        panel.pitchMilliDeg = static_cast<qint32>(std::llround(reference.value(RadarReference::Pitch) * 1000));
        panel.rollCentiDeg = static_cast<qint16>(std::llround(reference.value(RadarReference::Roll) * 100));
    }
    output = result;
    return true;
}
#pragma pack(pop)
#endif
