/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
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
    quint64 m_frameCount = 0;
    quint64 m_pointCount = 0;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // TBD2DISPMANAGER_H
