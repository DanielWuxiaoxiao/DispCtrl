#ifndef POINTINFOVALIDATION_H
#define POINTINFOVALIDATION_H

#include "Protocol.h"

#include <cmath>

// Validate before both public dispatch paths; RadarDataManager rejection alone
// does not prevent detInfoProcess subscribers from consuming a malformed point.
inline bool isValidDetectionPointInfo(const PointInfo& info)
{
    return std::isfinite(info.range)
        && std::isfinite(info.azimuth)
        && std::isfinite(info.elevation)
        && std::isfinite(info.altitute)
        && std::isfinite(info.SNR)
        && std::isfinite(info.speed)
        && std::isfinite(info.amp)
        && std::isfinite(info.targetConfidence)
        && info.range >= 0.0f
        && info.range <= 1000000.0f
        && info.azimuth >= 0.0f
        && info.azimuth < 360.0f
        && info.elevation >= -90.0f
        && info.elevation <= 90.0f;
}

#endif // POINTINFOVALIDATION_H
