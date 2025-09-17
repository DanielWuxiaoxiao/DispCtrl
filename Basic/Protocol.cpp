#include "Protocol.h"


unsigned char calculateXOR(char* data, unsigned len)
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
    frame.dataLen = sizeof(ProtocolFrame) + dataLen;
    memcpy(sendData, &frame, sizeof (frame));
    memcpy(sendData + sizeof(frame), data, dataLen);
    auto checksum = calculateXOR(sendData, frame.dataLen);

    ProtocolEnd end;
    end.checkCode = checksum;
    memcpy(sendData + sizeof(frame) + dataLen, &end, sizeof(end));

    return sendData;
}
