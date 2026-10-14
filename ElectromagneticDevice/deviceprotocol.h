#ifndef ELECTROMAGNETIC_DEVICE_PROTOCOL_H
#define ELECTROMAGNETIC_DEVICE_PROTOCOL_H

#include "Basic/Protocol.h"
#include "Basic/wgs84coordinate.h"
#include <QByteArray>
#include <QString>
#include <array>

namespace ElectromagneticProtocol {
constexpr int DetectionBytes = 64;
constexpr int TrackBytes = 61;
constexpr int MaxDatagramBytes = 1472;
struct Frame {
    quint16 source = 0;
    quint16 destination = 0;
    quint32 sequence = 0;
    quint16 message = 0;
    QByteArray payload;
};
QByteArray encode(quint16 source, quint16 destination, quint32 sequence,
                  quint16 message, const QByteArray& payload);
bool decode(const QByteArray& packet, Frame& frame, QString& error);
bool validCommand(quint16 message, const QByteArray& payload);
bool coordinates(float range, float azimuth, float elevation,
                 const Wgs84Coordinate::Lla& origin, qint32& lon, qint32& lat, qint32& height);
QByteArray detection(const detInfo& point, const Wgs84Coordinate::Lla& origin);
QByteArray track(const trackInfo& point, const Wgs84Coordinate::Lla& origin);
QByteArray panel(const BITReport& report);
BITReport withConfirmedSubArrayPower(const BITReport& report,
                                    const std::array<bool, 36>& normal, bool confirmed);
QByteArray software(const MonitorParam& report);
}
#endif
