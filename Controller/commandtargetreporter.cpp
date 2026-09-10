#include "commandtargetreporter.h"

#include "Basic/ConfigManager.h"
#include "Controller/commandtargetreportprotocol.h"

#include <QDateTime>
#include <QHostAddress>
#include <QtGlobal>
#include <cmath>

namespace {
constexpr int kTcpPendingLimit = 4096;
constexpr double kEarthRadiusM = 6378137.0;
constexpr double kDegToRad = 0.017453292519943295769236907684886;

QString lowerConfig(const QString& value)
{
    return value.trimmed().toLower();
}
}

CommandTargetReporter::CommandTargetReporter(QObject* parent)
    : QObject(parent)
{
    loadSettings();
    m_tcpReconnectTimer.setSingleShot(true);
    connect(&m_tcpReconnectTimer, &QTimer::timeout, this, &CommandTargetReporter::beginTcpConnection);
    connect(&m_tcpSocket, &QTcpSocket::connected, this, &CommandTargetReporter::onTcpConnected);
    connect(&m_tcpSocket, &QTcpSocket::disconnected, this, &CommandTargetReporter::onTcpDisconnected);
    connect(&m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &CommandTargetReporter::onTcpError);

    m_runtimeEnabled = CF_INS.commandTargetReportEnabled(true);
    startTransport();
}

void CommandTargetReporter::loadSettings()
{
    m_settings.transport = lowerConfig(CF_INS.commandTargetReportTransport("udp")) == QStringLiteral("tcp")
        ? Transport::Tcp : Transport::Udp;
    m_settings.payload = lowerConfig(CF_INS.commandTargetReportPayload("json")) == QStringLiteral("binary")
        ? Payload::Binary : Payload::Json;
    m_settings.bigEndian = lowerConfig(CF_INS.commandTargetReportByteOrder("little")) == QStringLiteral("big");
    m_settings.localIp = CF_INS.commandTargetReportLocalIp("0.0.0.0");
    m_settings.localPort = CF_INS.commandTargetReportLocalPort(0);
    m_settings.remoteIp = CF_INS.commandTargetReportRemoteIp("127.0.0.1");
    m_settings.remotePort = CF_INS.commandTargetReportRemotePort(21002);
    m_settings.tcpReconnectIntervalMs = qBound(250, CF_INS.commandTargetReportTcpReconnectMs(3000), 60000);
}

void CommandTargetReporter::setRuntimeEnabled(bool enabled)
{
    if (m_runtimeEnabled == enabled) {
        return;
    }
    // Change this before aborting TCP so the disconnected signal cannot schedule
    // a reconnect after the operator has turned the feature off.
    m_runtimeEnabled = enabled;
    stopTransport();
    startTransport();
    emit runtimeEnabledChanged(m_runtimeEnabled);
}

void CommandTargetReporter::startTransport()
{
    if (!m_runtimeEnabled) {
        setNetworkStatus(false, QStringLiteral("C2 target report disabled"));
        return;
    }

    const QHostAddress localAddress(m_settings.localIp);
    const QHostAddress remoteAddress(m_settings.remoteIp);
    if (localAddress.isNull() || remoteAddress.isNull() || m_settings.remotePort == 0) {
        setNetworkStatus(false, QStringLiteral("C2 report invalid local/remote endpoint"));
        return;
    }

    if (m_settings.transport == Transport::Tcp) {
        beginTcpConnection();
        return;
    }

    m_udpBound = m_udpSocket.bind(localAddress, m_settings.localPort,
                                  QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!m_udpBound) {
        setNetworkStatus(false, QStringLiteral("C2 UDP bind failed: ") + m_udpSocket.errorString());
        return;
    }
    setNetworkStatus(true, QStringLiteral("C2 UDP ready: %1:%2 -> %3:%4")
                     .arg(m_settings.localIp)
                     .arg(m_udpSocket.localPort())
                     .arg(m_settings.remoteIp)
                     .arg(m_settings.remotePort));
}

void CommandTargetReporter::stopTransport()
{
    m_tcpReconnectTimer.stop();
    m_tcpPending.clear();
    m_udpSocket.close();
    m_udpBound = false;
    m_tcpSocket.abort();
}

void CommandTargetReporter::processTrackPoint(const PointInfo& info)
{
    // Command-and-control reporting uses normal radar tracks only; TBD/cooperative tracks stay independent.
    if (info.type != PointType::Track) {
        return;
    }
    const unsigned int batch = info.batch;
    if (info.statMethod == 2) {
        m_latestTracks.remove(batch);
        m_targetClasses.remove(batch);
        return;
    }

    TimedTrack timed;
    timed.info = info;
    timed.timestampUtcMs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    m_latestTracks.insert(batch, timed);
    reportIfDrone(batch);
}

