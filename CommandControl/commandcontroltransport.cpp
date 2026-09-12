/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:45
 * @Description: 
 */
#include "commandcontroltransport.h"

#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>

namespace {

constexpr int kMaxDatagramsPerWorkerTurn = 256;

}  // namespace

CommandControlTransport::CommandControlTransport(QObject* parent)
    : QObject(parent)
{
}

void CommandControlTransport::start(const CommandControlSettings& settings)
{
    stop();
    m_settings = settings;
    const QHostAddress multicastAddress(m_settings.multicastGroup);
    if (!multicastAddress.isMulticast()) {
        emit started(false, QStringLiteral("组播地址无效：%1").arg(m_settings.multicastGroup));
        return;
    }

    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(QHostAddress::AnyIPv4, m_settings.localPort,
                        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        const QString error = m_socket->errorString();
        m_socket->deleteLater();
        m_socket = nullptr;
        emit started(false, QStringLiteral("UDP 绑定失败：%1").arg(error));
        return;
    }

    const QHostAddress localAddress(m_settings.localIp);
    QNetworkInterface multicastInterface;
    for (const QNetworkInterface& candidate : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry& entry : candidate.addressEntries()) {
            if (entry.ip() == localAddress) {
                multicastInterface = candidate;
                break;
            }
        }
        if (multicastInterface.isValid()) {
            break;
        }
    }
    if (multicastInterface.isValid()) {
        m_socket->setMulticastInterface(multicastInterface);
    }
    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
    const bool joined = multicastInterface.isValid()
        ? m_socket->joinMulticastGroup(multicastAddress, multicastInterface)
        : m_socket->joinMulticastGroup(multicastAddress);
    if (!joined) {
        const QString error = m_socket->errorString();
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
        emit started(false, QStringLiteral("加入组播组失败：%1").arg(error));
        return;
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &CommandControlTransport::processPendingDatagrams);
    emit started(true, multicastInterface.isValid()
        ? QStringLiteral("UDP 工作线程已绑定并加入指定网卡组播")
        : QStringLiteral("UDP 工作线程已绑定并加入系统默认网卡组播"));
}

void CommandControlTransport::stop()
{
    if (!m_socket) {
        return;
    }
    m_socket->close();
    m_socket->deleteLater();
    m_socket = nullptr;
}

void CommandControlTransport::sendDatagram(const QByteArray& packet, const QHostAddress& destination,
                                           quint16 port, quint16 messageType)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::BoundState) {
        emit datagramSent(messageType, false, QStringLiteral("UDP 未处于绑定状态"));
        return;
    }
    const qint64 written = m_socket->writeDatagram(packet, destination, port);
    if (written != packet.size()) {
        emit datagramSent(messageType, false, m_socket->errorString());
        return;
    }
    emit datagramSent(messageType, true, QString());
}

void CommandControlTransport::processPendingDatagrams()
{
    if (!m_socket) {
        return;
    }
    int processed = 0;
    while (m_socket->hasPendingDatagrams() && processed < kMaxDatagramsPerWorkerTurn) {
        const QNetworkDatagram datagram = m_socket->receiveDatagram();
        if (datagram.isValid()) {
            emit datagramReceived(datagram.data(), datagram.senderAddress(), datagram.senderPort());
        }
        ++processed;
    }
    if (m_socket->hasPendingDatagrams()) {
        QTimer::singleShot(0, this, &CommandControlTransport::processPendingDatagrams);
    }
}
