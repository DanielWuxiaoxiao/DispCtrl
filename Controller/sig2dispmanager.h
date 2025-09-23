/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:01
 * @Description: 
 */
#ifndef SIG2DISPMANAGER_H
#define SIG2DISPMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class sig2dispmanager : public QObject
{
    Q_OBJECT
public:
    explicit sig2dispmanager(QObject *parent = nullptr);
    ~sig2dispmanager();

public slots:
    void detInfoDecode(QByteArray data);

signals:
    void detInfoProcess(PointInfo info);

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

class sig2dispmanager2 : public QObject
{
    Q_OBJECT
public:
    explicit sig2dispmanager2(QObject *parent = nullptr);
    ~sig2dispmanager2();
private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // SIG2DISPMANAGER_H
