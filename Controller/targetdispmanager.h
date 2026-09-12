/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:50
 * @Description: 
 */
#ifndef TARGETDISPMANAGER_H
#define TARGETDISPMANAGER_H

#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class targetDispManager : public QObject
{
    Q_OBJECT
public:
    explicit targetDispManager(QObject *parent = nullptr);
    ~targetDispManager();
private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // TARGETDISPMANAGER_H
