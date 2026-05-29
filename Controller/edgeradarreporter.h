/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-05-29 09:31:02
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-29 09:49:42
 * @Description: 
 */
#ifndef EDGERADARREPORTER_H
#define EDGERADARREPORTER_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTimer>

#include "Basic/Protocol.h"

class EdgeRadarReporter : public QObject
{
    Q_OBJECT
public:
    explicit EdgeRadarReporter(QObject* parent = nullptr);
    ~EdgeRadarReporter() override;

    bool init();
    bool isEnabled() const { return m_enabled; }

public slots:
    void reportTrackPoint(const PointInfo& info);

signals:
    void logMessage(const QString& msg);

private slots:
    void sendHeartbeat();

private:
    QByteArray buildTargetJson(const PointInfo& info) const;
    QByteArray buildHeartbeatJson() const;
    void logTargetSent(const PointInfo& info, const QByteArray& payload, qint64 bytesWritten);
    void logWriteError(const QString& type, const QString& err);

    QUdpSocket* m_socket = nullptr;
    QTimer* m_heartbeatTimer = nullptr;
    bool m_enabled = false;
    QHostAddress m_targetHost;
    quint16 m_targetPort = 0;
    QHostAddress m_localHost;
    quint16 m_localPort = 0;
    int m_heartbeatIntervalMs = 5000;
    quint64 m_targetSentCount = 0;
};

#endif // EDGERADARREPORTER_H
