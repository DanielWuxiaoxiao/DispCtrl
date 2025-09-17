#include "disp2resmanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include "controller.h"

Disp2ResManager::Disp2ResManager(QObject* parent)
    : QObject(parent) {
    src = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    dst = CF_INS.id("RES_DIS_ID",RES_DIS_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_2_RES_PORT",DISP_2_RES_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    commCount = 1;

    host = QHostAddress(CF_INS.ip("RES_DIS_IP",RES_DIS_IP));
    port = CF_INS.port("RES_GET_DISP_PORT",RES_GET_DISP_PORT);
}

void Disp2ResManager::sendParam(char* paramData, unsigned paramSize)
{
    auto data = packData(paramData, paramSize, src, dst, commCount);
    commCount++;
    QByteArray byteArray = QByteArray::fromRawData(data, paramSize + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    socket->writeData(byteArray, host, port);
    free(data);
}

void Disp2ResManager::sendBCParam(BatteryControlM param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendTRParam(TranRecControl param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendFCParam(DirGramScan param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendSRParam(ScanRange param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendWCParam(BeamControl param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendSPParam(SigProParam param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendDPParam(DataProParam param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

Disp2ResManager::~Disp2ResManager() {
    thread->quit();
    thread->wait();
}
