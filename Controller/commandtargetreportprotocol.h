/*
 * Placeholder command-and-control target report wire format.
 *
 * This is intentionally self-contained.  When the peer publishes its formal
 * interface, replace only the serializers below; target filtering, coordinate
 * conversion and TCP/UDP transport remain unchanged.
 */
#ifndef COMMANDTARGETREPORTPROTOCOL_H
#define COMMANDTARGETREPORTPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

namespace CommandTargetReportProtocol {

constexpr quint32 kMagic = 0x31564155; // Serialized little-endian bytes: 55 41 56 31 ("UAV1").
constexpr quint16 kVersion = 1;
constexpr quint16 kBinaryPacketSize = 52;

#pragma pack(push, 1)
struct BinaryTargetReport {
    quint32 magic;
    quint16 version;
    quint16 packetSize;
    quint64 reportTimestampUtcMs;
    quint64 trackTimestampUtcMs;
    quint32 targetId;
    double longitudeDeg;
    double latitudeDeg;
    double altitudeM;
};
#pragma pack(pop)

static_assert(sizeof(BinaryTargetReport) == kBinaryPacketSize,
              "Command target binary placeholder must remain 52 bytes");

inline QByteArray serializeBinary(const BinaryTargetReport& report, bool bigEndian)
{
    QByteArray bytes;
    bytes.reserve(kBinaryPacketSize);
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_14);
    stream.setByteOrder(bigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream << report.magic << report.version << report.packetSize
           << report.reportTimestampUtcMs << report.trackTimestampUtcMs << report.targetId
           << report.longitudeDeg << report.latitudeDeg << report.altitudeM;
    return stream.status() == QDataStream::Ok && bytes.size() == kBinaryPacketSize
        ? bytes : QByteArray();
}

inline QByteArray serializeJson(const BinaryTargetReport& report, bool tcpFraming)
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), report.version);
    object.insert(QStringLiteral("report_timestamp_utc_ms"), static_cast<double>(report.reportTimestampUtcMs));
    object.insert(QStringLiteral("track_timestamp_utc_ms"), static_cast<double>(report.trackTimestampUtcMs));
    object.insert(QStringLiteral("target_id"), report.targetId);
    object.insert(QStringLiteral("longitude_deg"), report.longitudeDeg);
    object.insert(QStringLiteral("latitude_deg"), report.latitudeDeg);
    object.insert(QStringLiteral("altitude_m"), report.altitudeM);
    QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (tcpFraming) {
        bytes.append('\n'); // TCP needs a message boundary for JSON.
    }
    return bytes;
}

} // namespace CommandTargetReportProtocol

#endif // COMMANDTARGETREPORTPROTOCOL_H
