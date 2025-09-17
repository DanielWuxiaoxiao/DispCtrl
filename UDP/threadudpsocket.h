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
    
    // 错误处理信号
    void socketError(const QString& error);
    void connectionStatusChanged(bool connected);

public slots:
    void start();

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

private:
    QString m_Ip;
    quint16 m_Port;
    QUdpSocket* m_socket = nullptr;
    quint16 srcID;
    quint16 destID;
    quint32 commCount = 1;
    
    // 错误处理相关
    QTimer* m_reconnectTimer;
    int m_reconnectAttempts;
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    static const int RECONNECT_INTERVAL_MS = 3000;

    void handleDatagram(const QByteArray& datagram, int senderPort);
    bool validateFrame(const QByteArray& data);
    void attemptReconnect();
    void reportError(const QString& code, const QString& message);
};

#endif // THREADUDPSOCKET_H
