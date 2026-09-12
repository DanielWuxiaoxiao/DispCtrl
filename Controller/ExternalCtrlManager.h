/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-12-25 16:19:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:46
 * @Description: 
 */
#ifndef EXTERNALCTRLMANAGER_H
#define EXTERNALCTRLMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QVector>
#include <QByteArray>
#include <QMetaType>
#include "Basic/ConfigManager.h"
#include "Basic/Protocol.h"

// ========== 外部雷控 AD 数据解析结构（含采样） ==========
struct ExternalAdSample {
    qint16 i{0};
    qint16 q{0};
};

struct ExternalAdFrame {
    ExternalAdHeader header{};
    QVector<ExternalAdSample> samples;  // 大小 = beamCount * samplesPerPulse
    ExternalAdTrailerReserve trailer{};
    unsigned tail{EXTERNAL_AD_TAIL};
};

Q_DECLARE_METATYPE(ExternalAdFrame)
Q_DECLARE_METATYPE(ExternalSystemControl512)

/**
 * @brief 外部雷控/调度链路管理器
 * 负责向外部雷控发送系统控制/伺服控制空壳数据，接收回送ACK。
 */
class ExternalCtrlManager : public QObject {
    Q_OBJECT
public:
    explicit ExternalCtrlManager(QObject* parent = nullptr);

    // 发送512B系统控制表（调用方可传入 512B 完整帧或 504B 内容）
    bool sendSystemControl(const QByteArray& frame512or504);

    // 发送 504B 系统控制内容，自动拼接头/尾
    bool sendSystemControlPayload(const QByteArray& payload504);

    // 发送32B伺服控制（可用ExternalServoCmd32填充）
    bool sendServoControl(const QByteArray& frame32);

    // 发送 AD 数据帧（含头/尾/采样）
    bool sendAdFrame(const ExternalAdFrame& frame);

    // 同报文发送：系统控制 504B + AD 数据帧（顺序为系统控制帧在前）
    bool sendSystemControlWithAd(const QByteArray& payload504, const ExternalAdFrame& frame);

signals:
    void systemCtrlAck(const QByteArray& ack64);
    void servoCtrlAck(const ExternalServoAck32& ack32);
    void systemControlFrameReceived(const ExternalSystemControl512& frame);
    void adFrameReceived(const ExternalAdFrame& frame);
    void externalLog(const QString& msg);

private slots:
    void onReadyRead();

private:
    void initSockets();
    void bindAckSocket();
    bool sendDatagram(const QByteArray& data);
    QByteArray buildSystemControlFrame(const QByteArray& payload504) const;
    bool isValidSystemControlFrame(const QByteArray& frame) const;
    bool tryParseSystemControl(const QByteArray& frame, ExternalSystemControl512& outFrame) const;
    bool tryParseAdFrame(const QByteArray& frame, int offset, ExternalAdFrame& outFrame) const;
    QByteArray encodeAdFrame(const ExternalAdFrame& frame) const;

    QUdpSocket* sendSocket{nullptr};
    QUdpSocket* recvSocket{nullptr};
    QHostAddress targetHost;
    quint16 targetPort{0};
    quint16 sourcePort{0};
    quint16 ackPort{0};
    quint16 srcId{0};
    quint16 dstId{0};
};

#endif // EXTERNALCTRLMANAGER_H
