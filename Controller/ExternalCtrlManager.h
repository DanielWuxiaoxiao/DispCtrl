/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 15:42:47
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 16:19:34
 * @Description: 
 */
#ifndef EXTERNALCTRLMANAGER_H
#define EXTERNALCTRLMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include "Basic/ConfigManager.h"
#include "Basic/Protocol.h"

/**
 * @brief 外部雷控/调度链路管理器
 * 负责向外部雷控发送系统控制/伺服控制空壳数据，接收回送ACK。
 */
class ExternalCtrlManager : public QObject {
    Q_OBJECT
public:
    explicit ExternalCtrlManager(QObject* parent = nullptr);

    // 发送512B系统控制表（调用方需填充完整数据和校验）
    bool sendSystemControl(const QByteArray& frame512);

    // 发送32B伺服控制（可用ExternalServoCmd32填充）
    bool sendServoControl(const QByteArray& frame32);

signals:
    void systemCtrlAck(const QByteArray& ack64);
    void servoCtrlAck(const ExternalServoAck32& ack32);
    void externalLog(const QString& msg);

private slots:
    void onReadyRead();

private:
    void initSockets();
    void bindAckSocket();
    bool sendDatagram(const QByteArray& data);

    QUdpSocket* sendSocket{nullptr};
    QUdpSocket* recvSocket{nullptr};
    QHostAddress targetHost;
    quint16 targetPort{0};
    quint16 sourcePort{0};
    quint16 ackPort{0};
    quint16 srcId{0};
    quint16 dstId{0};
};

#endif // EXTERNALCTRLMANAGER_H
