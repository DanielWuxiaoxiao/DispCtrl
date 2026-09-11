/* 总控通信专用 UDP 工作对象：仅在其所属工作线程中操作 QUdpSocket。 */
#ifndef COMMANDCONTROL_TRANSPORT_H
#define COMMANDCONTROL_TRANSPORT_H

#include "commandcontrolconfig.h"

#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QtGlobal>

class QUdpSocket;

class CommandControlTransport final : public QObject
{
    Q_OBJECT
public:
    explicit CommandControlTransport(QObject* parent = nullptr);

public slots:
    void start(const CommandControlSettings& settings);
    void stop();
    void sendDatagram(const QByteArray& packet, const QHostAddress& destination,
                      quint16 port, quint16 messageType);

signals:
    void started(bool success, const QString& detail);
    void datagramReceived(const QByteArray& packet, const QHostAddress& sender, quint16 senderPort);
    void datagramSent(quint16 messageType, bool success, const QString& detail);

private slots:
    void processPendingDatagrams();

private:
    QUdpSocket* m_socket = nullptr;
    CommandControlSettings m_settings;
};

#endif  // COMMANDCONTROL_TRANSPORT_H
