/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:13:20
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
#include "commandcontrolprotocol.h"

#include <QDateTime>

namespace CommandControlProtocol {
namespace {

void appendU8(QByteArray& bytes, quint8 value)
{
    bytes.append(static_cast<char>(value));
}

void appendU16(QByteArray& bytes, quint16 value)
{
    appendU8(bytes, static_cast<quint8>(value));
    appendU8(bytes, static_cast<quint8>(value >> 8));
}

void appendU24(QByteArray& bytes, quint32 value)
{
    appendU8(bytes, static_cast<quint8>(value));
    appendU8(bytes, static_cast<quint8>(value >> 8));
    appendU8(bytes, static_cast<quint8>(value >> 16));
}

void appendU32(QByteArray& bytes, quint32 value)
{
    appendU16(bytes, static_cast<quint16>(value));
    appendU16(bytes, static_cast<quint16>(value >> 16));
}

void appendI16(QByteArray& bytes, qint16 value)
{
    appendU16(bytes, static_cast<quint16>(value));
}

void appendI32(QByteArray& bytes, qint32 value)
{
    appendU32(bytes, static_cast<quint32>(value));
}

bool readU8(const QByteArray& bytes, int& offset, quint8& value)
{
    if (offset >= bytes.size()) {
        return false;
    }
    value = static_cast<quint8>(bytes.at(offset));
    ++offset;
    return true;
}

bool readU16(const QByteArray& bytes, int& offset, quint16& value)
{
    quint8 low = 0;
    quint8 high = 0;
    if (!readU8(bytes, offset, low) || !readU8(bytes, offset, high)) {
        return false;
    }
    value = static_cast<quint16>(low) | (static_cast<quint16>(high) << 8);
    return true;
}

bool readU24(const QByteArray& bytes, int& offset, quint32& value)
{
    quint8 low = 0;
    quint8 middle = 0;
    quint8 high = 0;
    if (!readU8(bytes, offset, low) || !readU8(bytes, offset, middle) || !readU8(bytes, offset, high)) {
        return false;
    }
    value = static_cast<quint32>(low)
          | (static_cast<quint32>(middle) << 8)
          | (static_cast<quint32>(high) << 16);
    return true;
}

bool readU32(const QByteArray& bytes, int& offset, quint32& value)
{
    quint16 low = 0;
    quint16 high = 0;
    if (!readU16(bytes, offset, low) || !readU16(bytes, offset, high)) {
        return false;
    }
    value = static_cast<quint32>(low) | (static_cast<quint32>(high) << 16);
    return true;
}

bool readI16(const QByteArray& bytes, int& offset, qint16& value)
{
    quint16 raw = 0;
    if (!readU16(bytes, offset, raw)) {
        return false;
    }
    value = static_cast<qint16>(raw);
    return true;
}

bool readI32(const QByteArray& bytes, int& offset, qint32& value)
{
    quint32 raw = 0;
    if (!readU32(bytes, offset, raw)) {
        return false;
    }
    value = static_cast<qint32>(raw);
    return true;
}

QByteArray makePacket(const Header& header, const QByteArray& payload)
{
    QByteArray bytes;
    bytes.reserve(kHeaderSize + payload.size());
    // 严格按照公共报头字段偏移追加，不能直接 memcpy Header，避免对齐和字节序差异。
    appendU16(bytes, header.magic);
    appendU8(bytes, header.version);
    appendU32(bytes, header.senderId);
    appendU32(bytes, header.receiverId);
    appendU8(bytes, header.flags);
    appendU16(bytes, header.messageType);
    appendU8(bytes, header.sequence);
    appendU24(bytes, header.dayTicks10Ms);
    bytes.append(payload);
    return bytes;
}

}  // namespace

bool parseHeader(const QByteArray& packet, Header& header)
{
    if (packet.size() < kHeaderSize) {
        return false;
    }

    int offset = 0;
    if (!readU16(packet, offset, header.magic)
        || !readU8(packet, offset, header.version)
        || !readU32(packet, offset, header.senderId)
        || !readU32(packet, offset, header.receiverId)
        || !readU8(packet, offset, header.flags)
        || !readU16(packet, offset, header.messageType)
        || !readU8(packet, offset, header.sequence)
        || !readU24(packet, offset, header.dayTicks10Ms)) {
        return false;
    }
    return header.magic == kMagic && header.version == kVersion;
}

bool parseManagementNode(const QByteArray& packet, ManagementNode& node)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != ManagementNodeNotice
        || packet.size() != expectedPacketSize(ManagementNodeNotice)) {
        return false;
    }

    int offset = kHeaderSize;
    quint8 identity = 0;
    if (!readU32(packet, offset, node.controlId)
        || !readU8(packet, offset, identity)
        || !readU32(packet, offset, node.ipv4LowFirst)
        || !readU16(packet, offset, node.port)) {
        return false;
    }
    if (identity < static_cast<quint8>(LoginIdentity::ManagementNodeNotice)
        || identity > static_cast<quint8>(LoginIdentity::Logout)) {
        return false;
    }
    node.identity = static_cast<LoginIdentity>(identity);
    // DD31 剩余 7 字节是备份字段；保留原始值，仅供联调日志追溯。
    node.reserve = packet.mid(offset, 7);
    return node.reserve.size() == 7;
}

