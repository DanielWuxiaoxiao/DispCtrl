#ifndef TRACK3DCOORDINATE_H
#define TRACK3DCOORDINATE_H

#include "../Basic/Protocol.h"

#include <QtMath>

#include <cmath>

struct Track3DCoordinate
{
    double eastM = 0.0;
    double northM = 0.0;
    double heightM = 0.0;
    double azimuthDeg = 0.0;
};

inline bool calculateTrack3DCoordinate(const PointInfo& info, Track3DCoordinate& coordinate)
{
    if (!std::isfinite(info.range)
        || !std::isfinite(info.azimuth)
        || !std::isfinite(info.elevation)
        || !std::isfinite(info.altitute)
        || info.range < 0.0f
        || info.elevation < -90.0f
        || info.elevation > 90.0f) {
        return false;
    }

    double azimuthDeg = std::fmod(static_cast<double>(info.azimuth), 360.0);
    if (azimuthDeg < 0.0) {
        azimuthDeg += 360.0;
    }

    const double elevationRad = qDegreesToRadians(static_cast<double>(info.elevation));
    const double azimuthRad = qDegreesToRadians(azimuthDeg);
    const double horizontalM = static_cast<double>(info.range) * std::cos(elevationRad);

    coordinate.eastM = horizontalM * std::sin(azimuthRad);
    coordinate.northM = horizontalM * std::cos(azimuthRad);
    coordinate.heightM = static_cast<double>(info.altitute);
    coordinate.azimuthDeg = azimuthDeg;
    return true;
}

#endif // TRACK3DCOORDINATE_H
