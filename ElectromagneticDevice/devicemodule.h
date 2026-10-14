#ifndef ELECTROMAGNETIC_DEVICE_MODULE_H
#define ELECTROMAGNETIC_DEVICE_MODULE_H
#include "deviceprotocol.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include <QElapsedTimer>
#include <QPointer>
#include <QQueue>
#include <QMap>
#include <QSet>
#include <array>
class QDialog;
class QWidget;
class QLabel;
class ElectromagneticTransport;

class ElectromagneticDeviceModule final : public QObject {
    Q_OBJECT
public:
    explicit ElectromagneticDeviceModule(QObject* parent = nullptr);
    ~ElectromagneticDeviceModule() override;
    void init();
    bool isEnabled() const { return m_enabled; }
    bool isUiEnabled() const { return m_uiEnabled; }
    void showWindow(QWidget* parent = nullptr);
    bool isManualReportActive(quint32 batch) const { return m_manualTracks.contains(batch); }
    bool isAutoReportActive(quint32 batch) const {
        return batch <= 65535 && m_reportTracks && m_autoTracks && m_lastTracks.contains(quint16(batch));
    }
    bool toggleManualReport(quint32 batch);
    bool sendDetection(const PointInfo& point);
    void setInitialSoftwareStatus(MonitorParam report);
public slots:
    void prepareTestPoint(PointInfo point);
    void processDetectionPoint(PointInfo point);
    void processTrackPoint(PointInfo point);
    void setAutoTracks(bool enabled);
    void setAutoDetections(bool enabled);
    void setTestRadarPosition(double latitude, double longitude, double altitude);
    void processDetectionFrame(const QByteArray& frame);
    void processTrackFrame(const QByteArray& frame);
    void updateRadarPosition(double latitude, double longitude, double altitude);
    void processBitReport(BITReport report);
    void processSoftwareReport(MonitorParam report);
    void processTargetClassification(TargetClaRes report);
signals:
    void reportingStateChanged();
    void outbound(const QList<QByteArray>& packets);
    void controlRequested(quint16 message, const QByteArray& payload);
    void statusChanged(const QString& text);
    void linkStateChanged(bool ok, const QString& text);
    void diagnosticLog(const QString& text);
private:
    friend class ElectromagneticReportingTest;
    void receive(const QByteArray& packet);
    void flush();
    void pollNetwork();
    void finishPing(bool ok);
    void updateUi();
    enum class Warning { General, Origin, DetectionFrame, TrackFrame, Detection, Track,
                         Queue, Transmit, Software, Receive, Sequence, Count };
    void warn(const QString& message, Warning category = Warning::General);
    void logSummary();
    void logPoint(const char* action, const PointInfo& point, const QByteArray& record, bool manual = false);
    QString rejectionReason(const PointInfo& point) const;
    bool isTestPoint(const PointInfo& point) const;
    QByteArray wrap(quint16 message, const QByteArray& payload);
    QString textValue(const QString& key, const QString& fallback) const;
    int numberValue(const QString& key, int fallback, int low, int high) const;
    bool flagValue(const QString& key, bool fallback) const;
    bool originFor(const PointInfo& point, Wgs84Coordinate::Lla& origin) const;
    QByteArray detectionKey(const PointInfo& point) const;
    QByteArray encodeDetection(const PointInfo& point) const;
    QByteArray encodeTrack(const PointInfo& point) const;
    bool enqueueDetection(const PointInfo& point, bool automatic);

    bool m_enabled = false, m_uiEnabled = true, m_validConfig = true;
    bool m_reportDetections = true, m_reportTracks = true, m_remoteControl = true;
    bool m_reportStatus = true, m_networkEnabled = true;
    bool m_autoTracks = false, m_autoDetections = false, m_testOriginValid = false;
    bool m_ready = false, m_busy = false, m_originValid = false;
    bool m_localOk = false, m_peerOk = false, m_pingRunning = false;
    QString m_localIp, m_peerIp, m_transportText, m_peerText, m_lastCommand;
    quint16 m_localPort = 8022, m_peerPort = 6022, m_localId = 0xBB04, m_peerId = 0xBB08;
    quint32 m_sequence = 0, m_lastSequence = 0;
    quint16 m_trackCursor = 0;
    bool m_receivedSequence = false;
    int m_receiveTimeout = 15000, m_failures = 0;
    int m_statusInterval = 1000;
    qint64 m_lastReceive = -1, m_lastStatus = -1;
    std::array<qint64, size_t(Warning::Count)> m_warningTimes{};
    std::array<quint64, size_t(Warning::Count)> m_warningSuppressed{};
    bool m_dataLogEnabled = true;
    int m_logSummaryMs = 5000, m_logSampleMs = 1000;
    qint64 m_lastLogSummary = 0, m_lastTrackLog = -1000, m_lastDetectionLog = -1000;
    qint64 m_lastTxLog = -1000, m_inFlightStarted = 0, m_maxTxMs = 0;
    qint64 m_lastOriginLog = -5000;
    quint32 m_inFlightFirstSequence = 0;
    int m_inFlightPackets = 0;
    QString m_lastLinkLog;
    QString m_lastStatusSourceLog;
    qint64 m_lastStatusLog = -5000;
    quint64 m_detInputs = 0, m_trackInputs = 0, m_testInputs = 0;
    quint64 m_detQueued = 0, m_trackQueued = 0, m_removals = 0;
    quint64 m_flushNotReady = 0, m_flushBusy = 0;
    std::array<quint64, 10> m_messageCounts{};
    qint64 m_lastDetection = -1, m_lastTrack = -1, m_lastBit = -1, m_lastSoftware = -1;
    quint64 m_sent = 0, m_failed = 0, m_received = 0, m_rejected = 0, m_dropped = 0;
    Wgs84Coordinate::Lla m_origin;
    Wgs84Coordinate::Lla m_testOrigin;
    QMap<quint16, trackInfo> m_trackMetadata;
    QMap<quint16, QByteArray> m_testTrackKeys;
    QSet<QByteArray> m_testDetectionKeys;
    QQueue<QByteArray> m_testDetectionOrder;
    QMap<quint16, PointInfo> m_trackPoints;
    QSet<quint32> m_manualTracks, m_reportedTracks;
    QMap<QByteArray, detInfo> m_detectionMetadata;
    QQueue<QByteArray> m_detectionMetadataOrder;
    QQueue<QByteArray> m_detections;
    QQueue<QByteArray> m_manualDetections;
    QMap<quint16, QByteArray> m_pendingTracks, m_lastTracks;
    QMap<quint16, quint8> m_targetClasses;
    QByteArray m_bit, m_software;
    QByteArray m_lastUiSoftwareLog;
    QThread m_thread;
    ElectromagneticTransport* m_transport = nullptr;
    QTimer m_flushTimer, m_networkTimer, m_uiTimer, m_pingTimeout;
    QProcess m_ping;
    QElapsedTimer m_clock;
    QPointer<QDialog> m_window;
    std::array<QLabel*, 6> m_labels{};
};
#endif
