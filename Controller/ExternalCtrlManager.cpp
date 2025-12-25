/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 15:43:06
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 16:19:34
 * @Description: 
 */
#include "ExternalCtrlManager.h"
#include <QDebug>
#include <cstring>

ExternalCtrlManager::ExternalCtrlManager(QObject* parent)
    : QObject(parent) {
    initSockets();
}

void ExternalCtrlManager::initSockets() {
    const auto& cfg = CF_INS;
    targetHost = QHostAddress(cfg.ip("RADAR_CTRL_IP", "192.168.1.16"));
    targetPort = static_cast<quint16>(cfg.port("EXT_SYSCTRL_DST", 8001));
    sourcePort = static_cast<quint16>(cfg.port("EXT_SYSCTRL_SRC", 6001));
    ackPort = static_cast<quint16>(cfg.port("EXT_ACK_DST", 8002));
    srcId = static_cast<quint16>(cfg.id("DISP_CTRL_ID", DISP_CTRL_ID));
    dstId = static_cast<quint16>(cfg.id("RADAR_CTRL_ID", RADAR_CTRL_ID));

    sendSocket = new QUdpSocket(this);
    // 绑定源端口，便于对端识别
    if (!sendSocket->bind(QHostAddress::AnyIPv4, sourcePort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit externalLog(QStringLiteral("外部链路发送端口绑定失败: %1").arg(sendSocket->errorString()));
    }

    recvSocket = new QUdpSocket(this);
    bindAckSocket();
    connect(recvSocket, &QUdpSocket::readyRead, this, &ExternalCtrlManager::onReadyRead);
}

void ExternalCtrlManager::bindAckSocket() {
    if (!recvSocket) return;
    bool ok = recvSocket->bind(QHostAddress::AnyIPv4, ackPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!ok) {
        emit externalLog(QStringLiteral("外部链路ACK端口绑定失败: %1").arg(recvSocket->errorString()));
    }
}

bool ExternalCtrlManager::sendDatagram(const QByteArray& data) {
    if (!sendSocket) return false;
    qint64 sent = sendSocket->writeDatagram(data, targetHost, targetPort);
    if (sent != data.size()) {
        emit externalLog(QStringLiteral("外部链路发送失败，长度=%1 错误=%2").arg(data.size()).arg(sendSocket->errorString()));
        return false;
    }
    return true;
}

bool ExternalCtrlManager::sendSystemControl(const QByteArray& frame512) {
    if (frame512.size() != 512) {
        emit externalLog(QStringLiteral("系统控制帧长度错误，期望512，实际%1").arg(frame512.size()));
        return false;
    }
    return sendDatagram(frame512);
}

bool ExternalCtrlManager::sendServoControl(const QByteArray& frame32) {
    if (frame32.size() != 32) {
        emit externalLog(QStringLiteral("伺服控制帧长度错误，期望32，实际%1").arg(frame32.size()));
        return false;
    }
    return sendDatagram(frame32);
}

void ExternalCtrlManager::onReadyRead() {
    while (recvSocket && recvSocket->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(int(recvSocket->pendingDatagramSize()));
        QHostAddress sender; quint16 senderPort = 0;
        recvSocket->readDatagram(buf.data(), buf.size(), &sender, &senderPort);

        // 根据长度简单区分ACK类型
        if (buf.size() == 64) {
            emit systemCtrlAck(buf);
        } else if (buf.size() == 32) {
            ExternalServoAck32 ack{};
            memcpy(ack.data, buf.constData(), qMin(buf.size(), 32));
            emit servoCtrlAck(ack);
        } else {
            emit externalLog(QStringLiteral("收到未知长度ACK=%1，忽略").arg(buf.size()));
        }
    }
}
