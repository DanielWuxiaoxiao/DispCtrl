#include "disp2sigmanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>

Disp2SigManager::Disp2SigManager(QObject *parent) : QObject(parent)
{
    src = DISP_CTRL_ID;
    dst = SIG_PRO_ID;
    socket = new ThreadedUdpSocket(DISP_CTRL_IP, DISP_2_PHOTO_PORT);
    socket->setSourceAndDestID(DISP_CTRL_ID, SIG_PRO_ID);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    commCount = 1;

    host = QHostAddress(PHOTO_ELE_IP);
    port = PHOTO_GET_DISP_PORT;
}

void Disp2SigManager::sendParam(char* paramData, unsigned paramSize)
{
    auto data = packData(paramData, paramSize, src, dst, commCount);
    commCount++;
    QByteArray byteArray = QByteArray::fromRawData(data, paramSize + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    socket->writeData(byteArray, host, port);
    free(data);
}

void Disp2SigManager::sendDSParam(DataSet param)
{
    if (param.ifsave == 1)
    {
        sendParam(reinterpret_cast<char *>(&param.save),sizeof(param.save));
    }

    if (param.ifdel == 1)
    {
        sendParam(reinterpret_cast<char *>(&param.del),sizeof(param.del));
    }

    if (param.ifoffline == 1)
    {
        sendParam(reinterpret_cast<char *>(&param.off),sizeof(param.off));
    }
}
