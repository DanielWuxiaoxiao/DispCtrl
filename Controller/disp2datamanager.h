/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:55
 * @Description: 
 */
#ifndef DISP2DATAMANAGER_H
#define DISP2DATAMANAGER_H

#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Disp2DataManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2DataManager(QObject *parent = nullptr);
    ~Disp2DataManager();

public slots:
    void setManual(SetTrackManual data);
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

#endif // DISP2DATAMANAGER_H
