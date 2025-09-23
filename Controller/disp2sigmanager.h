/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:58
 * @Description: 
 */
#ifndef DISP2SIGMANAGER_H
#define DISP2SIGMANAGER_H

#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Disp2SigManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2SigManager(QObject *parent = nullptr);
    ~Disp2SigManager();

public slots:
    void sendDSParam(DataSet param);
    void sendParam(char* paramData, unsigned paramSize);

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // DISP2SIGMANAGER_H
