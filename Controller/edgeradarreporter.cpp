/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-05-29 09:49:42
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:48
 * @Description: 
 */
#include "edgeradarreporter.h"

#include "Basic/ConfigManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <cmath>

namespace {

constexpr int kMaxUdpPayloadBytes = 1472;

int targetTypeForReport(const PointInfo& info)
{
    return (info.targetRecResult == 1) ? 1 : 0;
}

double normalizedAzimuth(double azimuth)
{
    double value = std::fmod(azimuth, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }
    return value;
}

}

EdgeRadarReporter::EdgeRadarReporter(QObject* parent)
    : QObject(parent)
{
}

EdgeRadarReporter::~EdgeRadarReporter()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    if (m_targetTimer) {
        m_targetTimer->stop();
    }
    if (m_socket) {
        m_socket->close();
    }
}

bool EdgeRadarReporter::init()
{
    m_enabled = CF_INS.edgeRadarReportEnabled(false);
    if (!m_enabled) {
        emit logMessage(QStringLiteral("[EDGE_REPORT][INIT] disabled by config"));
        return false;
    }

    m_targetHost = QHostAddress(CF_INS.edgeRadarReportIp("192.168.1.100"));
    m_targetPort = CF_INS.edgeRadarReportPort(9001);
    m_localHost = QHostAddress(CF_INS.edgeRadarReportLocalIp("0.0.0.0"));
    m_localPort = CF_INS.edgeRadarReportLocalPort(0);
    m_heartbeatIntervalMs = qMax(1000, CF_INS.edgeRadarHeartbeatIntervalMs(5000));
    m_targetReportIntervalMs = qMax(500, CF_INS.edgeRadarTargetReportIntervalMs(4000));
    m_maxTargetDistanceM = qMax(0.0, CF_INS.edgeRadarMaxTargetDistanceM(2000.0));

    emit logMessage(QString("[EDGE_REPORT][INIT] config enabled=%1 target=%2:%3 local=%4:%5 heartbeat=%6ms targetInterval=%7ms maxDistance=%8m maxPayload=%9")
                        .arg(m_enabled)
                        .arg(m_targetHost.toString()).arg(m_targetPort)
                        .arg(m_localHost.toString()).arg(m_localPort)
                        .arg(m_heartbeatIntervalMs)
                        .arg(m_targetReportIntervalMs)
                        .arg(m_maxTargetDistanceM, 0, 'f', 1)
                        .arg(kMaxUdpPayloadBytes));

    if (m_targetHost.isNull() || m_targetPort == 0) {
        emit logMessage(QString("[EDGE_REPORT][INIT][ERROR] invalid target %1:%2")
                            .arg(m_targetHost.toString()).arg(m_targetPort));
        m_enabled = false;
        return false;
    }

    m_socket = new QUdpSocket(this);
    if (m_localPort != 0 || CF_INS.edgeRadarReportLocalIp("0.0.0.0") != QStringLiteral("0.0.0.0")) {
        if (!m_socket->bind(m_localHost, m_localPort, QUdpSocket::ShareAddress)) {
            emit logMessage(QString("[EDGE_REPORT][INIT][ERROR] bind %1:%2 failed: %3")
                                .arg(m_localHost.toString()).arg(m_localPort)
                                .arg(m_socket->errorString()));
            m_enabled = false;
            return false;
        }
        emit logMessage(QString("[EDGE_REPORT][INIT] bind local=%1:%2 ok")
                            .arg(m_localHost.toString()).arg(m_localPort));
    } else {
        emit logMessage(QStringLiteral("[EDGE_REPORT][INIT] using OS-selected UDP source address/port"));
    }

    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &EdgeRadarReporter::sendHeartbeat);
    m_heartbeatTimer->start(m_heartbeatIntervalMs);
    sendHeartbeat();

    m_targetTimer = new QTimer(this);
    m_targetTimer->setTimerType(Qt::PreciseTimer);
    connect(m_targetTimer, &QTimer::timeout, this, &EdgeRadarReporter::sendTargetSnapshot);
    m_targetTimer->start(m_targetReportIntervalMs);

    emit logMessage(QString("[EDGE_REPORT][INIT] ready target=%1:%2 local=%3:%4 heartbeat=%5ms")
                        .arg(m_targetHost.toString()).arg(m_targetPort)
                        .arg(m_localHost.toString()).arg(m_localPort)
                        .arg(m_heartbeatIntervalMs));
    return true;
}

void EdgeRadarReporter::reportTrackPoint(const PointInfo& info)
{
    if (!m_enabled) {
        return;
    }

    if (info.type != PointType::Track) {
        return;
    }

    m_latestTracks.insert(info.batch, info);
}

void EdgeRadarReporter::removeTrackPoint(unsigned int batch)
{
    m_latestTracks.remove(batch);
}

