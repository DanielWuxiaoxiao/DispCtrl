/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:55
 * @Description: 
 */
#include "disp2monmanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include "controller.h"

Disp2MonManager::Disp2MonManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    dst = CF_INS.id("MONITOR_ID",MONITOR_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP), CF_INS.port("DISP_2_MONITOR",DISP_2_MONITOR));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    commCount = 1;

    host = QHostAddress(CF_INS.ip("MONITOR_IP",MONITOR_IP));
    port = CF_INS.port("MONITOR_GET_DISP_PORT",MONITOR_GET_DISP_PORT);
}

void Disp2MonManager::sendParam(char* paramData, unsigned paramSize)
{
    auto data = packData(paramData, paramSize, src, dst, commCount);
    commCount++;
    QByteArray byteArray = QByteArray::fromRawData(data, paramSize + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    socket->writeData(byteArray, host, port);
    free(data);
}

void Disp2MonManager::sendSysStart(StartSysParam data)
{
    sendParam(reinterpret_cast<char *>(&data),sizeof(data));
}

Disp2MonManager::~Disp2MonManager() {
    thread->quit();
    thread->wait();
}

