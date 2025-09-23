/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:55
 * @Description: 
 */
#include "disp2photomanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include <QDateTime>
#include "controller.h"

Disp2PhotoManager::Disp2PhotoManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    dst = CF_INS.id("RES_DIS_ID",RES_DIS_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP), CF_INS.port("DISP_2_PHOTO_PORT",DISP_2_PHOTO_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();

    host = QHostAddress(CF_INS.ip("PHOTO_ELE_IP",PHOTO_ELE_IP));
    port = CF_INS.port("PHOTO_GET_DISP_PORT",PHOTO_GET_DISP_PORT);
    enableHeartBeat();
}

void Disp2PhotoManager::sendPEParam(PhotoElectricParamSet param)
{
    char *sendData = (char *)malloc(sizeof(PhotoElectricParamSet));
    param.dataLen = sizeof(PhotoElectricParamSet)-sizeof(param.versionNumber)-sizeof(param.head)-sizeof(param.dataLen)-sizeof(param.checkCode);

    param.timeStamp = QDateTime::currentMSecsSinceEpoch();

    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    QByteArray byteArray = QByteArray::fromRawData(sendData, sizeof(param));
    socket->writeData(byteArray, host, port);
    free(sendData);
}

void Disp2PhotoManager::sendPEParam2(PhotoElectricParamSet2 param)
{
    char *sendData = (char *)malloc(sizeof(PhotoElectricParamSet2));
    param.dataLen = sizeof(PhotoElectricParamSet2)-sizeof(param.versionNumber)-sizeof(param.head)-sizeof(param.dataLen)-sizeof(param.checkCode);

    param.timeStamp = QDateTime::currentMSecsSinceEpoch();

    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    QByteArray byteArray = QByteArray::fromRawData(sendData, sizeof(param));
    socket->writeData(byteArray, host, port);
    free(sendData);
}

void Disp2PhotoManager::enableHeartBeat()
{
    // 示例：根据之前的协议构造心跳包
    char* sendData = (char *)malloc(sizeof(HeartbeatPacket));
    HeartbeatPacket param;
    param.dataLen = sizeof(HeartbeatPacket)-sizeof(param.versionNumber)-sizeof(param.head)-sizeof(param.dataLen)-sizeof(param.checkCode);
    // ... 填充版本号、帧头、功能号等字段 ...
    // 转换为 QByteArray
    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    heartbeatPacket = QByteArray::fromRawData(sendData, sizeof(param));

    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, &Disp2PhotoManager::sendHeartbeat);
    heartbeatTimer->start(HEARTBEAT_INTERVAL); // 启动定时器
}

void Disp2PhotoManager::sendHeartbeat() {
    if (!heartbeatPacket.isEmpty() && socket) {
        // 替换为你的目标 IP 和端口（如心跳包的接收端）
        socket->writeData(
            heartbeatPacket,
            host,
            port
        );
    }
}

Disp2PhotoManager::~Disp2PhotoManager() {
    thread->quit();
    thread->wait();
}


