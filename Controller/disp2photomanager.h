/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:56
 * @Description: 
 */
#ifndef DISP2PHOTOMANAGER_H
#define DISP2PHOTOMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Disp2PhotoManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2PhotoManager(QObject *parent = nullptr);
    ~Disp2PhotoManager();

public slots:
    void sendPEParam(PhotoElectricParamSet param);
    void sendPEParam2(PhotoElectricParamSet2 param);
    void enableHeartBeat();
    void sendHeartbeat();

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;

    // 新增：定时器用于循环发送心跳
    QTimer *heartbeatTimer = nullptr;
    // 心跳包内容（根据你的协议构造）
    QByteArray heartbeatPacket = nullptr;
    // 心跳包发送间隔（毫秒）
    const int HEARTBEAT_INTERVAL = 1000;
};

#endif // DISP2PHOTOMANAGER_H
