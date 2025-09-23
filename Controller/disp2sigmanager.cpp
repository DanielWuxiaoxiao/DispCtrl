/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:57
 * @Description: 
 */
#include "disp2sigmanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include "controller.h"

Disp2SigManager::Disp2SigManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    dst = CF_INS.id("SIG_PRO_ID",SIG_PRO_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_2_PHOTO_PORT",DISP_2_PHOTO_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    commCount = 1;

    host = QHostAddress(CF_INS.ip("PHOTO_ELE_IP",PHOTO_ELE_IP));
    port = CF_INS.port("PHOTO_GET_DISP_PORT",PHOTO_GET_DISP_PORT);
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

Disp2SigManager::~Disp2SigManager() {
    thread->quit();
    thread->wait();
}

