/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:59
 * @Description: 
 */
#include "mon2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include <QThread>

Mon2DispManager::Mon2DispManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("MONITOR_ID",MONITOR_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_GET_MONITOR_PORT",DISP_GET_MONITOR_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    connect(socket, &ThreadedUdpSocket::monitorParamSend, CON_INS, &Controller::monitorParamSend);
}

Mon2DispManager::~Mon2DispManager() {
    thread->quit();
    thread->wait();
}

