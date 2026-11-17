#ifndef RADARPOSITIONCONTROL_H
#define RADARPOSITIONCONTROL_H
#include "Basic/Protocol.h"
#include "Basic/radarreference.h"
#pragma pack(push, 8)
using RadarPanelValues = std::array<RadarReference::Pose, 4>;

inline void encodeRadarPanelPose(const RadarReference::Pose& pose, RadarPanelPosition& panel) {
    panel.longitudeE9 = static_cast<qint64>(std::llround(pose[RadarReference::Longitude] * 1e9));
    panel.latitudeE9 = static_cast<qint64>(std::llround(pose[RadarReference::Latitude] * 1e9));
    panel.altitudeCm = static_cast<qint32>(std::llround(pose[RadarReference::Altitude] * 100));
    panel.headingMilliDeg = static_cast<quint32>(std::llround(pose[RadarReference::Yaw] * 1000)) % 360000;
    panel.pitchMilliDeg = static_cast<qint32>(std::llround(pose[RadarReference::Pitch] * 1000));
    panel.rollCentiDeg = static_cast<qint16>(std::llround(pose[RadarReference::Roll] * 100));
}

inline bool buildMechanicalRadarPositionControl(const RadarReference::Pose& pose, RadarPositionControl& output) {
    for (int field = 0; field < RadarReference::Count; ++field)
        if (!RadarReference::valid(static_cast<RadarReference::Field>(field), pose[field])) return false;
    RadarPositionControl command;
    encodeRadarPanelPose(pose, command.panels[0]);
    output = command;
    return true;
}

inline bool buildRadarPositionControl(const RadarPanelValues& values, RadarPositionControl& output)
{
    RadarPositionControl result;
    for (int id = 0; id < 4; ++id) {
        const auto& v = values[id];
        for (int f = 0; f < RadarReference::Count; ++f)
            if (!RadarReference::validUi(static_cast<RadarReference::Field>(f), v[f])) return false;
        encodeRadarPanelPose(v, result.panels[id]);
    }
    output = result;
    return true;
}

inline bool buildRadarPositionControl(const RadarReference& reference, RadarPositionControl& output)
{
    for (int i = 0; i < RadarReference::Count; ++i)
        if (!RadarReference::valid(static_cast<RadarReference::Field>(i), reference.value(static_cast<RadarReference::Field>(i)))) return false;
    RadarPositionControl result;
    auto pose = reference.pose();
    const double firstYaw = pose[RadarReference::Yaw];
    for (int id = 0; id < 4; ++id) {
        pose[RadarReference::Yaw] = RadarReference::panelHeading(firstYaw, id);
        encodeRadarPanelPose(pose, result.panels[id]);
    }
    output = result;
    return true;
}
#pragma pack(pop)
#endif
