/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-06-17
 * @Description: MQTT publisher for total-control terminal recognition results
 */
#ifndef TOTALCONTROLMQTTCLIENT_H
#define TOTALCONTROLMQTTCLIENT_H

#include <QHash>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include "Basic/Protocol.h"
#include "edgerecognitionresult.h"

class TotalControlMqttClient : public QObject
{
    Q_OBJECT
public:
    explicit TotalControlMqttClient(QObject* parent = nullptr);
    ~TotalControlMqttClient() override;

    bool init();
    bool isEnabled() const { return m_enabled; }

public slots:
    void updateOwnTrack(const PointInfo& info);
    void removeTrack(unsigned int batch);
    void updateEdgeResult(const EdgeRecognitionResult& result);

signals:
    void logMessage(const QString& msg);

private slots:
    void connectToBroker();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void sendPing();
    void publishSnapshot();

private:
    QByteArray encodeString(const QString& text) const;
    QByteArray encodeRemainingLength(int length) const;
    void sendConnectPacket();
    void publishJson(const QByteArray& payload);
    QByteArray buildRecognitionJson(const PointInfo* info,
                                    const QString& trackId,
                                    const EdgeRecognitionResult* edge) const;
    void markDisconnected(const QString& reason);

    QTcpSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QTimer* m_pingTimer = nullptr;
    QTimer* m_publishTimer = nullptr;
    bool m_enabled = false;
    bool m_connected = false;
    QString m_host;
    quint16 m_port = 1883;
    QString m_clientId;
    QString m_topic;
    int m_keepAliveSec = 30;
    int m_publishIntervalMs = 4000;
    quint64 m_publishCount = 0;
    QHash<unsigned int, PointInfo> m_latestTracks;
    QHash<QString, EdgeRecognitionResult> m_edgeResults;
};

#endif // TOTALCONTROLMQTTCLIENT_H