void CommandTargetReporter::processTargetClassification(TargetClaRes result)
{
    m_targetClasses.insert(result.batchID, result.claRes);
    reportIfDrone(result.batchID);
}

bool CommandTargetReporter::isDrone(unsigned int batch, const PointInfo& info) const
{
    // The regular track message already carries 0/1.  The classification message can arrive later.
    return info.targetRecResult == 1 || m_targetClasses.value(batch, 0) == 1;
}

void CommandTargetReporter::reportIfDrone(unsigned int batch)
{
    if (!m_runtimeEnabled || !m_latestTracks.contains(batch)) {
        return;
    }
    const TimedTrack& track = m_latestTracks.value(batch);
    if (!isDrone(batch, track.info)) {
        return;
    }

    const QByteArray payload = buildPayload(track);
    if (payload.isEmpty()) {
        emit logMessage(QString("[C2_REPORT] skip batch=%1: invalid RAE or serialization failed").arg(batch));
        return;
    }
    sendPayload(payload);
}

QByteArray CommandTargetReporter::buildPayload(const TimedTrack& track) const
{
    double longitudeDeg = 0.0;
    double latitudeDeg = 0.0;
    double altitudeM = 0.0;
    if (!targetLla(track.info, longitudeDeg, latitudeDeg, altitudeM)) {
        return QByteArray();
    }

    CommandTargetReportProtocol::BinaryTargetReport report{};
    report.magic = CommandTargetReportProtocol::kMagic;
    report.version = CommandTargetReportProtocol::kVersion;
    report.packetSize = CommandTargetReportProtocol::kBinaryPacketSize;
    report.reportTimestampUtcMs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    report.trackTimestampUtcMs = track.timestampUtcMs;
    report.targetId = track.info.batch;
    report.longitudeDeg = longitudeDeg;
    report.latitudeDeg = latitudeDeg;
    report.altitudeM = altitudeM;

    if (m_settings.payload == Payload::Json) {
        return CommandTargetReportProtocol::serializeJson(report, m_settings.transport == Transport::Tcp);
    }
    return CommandTargetReportProtocol::serializeBinary(report, m_settings.bigEndian);
}

bool CommandTargetReporter::targetLla(const PointInfo& info,
                                      double& longitudeDeg,
                                      double& latitudeDeg,
                                      double& altitudeM) const
{
    if (!std::isfinite(info.range) || !std::isfinite(info.azimuth) || !std::isfinite(info.elevation)
        || info.range < 0.0f) {
        return false;
    }

    const double radarLongitude = CF_INS.longitude("longitude", 108.9138);
    const double radarLatitude = CF_INS.latitude("latitude", 34.2311);
    const double radarAltitude = CF_INS.altitude("altitude", 400.0);
    if (!std::isfinite(radarLongitude) || !std::isfinite(radarLatitude) || !std::isfinite(radarAltitude)
        || radarLongitude < -180.0 || radarLongitude > 180.0
        || radarLatitude < -90.0 || radarLatitude > 90.0) {
        return false;
    }

    // PPI receives north-referenced azimuth. Do not add radar.yaw again here.
    const double elevationRad = static_cast<double>(info.elevation) * kDegToRad;
    const double horizontalM = static_cast<double>(info.range) * std::cos(elevationRad);
    const double eastM = horizontalM * std::sin(static_cast<double>(info.azimuth) * kDegToRad);
    const double northM = horizontalM * std::cos(static_cast<double>(info.azimuth) * kDegToRad);
    const double latitudeRad = radarLatitude * kDegToRad;
    latitudeDeg = radarLatitude + northM / (kEarthRadiusM * kDegToRad);
    longitudeDeg = radarLongitude + eastM / (kEarthRadiusM * std::cos(latitudeRad) * kDegToRad);
    altitudeM = radarAltitude + static_cast<double>(info.range) * std::sin(elevationRad);
    return std::isfinite(longitudeDeg) && std::isfinite(latitudeDeg) && std::isfinite(altitudeM);
}

