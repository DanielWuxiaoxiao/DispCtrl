/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-06-17
 * @Description: Edge terminal optical recognition result receiver
 */
#ifndef EDGERADARRESULTRECEIVER_H
#define EDGERADARRESULTRECEIVER_H

#include <QHostAddress>
#include <QObject>
#include <QUdpSocket>

#include "edgerecognitionresult.h"

class EdgeRadarResultReceiver : public QObject
{
    Q_OBJECT
public:
    explicit EdgeRadarResultReceiver(QObject* parent = nullptr);
    ~EdgeRadarResultReceiver() override;

    bool init();
    bool isEnabled() const { return m_enabled; }

signals:
    void edgeResultReceived(const EdgeRecognitionResult& result);
    void logMessage(const QString& msg);

private slots:
    void processPendingDatagrams();

private:
    void parseDatagram(const QByteArray& payload, const QHostAddress& sender, quint16 senderPort);

    QUdpSocket* m_socket = nullptr;
    bool m_enabled = false;
    QHostAddress m_localHost;
    quint16 m_localPort = 9002;
};

#endif // EDGERADARRESULTRECEIVER_H
