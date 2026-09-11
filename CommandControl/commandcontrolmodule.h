/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:18:30
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
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

class CommandControlWindow;
class CommandControlRecordWriter;
class CommandControlTransport;

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
    bool isReady() const { return m_ready; }
    bool isAutoReportEnabled() const { return m_autoReportEnabled; }
    bool isManualReportActive(quint32 sourceBatch) const;
    QString statusText() const { return m_statusText; }
    QString sessionRecordPath() const { return m_sessionRecordPath; }
    const QVector<CommandControlRecord>& recentRecords() const { return m_recentRecords; }
    QList<CommandControlPeer> peers() const;

public slots:
    void processTrackPoint(const PointInfo& info);
    void processTargetClassification(TargetClaRes result);
    void toggleManualReport(quint32 sourceBatch);
    void setAutoReportEnabled(bool enabled);
    void requestLogin();
    void requestLogout();
    void showControlWindow();
    void startReplay(const QString& recordFilePath);
    void stopReplay();
    void setRadiationStatus(bool transmitting);
    void updateRadarPosition(double latitude, double longitude, double altitude);
    void setPendingBeamControl(BeamControl beamControl);
    void setPendingServoControl(ServoControlParam servoControl);
    void processBitReport(BITReport report);
    void processServoControlReply(ServoCtrlRet reply);

signals:
    void statusChanged(const QString& text);
    // 仅投递本机与当前总控的协议要点，供主界面日志栏排障；完整字段始终写项目日志。
    void diagnosticLog(const QString& text);
    void autoReportChanged(bool enabled);
    void peersChanged();
    void recordsChanged();
    void readyChanged(bool ready);
    void replayPointReady(const PointInfo& info);

private slots:
    void onTransportStarted(bool success, const QString& detail);
    void onTransportSendResult(quint16 messageType, bool success, const QString& detail);
    void onRecordSessionOpened(bool success, const QString& filePath, const QString& detail);
    void onRecordWriteFailed(const QString& detail);
    void onReplayRecordsLoaded(const QVector<CommandControlRecord>& records, const QString& detail);
    void handleDatagram(const QByteArray& packet, const QHostAddress& sender, quint16 senderPort);
    void sendLinkCheck();
    void sendEquipmentStatus();
    void retryLogin();
    void updatePeerLiveness();
    void emitNextReplayPoint();

private:
    struct ControlEndpoint {
        quint32 id = 0;
        QHostAddress address;
        quint16 port = 0;

        bool isValid() const { return id != 0 && !address.isNull() && port != 0; }
    };

    bool startNetwork();
    void startRecordWriter();
    void stopRecordWriter();
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
    bool isDrone(quint32 sourceBatch, const PointInfo& info) const;
    quint32 localBatchFor(quint32 sourceBatch);
    CommandControlProtocol::Dda4Track makeDda4Track(const PointInfo& info, bool manualMode) const;
    bool targetLla(const PointInfo& info, double& longitudeDeg, double& latitudeDeg, double& altitudeM) const;
    PointInfo replayPointFor(const CommandControlRecord& record);
    void storeDda4(bool outbound, const CommandControlProtocol::Dda4Track& track, const QByteArray& packet);
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
    QTimer m_linkTimer;
    QTimer m_equipmentStatusTimer;
    QTimer m_loginRetryTimer;
    QTimer m_peerLivenessTimer;
    QTimer m_replayTimer;
    QTimer m_peerRefreshTimer;
    QTimer m_recordRefreshTimer;
    CommandControlWindow* m_window = nullptr;
    QHash<quint32, PointInfo> m_latestTracks;
    QHash<quint32, quint8> m_targetClasses;
    QHash<quint32, quint32> m_localBatches;
    QSet<quint32> m_manualBatches;
    QHash<quint32, CommandControlPeer> m_peers;
    QVector<CommandControlRecord> m_recentRecords;
    QVector<CommandControlRecord> m_replayRecords;
    QHash<quint64, quint32> m_replayBatches;
    ControlEndpoint m_controlEndpoint;
    quint32 m_nextLocalBatch = 1;
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
    quint8 m_radiationStatus = 0;
    double m_radarLongitudeDeg = 0.0;
    double m_radarLatitudeDeg = 0.0;
    double m_radarAltitudeM = 0.0;
    double m_selectedRadarYawDeg = 0.0;
    BeamControl m_pendingBeamControl;
    BeamControl m_confirmedBeamControl;
    ServoControlParam m_pendingServoControl;
    bool m_hasRadarPosition = false;
    bool m_hasSelectedRadarYaw = false;
    bool m_hasPendingBeamControl = false;
    bool m_waitingForServoReply = false;
    bool m_hasConfirmedBeamControl = false;
    bool m_missingPositionLogged = false;
    qint64 m_lastDda4TxLogMs = 0;
    qint64 m_lastDda4RxLogMs = 0;
    int m_replayIndex = 0;
};

#endif  // COMMANDCONTROL_MODULE_H
