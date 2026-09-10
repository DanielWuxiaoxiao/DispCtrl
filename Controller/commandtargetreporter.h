/* Independent command-and-control reporting module for identified drone tracks. */
#ifndef COMMANDTARGETREPORTER_H
#define COMMANDTARGETREPORTER_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

#include "Basic/Protocol.h"

class CommandTargetReporter final : public QObject
{
    Q_OBJECT
public:
    explicit CommandTargetReporter(QObject* parent = nullptr);

    bool isRuntimeEnabled() const { return m_runtimeEnabled; }
    bool isNetworkReady() const { return m_networkReady; }
    QString networkStatusText() const { return m_networkStatusText; }

public slots:
    // Called for every normal-track point.  Each point of an identified drone is reported.
    void processTrackPoint(const PointInfo& info);
    // Classification can arrive after its matching track point; this publishes the cached point immediately.
    void processTargetClassification(TargetClaRes result);
    // UI-only switch. It does not rewrite config.toml; restart restores the configured default.
    void setRuntimeEnabled(bool enabled);

signals:
    void networkStatusChanged(bool ready, const QString& detail);
    void runtimeEnabledChanged(bool enabled);
    void logMessage(const QString& message);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(QAbstractSocket::SocketError error);

private:
    enum class Transport { Udp, Tcp };
    enum class Payload { Json, Binary };

    struct Settings {
        Transport transport = Transport::Udp;
        Payload payload = Payload::Json;
        bool bigEndian = false;
        QString localIp = QStringLiteral("0.0.0.0");
        quint16 localPort = 0;
        QString remoteIp = QStringLiteral("127.0.0.1");
        quint16 remotePort = 21002;
        int tcpReconnectIntervalMs = 3000;
    };

    struct TimedTrack {
        PointInfo info;
        quint64 timestampUtcMs = 0;
    };

    void loadSettings();
    void startTransport();
    void stopTransport();
    void beginTcpConnection();
    void scheduleTcpReconnect();
    void flushTcpPending();
    void sendPayload(const QByteArray& payload);
    void reportIfDrone(unsigned int batch);
    QByteArray buildPayload(const TimedTrack& track) const;
    bool targetLla(const PointInfo& info, double& longitudeDeg, double& latitudeDeg, double& altitudeM) const;
    bool isDrone(unsigned int batch, const PointInfo& info) const;
    void setNetworkStatus(bool ready, const QString& detail);

    Settings m_settings;
    QUdpSocket m_udpSocket;
    QTcpSocket m_tcpSocket;
    QTimer m_tcpReconnectTimer;
    QQueue<QByteArray> m_tcpPending;
    bool m_runtimeEnabled = false;
    bool m_udpBound = false;
    bool m_networkReady = false;
    QString m_networkStatusText;
    QHash<unsigned int, TimedTrack> m_latestTracks;
    QHash<unsigned int, unsigned char> m_targetClasses;
};

#endif // COMMANDTARGETREPORTER_H
