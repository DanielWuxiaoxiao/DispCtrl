#include "devicetransport.h"
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>
#include "Basic/log.h"

ElectromagneticTransport::ElectromagneticTransport(const QString& localIp, quint16 localPort,
                                                 const QString& peerIp, quint16 peerPort)
    : m_local(localIp), m_peer(peerIp), m_localPort(localPort), m_peerPort(peerPort) { m_clock.start(); }

void ElectromagneticTransport::start()
{
    stop();
    m_socket = new QUdpSocket(this);
    const bool ok = m_socket->bind(m_local, m_localPort, QUdpSocket::DontShareAddress);
    if (ok) connect(m_socket, &QUdpSocket::readyRead, this, &ElectromagneticTransport::receive);
    emit ready(ok, ok ? QStringLiteral("UDP 已绑定 %1:%2").arg(m_local.toString()).arg(m_localPort)
                      : QStringLiteral("UDP 绑定失败：%1").arg(m_socket->errorString()));
}

void ElectromagneticTransport::stop()
{
    if (m_socket) { delete m_socket; m_socket = nullptr; }
}

void ElectromagneticTransport::sendBatch(const QList<QByteArray>& packets)
{
    int sent = 0, failed = 0;
    QString error;
    for (const QByteArray& packet : packets) {
        if (!m_socket || m_socket->state() != QAbstractSocket::BoundState) {
            ++failed; error = QStringLiteral("UDP 未就绪");
        } else if (m_socket->writeDatagram(packet, m_peer, m_peerPort) != packet.size()) {
            ++failed; error = m_socket->errorString();
        } else ++sent;
    }
    emit completed(sent, failed, error);
}

void ElectromagneticTransport::receive()
{
    // Limit work per event-loop turn. Ignore other senders before crossing threads.
    int count = 0;
    while (m_socket && m_socket->hasPendingDatagrams() && count++ < 64) {
        const auto packet = m_socket->receiveDatagram(1473);
        const bool wrongPeer = packet.senderAddress() != m_peer || packet.senderPort() != m_peerPort;
        if (packet.isValid() && !wrongPeer && packet.data().size() <= 1472) emit received(packet.data());
        else {
            if (wrongPeer) ++m_wrongPeer; else ++m_badDatagram;
            if (m_clock.elapsed() - m_lastRejectLog >= 5000) {
                m_lastRejectLog = m_clock.elapsed();
                LOG_WARNING(QStringLiteral("[电磁设备][RX_TRANSPORT_REJECT] actual=%1:%2 expected=%3:%4 bytes=%5 valid=%6 wrongPeerTotal=%7 badDatagramTotal=%8")
                    .arg(packet.senderAddress().toString()).arg(packet.senderPort()).arg(m_peer.toString()).arg(m_peerPort)
                    .arg(packet.data().size()).arg(packet.isValid()).arg(m_wrongPeer).arg(m_badDatagram));
            }
        }
    }
    if (m_socket && m_socket->hasPendingDatagrams()) QTimer::singleShot(0, this, &ElectromagneticTransport::receive);
}