bool parseLoginRequest(const QByteArray& packet, LoginRequestPayload& request)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != LoginRequest
        || packet.size() != expectedPacketSize(LoginRequest)) {
        return false;
    }

    int offset = kHeaderSize;
    quint8 type = 0;
    if (!readU32(packet, offset, request.userId)
        || !readU32(packet, offset, request.ipv4LowFirst)
        || !readU16(packet, offset, request.port)
        || !readU8(packet, offset, type)
        || !readU16(packet, offset, request.year)
        || !readU8(packet, offset, request.month)
        || !readU8(packet, offset, request.day)
        || !readU8(packet, offset, request.hour)
        || !readU8(packet, offset, request.minute)
        || !readU8(packet, offset, request.second)
        || !readU16(packet, offset, request.millisecond)) {
        return false;
    }
    if (type > static_cast<quint8>(LoginRequestType::Logout)) {
        return false;
    }
    request.type = static_cast<LoginRequestType>(type);
    return true;
}

bool parseLoginReply(const QByteArray& packet, LoginReplyPayload& reply)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != LoginReply
        || packet.size() != expectedPacketSize(LoginReply)) {
        return false;
    }

    int offset = kHeaderSize;
    return readU8(packet, offset, reply.result)
        && readU8(packet, offset, reply.rejectReason)
        && readU32(packet, offset, reply.controlId);
}

bool parseLinkCheck(const QByteArray& packet, LinkCheckPayload& linkCheck)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != LinkCheck
        || packet.size() != expectedPacketSize(LinkCheck)) {
        return false;
    }

    int offset = kHeaderSize;
    return readU32(packet, offset, linkCheck.monthTimestampMs)
        && readU8(packet, offset, linkCheck.cooperationStatus);
}

bool parseDda4Track(const QByteArray& packet, Dda4Track& track)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != SituationIntelligence
        || packet.size() != expectedPacketSize(SituationIntelligence)) {
        return false;
    }

    int offset = kHeaderSize;
    if (!readU32(packet, offset, track.comprehensiveBatch)
        || !readU32(packet, offset, track.localBatch)
        || !readU32(packet, offset, track.deviceId)
        || !readU8(packet, offset, track.deviceType)
        || !readU8(packet, offset, track.deviceNumber)
        || !readU16(packet, offset, track.rateCentiHz)
        || !readU8(packet, offset, track.targetAttribute)
        || !readU8(packet, offset, track.targetType)
        || !readU16(packet, offset, track.reserve)
        || !readU8(packet, offset, track.trackQuality)
        || !readI32(packet, offset, track.longitudeE7)
        || !readI32(packet, offset, track.latitudeE7)
        || !readI32(packet, offset, track.altitudeM)
        || !readI16(packet, offset, track.velocityEast)
        || !readI16(packet, offset, track.velocityNorth)
        || !readI16(packet, offset, track.velocityUp)
        || !readU16(packet, offset, track.rcsMilliSquareM)
        || !readU8(packet, offset, track.interferenceStatus)
        || !readU8(packet, offset, track.updateMethod)
        || !readU16(packet, offset, track.relativeDelayMs)
        || !readU32(packet, offset, track.monthTimestampMs)) {
        return false;
    }
    return true;
}

bool parseDda1Status(const QByteArray& packet, Dda1Status& status)
{
    Header header;
    if (!parseHeader(packet, header) || header.messageType != EquipmentStatus
        || packet.size() != expectedPacketSize(EquipmentStatus)) {
        return false;
    }

    int offset = kHeaderSize;
    quint8 elevationStart = 0;
    quint8 elevationEnd = 0;
    if (!readI32(packet, offset, status.longitudeE7)
        || !readI32(packet, offset, status.latitudeE7)
        || !readI32(packet, offset, status.altitudeM)
        || !readU8(packet, offset, status.workStatus)
        || !readU8(packet, offset, status.healthStatus)
        || !readU8(packet, offset, status.deviceCount)
        || !readU8(packet, offset, status.deviceType)
        || !readU8(packet, offset, status.deviceNumber)
        || !readU8(packet, offset, status.deviceStatus)
        || !readU8(packet, offset, status.workMode)
        || !readU8(packet, offset, status.radiationStatus)
        || !readU16(packet, offset, status.azimuthStartDeg)
        || !readU16(packet, offset, status.azimuthEndDeg)
        || !readU8(packet, offset, elevationStart)
        || !readU8(packet, offset, elevationEnd)
        || !readU16(packet, offset, status.reserve)) {
        return false;
    }
    status.elevationStartDeg = static_cast<qint8>(elevationStart);
    status.elevationEndDeg = static_cast<qint8>(elevationEnd);
    return true;
}

