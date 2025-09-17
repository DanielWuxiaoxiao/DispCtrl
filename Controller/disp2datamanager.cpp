#include "disp2datamanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include "controller.h"

Disp2DataManager::Disp2DataManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    dst = CF_INS.id("DATA_PRO_ID",DATA_PRO_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP), CF_INS.port("DISP_2_DATA",DISP_2_DATA));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    commCount = 1;

    host = QHostAddress(CF_INS.ip("DATA_PRO_IP",DATA_PRO_IP));
    port = CF_INS.port("DATA_GET_DISP",DATA_GET_DISP);
}

void Disp2DataManager::sendParam(char* paramData, unsigned paramSize)
{
    auto data = packData(paramData, paramSize, src, dst, commCount);
    commCount++;
    QByteArray byteArray = QByteArray::fromRawData(data, paramSize + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    socket->writeData(byteArray, host, port);
    free(data);
}

void Disp2DataManager::setManual(SetTrackManual param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

Disp2DataManager::~Disp2DataManager() {
    thread->quit();
    thread->wait();
}

