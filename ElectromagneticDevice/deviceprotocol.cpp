#include "deviceprotocol.h"
#include <QDataStream>
#include <QtEndian>
#include <cmath>
#include <limits>
#include <initializer_list>

namespace ElectromagneticProtocol {
namespace {
void configure(QDataStream& stream)
{
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
}
quint8 checksum(const QByteArray& data, int length)
{
    quint8 sum = 0;
    for (int i = 0; i < length; ++i) sum ^= static_cast<quint8>(data[i]);
    return sum;
}
bool finite(std::initializer_list<float> values)
{
    for (float value : values) if (!std::isfinite(value)) return false;
    return true;
}
}

QByteArray encode(quint16 source, quint16 destination, quint32 sequence,
                  quint16 message, const QByteArray& payload)
{
    if (payload.size() > MaxDatagramBytes - 21) return {};
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly);
    configure(out);
    out << quint32(0xFA55FA55) << source << destination << sequence
        << quint16(16 + payload.size()) << message;
    out.writeRawData(payload.constData(), payload.size());
    out << checksum(result, result.size()) << quint32(0x55FA55FA);
    return result;
}

bool decode(const QByteArray& packet, Frame& frame, QString& error)
{
    error.clear();
    if (packet.size() < 21 || packet.size() > MaxDatagramBytes) {
        error = QStringLiteral("报文长度无效"); return false;
    }
    const auto* bytes = reinterpret_cast<const uchar*>(packet.constData());
    const quint16 length = qFromLittleEndian<quint16>(bytes + 12);
    if (qFromLittleEndian<quint32>(bytes) != 0xFA55FA55
        || qFromLittleEndian<quint32>(bytes + packet.size() - 4) != 0x55FA55FA
        || length != packet.size() - 5
        || checksum(packet, length) != bytes[length]) {
        error = QStringLiteral("帧头/帧尾/长度/校验码错误"); return false;
    }
    frame.source = qFromLittleEndian<quint16>(bytes + 4);
    frame.destination = qFromLittleEndian<quint16>(bytes + 6);
    frame.sequence = qFromLittleEndian<quint32>(bytes + 8);
    frame.message = qFromLittleEndian<quint16>(bytes + 14);
    frame.payload = packet.mid(16, length - 16);
    return true;
}

bool validCommand(quint16 message, const QByteArray& payload)
{
    const int expected = message == 2 ? 2 : 1;
    if (payload.size() != expected) return false;
    if (message == 4) return quint8(payload[0]) >= 1 && quint8(payload[0]) <= 5;
    if (message != 1 && message != 2 && message != 3 && message != 8) return false;
    for (char value : payload) if (quint8(value) > 1) return false;
    return true;
}

bool coordinates(float range, float azimuth, float elevation,
                 const Wgs84Coordinate::Lla& origin, qint32& lon, qint32& lat, qint32& height)
{
    double east, north, up;
    Wgs84Coordinate::Lla target;
    if (!Wgs84Coordinate::polarToEnu(range, azimuth, elevation, east, north, up)
        || !Wgs84Coordinate::enuToLla(origin, east, north, up, target)) return false;
    const double values[] = {std::round(target.longitudeDeg * 1e7),
                             std::round(target.latitudeDeg * 1e7), std::round(target.altitudeM)};
    for (double value : values) {
        if (!std::isfinite(value) || value < std::numeric_limits<qint32>::min()
            || value > std::numeric_limits<qint32>::max()) return false;
    }
    lon = qint32(values[0]); lat = qint32(values[1]); height = qint32(values[2]);
    return true;
}

QByteArray detection(const detInfo& point, const Wgs84Coordinate::Lla& origin)
{
    qint32 lon, lat, height;
    if (!finite({point.vel, point.amp, point.CFARSNR, point.statSNR, point.aziBeam,
                 point.eleBeam, point.targetConfidence})
        || point.targetConfidence < 0 || point.targetConfidence > 1
        || !coordinates(point.dis, point.azi, point.ele, origin, lon, lat, height)) return {};
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly); configure(out);
    out << point.dis << point.vel << point.azi << point.ele << height << point.amp
        << point.CFARSNR << point.statSNR << point.aziBeam << point.eleBeam
        << quint32(point.disChannel) << quint32(point.dopChannel) << point.targetConfidence
        << quint32(point.targetRecResult == 1 ? 1 : 0) << lon << lat;
    return result;
}

QByteArray track(const trackInfo& point, const Wgs84Coordinate::Lla& origin)
{
    qint32 lon, lat, height;
    if (point.statMethod > 2 || !finite({point.amp, point.SNR, point.vel, point.spaceVel, point.accelerate})
        || !coordinates(point.dis, point.azi, point.ele, origin, lon, lat, height)) return {};
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly); configure(out);
    out << quint16(point.batch) << quint16(point.CPIID) << quint32(point.UTCtime)
        << quint32(point.nsecond) << quint8(point.statMethod) << point.amp << point.SNR
        << point.dis << point.azi << point.ele << height << point.vel << point.spaceVel
        << point.accelerate << quint32(point.targetRecResult == 1 ? 1 : 0) << lon << lat;
    return result;
}

QByteArray panel(const BITReport& report)
{
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly); configure(out);
    out << quint8(report.bitGroup) << quint8(report.powerState) << quint16(report.fpgaTemp)
        << quint16(report.panelTemp) << quint16(report.yaw);
    out.writeRawData(reinterpret_cast<const char*>(report.subArrayPower), 5);
    out << quint16(report.scanAngle);
    return result;
}

BITReport withConfirmedSubArrayPower(const BITReport& report,
                                    const std::array<bool, 36>& normal, bool confirmed)
{
    BITReport result = report;
    // Only the 36 meaningful bits change; preserve all other fields/reserved bits.
    for (int i = 0; i < 36; ++i) {
        const unsigned char mask = static_cast<unsigned char>(1u << (i % 8));
        result.subArrayPower[i / 8] &= static_cast<unsigned char>(~mask);
        if (confirmed && normal[i]) result.subArrayPower[i / 8] |= mask;
    }
    return result;
}

QByteArray software(const MonitorParam& report)
{
    if (report.dataProSta > 5 || report.beamConSta > 5
        || report.sigProSta > 5 || report.targetRecSta > 5) return {};
    // The UI has shutdown results 4/5; the partner retains its agreed 0..3 wire enum.
    auto wireStatus = [](quint8 value) { return char(value >= 4 ? 1 : value); };
    QByteArray result;
    result.append(wireStatus(report.dataProSta)); result.append(wireStatus(report.beamConSta));
    result.append(wireStatus(report.sigProSta)); result.append(wireStatus(report.targetRecSta));
    return result;
}
}
