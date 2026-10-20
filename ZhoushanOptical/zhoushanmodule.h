#ifndef ZHOUSHAN_OPTICAL_MODULE_H
#define ZHOUSHAN_OPTICAL_MODULE_H
// Section 6 only; the existing optical protocols retain their own state and sockets.
#include "Basic/Protocol.h"
#include "protocol.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QVariantList>
#include <QStringList>
#include <atomic>
#include <memory>

class QTcpServer;
class QTcpSocket;
class QWidget;
class QLabel;
class QTableWidget;
class QPushButton;

namespace ZhoushanOptical {
struct Settings {
    QString localIp = "192.168.101.14", peerIp = "192.168.101.10";
    quint16 port = 9010, senderId = 1100, receiverId = 3101;
    quint8 kind = 1;
    bool utc = false;
    int intervalMs = 1000, responseTimeoutMs = 3000, trackTimeoutMs = 10000, logIntervalMs = 5000, maxTracks = 1024;
    qint64 timeOriginMs = 0;
};

class Transport final : public QObject {
    Q_OBJECT
public:
    Transport(const Settings& settings, const std::shared_ptr<std::atomic<quint64>>& allowedToken)
        : m_settings(settings), m_allowedToken(allowedToken) {}
public slots:
    void start();
    void submit(quint64 session, quint64 token, const QByteArray& packet);
    void acknowledge() { m_deliveryPending = false; }
signals:
    void connectionChanged(bool connected, quint64 session, const QString& text);
    void submitted(quint64 token, bool ok, qint64 acceptedAt, const QString& text);
    void received(const QVariantList& packets);
private:
    void accept();
    void drain();
    void deliver();
    void reject(const QString& reason);
    Settings m_settings;
    const std::shared_ptr<std::atomic<quint64>> m_allowedToken;
    QTcpServer* m_server = nullptr;
    QPointer<QTcpSocket> m_socket;
    StreamDecoder m_decoder;
    QTimer* m_timer = nullptr;
    QHash<quint64, QVariantMap> m_latest;
    quint64 m_session = 0, m_rejected = 0, m_accepted = 0, m_discardedBytes = 0, m_responseOrder = 0;
    QString m_error;
    bool m_drainScheduled = false, m_deliveryPending = false;
    QElapsedTimer m_partialTimer;
};

class Module final : public QObject {
    Q_OBJECT
public:
    explicit Module(QObject* parent = nullptr);
    ~Module() override;
    void init();
    bool startReport(int batch);
    void stopReport();
    bool isReporting(int batch) const;
    bool isSelected(int batch) const { return batch >= 0 && batch == m_selected; }
    QString statusText() const;
    void showWindow(QWidget* parent);
public slots:
    void processTrack(const PointInfo& point);
    void updateRadarPosition(double latitude, double longitude, double altitude);
signals:
    void logMessage(const QString& text);
    void reportingStateChanged();
    void statusChanged(bool ok, const QString& text);
    void submitPacket(quint64 session, quint64 token, const QByteArray& packet);
    void acknowledgeReceive();
private:
    friend class ModuleTest;
    friend class ModuleUiTest;
    void tick();
    void consume(const QVariantList& packets);
    void publish();
    void logLimited(const QString& category, const QString& text);
    bool hasTrack(int batch) const;
    QByteArray encode(const PointInfo& point) const;
    void refreshWindow();
    void recordSentPacket(qint64 acceptedAt);
    quint64 nextToken();
    Settings m_settings;
    struct Track { PointInfo point; qint64 at = 0; };
    QHash<int, Track> m_tracks;
    Wgs84Coordinate::Lla m_origin;
    bool m_originValid = false, m_dd05 = false, m_connected = false, m_busy = false;
    bool m_pending = false, m_confirmationSent = false, m_timedOut = false;
    quint64 m_session = 0, m_token = 0, m_writeToken = 0;
    const std::shared_ptr<std::atomic<quint64>> m_allowedToken = std::make_shared<std::atomic<quint64>>(0);
    int m_selected = -1;
    qint64 m_sentAt = 0, m_nextSend = 0;
    quint64 m_sentCount = 0, m_receivedCount = 0, m_rejectedCount = 0, m_discardedBytes = 0;
    QByteArray m_request, m_writePacket;
    QStringList m_lastSentFields;
    QElapsedTimer m_clock;
    QTimer m_timer;
    QThread m_thread;
    QHash<QString, qint64> m_logs;
    QString m_connectionText = QStringLiteral("TCP未启动"), m_resultText = QStringLiteral("未请求"), m_published;
    // History follows manual target selection, never arbitrary/late response arrival order.
    struct ResponseDisplay { int batch = -1; quint64 order = 0; QString text; QStringList fields; };
    ResponseDisplay m_currentResponse, m_previousResponse;
    QPointer<QWidget> m_window;
    QLabel* m_tcpStatusLabel = nullptr;
    QLabel* m_reportStatusLabel = nullptr;
    QLabel* m_localEndpointLabel = nullptr;
    QLabel* m_peerEndpointLabel = nullptr;
    QTableWidget* m_exchangeTable = nullptr;
    QPushButton* m_stopButton = nullptr;
};
}
#endif
