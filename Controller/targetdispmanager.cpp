#include "targetdispmanager.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include <QThread>

targetDispManager::targetDispManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("TAR_CLA_ID",TAR_CLA_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_GET_TARGET_PORT",DISP_GET_TARGET_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    connect(socket, &ThreadedUdpSocket::targetClaRes, CON_INS, &Controller::targetClaRes);
}

targetDispManager::~targetDispManager() {
    thread->quit();
    thread->wait();
}
