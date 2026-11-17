#ifndef BITREPORTDECODER_H
#define BITREPORTDECODER_H
#include "Basic/Protocol.h"
#pragma pack(push, 8)
#include <QtEndian>

// The current DE02 format carries the reference for both installations.
// Only the legacy single-array installation also accepts the exact old size.
inline bool decodeBitReportPayload(const char* bytes, int size, bool multiPanel, BITReport& status,
                                  BITReportV2& extended, bool& hasReference)
{
    hasReference = false;
    const bool extendedSize = size == sizeof(BITReportV2);
    if (!bytes || (!extendedSize && (multiPanel || size != sizeof(BITReport)))
        || qFromLittleEndian<quint16>(bytes) != 0xDE02) return false;
    status = BITReport{};
    status.bitGroup = static_cast<unsigned char>(bytes[2]);
    status.powerState = static_cast<unsigned char>(bytes[3]);
    status.fpgaTemp = qFromLittleEndian<quint16>(bytes + 4);
    status.panelTemp = qFromLittleEndian<quint16>(bytes + 6);
    status.yaw = qFromLittleEndian<quint16>(bytes + 8);
    memcpy(status.subArrayPower, bytes + 10, 5);
    status.scanAngle = qFromLittleEndian<quint16>(bytes + 15);
    status.radarId = static_cast<unsigned char>(bytes[17]);
    memcpy(status.reserve, bytes + 18, sizeof(status.reserve));
    if (!extendedSize) return true;
    extended = BITReportV2{};
    extended.bitGroup = status.bitGroup;
    extended.powerState = status.powerState;
    extended.fpgaTemp = status.fpgaTemp;
    extended.panelTemp = status.panelTemp;
    extended.yaw = status.yaw;
    memcpy(extended.subArrayPower, status.subArrayPower, sizeof(status.subArrayPower));
    extended.scanAngle = status.scanAngle;
    extended.radarId = status.radarId;
    memcpy(extended.legacyReserve, bytes + 18, sizeof(extended.legacyReserve));
    extended.latitudeE9 = qFromLittleEndian<qint64>(bytes + 39);
    extended.longitudeE9 = qFromLittleEndian<qint64>(bytes + 47);
    extended.altitudeCm = qFromLittleEndian<qint32>(bytes + 55);
    extended.headingMilliDeg = qFromLittleEndian<quint32>(bytes + 59);
    extended.pitchMilliDeg = qFromLittleEndian<qint32>(bytes + 63);
    extended.rollCentiDeg = qFromLittleEndian<qint16>(bytes + 67);
    extended.servoEncoderMilliDeg = qFromLittleEndian<quint32>(bytes + 69);
    memcpy(extended.reserve, bytes + 73, sizeof(extended.reserve));
    hasReference = true;
    return true;
}
#pragma pack(pop)
#endif
