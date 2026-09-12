/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:44
 * @Description: 
 */
/* 雷达局地东-北-天（ENU）与 WGS84 经纬高的共享换算。 */
#ifndef WGS84_COORDINATE_H
#define WGS84_COORDINATE_H

#include <cmath>

namespace Wgs84Coordinate {

struct Lla {
    double longitudeDeg = 0.0;
    double latitudeDeg = 0.0;
    double altitudeM = 0.0;
};

// 与既有 GCS 目标下发一致：局地切平面使用 WGS84 长半轴作为地球半径。
constexpr double kEarthRadiusM = 6378137.0;
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

inline bool isValid(const Lla& value)
{
    return std::isfinite(value.longitudeDeg) && std::isfinite(value.latitudeDeg)
        && std::isfinite(value.altitudeM) && value.longitudeDeg >= -180.0
        && value.longitudeDeg <= 180.0 && value.latitudeDeg >= -90.0
        && value.latitudeDeg <= 90.0;
}

// 输入方位按雷达业务定义：0° 正北、90° 正东；距离单位米，角度单位度。
inline bool polarToEnu(double rangeM, double azimuthDeg, double elevationDeg,
                       double& eastM, double& northM, double& upM)
{
    if (!std::isfinite(rangeM) || !std::isfinite(azimuthDeg) || !std::isfinite(elevationDeg)
        || rangeM < 0.0 || elevationDeg < -90.0 || elevationDeg > 90.0) {
        return false;
    }
    const double normalizedAzimuthDeg = std::fmod(azimuthDeg, 360.0);
    const double azimuthRad = (normalizedAzimuthDeg < 0.0
        ? normalizedAzimuthDeg + 360.0 : normalizedAzimuthDeg) * kDegToRad;
    const double elevationRad = elevationDeg * kDegToRad;
    const double horizontalM = rangeM * std::cos(elevationRad);
    eastM = horizontalM * std::sin(azimuthRad);
    northM = horizontalM * std::cos(azimuthRad);
    upM = rangeM * std::sin(elevationRad);
    return std::isfinite(eastM) && std::isfinite(northM) && std::isfinite(upM);
}

// 目标高按现有 GCS 定义使用 radar.altitudeM + upM，不引入地球曲率附加高度。
inline bool enuToLla(const Lla& radar, double eastM, double northM, double upM, Lla& target)
{
    if (!isValid(radar) || !std::isfinite(eastM) || !std::isfinite(northM) || !std::isfinite(upM)) {
        return false;
    }
    const double latitudeRad = radar.latitudeDeg * kDegToRad;
    const double cosineLatitude = std::cos(latitudeRad);
    if (std::abs(cosineLatitude) < 1e-12) {
        return false;
    }
    target.latitudeDeg = radar.latitudeDeg + northM / (kEarthRadiusM * kDegToRad);
    target.longitudeDeg = radar.longitudeDeg
        + eastM / (kEarthRadiusM * cosineLatitude * kDegToRad);
    target.altitudeM = radar.altitudeM + upM;
    return isValid(target);
}

inline bool llaToEnu(const Lla& radar, const Lla& target, double& eastM, double& northM, double& upM)
{
    if (!isValid(radar) || !isValid(target)) {
        return false;
    }
    const double cosineLatitude = std::cos(radar.latitudeDeg * kDegToRad);
    if (std::abs(cosineLatitude) < 1e-12) {
        return false;
    }
    eastM = (target.longitudeDeg - radar.longitudeDeg) * kDegToRad
        * kEarthRadiusM * cosineLatitude;
    northM = (target.latitudeDeg - radar.latitudeDeg) * kDegToRad * kEarthRadiusM;
    upM = target.altitudeM - radar.altitudeM;
    return std::isfinite(eastM) && std::isfinite(northM) && std::isfinite(upM);
}

}  // namespace Wgs84Coordinate

#endif  // WGS84_COORDINATE_H
