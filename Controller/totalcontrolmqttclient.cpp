/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-06-17
 * @Description: MQTT publisher for total-control terminal recognition results
 */
#include "totalcontrolmqttclient.h"

#include "Basic/ConfigManager.h"

#include <QDateTime>
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStringList>

namespace {
constexpr int kReconnectIntervalMs = 3000;
constexpr int kMqttProtocolLevel = 4; // MQTT 3.1.1
constexpr quint8 kPacketConnect = 0x10;
constexpr quint8 kPacketConnack = 0x20;
constexpr quint8 kPacketPublishQoS0 = 0x30;
constexpr quint8 kPacketPingReq = 0xC0;
}

TotalControlMqttClient::TotalControlMqttClient(QObject* parent)
    : QObject(parent)
{
}

TotalControlMqttClient::~TotalControlMqttClient()
{
    if (m_reconnectTimer) m_reconnectTimer->stop();
    if (m_pingTimer) m_pingTimer->stop();
    if (m_publishTimer) m_publishTimer->stop();
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->close();
    }
}

bool TotalControlMqttClient::init()
{
    m_enabled = CF_INS.totalControlMqttEnabled(false);
    if (!m_enabled) {
        emit logMessage(QStringLiteral("[TOTAL_MQTT][INIT] disabled by config"));
        return false;
    }

    m_host = CF_INS.totalControlMqttHost("192.168.1.30");
    m_port = CF_INS.totalControlMqttPort(1883);
    m_clientId = CF_INS.totalControlMqttClientId("DispCtrl-X576");
    m_topic = CF_INS.totalControlMqttTopic("x576/target/result");
    m_keepAliveSec = qMax(5, CF_INS.totalControlMqttKeepAliveSec(30));
    m_publishIntervalMs = qMax(500, CF_INS.totalControlMqttPublishIntervalMs(4000));

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &TotalControlMqttClient::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TotalControlMqttClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TotalControlMqttClient::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, [this](QAbstractSocket::SocketError) {
                markDisconnected(m_socket ? m_socket->errorString() : QStringLiteral("socket error"));
            });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(kReconnectIntervalMs);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TotalControlMqttClient::connectToBroker);

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(qMax(1000, (m_keepAliveSec * 1000) / 2));
    connect(m_pingTimer, &QTimer::timeout, this, &TotalControlMqttClient::sendPing);

    m_publishTimer = new QTimer(this);
    m_publishTimer->setInterval(m_publishIntervalMs);
    connect(m_publishTimer, &QTimer::timeout, this, &TotalControlMqttClient::publishSnapshot);
    m_publishTimer->start();

    emit logMessage(QString("[TOTAL_MQTT][INIT] broker=%1:%2 clientId=%3 topic=%4 publishInterval=%5ms keepAlive=%6s")
                        .arg(m_host).arg(m_port).arg(m_clientId).arg(m_topic)
                        .arg(m_publishIntervalMs).arg(m_keepAliveSec));
    connectToBroker();
    return true;
}

void TotalControlMqttClient::updateOwnTrack(const PointInfo& info)
{
    if (info.type != PointType::Track) {
        return;
    }
    m_latestTracks.insert(info.batch, info);
}

void TotalControlMqttClient::removeTrack(unsigned int batch)
{
    m_latestTracks.remove(batch);
    emit logMessage(QString("[TOTAL_MQTT][CACHE] removed track=%1 ownRemaining=%2 edgeRemaining=%3")
                        .arg(batch)
                        .arg(m_latestTracks.size())
                        .arg(m_edgeResults.size()));
}

void TotalControlMqttClient::updateEdgeResult(const EdgeRecognitionResult& result)
{
    if (result.trackId.isEmpty()) {
        return;
    }

    bool numericOk = false;
    const unsigned int numericTrackId = result.trackId.toUInt(&numericOk);
    m_edgeResults.insert(result.trackId, result);
    emit logMessage(QString("[TOTAL_MQTT][CACHE] edge result track=%1 isDrone=%2 count=%3 detections=%4 ownKnown=%5 edgeCached=%6")
                        .arg(result.trackId)
                        .arg(result.isDrone)
                        .arg(result.count)
                        .arg(result.detections.size())
                        .arg(numericOk && m_latestTracks.contains(numericTrackId))
                        .arg(m_edgeResults.size()));
}

