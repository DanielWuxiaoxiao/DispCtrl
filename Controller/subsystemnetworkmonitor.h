/*
 * Health-page network monitor for the radar-array, laser and optical V2 networks.
 * It intentionally reports local interface readiness separately from ICMP
 * reachability: UDP has no connection handshake and a successful ping is not
 * evidence that an application endpoint accepted or executed a frame.
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
        OpticalLocal,
        OpticalPeer,
        ZhoushanLocal,
        ZhoushanPeer,
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

    QProcess* m_ping[4] = {};
    QTimer* m_timeout[4] = {};
    QTimer m_pollTimer;
    QString m_localIp[4];
    QString m_peerIp[4];
    bool m_localReady[4] = {};
    bool m_pingRunning[4] = {};
    int m_consecutiveFailures[4] = {};
};

#endif // SUBSYSTEMNETWORKMONITOR_H
