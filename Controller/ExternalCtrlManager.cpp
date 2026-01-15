/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 16:19:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:11
 * @Description: 
 */
#include "ExternalCtrlManager.h"
#include <QDebug>
#include <QtGlobal>
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

bool ExternalCtrlManager::sendSystemControl(const QByteArray& frame512or504) {
    if (frame512or504.size() == EXTERNAL_SYSCTRL_FRAME_LEN) {
        // 长度 512：已包含头尾，校验后直接发送
        if (!isValidSystemControlFrame(frame512or504)) {
            emit externalLog(QStringLiteral("系统控制帧头/尾校验失败"));
            return false;
        }
        return sendDatagram(frame512or504);
    }
    if (frame512or504.size() == EXTERNAL_SYSCTRL_CONTENT_LEN) {
        // 长度 504：仅内容，自动拼头尾
        return sendSystemControlPayload(frame512or504);
    }
    emit externalLog(QStringLiteral("系统控制帧长度错误，期望512或504，实际%1").arg(frame512or504.size()));
    return false;
}

bool ExternalCtrlManager::sendSystemControlPayload(const QByteArray& payload504) {
    if (payload504.size() != EXTERNAL_SYSCTRL_CONTENT_LEN) {
        emit externalLog(QStringLiteral("系统控制内容长度错误，期望504，实际%1").arg(payload504.size()));
        return false;
    }
    QByteArray frame = buildSystemControlFrame(payload504);
    return sendDatagram(frame);
}

bool ExternalCtrlManager::sendServoControl(const QByteArray& frame32) {
    if (frame32.size() != 32) {
        emit externalLog(QStringLiteral("伺服控制帧长度错误，期望32，实际%1").arg(frame32.size()));
        return false;
    }
    return sendDatagram(frame32);
}

bool ExternalCtrlManager::sendAdFrame(const ExternalAdFrame& frame) {
    QByteArray data = encodeAdFrame(frame);
    if (data.isEmpty()) {
        emit externalLog(QStringLiteral("AD帧编码失败"));
        return false;
    }
    return sendDatagram(data);
}

bool ExternalCtrlManager::sendSystemControlWithAd(const QByteArray& payload504, const ExternalAdFrame& frame) {
    if (payload504.size() != EXTERNAL_SYSCTRL_CONTENT_LEN) {
        emit externalLog(QStringLiteral("系统控制内容长度错误，期望504，实际%1").arg(payload504.size()));
        return false;
    }
    QByteArray sys = buildSystemControlFrame(payload504);
    QByteArray ad = encodeAdFrame(frame);
    if (ad.isEmpty()) {
        emit externalLog(QStringLiteral("AD帧编码失败"));
        return false;
    }
    QByteArray combined;
    combined.reserve(sys.size() + ad.size());
    combined.append(sys);
    combined.append(ad);
    return sendDatagram(combined);
}

void ExternalCtrlManager::onReadyRead() {
    while (recvSocket && recvSocket->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(int(recvSocket->pendingDatagramSize()));
        QHostAddress sender; quint16 senderPort = 0;
        recvSocket->readDatagram(buf.data(), buf.size(), &sender, &senderPort);

        const int len = buf.size();

        // 1) 回执快速路径
        if (len == 64) {
            emit systemCtrlAck(buf);
            continue;
        }
        if (len == 32) {
            ExternalServoAck32 ack{};
            memcpy(ack.data, buf.constData(), qMin(buf.size(), 32));
            emit servoCtrlAck(ack);
            continue;
        }

        int offset = 0;

        // 2) 尝试解析系统控制 512B
        if (len - offset >= int(EXTERNAL_SYSCTRL_FRAME_LEN)) {
            QByteArray sysPart = buf.mid(offset, EXTERNAL_SYSCTRL_FRAME_LEN);
            ExternalSystemControl512 sysFrame{};
            if (tryParseSystemControl(sysPart, sysFrame)) {
                emit systemControlFrameReceived(sysFrame);
                offset += EXTERNAL_SYSCTRL_FRAME_LEN;
            }
        }

        // 3) 尝试解析 AD 数据帧（可选，紧随系统控制或单独发送）
        if (len - offset >= int(sizeof(ExternalAdHeader) + sizeof(ExternalAdTrailerReserve) + sizeof(quint32))) {
            ExternalAdFrame adFrame{};
            if (tryParseAdFrame(buf, offset, adFrame)) {
                emit adFrameReceived(adFrame);
                continue;
            }
        }

        // 4) 如果上述都未匹配，记录日志
        if (len != 64 && len != 32) {
            emit externalLog(QStringLiteral("外部链路收到未知报文，长度=%1").arg(len));
        }
    }
}