QByteArray makeLoginRequest(const Header& header, quint32 userId, quint32 ipv4LowFirst,
                            quint16 port, LoginRequestType type, const QDateTime& beijingNow)
{
    QByteArray payload;
    payload.reserve(20);
    appendU32(payload, userId); // DD33 字段 1：本机用户 ID。
    appendU32(payload, ipv4LowFirst); // 字段 2：本机 IPv4，八位组低到高存放。
    appendU16(payload, port); // 字段 3：本机 UDP 端口。
    appendU8(payload, static_cast<quint8>(type)); // 字段 4：0 默认、1 登录、2 退出。
    appendU16(payload, static_cast<quint16>(beijingNow.date().year())); // 字段 5：北京时间年。
    appendU8(payload, static_cast<quint8>(beijingNow.date().month())); // 字段 6：北京时间月。
    appendU8(payload, static_cast<quint8>(beijingNow.date().day())); // 字段 7：北京时间日。
    appendU8(payload, static_cast<quint8>(beijingNow.time().hour())); // 字段 8：北京时间时。
    appendU8(payload, static_cast<quint8>(beijingNow.time().minute())); // 字段 9：北京时间分。
    appendU8(payload, static_cast<quint8>(beijingNow.time().second())); // 字段 10：北京时间秒。
    appendU16(payload, static_cast<quint16>(beijingNow.time().msec())); // 字段 11：北京时间毫秒。
    return makePacket(header, payload);
}

QByteArray makeLinkCheck(const Header& header, quint32 monthTimestampMs, quint8 cooperationStatus)
{
    QByteArray payload;
    payload.reserve(5);
    appendU32(payload, monthTimestampMs); // DD25 字段 1：本月首日零点以来的毫秒数。
    appendU8(payload, cooperationStatus); // 字段 2：协同参与状态，定版默认 0x00。
    return makePacket(header, payload);
}

QByteArray makeDda4Track(const Header& header, const Dda4Track& track)
{
    QByteArray payload;
    payload.reserve(49);
    appendU32(payload, track.comprehensiveBatch);
    appendU32(payload, track.localBatch);
    appendU32(payload, track.deviceId);
    appendU8(payload, track.deviceType);
    appendU8(payload, track.deviceNumber);
    appendU16(payload, track.rateCentiHz);
    appendU8(payload, track.targetAttribute);
    appendU8(payload, track.targetType);
    appendU16(payload, 0); // DDA4 字段 9：备份字段，固定填 0。
    appendU8(payload, track.trackQuality);
    appendI32(payload, track.longitudeE7);
    appendI32(payload, track.latitudeE7);
    appendI32(payload, track.altitudeM);
    appendI16(payload, track.velocityEast);
    appendI16(payload, track.velocityNorth);
    appendI16(payload, track.velocityUp);
    appendU16(payload, track.rcsMilliSquareM);
    appendU8(payload, track.interferenceStatus);
    appendU8(payload, track.updateMethod);
    appendU16(payload, track.relativeDelayMs);
    appendU32(payload, track.monthTimestampMs);
    return makePacket(header, payload);
}

QByteArray makeDda1Status(const Header& header, const Dda1Status& status)
{
    QByteArray payload;
    payload.reserve(28);
    appendI32(payload, status.longitudeE7);
    appendI32(payload, status.latitudeE7);
    appendI32(payload, status.altitudeM);
    appendU8(payload, status.workStatus);
    appendU8(payload, status.healthStatus);
    appendU8(payload, status.deviceCount);
    appendU8(payload, status.deviceType);
    appendU8(payload, status.deviceNumber);
    appendU8(payload, status.deviceStatus);
    appendU8(payload, status.workMode);
    appendU8(payload, status.radiationStatus);
    appendU16(payload, status.azimuthStartDeg);
    appendU16(payload, status.azimuthEndDeg);
    appendU8(payload, static_cast<quint8>(status.elevationStartDeg));
    appendU8(payload, static_cast<quint8>(status.elevationEndDeg));
    appendU16(payload, 0); // DDA1 字段 16：备份字段，固定填 0。
    return makePacket(header, payload);
}

const char* messageTypeName(quint16 type)
{
    switch (type) {
    case ManagementNodeNotice: return "DD31 管理节点通报";
    case LoginRequest: return "DD33 登陆请求";
    case LoginReply: return "DD34 登陆回复";
    case LinkCheck: return "DD25 链路检测";
    case SituationIntelligence: return "DDA4 态势情报";
    case EquipmentStatus: return "DDA1 装备状态";
    default: return "未知报文";
    }
}

int expectedPacketSize(quint16 type)
{
    switch (type) {
    case ManagementNodeNotice: return kHeaderSize + 18;
    case LoginRequest: return kHeaderSize + 20;
    case LoginReply: return kHeaderSize + 6;
    case LinkCheck: return kHeaderSize + 5;
    case SituationIntelligence: return kHeaderSize + 49;
    case EquipmentStatus: return kHeaderSize + 28;
    default: return -1;
    }
}

}  // namespace CommandControlProtocol
