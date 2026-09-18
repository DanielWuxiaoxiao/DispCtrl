/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-18 23:42:17
 * @Description: 
 */
/*
 * 指控系统通信核心模块。
 *
 * 负责 UDP 组播/单播、DD31 登录发现、DD25/DDA1 周期上报、DDA4 自动与手动
 * 上报、DDA4 会话记录和 PPI 回放。UDP 套接字和 JSONL 文件 I/O 分别运行在独立
 * 工作线程；GUI 线程只维护协议状态和受限界面缓存，不在实时航迹路径阻塞等待。
 */
#ifndef COMMANDCONTROL_MODULE_H
#define COMMANDCONTROL_MODULE_H

#include "commandcontrolconfig.h"
#include "commandcontrolprotocol.h"
#include "commandcontrolrecordstore.h"
#include "commandcontroltrackreportstore.h"

#include "Basic/Protocol.h"

#include <QHash>
#include <QDateTime>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QMetaObject>
#include <QMetaType>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QVector>

#include <array>
#include <cstddef>

class CommandControlWindow;
class CommandControlRecordWriter;
class CommandControlTransport;
class NtpTimeSync;
class CommandControlNetworkMonitor;

struct CommandControlPeer {
    quint32 deviceId = 0;
    QString ip;
    quint16 port = 0;
    QString role;
    QDateTime lastManagementNotice;
    QDateTime lastLinkCheck;
    QDateTime lastEquipmentStatus;
    quint8 healthStatus = 0;
    quint8 radiationStatus = 0;
    bool online = false;
};

class CommandControlModule final : public QObject
{
    Q_OBJECT
public:
    explicit CommandControlModule(QObject* parent = nullptr);
    ~CommandControlModule() override;

    bool init();
    bool isEnabled() const { return m_settings.enabled; }
    bool isReady() const { return m_ready; }
    bool isLoggedIn() const { return m_loggedIn; }
    bool isAutoReportEnabled() const { return m_autoReportEnabled; }
    double autoReportHeightMaxM() const { return m_settings.autoReportHeightMaxM; }
    double autoReportRangeMinM() const { return m_settings.autoReportRangeMinM; }
    double autoReportRangeMaxM() const { return m_settings.autoReportRangeMaxM; }
    double autoReportAzimuthStartDeg() const { return m_settings.autoReportAzimuthStartDeg; }
    double autoReportAzimuthEndDeg() const { return m_settings.autoReportAzimuthEndDeg; }
    bool isManualReportActive(quint32 sourceBatch) const;
    bool isAutoReportActive(quint32 sourceBatch) const;
    bool isTrackReporting(quint32 sourceBatch) const;
    QString statusText() const { return m_statusText; }
    // DD31 是组播发现报文；此文本单独描述发现结果，避免被最后一次登录状态覆盖。
    QString controlDiscoveryText() const;
    QString timeSyncText() const;
    bool isTimeSyncEnabled() const { return m_settings.timeSyncEnabled; }
    bool networkStatusOk(int index) const;
    QString networkStatusText(int index) const;
    QString sessionRecordPath() const { return m_sessionRecordPath; }
    const QVector<CommandControlRecord>& recentRecords() const { return m_recentRecords; }
    QList<CommandControlPeer> peers() const;
    bool isReplayActive() const { return !m_replayRecords.isEmpty(); }
    bool isReplayPaused() const { return m_replayPaused; }
    bool isReplayRebuilding() const { return m_replayRebuilding; }
    int replayIndex() const { return m_replayIndex; }
    int replayCount() const { return m_replayRecords.size(); }

public slots:
    void processTrackPoint(const PointInfo& info);
    void processTargetClassification(TargetClaRes result);
    void toggleManualReport(quint32 sourceBatch);
    /// 仅开启指定普通航迹的手动 DDA4 上报；已开启时保持开启，供 PPI 快捷入口调用。
    bool startManualReport(quint32 sourceBatch, bool allowOfflineTest = false);
    void setAutoReportEnabled(bool enabled);
    /// 运行期更新自动上报判据；仅影响本次运行，配置文件保存默认值。
    void setAutoReportRange(double heightMaxM, double rangeMinM, double rangeMaxM,
                            double azimuthStartDeg, double azimuthEndDeg);
    void requestLogin();
    void requestLogout();
    void requestTimeSync();
    void showControlWindow();
    void startReplay(const QString& recordFilePath);
    void stopReplay();
    void toggleReplayPause();
    void rewindReplay();
    void fastForwardReplay();
    void setRadiationStatus(bool transmitting);
    void updateRadarPosition(double latitude, double longitude, double altitude);
    void setTestRadarPosition(double latitude, double longitude, double altitude);
    void setPendingBeamControl(BeamControl beamControl);
    void setPendingServoControl(ServoControlParam servoControl);
    void processBitReport(BITReport report);
    void processServoControlReply(ServoCtrlRet reply);

signals:
    void statusChanged(const QString& text);
    // 仅投递本机与当前总控的协议要点，供主界面日志栏排障；完整字段始终写项目日志。
    void diagnosticLog(const QString& text);
    void autoReportChanged(bool enabled);
    void autoReportRangeChanged();
    void reportingStateChanged();
    void peersChanged();
    void recordsChanged();
    void networkStatusChanged(int index, bool ok, const QString& text);
    void readyChanged(bool ready);
    void replayStateChanged();
    // 回放使用内部批号隔离不同设备的同名航迹；同时携带原始设备/批号供 PPI 显示。
    void replayPointReady(const PointInfo& info, quint32 sourceDeviceId,
                          quint32 sourceBatch, bool sourceIsLocal);
    // 仅清理回放分配的内部批号，绝不清理同屏实时航迹。
    void replayTrackRemovalRequested(quint32 replayBatch);
    void replayTracksCleared();

private slots:
    void onTransportStarted(bool success, const QString& detail);
    void onTransportSendResult(quint16 messageType, bool success, const QString& detail);
    void onRecordSessionOpened(bool success, const QString& filePath, const QString& detail);
    void onTrackReportSessionOpened(bool success, const QString& filePath, const QString& detail);
    void onRecordWriteFailed(const QString& detail);
    void onReplayRecordsLoaded(const QVector<CommandControlRecord>& records, const QString& detail);
    void handleDatagram(const QByteArray& packet, const QHostAddress& sender, quint16 senderPort);
    void sendLinkCheck();
    void sendEquipmentStatus();
    void retryLogin();
    void updatePeerLiveness();
    void emitNextReplayPoint();
    void rebuildReplayChunk();

private:
    struct ControlEndpoint {
        quint32 id = 0;
        QHostAddress address;
        quint16 port = 0;

