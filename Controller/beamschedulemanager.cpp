/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:18
 * @Description: 
 */
/**
 * @file beamschedulemanager.cpp
 * @brief 波束调度报告 UDP 接收管理器实现
 */
#include "beamschedulemanager.h"
#include <QNetworkDatagram>
#include <QDebug>

BeamScheduleManager::BeamScheduleManager(QObject* parent)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &BeamScheduleManager::onReadyRead);
}

void BeamScheduleManager::init(const QString& localIp, quint16 localPort)
{
    if (m_socket->state() == QAbstractSocket::BoundState)
        m_socket->close();

    const QHostAddress addr = localIp.isEmpty()
                              ? QHostAddress::AnyIPv4
                              : QHostAddress(localIp);

    if (!m_socket->bind(addr, localPort,
                        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit logMessage(QStringLiteral("BeamScheduleMgr: 绑定失败 %1:%2 — %3")
                        .arg(localIp).arg(localPort).arg(m_socket->errorString()));
    } else {
        emit logMessage(QStringLiteral("BeamScheduleMgr: 监听 %1:%2")
                        .arg(localIp).arg(localPort));
    }
}

void BeamScheduleManager::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dgram = m_socket->receiveDatagram();
        if (!dgram.isValid()) continue;
        processData(dgram.data());
    }
}

void BeamScheduleManager::processData(const QByteArray& data)
{
    constexpr int kHeaderSize = static_cast<int>(sizeof(BeamScheduleReport));
    if (data.size() < kHeaderSize) return;

    BeamScheduleReport header;
    memcpy(&header, data.constData(), kHeaderSize);

    if (header.mesID != 0xEE11) return;

    const int slotCount = static_cast<int>(header.slotNum);
    if (slotCount < 0 || slotCount > 1024) return;

    const int expectedSize = kHeaderSize + slotCount * static_cast<int>(sizeof(BeamSlot));
    if (data.size() < expectedSize) return;

    QVector<BeamSlot> beamSlots(slotCount);
    if (slotCount > 0) {
        memcpy(beamSlots.data(), data.constData() + kHeaderSize,
               static_cast<size_t>(slotCount) * sizeof(BeamSlot));
    }

    emit beamScheduleReceived(header, beamSlots);
}
