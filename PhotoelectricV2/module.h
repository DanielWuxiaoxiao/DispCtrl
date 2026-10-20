#ifndef PHOTOELECTRIC_V2_MODULE_H
#define PHOTOELECTRIC_V2_MODULE_H
#include "Basic/Protocol.h"
#include "Basic/wgs84coordinate.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QQueue>
#include <QVariantList>
#include <utility>

class QWidget;
class QLabel;
class QTableWidget;
class QUdpSocket;

namespace PhotoelectricV2 {
struct Settings {
    QString localIp, peerIp;
    quint16 targetPort = 10100, statusPort = 11100, controlLocalPort = 10101, controlPort = 21001;
    quint16 peerTargetPort = 0, peerStatusPort = 0;
    quint32 device = 1, terminal = 0;
    int intervalMs = 1000, timeoutMs = 5000, logIntervalMs = 5000;
    int trackTtlMs = 10000, maxTracks = 1024;
    qint64 timeOriginMs = 0;
};

class Worker final : public QObject {
    Q_OBJECT
public:
    explicit Worker(Settings settings) : m_settings(std::move(settings)) {}
public slots:
    void start();
    void send(const QList<QByteArray>& frames);
    void acknowledge() { m_deliveryPending = false; }
signals:
    void ready(bool ok, const QString& text);
    void sent(int count, const QString& error);
    void received(const QVariantList& frames);
private:
    void receive(QUdpSocket* socket, bool statusChannel);
    void deliver();
    Settings m_settings;
    QUdpSocket* m_control = nullptr;
    QUdpSocket* m_target = nullptr;
    QUdpSocket* m_status = nullptr;
    QTimer* m_deliveryTimer = nullptr;
    QHash<QString, QVariantMap> m_latest;
    bool m_drainScheduled[2] = {false, false};
    quint64 m_rejected = 0;
    quint64 m_received = 0;
    QString m_lastError;
    bool m_deliveryPending = false;
};

class Module final : public QObject {
    Q_OBJECT
public:
    explicit Module(QObject* parent = nullptr);
    ~Module() override;
    void init();
    bool isReady() const { return m_ready; }
    bool hasTrack(int batch) const;
    bool isReporting(int batch) const;
    bool startReport(int batch);
    void stopReport();
    bool isAutoReport() const { return m_auto; }
    void setAutoReport(bool enabled);
    void showWindow(QWidget* parent);
    QString statusText() const;
public slots:
    void processTrack(const PointInfo& point);
    void updateRadarPosition(double latitude, double longitude, double altitude);
signals:
    void logMessage(const QString& text);
    void reportingStateChanged();
    void statusChanged(bool ok, const QString& text);
    void sendFrames(const QList<QByteArray>& frames);
    void acknowledgeReceive();
private:
    friend class ModuleTest;
    void flush();
    void consume(const QVariantList& frames);
    void refreshWindow();
    void publishStatus();
    void logLimited(const QString& category, const QString& text);
    QByteArray encode(const PointInfo& point) const;
    Settings m_settings;
    struct Track { PointInfo point; qint64 updated = 0; };
    QHash<int, Track> m_tracks;
    QHash<quint32, QVariantMap> m_targets;
    QVariantMap m_deviceState, m_heartbeat;
    QThread m_thread;
    QTimer m_timer;
    QElapsedTimer m_clock;
    Wgs84Coordinate::Lla m_origin;
    bool m_originValid = false, m_dd05 = false, m_ready = false, m_busy = false, m_auto = false;
    bool m_reportingDirty = false;
    int m_selected = -1;
    qint64 m_lastTarget = -1, m_lastStatus = -1, m_lastHeartbeat = -1;
    qint64 m_lastTx = -1;
    qint64 m_lastUi = -1;
    quint64 m_rxCount = 0, m_txCount = 0, m_rejectedCount = 0;
    QHash<QString, qint64> m_lastLogs;
    QQueue<QByteArray> m_controls;
    QString m_socketStatus = QStringLiteral("UDP未就绪"), m_publishedStatus;
    QPointer<QWidget> m_window;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_deviceLabel = nullptr;
    QTableWidget* m_targetTable = nullptr;
};
}
#endif
