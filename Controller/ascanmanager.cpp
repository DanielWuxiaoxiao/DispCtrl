/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 16:58:32
 * @Description: 
 */
/**
 * @file ascanmanager.cpp
 * @brief A显数据 UDP 接收管理器实现
 */
#include "ascanmanager.h"
#include <QNetworkDatagram>
#include <QDebug>

AScanManager::AScanManager(QObject* parent)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &AScanManager::onReadyRead);
}

void AScanManager::init(const QString& localIp, quint16 localPort)
{
    if (m_socket->state() == QAbstractSocket::BoundState)
        m_socket->close();

    QHostAddress addr = localIp.isEmpty() ? QHostAddress::AnyIPv4 : QHostAddress(localIp);
    if (!m_socket->bind(addr, localPort,
                        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit logMessage(QStringLiteral("AScanManager: 绑定失败 %1:%2 — %3")
                        .arg(localIp).arg(localPort).arg(m_socket->errorString()));
    } else {
        emit logMessage(QStringLiteral("AScanManager: 监听 %1:%2").arg(localIp).arg(localPort));
    }
}

void AScanManager::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dgram = m_socket->receiveDatagram();
        if (!dgram.isValid()) continue;
        processData(dgram.data());
    }
}

void AScanManager::processData(const QByteArray& data)
{
    constexpr int kHeaderSize = static_cast<int>(sizeof(AScanFrame));

    if (data.size() < kHeaderSize) return;

    AScanFrame frame;
    memcpy(&frame, data.constData(), kHeaderSize);

    if (frame.mesID != 0xEE10) return;

    const int pointCount = frame.pointNum;
    const int expectedMin = kHeaderSize + pointCount * static_cast<int>(sizeof(float));
    if (pointCount <= 0 || pointCount > 4096 || data.size() < expectedMin) {
        emit logMessage(QStringLiteral("AScanManager: 帧格式错误 pointNum=%1 dataSize=%2")
                        .arg(pointCount).arg(data.size()));
        return;
    }

    // 解析 PC后幅度
    QVector<float> pcAmps(pointCount);
    memcpy(pcAmps.data(), data.constData() + kHeaderSize, pointCount * sizeof(float));

    // 解析 MTD后幅度（如果存在）
    QVector<float> mtdAmps;
    const int mtdOffset = kHeaderSize + pointCount * static_cast<int>(sizeof(float));
    if (data.size() >= mtdOffset + pointCount * static_cast<int>(sizeof(float))) {
        mtdAmps.resize(pointCount);
        memcpy(mtdAmps.data(), data.constData() + mtdOffset, pointCount * sizeof(float));
    }

    emit ascanReceived(frame, pcAmps, mtdAmps);
}