QByteArray ExternalCtrlManager::buildSystemControlFrame(const QByteArray& payload504) const {
    QByteArray frame;
    frame.reserve(int(EXTERNAL_SYSCTRL_FRAME_LEN));
    quint32 head = EXTERNAL_SYSCTRL_HEAD;
    quint32 tail = EXTERNAL_SYSCTRL_TAIL;
    frame.append(reinterpret_cast<const char*>(&head), sizeof(head));
    frame.append(payload504);
    frame.append(reinterpret_cast<const char*>(&tail), sizeof(tail));
    return frame;
}

bool ExternalCtrlManager::isValidSystemControlFrame(const QByteArray& frame) const {
    if (frame.size() != int(EXTERNAL_SYSCTRL_FRAME_LEN)) return false;
    quint32 head = 0, tail = 0;
    memcpy(&head, frame.constData(), sizeof(head));
    memcpy(&tail, frame.constData() + EXTERNAL_SYSCTRL_FRAME_LEN - sizeof(tail), sizeof(tail));
    return head == EXTERNAL_SYSCTRL_HEAD && tail == EXTERNAL_SYSCTRL_TAIL;
}

bool ExternalCtrlManager::tryParseSystemControl(const QByteArray& frame, ExternalSystemControl512& outFrame) const {
    if (!isValidSystemControlFrame(frame)) return false;
    memcpy(&outFrame.head, frame.constData(), sizeof(outFrame.head));
    memcpy(outFrame.content, frame.constData() + sizeof(outFrame.head), EXTERNAL_SYSCTRL_CONTENT_LEN);
    memcpy(&outFrame.tail, frame.constData() + sizeof(outFrame.head) + EXTERNAL_SYSCTRL_CONTENT_LEN, sizeof(outFrame.tail));
    return true;
}

bool ExternalCtrlManager::tryParseAdFrame(const QByteArray& frame, int offset, ExternalAdFrame& outFrame) const {
    const int minFixed = int(sizeof(ExternalAdHeader) + sizeof(ExternalAdTrailerReserve) + sizeof(quint32));
    if (frame.size() - offset < minFixed) return false;

    ExternalAdHeader hdr{};
    memcpy(&hdr, frame.constData() + offset, sizeof(ExternalAdHeader));
    if (hdr.head != EXTERNAL_AD_HEAD) return false;

    const int sampleCount = int(hdr.beamCount) * int(hdr.samplesPerPulse);
    if (sampleCount < 0) return false;
    const int sampleBytes = sampleCount * int(sizeof(quint16)) * 2; // I+Q

    const int totalNeeded = int(sizeof(ExternalAdHeader)) + sampleBytes + int(sizeof(ExternalAdTrailerReserve)) + int(sizeof(quint32));
    if (frame.size() - offset < totalNeeded) return false;

    outFrame.header = hdr;
    outFrame.samples.clear();
    outFrame.samples.reserve(sampleCount);

    const char* p = frame.constData() + offset + sizeof(ExternalAdHeader);
    for (int i = 0; i < sampleCount; ++i) {
        quint16 iPart = 0, qPart = 0;
        memcpy(&iPart, p + i * 4, 2);
        memcpy(&qPart, p + i * 4 + 2, 2);
        ExternalAdSample s{};
        s.i = static_cast<qint16>(iPart);
        s.q = static_cast<qint16>(qPart);
        outFrame.samples.push_back(s);
    }

    memcpy(&outFrame.trailer, frame.constData() + offset + sizeof(ExternalAdHeader) + sampleBytes, sizeof(ExternalAdTrailerReserve));
    memcpy(&outFrame.tail, frame.constData() + offset + sizeof(ExternalAdHeader) + sampleBytes + sizeof(ExternalAdTrailerReserve), sizeof(quint32));

    return outFrame.tail == EXTERNAL_AD_TAIL;
}

QByteArray ExternalCtrlManager::encodeAdFrame(const ExternalAdFrame& frame) const {
    const int sampleCount = frame.header.beamCount * frame.header.samplesPerPulse;
    if (frame.samples.size() != sampleCount) {
        qWarning() << "AD samples size mismatch" << frame.samples.size() << "expected" << sampleCount;
        return {};
    }

    const int sampleBytes = sampleCount * int(sizeof(quint16)) * 2;
    QByteArray data;
    data.reserve(int(sizeof(ExternalAdHeader)) + sampleBytes + int(sizeof(ExternalAdTrailerReserve)) + int(sizeof(quint32)));

    data.append(reinterpret_cast<const char*>(&frame.header), sizeof(ExternalAdHeader));
    for (const auto& s : frame.samples) {
        quint16 iPart = static_cast<quint16>(s.i);
        quint16 qPart = static_cast<quint16>(s.q);
        data.append(reinterpret_cast<const char*>(&iPart), 2);
        data.append(reinterpret_cast<const char*>(&qPart), 2);
    }
    data.append(reinterpret_cast<const char*>(&frame.trailer), sizeof(ExternalAdTrailerReserve));
    quint32 tail = EXTERNAL_AD_TAIL;
    data.append(reinterpret_cast<const char*>(&tail), sizeof(tail));
    return data;
}
