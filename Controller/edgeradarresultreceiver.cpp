/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-06-17
 * @Description: Edge terminal optical recognition result receiver
 */
#include "edgeradarresultreceiver.h"

#include "Basic/ConfigManager.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkDatagram>
#include <QVariant>

namespace {
QString jsonTrackIdToString(const QJsonValue& value)
{
    if (value.isString()) {
        return value.toString().trimmed();
    }
    if (value.isDouble()) {
        const double numeric = value.toDouble();
        const qint64 asInt = static_cast<qint64>(numeric);
        if (qFuzzyCompare(numeric + 1.0, static_cast<double>(asInt) + 1.0)) {
            return QString::number(asInt);
        }
    }
    return value.toVariant().toString().trimmed();
}
}

EdgeRadarResultReceiver::EdgeRadarResultReceiver(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<EdgeRecognitionResult>("EdgeRecognitionResult");
}

EdgeRadarResultReceiver::~EdgeRadarResultReceiver()
{
    if (m_socket) {
        m_socket->close();
    }
}

bool EdgeRadarResultReceiver::init()
{
    m_enabled = CF_INS.edgeRadarResultReceiveEnabled(true);
    if (!m_enabled) {
        emit logMessage(QStringLiteral("[EDGE_RESULT][INIT] disabled by config"));
        return false;
    }

    m_localHost = QHostAddress(CF_INS.edgeRadarResultLocalIp("0.0.0.0"));
    m_localPort = CF_INS.edgeRadarResultLocalPort(9002);
    if (m_localHost.isNull() || m_localPort == 0) {
        emit logMessage(QString("[EDGE_RESULT][INIT][ERROR] invalid local %1:%2")
                            .arg(m_localHost.toString()).arg(m_localPort));
        m_enabled = false;
        return false;
    }

    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(m_localHost, m_localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit logMessage(QString("[EDGE_RESULT][INIT][ERROR] bind %1:%2 failed: %3")
                            .arg(m_localHost.toString()).arg(m_localPort)
                            .arg(m_socket->errorString()));
        m_enabled = false;
        return false;
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &EdgeRadarResultReceiver::processPendingDatagrams);
    emit logMessage(QString("[EDGE_RESULT][INIT] listening local=%1:%2")
                        .arg(m_localHost.toString()).arg(m_localPort));
    return true;
}

void EdgeRadarResultReceiver::processPendingDatagrams()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket->receiveDatagram();
        parseDatagram(datagram.data(), datagram.senderAddress(), datagram.senderPort());
    }
}

void EdgeRadarResultReceiver::parseDatagram(const QByteArray& payload,
                                            const QHostAddress& sender,
                                            quint16 senderPort)
{
    emit logMessage(QString("[EDGE_RESULT][RAW] bytes=%1 from=%2:%3 payload=%4")
                        .arg(payload.size())
                        .arg(sender.toString()).arg(senderPort)
                        .arg(QString::fromUtf8(payload)));

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("[EDGE_RESULT][ERROR] invalid JSON from %1:%2 err=%3 payload=%4")
                            .arg(sender.toString()).arg(senderPort)
                            .arg(error.errorString())
                            .arg(QString::fromUtf8(payload)));
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("type")).toString() != QStringLiteral("recognition_result")) {
        return;
    }

    if (!obj.contains(QStringLiteral("track_id")) ||
        !obj.contains(QStringLiteral("is_drone")) ||
        !obj.contains(QStringLiteral("count")) ||
        !obj.contains(QStringLiteral("detections")) ||
        !obj.contains(QStringLiteral("timestamp"))) {
        emit logMessage(QString("[EDGE_RESULT][ERROR] missing required recognition_result fields from %1:%2 payload=%3")
                            .arg(sender.toString()).arg(senderPort)
                            .arg(QString::fromUtf8(payload)));
        return;
    }

    EdgeRecognitionResult result;
    result.trackId = jsonTrackIdToString(obj.value(QStringLiteral("track_id")));
    result.isDrone = obj.value(QStringLiteral("is_drone")).toBool(false);
    result.count = obj.value(QStringLiteral("count")).toInt(-1);
    result.timestampSec = obj.value(QStringLiteral("timestamp")).toDouble(0.0);
    result.receivedTimestampMs = QDateTime::currentMSecsSinceEpoch();
    result.senderAddress = sender.toString();
    result.senderPort = senderPort;
    result.rawPayload = payload;

    const QJsonValue detectionsValue = obj.value(QStringLiteral("detections"));
    if (result.trackId.isEmpty() || result.count < 0 || !detectionsValue.isArray()) {
        emit logMessage(QString("[EDGE_RESULT][ERROR] invalid recognition_result values from %1:%2 track=%3 count=%4 payload=%5")
                            .arg(sender.toString()).arg(senderPort)
                            .arg(result.trackId).arg(result.count)
                            .arg(QString::fromUtf8(payload)));
        return;
    }

    const QJsonArray detections = detectionsValue.toArray();
    for (const QJsonValue& item : detections) {
        if (!item.isObject()) {
            emit logMessage(QString("[EDGE_RESULT][ERROR] invalid detection item from %1:%2 track=%3 payload=%4")
                                .arg(sender.toString()).arg(senderPort)
                                .arg(result.trackId)
                                .arg(QString::fromUtf8(payload)));
            return;
        }
        const QJsonObject detObj = item.toObject();
        const QJsonArray bboxArray = detObj.value(QStringLiteral("bbox")).toArray();
        if (!detObj.contains(QStringLiteral("class")) ||
            !detObj.contains(QStringLiteral("confidence")) ||
            bboxArray.size() != 4) {
            emit logMessage(QString("[EDGE_RESULT][ERROR] invalid detection fields from %1:%2 track=%3 payload=%4")
                                .arg(sender.toString()).arg(senderPort)
                                .arg(result.trackId)
                                .arg(QString::fromUtf8(payload)));
            return;
        }

        EdgeDetection det;
        det.className = detObj.value(QStringLiteral("class")).toString();
        det.confidence = detObj.value(QStringLiteral("confidence")).toDouble(-1.0);
        for (const QJsonValue& bboxValue : bboxArray) {
            det.bbox.append(bboxValue.toInt());
        }
        if (det.className.isEmpty() || det.confidence < 0.0 || det.confidence > 1.0) {
            emit logMessage(QString("[EDGE_RESULT][ERROR] invalid detection values from %1:%2 track=%3 payload=%4")
                                .arg(sender.toString()).arg(senderPort)
                                .arg(result.trackId)
                                .arg(QString::fromUtf8(payload)));
            return;
        }
        result.detections.append(det);
    }

    if (result.count != result.detections.size()) {
        emit logMessage(QString("[EDGE_RESULT][WARN] count mismatch track=%1 count=%2 detections=%3 from=%4:%5")
                            .arg(result.trackId)
                            .arg(result.count)
                            .arg(result.detections.size())
                            .arg(sender.toString()).arg(senderPort));
    }

    emit logMessage(QString("[EDGE_RESULT][RECOGNITION] track=%1 isDrone=%2 count=%3 detections=%4 timestamp=%5 from=%6:%7 payload=%8")
                        .arg(result.trackId)
                        .arg(result.isDrone)
                        .arg(result.count)
                        .arg(result.detections.size())
                        .arg(result.timestampSec, 0, 'f', 3)
                        .arg(sender.toString()).arg(senderPort)
                        .arg(QString::fromUtf8(payload)));
    emit edgeResultReceived(result);
}