void CommandTargetReporter::sendPayload(const QByteArray& payload)
{
    if (m_settings.transport == Transport::Tcp) {
        if (m_tcpSocket.state() != QAbstractSocket::ConnectedState) {
            if (m_tcpPending.size() >= kTcpPendingLimit) {
                setNetworkStatus(false, QStringLiteral("C2 TCP queue full; newest report dropped"));
                return;
            }
            m_tcpPending.enqueue(payload);
            setNetworkStatus(false, QStringLiteral("C2 TCP disconnected; buffering drone reports"));
            beginTcpConnection();
            return;
        }
        if (m_tcpSocket.write(payload) != payload.size()) {
            m_tcpPending.prepend(payload);
            setNetworkStatus(false, QStringLiteral("C2 TCP write failed: ") + m_tcpSocket.errorString());
            return;
        }
        m_tcpSocket.flush();
        setNetworkStatus(true, QStringLiteral("C2 TCP connected: %1:%2")
                         .arg(m_settings.remoteIp).arg(m_settings.remotePort));
        return;
    }

    if (!m_udpBound) {
        setNetworkStatus(false, QStringLiteral("C2 UDP is not bound"));
        return;
    }
    const qint64 written = m_udpSocket.writeDatagram(payload, QHostAddress(m_settings.remoteIp), m_settings.remotePort);
    if (written != payload.size()) {
        setNetworkStatus(false, QStringLiteral("C2 UDP write failed: ") + m_udpSocket.errorString());
    } else {
        // UDP has no peer acknowledgement: this only proves local stack acceptance.
        setNetworkStatus(true, QStringLiteral("C2 UDP local send accepted: %1:%2")
                         .arg(m_settings.remoteIp).arg(m_settings.remotePort));
    }
}

void CommandTargetReporter::beginTcpConnection()
{
    if (!m_runtimeEnabled || m_settings.transport != Transport::Tcp || m_tcpReconnectTimer.isActive()
        || m_tcpSocket.state() == QAbstractSocket::ConnectedState
        || m_tcpSocket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    m_tcpSocket.abort();
    if (!m_tcpSocket.bind(QHostAddress(m_settings.localIp), m_settings.localPort,
                          QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        setNetworkStatus(false, QStringLiteral("C2 TCP bind failed: ") + m_tcpSocket.errorString());
        scheduleTcpReconnect();
        return;
    }
    setNetworkStatus(false, QStringLiteral("C2 TCP connecting: %1:%2")
                     .arg(m_settings.remoteIp).arg(m_settings.remotePort));
    m_tcpSocket.connectToHost(m_settings.remoteIp, m_settings.remotePort);
}

void CommandTargetReporter::scheduleTcpReconnect()
{
    if (m_runtimeEnabled && m_settings.transport == Transport::Tcp && !m_tcpReconnectTimer.isActive()) {
        m_tcpReconnectTimer.start(m_settings.tcpReconnectIntervalMs);
    }
}

void CommandTargetReporter::flushTcpPending()
{
    while (!m_tcpPending.isEmpty() && m_tcpSocket.state() == QAbstractSocket::ConnectedState) {
        const QByteArray payload = m_tcpPending.dequeue();
        if (m_tcpSocket.write(payload) != payload.size()) {
            m_tcpPending.prepend(payload);
            setNetworkStatus(false, QStringLiteral("C2 TCP write failed: ") + m_tcpSocket.errorString());
            return;
        }
    }
    m_tcpSocket.flush();
}

void CommandTargetReporter::onTcpConnected()
{
    setNetworkStatus(true, QStringLiteral("C2 TCP connected: %1:%2")
                     .arg(m_settings.remoteIp).arg(m_settings.remotePort));
    flushTcpPending();
}

void CommandTargetReporter::onTcpDisconnected()
{
    if (!m_runtimeEnabled || m_settings.transport != Transport::Tcp) {
        return;
    }
    setNetworkStatus(false, QStringLiteral("C2 TCP disconnected; reconnecting"));
    scheduleTcpReconnect();
}

void CommandTargetReporter::onTcpError(QAbstractSocket::SocketError)
{
    if (!m_runtimeEnabled || m_settings.transport != Transport::Tcp) {
        return;
    }
    setNetworkStatus(false, QStringLiteral("C2 TCP error: ") + m_tcpSocket.errorString());
    scheduleTcpReconnect();
}

void CommandTargetReporter::setNetworkStatus(bool ready, const QString& detail)
{
    if (m_networkReady == ready && m_networkStatusText == detail) {
        return;
    }
    m_networkReady = ready;
    m_networkStatusText = detail;
    emit logMessage(QStringLiteral("[C2_REPORT] ") + detail);
    emit networkStatusChanged(ready, detail);
}
