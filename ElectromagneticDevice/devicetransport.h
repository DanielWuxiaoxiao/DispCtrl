#ifndef ELECTROMAGNETIC_DEVICE_TRANSPORT_H
#define ELECTROMAGNETIC_DEVICE_TRANSPORT_H
#include <QObject>
#include <QHostAddress>
#include <QList>
#include <QByteArray>
#include <QElapsedTimer>
class QUdpSocket;

// Socket belongs exclusively to the worker thread; one bounded batch is in flight.
class ElectromagneticTransport final : public QObject {
    Q_OBJECT
public:
    ElectromagneticTransport(const QString& localIp, quint16 localPort,
                             const QString& peerIp, quint16 peerPort);
public slots:
    void start();
    void stop();
    void sendBatch(const QList<QByteArray>& packets);
signals:
    void ready(bool ok, const QString& detail);
    void received(const QByteArray& packet);
    void completed(int sent, int failed, const QString& error);
private:
    void receive();
    QHostAddress m_local, m_peer;
    quint16 m_localPort, m_peerPort;
    QUdpSocket* m_socket = nullptr;
    QElapsedTimer m_clock;
    qint64 m_lastRejectLog = -5000;
    quint64 m_wrongPeer = 0, m_badDatagram = 0;
};
#endif
