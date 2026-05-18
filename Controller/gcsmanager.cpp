/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 16:58:32
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:19
 * @Description: 
 */
#include "gcsmanager.h"
#include "Basic/ConfigManager.h"
#include <QDateTime>
#include <QNetworkDatagram>
#include <QtEndian>
#include <cstring>

namespace {

constexpr qint64 kHeartbeatLogIntervalMs = 10000;

}

GCSManager::GCSManager(QObject* parent)
    : QObject(parent)
{
}

GCSManager::~GCSManager()
{
    if (m_socket) {
        m_socket->close();
    }
}

bool GCSManager::init()
{
    const QHostAddress localHost(CF_INS.gcsLocalIp());
    m_gcsHost = QHostAddress(CF_INS.gcsIp());
    m_dstPort = CF_INS.gcsDstPort();
    m_srcPort = CF_INS.gcsSrcPort();

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &GCSManager::onReadyRead);

    bool ok = m_socket->bind(localHost, m_srcPort, QUdpSocket::ShareAddress);
    if (!ok) {
        emit logMessage(QString("[GCS][INIT][ERROR] 绑定 %1:%2 失败: %3")
                            .arg(localHost.toString()).arg(m_srcPort).arg(m_socket->errorString()));
    } else {
        emit logMessage(QString("[GCS][INIT] 初始化完成，本地=%1:%2，目标=%3:%4")
                            .arg(localHost.toString()).arg(m_srcPort)
                            .arg(m_gcsHost.toString()).arg(m_dstPort));
    }
    return ok;
}

// ---------------------------------------------------------------------------
// 发送目标下发帧（0x52）
// ---------------------------------------------------------------------------
void GCSManager::sendTargetAssignment(const GcsTargetParams& params)
{
    if (!m_socket) {
        emit logMessage(QStringLiteral("[GCS][TARGET_SEND][ERROR] socket 未初始化，取消发送"));
        return;
    }

    if (m_socket->state() != QAbstractSocket::BoundState) {
        emit logMessage(QString("[GCS][TARGET_SEND][ERROR] socket 未绑定，state=%1")
                            .arg(static_cast<int>(m_socket->state())));
        return;
    }

    QByteArray payload;
    payload.resize(static_cast<int>(sizeof(GcsTargetParams)));
    std::memcpy(payload.data(), &params, sizeof(GcsTargetParams));

    QByteArray frame = buildFrame(GCS_ADDR_RADAR, GCS_ADDR_GCS, GCS_CMD_TARGET, payload);

    const QHostAddress targetHost = m_gcsHost;
    const quint16 targetPort = m_dstPort;

    const qint64 bytesWritten = m_socket->writeDatagram(frame, targetHost, targetPort);
    if (bytesWritten < 0) {
        emit logMessage(QString("[GCS][TARGET_SEND][ERROR] writeDatagram失败 target=%1:%2 bytes=%3 err=%4")
                            .arg(targetHost.toString())
                            .arg(targetPort)
                            .arg(frame.size())
                            .arg(m_socket->errorString()));
        return;
    }

    emit logMessage(QString("[GCS][TARGET_SEND] id=%1 lon=%2 lat=%3 alt=%4 bytes=%5 target=%6:%7 route=%8")
                        .arg(params.targetId)
                        .arg(params.longitude, 0, 'f', 6)
                        .arg(params.latitude,  0, 'f', 6)
                        .arg(params.altitude,  0, 'f', 1)
                        .arg(bytesWritten)
                        .arg(targetHost.toString())
                        .arg(targetPort)
                        .arg(QStringLiteral("config")));
}

