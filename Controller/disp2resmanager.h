#ifndef DISP2RESMANAGER_H
#define DISP2RESMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class Disp2ResManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2ResManager(QObject *parent = nullptr);
    ~Disp2ResManager();

public slots:
    void sendBCParam(BatteryControlM param);
    void sendTRParam(TranRecControl param);
    void sendFCParam(DirGramScan param);
    void sendSRParam(ScanRange param);
    void sendWCParam(BeamControl param);
    void sendSPParam(SigProParam param);
    void sendDPParam(DataProParam param);
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

#endif // DISP2RESMANAGER_H
