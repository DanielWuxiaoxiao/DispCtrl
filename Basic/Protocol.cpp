/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:44
 * @Description: 
 */
#include "Protocol.h"
#pragma pack(push, 8)
#include <QtEndian>
#pragma pack(pop)

unsigned char calculateXOR(const char* data, unsigned len)
{
    unsigned char checksum = 0;
    for(unsigned i =0; i<len ; i++)
    {
        checksum^= data[i];
    }
    return checksum;
}

char checkAccusation(char* data, unsigned len)
{
    int checksum = 0;
    for(unsigned i =0; i<len ; i++)
    {
        checksum += data[i];
    }
    return checksum & 0xff;
}

char* packData(char* data, unsigned dataLen, unsigned short srcID, unsigned short destID, unsigned commCount)
{
    char* sendData = (char*) malloc(sizeof(ProtocolFrame) + sizeof(ProtocolEnd) + dataLen);
    ProtocolFrame frame;
    frame.srcID = srcID;
    frame.destID = destID;
    frame.commCount = commCount;
    frame.dataLen = static_cast<unsigned short>(sizeof(ProtocolFrame) + dataLen); // 显式转换避免警告
    memcpy(sendData, &frame, sizeof (frame));
    memcpy(sendData + sizeof(frame), data, dataLen);
    auto checksum = calculateXOR(sendData, frame.dataLen);

    ProtocolEnd end;
    end.checkCode = checksum;
    memcpy(sendData + sizeof(frame) + dataLen, &end, sizeof(end));

    return sendData;
}

bool decodeSystemTimeSyncReply(const char* frame, int size, quint16 sourceId, quint16 destId, bool& succeeded)
{
    constexpr int payloadOffset = sizeof(ProtocolFrame);
    constexpr int checksumOffset = payloadOffset + sizeof(SystemTimeSyncReply);
    constexpr int frameSize = checksumOffset + sizeof(ProtocolEnd);
    if (!frame || size != frameSize) return false;

    if (qFromLittleEndian<quint32>(frame) != HEADCODE
        || qFromLittleEndian<quint16>(frame + offsetof(ProtocolFrame, srcID)) != sourceId
        || qFromLittleEndian<quint16>(frame + offsetof(ProtocolFrame, destID)) != destId
        || qFromLittleEndian<quint16>(frame + offsetof(ProtocolFrame, dataLen)) != checksumOffset
        || qFromLittleEndian<quint16>(frame + payloadOffset) != SystemTimeSyncReply::MessageId
        || static_cast<quint8>(frame[checksumOffset]) != calculateXOR(frame, checksumOffset)
        || qFromLittleEndian<quint32>(frame + checksumOffset + offsetof(ProtocolEnd, end)) != ENDCODE) {
        return false;
    }

    const auto result = static_cast<quint8>(frame[payloadOffset + offsetof(SystemTimeSyncReply, result)]);
    if (result > 1) return false;
    succeeded = result == 1;
    return true;
}
