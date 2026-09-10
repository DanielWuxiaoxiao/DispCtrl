/*
 * Health-page network monitor for the radar-array and laser networks.
 * It intentionally reports local interface readiness separately from ICMP
 * reachability: UDP has no connection handshake and a successful ping is not
 * evidence that the 9009 application endpoint accepted a frame.
 */
#ifndef SUBSYSTEMNETWORKMONITOR_H
#define SUBSYSTEMNETWORKMONITOR_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class SubsystemNetworkMonitor final : public QObject
{
    Q_OBJECT
public:
    enum StatusIndex {
        RadarLocal = 0,
        LaserLocal,
        RadarPeer,
        LaserPeer,
        StatusCount
    };

    explicit SubsystemNetworkMonitor(QObject* parent = nullptr);
    ~SubsystemNetworkMonitor() override;

    void start();

signals:
    void statusChanged(int index, bool ok, const QString& text);

private:
    void poll();
    void ping(int endpoint);
    void finishPing(int endpoint, bool reachable, const QString& reason);
    QString localInterfaceError(const QString& ip) const;
    void publishPeerUnavailable(int endpoint, const QString& reason);

    QProcess* m_ping[2] = {nullptr, nullptr};
    QTimer* m_timeout[2] = {nullptr, nullptr};
    QTimer m_pollTimer;
    QString m_localIp[2];
    QString m_peerIp[2];
    bool m_localReady[2] = {false, false};
    bool m_pingRunning[2] = {false, false};
    int m_consecutiveFailures[2] = {0, 0};
};

#endif // SUBSYSTEMNETWORKMONITOR_H