        bool isValid() const { return id != 0 && !address.isNull() && port != 0; }
    };

    // 只在 GUI/总控模块线程缓存；实际 JSONL 序列化及文件写入始终投递到记录线程。
    struct TrackHistorySample {
        PointInfo point;
        qint64 observedUtcMs = 0;
        bool radarOriginValid = false;
        double radarLongitudeDeg = 0.0;
        double radarLatitudeDeg = 0.0;
        double radarAltitudeM = 0.0;
    };

    struct TrackReportSession {
        QString id;
        QString mode;
        qint64 sourceTrackStartedUtcMs = 0;
        qint64 reportingStartedUtcMs = 0;
        quint64 nextPointIndex = 0;
    };

    bool startNetwork();
    void startRecordWriter();
    void stopRecordWriter();
    void startTimeSync();
    void startNetworkMonitor();
    void stopNetwork();
    void setStatus(const QString& text);
    CommandControlProtocol::Header nextHeader(quint16 type, quint32 receiverId,
                                               bool requestReceipt, quint8 priority);
    void sendMulticast(const QByteArray& packet, quint16 type);
    void sendUnicast(const QByteArray& packet, quint16 type);
    void handleManagementNode(const QByteArray& packet, const CommandControlProtocol::Header& header,
                              const QHostAddress& sender, quint16 senderPort);
    void handleLoginRequest(const QByteArray& packet, const CommandControlProtocol::Header& header,
                            const QHostAddress& sender, quint16 senderPort);
    void handleLoginReply(const QByteArray& packet, const CommandControlProtocol::Header& header,
                          const QHostAddress& sender, quint16 senderPort);
    void handleLinkCheck(const QByteArray& packet, const CommandControlProtocol::Header& header,
                         const QHostAddress& sender, quint16 senderPort);
    void handleIncomingDda4(const QByteArray& packet, const CommandControlProtocol::Header& header,
                            const QHostAddress& sender, quint16 senderPort);
    void handleIncomingDda1(const QByteArray& packet, const CommandControlProtocol::Header& header,
                            const QHostAddress& sender, quint16 senderPort);
    void beginLogin(CommandControlProtocol::LoginRequestType type, bool allowRetries);
    void reportTrack(const PointInfo& info, bool manualMode);
    void captureTrackHistory(const PointInfo& info);
    void startTrackReportSession(quint32 sourceBatch, const QString& mode);
    void finishTrackReportSession(quint32 sourceBatch, const QString& reason);
    void appendTrackReportPoint(TrackReportSession& session, const TrackHistorySample& sample);
    CommandControlTrackReportRecord makeTrackReportRecord(const TrackReportSession& session,
                                                           const TrackHistorySample& sample,
                                                           const QString& event,
                                                           const QString& stopReason = QString()) const;
    void enqueueTrackReportRecord(const CommandControlTrackReportRecord& record) const;
    /// 范围或开关变更后，以当前缓存的全部普通航迹重新判定自动上报状态。
    void reconcileAutoReporting();
    bool isDrone(quint32 sourceBatch, const PointInfo& info) const;
    bool isWithinAutoReportRange(const PointInfo& info) const;
    bool isConfiguredTestTrack(quint32 sourceBatch) const;
    CommandControlProtocol::Dda4Track makeDda4Track(const PointInfo& info, bool manualMode) const;
    bool targetLla(const PointInfo& info, double& longitudeDeg, double& latitudeDeg, double& altitudeM) const;
    bool replayPointFor(const CommandControlRecord& record, PointInfo& point);
    bool emitReplayRecord();
    void removeStaleReplayTracks(qint64 replayTimeUtcMs);
    void seekReplay(int targetIndex);
    void clearReplayTracks();
    void finishReplay(const QString& statusText);
    void storeDda4(bool outbound, const CommandControlProtocol::Dda4Track& track, const QByteArray& packet,
                   const PointInfo* sourcePoint = nullptr);
    void updatePeer(quint32 deviceId, const QHostAddress& sender, quint16 senderPort,
                    const QString& role, bool managementNotice);
    void schedulePeerRefresh();
    void scheduleRecordRefresh();
    bool shouldLogDda4(bool outbound);
    void logPacketHex(const QString& direction, quint16 type, const QByteArray& packet,
                      const QString& endpoint) const;
    bool isCurrentControl(quint32 deviceId) const;
    void logControlPacket(const QString& direction, const CommandControlProtocol::Header& header,
                          const QString& endpoint, const QString& fields,
                          const QString& uiSummary = QString());
    bool currentRadarPosition(double& longitudeDeg, double& latitudeDeg, double& altitudeM) const;
    bool confirmedScanRange(quint16& azimuthStartDeg, quint16& azimuthEndDeg,
                            qint8& elevationStartDeg, qint8& elevationEndDeg) const;

