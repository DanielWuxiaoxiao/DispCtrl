/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:00
 * @Description: 
 */
#ifndef MON2DISPMANAGER_H
#define MON2DISPMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Mon2DispManager : public QObject
{
    Q_OBJECT
public:
    explicit Mon2DispManager(QObject *parent = nullptr);
    ~Mon2DispManager();
private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // MON2DISPMANAGER_H