void TotalControlMqttClient::connectToBroker()
{
    if (!m_enabled || !m_socket || m_connected || m_socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    emit logMessage(QString("[TOTAL_MQTT][CONNECT] connecting broker=%1:%2").arg(m_host).arg(m_port));
    m_socket->connectToHost(m_host, m_port);
}

void TotalControlMqttClient::onConnected()
{
    emit logMessage(QString("[TOTAL_MQTT][CONNECT] tcp connected broker=%1:%2").arg(m_host).arg(m_port));
    sendConnectPacket();
}

void TotalControlMqttClient::onReadyRead()
{
    const QByteArray data = m_socket->readAll();
    if (data.size() >= 4 && static_cast<quint8>(data.at(0)) == kPacketConnack) {
        const quint8 returnCode = static_cast<quint8>(data.at(3));
        if (returnCode == 0) {
            m_connected = true;
            if (m_reconnectTimer) m_reconnectTimer->stop();
            if (m_pingTimer && !m_pingTimer->isActive()) m_pingTimer->start();
            emit logMessage(QString("[TOTAL_MQTT][CONNACK] accepted topic=%1").arg(m_topic));
        } else {
            markDisconnected(QString("CONNACK refused code=%1").arg(returnCode));
        }
    }
}

void TotalControlMqttClient::onDisconnected()
{
    markDisconnected(QStringLiteral("disconnected"));
}

void TotalControlMqttClient::sendPing()
{
    if (!m_connected || !m_socket) {
        return;
    }

    QByteArray packet;
    packet.append(static_cast<char>(kPacketPingReq));
    packet.append('\0');
    m_socket->write(packet);
}

void TotalControlMqttClient::publishSnapshot()
{
    if (!m_enabled) {
        return;
    }

    if (!m_connected) {
        if (!m_latestTracks.isEmpty() || !m_edgeResults.isEmpty()) {
            emit logMessage(QString("[TOTAL_MQTT][PUBLISH][SKIP] not connected ownCached=%1 edgeCached=%2 broker=%3:%4")
                                .arg(m_latestTracks.size())
                                .arg(m_edgeResults.size())
                                .arg(m_host).arg(m_port));
        }
        return;
    }

    if (m_latestTracks.isEmpty() && m_edgeResults.isEmpty()) {
        emit logMessage(QString("[TOTAL_MQTT][PUBLISH][SKIP] no current tracks edgeCached=%1")
                            .arg(m_edgeResults.size()));
        return;
    }

    int publishCount = 0;
    int withEdgeResultCount = 0;
    QSet<QString> publishedTrackIds;
    const auto tracks = m_latestTracks;
    for (auto it = tracks.cbegin(); it != tracks.cend(); ++it) {
        const QString trackId = QString::number(it.key());
        const auto edgeIt = m_edgeResults.constFind(trackId);
        EdgeRecognitionResult edgeCopy;
        const EdgeRecognitionResult* edge = nullptr;
        if (edgeIt != m_edgeResults.cend()) {
            edgeCopy = edgeIt.value();
            edge = &edgeCopy;
            ++withEdgeResultCount;
        }
        const PointInfo info = it.value();
        publishJson(buildRecognitionJson(&info, trackId, edge));
        publishedTrackIds.insert(trackId);
        ++publishCount;
    }

    const auto edgeResults = m_edgeResults;
    QStringList edgeOnlyForwarded;
    for (auto it = edgeResults.cbegin(); it != edgeResults.cend(); ++it) {
        if (publishedTrackIds.contains(it.key())) {
            continue;
        }
        publishJson(buildRecognitionJson(nullptr, it.key(), &it.value()));
        edgeOnlyForwarded.append(it.key());
        ++publishCount;
        ++withEdgeResultCount;
    }
    for (const QString& trackId : edgeOnlyForwarded) {
        m_edgeResults.remove(trackId);
    }
    emit logMessage(QString("[TOTAL_MQTT][PUBLISH_SNAPSHOT] ownCached=%1 edgeCached=%2 published=%3 withEdgeResult=%4 topic=%5")
                        .arg(m_latestTracks.size())
                        .arg(m_edgeResults.size())
                        .arg(publishCount)
                        .arg(withEdgeResultCount)
                        .arg(m_topic));
}

QByteArray TotalControlMqttClient::encodeString(const QString& text) const
{
    const QByteArray utf8 = text.toUtf8();
    QByteArray out;
    out.append(static_cast<char>((utf8.size() >> 8) & 0xFF));
    out.append(static_cast<char>(utf8.size() & 0xFF));
    out.append(utf8);
    return out;
}

QByteArray TotalControlMqttClient::encodeRemainingLength(int length) const
{
    QByteArray encoded;
    do {
        quint8 byte = static_cast<quint8>(length % 128);
        length /= 128;
        if (length > 0) byte |= 0x80;
        encoded.append(static_cast<char>(byte));
    } while (length > 0);
    return encoded;
}

void TotalControlMqttClient::sendConnectPacket()
{
    QByteArray variable;
    variable.append(encodeString(QStringLiteral("MQTT")));
    variable.append(static_cast<char>(kMqttProtocolLevel));
    variable.append(static_cast<char>(0x02)); // clean session
    variable.append(static_cast<char>((m_keepAliveSec >> 8) & 0xFF));
    variable.append(static_cast<char>(m_keepAliveSec & 0xFF));

    QByteArray payload = encodeString(m_clientId);

    QByteArray packet;
    packet.append(static_cast<char>(kPacketConnect));
    packet.append(encodeRemainingLength(variable.size() + payload.size()));
    packet.append(variable);
    packet.append(payload);
    m_socket->write(packet);
}

void TotalControlMqttClient::publishJson(const QByteArray& payload)
{
    if (!m_connected || !m_socket) {
        return;
    }

    QByteArray variable = encodeString(m_topic);
    QByteArray packet;
    packet.append(static_cast<char>(kPacketPublishQoS0));
    packet.append(encodeRemainingLength(variable.size() + payload.size()));
    packet.append(variable);
    packet.append(payload);

    const qint64 written = m_socket->write(packet);
    ++m_publishCount;
    emit logMessage(QString("[TOTAL_MQTT][PUBLISH] count=%1 bytes=%2 topic=%3 payload=%4")
                        .arg(m_publishCount).arg(written).arg(m_topic).arg(QString::fromUtf8(payload)));
}

QByteArray TotalControlMqttClient::buildRecognitionJson(const PointInfo* info,
                                                        const QString& trackId,
                                                        const EdgeRecognitionResult* edge) const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("target_recognition"));
    obj.insert(QStringLiteral("source"), QStringLiteral("x576"));
    obj.insert(QStringLiteral("timestamp_ms"), static_cast<double>(QDateTime::currentMSecsSinceEpoch()));
    obj.insert(QStringLiteral("track_id"), trackId);

    if (info) {
        QJsonObject radar;
        radar.insert(QStringLiteral("target_type"), static_cast<int>(info->targetRecResult == 1 ? 1 : 0));
        radar.insert(QStringLiteral("azimuth"), static_cast<double>(info->azimuth));
        radar.insert(QStringLiteral("elevation"), static_cast<double>(info->elevation));
        radar.insert(QStringLiteral("distance"), static_cast<double>(info->range));
        radar.insert(QStringLiteral("speed"), static_cast<double>(info->speed));
        obj.insert(QStringLiteral("radar"), radar);
    } else {
        obj.insert(QStringLiteral("radar"), QJsonValue(QJsonValue::Null));
    }

    if (edge) {
        QJsonObject optical;
        optical.insert(QStringLiteral("is_drone"), edge->isDrone);
        optical.insert(QStringLiteral("count"), edge->count);
        optical.insert(QStringLiteral("timestamp"), edge->timestampSec);
        optical.insert(QStringLiteral("received_timestamp_ms"), static_cast<double>(edge->receivedTimestampMs));

        QJsonArray detections;
        for (const EdgeDetection& detection : edge->detections) {
            QJsonObject det;
            det.insert(QStringLiteral("class"), detection.className);
            det.insert(QStringLiteral("confidence"), detection.confidence);
            QJsonArray bbox;
            for (int value : detection.bbox) {
                bbox.append(value);
            }
            det.insert(QStringLiteral("bbox"), bbox);
            detections.append(det);
        }
        optical.insert(QStringLiteral("detections"), detections);
        obj.insert(QStringLiteral("optical"), optical);
    } else {
        obj.insert(QStringLiteral("optical"), QJsonValue(QJsonValue::Null));
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

void TotalControlMqttClient::markDisconnected(const QString& reason)
{
    if (!m_enabled) {
        return;
    }

    const bool wasConnected = m_connected;
    m_connected = false;
    if (m_pingTimer) m_pingTimer->stop();
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    if (m_reconnectTimer && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
    if (wasConnected || reason != QStringLiteral("disconnected")) {
        emit logMessage(QString("[TOTAL_MQTT][DISCONNECT] %1, reconnect in %2ms")
                            .arg(reason).arg(kReconnectIntervalMs));
    }
}
