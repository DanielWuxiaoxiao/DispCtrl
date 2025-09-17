#ifndef THREADUDPSOCKET_H
#define THREADUDPSOCKET_H

#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QTimer>
#include "Basic/Protocol.h"

class ThreadedUdpSocket : public QObject {
    Q_OBJECT
public:
    ThreadedUdpSocket(QString ip, quint16 port, QObject* parent = nullptr);
    void setSourceAndDestID(quint16 src, quint16 dst);
    void writeData(const QByteArray &datagram, const QHostAddress &host, quint16 port);

signals:
    void detInfo(QByteArray);
    void traInfo(QByteArray);
    void dataSaveOK(DataSaveOK);
    void dataDelOK(DataDelOK);
    void offLineStat(OfflineStat);
    void targetClaRes(TargetClaRes);
    void monitorParamSend(MonitorParam);

public slots:
    void start();

private slots:
    void onReadyRead();

private:
    QString m_Ip;
    quint16 m_Port;
    QUdpSocket* m_socket = nullptr;
    quint16 srcID;
    quint16 destID;
    quint32 commCount = 1;

    void handleDatagram(const QByteArray& datagram, int senderPort);
    bool validateFrame(const QByteArray& data);
};

#endif // THREADUDPSOCKET_H