void EdgeRadarReporter::sendTargetDatagram(const PointInfo& info)
{
    if (!m_enabled || !m_socket) {
        return;
    }

    const QByteArray payload = buildTargetJson(info);
    if (payload.size() > kMaxUdpPayloadBytes) {
        emit logMessage(QString("[EDGE_REPORT][TARGET][ERROR] payload too large bytes=%1 track=%2")
                            .arg(payload.size()).arg(info.batch));
        return;
    }

    const qint64 written = m_socket->writeDatagram(payload, m_targetHost, m_targetPort);
    if (written < 0) {
        logWriteError(QStringLiteral("TARGET"), m_socket->errorString());
        return;
    }

    ++m_targetSentCount;
    logTargetSent(info, payload, written);
}

void EdgeRadarReporter::sendTargetSnapshot()
{
    if (!m_enabled || !m_socket) {
        return;
    }

    if (m_latestTracks.isEmpty()) {
        emit logMessage(QStringLiteral("[EDGE_REPORT][TARGET_SNAPSHOT] no current tracks, target packet skipped"));
        return;
    }

    int sentCount = 0;
    int skippedByDistance = 0;
    const auto tracks = m_latestTracks;
    for (auto it = tracks.cbegin(); it != tracks.cend(); ++it) {
        if (m_maxTargetDistanceM <= 0.0 || it.value().range <= m_maxTargetDistanceM) {
            sendTargetDatagram(it.value());
            ++sentCount;
        } else {
            ++skippedByDistance;
        }
    }
    emit logMessage(QString("[EDGE_REPORT][TARGET_SNAPSHOT] cached=%1 sent=%2 skippedByDistance=%3 maxDistance=%4m interval=%5ms")
                        .arg(tracks.size())
                        .arg(sentCount)
                        .arg(skippedByDistance)
                        .arg(m_maxTargetDistanceM, 0, 'f', 1)
                        .arg(m_targetReportIntervalMs));
}

void EdgeRadarReporter::sendHeartbeat()
{
    if (!m_enabled || !m_socket) {
        return;
    }

    const QByteArray payload = buildHeartbeatJson();
    const qint64 written = m_socket->writeDatagram(payload, m_targetHost, m_targetPort);
    if (written < 0) {
        logWriteError(QStringLiteral("HEARTBEAT"), m_socket->errorString());
        return;
    }

    emit logMessage(QString("[EDGE_REPORT][HEARTBEAT] bytes=%1 target=%2:%3 lat=%4 lon=%5 alt=%6 payload=%7")
                        .arg(written)
                        .arg(m_targetHost.toString()).arg(m_targetPort)
                        .arg(CF_INS.latitude(), 0, 'f', 6)
                        .arg(CF_INS.longitude(), 0, 'f', 6)
                        .arg(CF_INS.altitude(), 0, 'f', 1)
                        .arg(QString::fromUtf8(payload)));
}

QByteArray EdgeRadarReporter::buildTargetJson(const PointInfo& info) const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("target"));
    obj.insert(QStringLiteral("track_id"), static_cast<int>(info.batch));
    obj.insert(QStringLiteral("azimuth"), normalizedAzimuth(info.azimuth));
    obj.insert(QStringLiteral("elevation"), static_cast<double>(info.elevation));
    obj.insert(QStringLiteral("distance"), static_cast<double>(info.range));
    obj.insert(QStringLiteral("speed"), static_cast<double>(info.speed));
    obj.insert(QStringLiteral("target_type"), targetTypeForReport(info));
    obj.insert(QStringLiteral("confidence"), 1.0);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray EdgeRadarReporter::buildHeartbeatJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("type"), QStringLiteral("heartbeat"));
    obj.insert(QStringLiteral("latitude"), CF_INS.latitude());
    obj.insert(QStringLiteral("longitude"), CF_INS.longitude());
    obj.insert(QStringLiteral("altitude"), CF_INS.altitude());
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

void EdgeRadarReporter::logTargetSent(const PointInfo& info, const QByteArray& payload, qint64 bytesWritten)
{
    emit logMessage(QString("[EDGE_REPORT][TARGET] count=%1 track=%2 az=%3 el=%4 distance=%5 speed=%6 targetType=%7 rec=%8 bytes=%9 target=%10:%11 payload=%12")
                        .arg(m_targetSentCount)
                        .arg(info.batch)
                        .arg(normalizedAzimuth(info.azimuth), 0, 'f', 2)
                        .arg(static_cast<double>(info.elevation), 0, 'f', 2)
                        .arg(static_cast<double>(info.range), 0, 'f', 1)
                        .arg(static_cast<double>(info.speed), 0, 'f', 1)
                        .arg(targetTypeForReport(info))
                        .arg(info.targetRecResult)
                        .arg(bytesWritten)
                        .arg(m_targetHost.toString()).arg(m_targetPort)
                        .arg(QString::fromUtf8(payload)));
}

void EdgeRadarReporter::logWriteError(const QString& type, const QString& err)
{
    emit logMessage(QString("[EDGE_REPORT][%1][ERROR] writeDatagram failed target=%2:%3 err=%4")
                        .arg(type)
                        .arg(m_targetHost.toString()).arg(m_targetPort)
                        .arg(err));
}
