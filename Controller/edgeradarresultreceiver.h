/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-06-24 00:30:34 -0700
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:48
 * @Description: 
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