    CommandControlSettings m_settings;
    QThread m_transportThread;
    CommandControlTransport* m_transport = nullptr;
    QThread m_recordThread;
    CommandControlRecordWriter* m_recordWriter = nullptr;
    NtpTimeSync* m_timeSync = nullptr;
    CommandControlNetworkMonitor* m_networkMonitor = nullptr;
    QTimer m_linkTimer;
    QTimer m_equipmentStatusTimer;
    QTimer m_loginRetryTimer;
    QTimer m_peerLivenessTimer;
    QTimer m_replayTimer;
    QTimer m_replayRebuildTimer;
    QTimer m_peerRefreshTimer;
    QTimer m_recordRefreshTimer;
    CommandControlWindow* m_window = nullptr;
    QHash<quint32, PointInfo> m_latestTracks;
    QHash<quint32, quint8> m_targetClasses;
    QSet<quint32> m_manualBatches;
    QHash<quint32, QVector<TrackHistorySample>> m_trackHistory;
    QHash<quint32, TrackReportSession> m_trackReportSessions;
    QHash<quint32, CommandControlPeer> m_peers;
    QVector<CommandControlRecord> m_recentRecords;
    QVector<CommandControlRecord> m_replayRecords;
    QHash<quint64, quint32> m_replayBatches;
    QHash<quint64, qint64> m_replayLastUpdateUtcMs;
    std::array<bool, 3> m_networkStatusOk{};
    std::array<QString, 3> m_networkStatusText{};
    ControlEndpoint m_controlEndpoint;
    quint32 m_nextReplayBatch = 0x80000000U;
    quint8 m_sequence = 0;
    int m_loginRetriesRemaining = 0;
    bool m_waitingForLoginReply = false;
    bool m_loginRetryExhausted = false;
    bool m_loggedIn = false;
    bool m_logoutRequested = false;
    bool m_autoReportEnabled = true;
    bool m_ready = false;
    QString m_statusText;
    QString m_sessionRecordPath;
    QString m_trackReportSessionPath;
    quint8 m_radiationStatus = 0;
    double m_radarLongitudeDeg = 0.0;
    double m_radarLatitudeDeg = 0.0;
    double m_radarAltitudeM = 0.0;
    double m_selectedRadarYawDeg = 0.0;
    BeamControl m_pendingBeamControl;
    BeamControl m_confirmedBeamControl;
    ServoControlParam m_pendingServoControl;
    bool m_hasRadarPosition = false;
    bool m_hasLiveRadarPosition = false;
    bool m_hasSelectedRadarYaw = false;
    bool m_hasPendingBeamControl = false;
    bool m_waitingForServoReply = false;
    bool m_hasConfirmedBeamControl = false;
    bool m_missingPositionLogged = false;
    bool m_invalidDda1AltitudeLogged = false;
    qint64 m_lastDda4TxLogMs = 0;
    qint64 m_lastDda4RxLogMs = 0;
    int m_replayIndex = 0;
    int m_replayRebuildTargetIndex = 0;
    bool m_replayPaused = false;
    bool m_replayRebuilding = false;
    bool m_resumeReplayAfterRebuild = false;
};

#endif  // COMMANDCONTROL_MODULE_H