// ---------------------------------------------------------------------------
// 接收处理
// ---------------------------------------------------------------------------
void GCSManager::onReadyRead()
{
    if (!m_socket) {
        emit logMessage(QStringLiteral("[GCS][HEARTBEAT][ERROR] socket 未初始化，无法接收"));
        return;
    }

    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket->receiveDatagram();
        QByteArray data = dg.data();

        quint8 cmd = 0;
        QByteArray params;
        if (!parseFrame(data, cmd, params)) {
            emit logMessage(QStringLiteral("[GCS][RX][ERROR] 收到非法帧，丢弃"));
            continue;
        }

        if (cmd == GCS_CMD_HEARTBEAT) {
            m_lastPeerHost = dg.senderAddress();
            m_lastPeerPort = dg.senderPort();

            // 回复心跳：空参数帧，src/dst互换
            QByteArray reply = buildFrame(GCS_ADDR_RADAR, GCS_ADDR_GCS, GCS_CMD_HEARTBEAT, QByteArray());
            const qint64 replyBytes = m_socket->writeDatagram(reply, dg.senderAddress(), dg.senderPort());
            if (replyBytes < 0) {
                emit logMessage(QString("[GCS][HEARTBEAT][ERROR] 回复失败 sender=%1:%2 err=%3")
                                    .arg(dg.senderAddress().toString())
                                    .arg(dg.senderPort())
                                    .arg(m_socket->errorString()));
            } else {
                const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                const bool shouldLog = (m_lastHeartbeatLogMs == 0)
                                    || (nowMs - m_lastHeartbeatLogMs >= kHeartbeatLogIntervalMs);
                if (shouldLog) {
                    QString suffix;
                    if (m_suppressedHeartbeatCount > 0) {
                        suffix = QString(" suppressed=%1").arg(m_suppressedHeartbeatCount);
                    }
                    emit logMessage(QString("[GCS][HEARTBEAT][RX] sender=%1:%2 replyBytes=%3%4")
                                        .arg(dg.senderAddress().toString())
                                        .arg(dg.senderPort())
                                        .arg(replyBytes)
                                        .arg(suffix));
                    m_lastHeartbeatLogMs = nowMs;
                    m_suppressedHeartbeatCount = 0;
                } else {
                    ++m_suppressedHeartbeatCount;
                }
            }
            emit heartbeatReceived();
        }
    }
}

// ---------------------------------------------------------------------------
// 帧构造
// ---------------------------------------------------------------------------
QByteArray GCSManager::buildFrame(quint8 src, quint8 dst, quint8 cmd, const QByteArray& params)
{
    quint16 len = static_cast<quint16>(params.size());
    quint8 lenL = static_cast<quint8>(len & 0xFF);
    quint8 lenH = static_cast<quint8>((len >> 8) & 0xFF);

    // 校验和：src + dst + cmd + lenL + lenH + params
    quint8 chk = src + dst + cmd + lenL + lenH;
    for (int i = 0; i < params.size(); ++i) {
        chk += static_cast<quint8>(params.at(i));
    }

    QByteArray frame;
    frame.reserve(7 + params.size() + 1);
    frame.append(static_cast<char>(GCS_FRAME_HEAD0));
    frame.append(static_cast<char>(GCS_FRAME_HEAD1));
    frame.append(static_cast<char>(src));
    frame.append(static_cast<char>(dst));
    frame.append(static_cast<char>(cmd));
    frame.append(static_cast<char>(lenL));
    frame.append(static_cast<char>(lenH));
    frame.append(params);
    frame.append(static_cast<char>(chk));
    return frame;
}

// ---------------------------------------------------------------------------
// 帧解析
// ---------------------------------------------------------------------------
bool GCSManager::parseFrame(const QByteArray& data, quint8& outCmd, QByteArray& outParams)
{
    if (data.size() < 8) return false;  // 最短：7字节头 + 1字节校验，无参数

    if (static_cast<quint8>(data.at(0)) != GCS_FRAME_HEAD0
        || static_cast<quint8>(data.at(1)) != GCS_FRAME_HEAD1) {
        return false;
    }

    quint8 src  = static_cast<quint8>(data.at(2));
    quint8 dst  = static_cast<quint8>(data.at(3));
    outCmd      = static_cast<quint8>(data.at(4));
    quint8 lenL = static_cast<quint8>(data.at(5));
    quint8 lenH = static_cast<quint8>(data.at(6));
    quint16 paramLen = static_cast<quint16>(lenL) | (static_cast<quint16>(lenH) << 8);

    if (data.size() < 7 + static_cast<int>(paramLen) + 1) return false;

    outParams = data.mid(7, static_cast<int>(paramLen));

    // 校验和验证
    quint8 chk = src + dst + outCmd + lenL + lenH;
    for (int i = 0; i < outParams.size(); ++i) {
        chk += static_cast<quint8>(outParams.at(i));
    }
    quint8 rxChk = static_cast<quint8>(data.at(7 + static_cast<int>(paramLen)));
    return chk == rxChk;
}
