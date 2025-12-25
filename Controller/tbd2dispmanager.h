/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 14:13:45
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 16:19:34
 * @Description: 
 */
/*
 * TBD track receiver manager
 */
#ifndef TBD2DISPMANAGER_H
#define TBD2DISPMANAGER_H

#pragma once
#include <QObject>
#include <QHostAddress>
#include "Basic/Protocol.h"

class ThreadedUdpSocket;
class QThread;

class Tbd2DispManager : public QObject
{
    Q_OBJECT
public:
    explicit Tbd2DispManager(QObject *parent = nullptr);
    ~Tbd2DispManager();

public slots:
    void tbdInfoDecode(QByteArray data);

signals:
    void tbdInfoProcess(PointInfo info);

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // TBD2DISPMANAGER_H
