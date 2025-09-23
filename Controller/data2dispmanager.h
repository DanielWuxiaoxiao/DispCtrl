/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
#ifndef DATA2DISPMANAGER_H
#define DATA2DISPMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Data2DispManager : public QObject
{
    Q_OBJECT
public:
    explicit Data2DispManager(QObject *parent = nullptr);
    ~Data2DispManager();

public slots:
    void traInfoDecode(QByteArray data);

signals:
    void traInfoProcess(PointInfo info);

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // DATA2DISPMANAGER_H
