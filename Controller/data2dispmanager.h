/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:19
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

private slots:
    void dispatchTrackBatch(const QList<PointInfo>& tracks, int packetSize);

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
