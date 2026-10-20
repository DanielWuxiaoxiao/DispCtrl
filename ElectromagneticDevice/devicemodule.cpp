#include "devicemodule.h"
#include "devicetransport.h"
#include "Basic/ConfigManager.h"
#include "Basic/log.h"
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLocale>
#include <QDateTime>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QDataStream>
#include <QtEndian>
#include <cstring>

namespace {
constexpr int kMaxPendingDetections = 2048;
constexpr int kMaxPacketsPerBatch = 128;
float littleEndianFloat(const uchar* bytes)
{
    const quint32 bits = qFromLittleEndian<quint32>(bytes);
    float value; std::memcpy(&value, &bits, sizeof(value)); return value;
}
bool internalFrame(const QByteArray& data, quint16 message, int prefix, int recordSize, int& count)
{
    if (data.size() < prefix + 5) return false;
    const auto* bytes = reinterpret_cast<const uchar*>(data.constData());
    if (qFromLittleEndian<quint16>(bytes + 14) != message) return false;
    count = qFromLittleEndian<quint16>(bytes + prefix - 2);
    return data.size() == prefix + count * recordSize + 5;
}
}

ElectromagneticDeviceModule::ElectromagneticDeviceModule(QObject* parent) : QObject(parent)
{
    m_clock.start();
    m_warningTimes.fill(-5000);
    // Defaults belong only to this simulator link; never inject them into Controller/health state.
    m_bit = ElectromagneticProtocol::panel(BITReport{});
    m_software = QByteArray(4, char(1)); // UI starts with all four software states abnormal.
    qRegisterMetaType<QList<QByteArray>>("QList<QByteArray>");
    connect(&m_flushTimer, &QTimer::timeout, this, &ElectromagneticDeviceModule::flush);
    connect(&m_networkTimer, &QTimer::timeout, this, &ElectromagneticDeviceModule::pollNetwork);
    connect(&m_uiTimer, &QTimer::timeout, this, &ElectromagneticDeviceModule::updateUi);
    m_pingTimeout.setSingleShot(true);
    connect(&m_pingTimeout, &QTimer::timeout, this, [this] { finishPing(false); m_ping.kill(); });
    connect(&m_ping, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
        const QByteArray output = m_ping.readAllStandardOutput();
        finishPing(code == 0 && status == QProcess::NormalExit && output.toLower().contains("ttl="));
    });
    connect(&m_ping, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finishPing(false);
    });
}

ElectromagneticDeviceModule::~ElectromagneticDeviceModule()
{
    m_flushTimer.stop(); m_networkTimer.stop(); m_uiTimer.stop(); m_pingTimeout.stop();
    if (m_ping.state() != QProcess::NotRunning) m_ping.kill();
    if (m_thread.isRunning()) {
        QMetaObject::invokeMethod(m_transport, "stop", Qt::BlockingQueuedConnection);
        m_thread.quit(); m_thread.wait();
    }
    if (m_enabled) LOG_INFO(QStringLiteral("[电磁设备][STOP] socketSent=%1 failed=%2 pendingDet=%3 pendingTrack=%4")
        .arg(m_sent).arg(m_failed).arg(m_detections.size() + m_manualDetections.size()).arg(m_pendingTracks.size()));
    delete m_window.data();
}

QString ElectromagneticDeviceModule::textValue(const QString& key, const QString& fallback) const
{
    QString value = CF_INS.electromagneticDeviceValue(key, fallback).toString().trimmed();
    while (value.size() >= 2 && ((value.startsWith('"') && value.endsWith('"'))
        || (value.startsWith('\'') && value.endsWith('\'')))) value = value.mid(1, value.size() - 2).trimmed();
    return value;
}

int ElectromagneticDeviceModule::numberValue(const QString& key, int fallback, int low, int high) const
{
    bool ok = false;
    const qlonglong value = textValue(key, QString::number(fallback)).toLongLong(&ok, 0);
    if (!ok || value < low || value > high) {
        LOG_WARNING(QStringLiteral("[电磁设备][CONFIG] %1 无效，使用 %2").arg(key).arg(fallback));
        return fallback;
    }
    return int(value);
}

bool ElectromagneticDeviceModule::flagValue(const QString& key, bool fallback) const
{
    const QString value = textValue(key, fallback ? "true" : "false").toLower();
    if (value == "true") return true;
    if (value == "false") return false;
    LOG_WARNING(QStringLiteral("[电磁设备][CONFIG] %1 必须为 true/false，使用默认值").arg(key));
    return fallback;
}

void ElectromagneticDeviceModule::init()
{
    m_enabled = flagValue("enabled", false);
    m_uiEnabled = flagValue("ui_enabled", true);
    if (!m_enabled) { LOG_INFO("[电磁设备][INIT] enabled=false，未启动网络或发送链路"); return; }
    m_uiTimer.start(500);
    m_localIp = textValue("local_ip", "192.168.64.4");
    m_peerIp = textValue("peer_ip", "192.168.64.8");
    m_localPort = quint16(numberValue("local_port", 8022, 1, 65535));
    m_peerPort = quint16(numberValue("peer_port", 6022, 1, 65535));
    m_localId = quint16(numberValue("local_device_id", 0xBB04, 1, 65535));
    m_peerId = quint16(numberValue("peer_device_id", 0xBB08, 1, 65535));
    m_reportDetections = flagValue("report_detections", true);
    m_reportTracks = flagValue("report_tracks", true);
    m_autoTracks = flagValue("auto_report_tracks", false);
    m_autoDetections = flagValue("auto_report_detections", false);
    m_remoteControl = flagValue("remote_control_enabled", true);
    m_reportStatus = flagValue("report_status", true);
    m_networkEnabled = flagValue("network_check_enabled", true);
    m_dataLogEnabled = flagValue("data_log_enabled", true);
    m_logSummaryMs = numberValue("log_summary_interval_ms", 5000, 1000, 60000);
    m_logSampleMs = numberValue("data_log_interval_ms", 1000, 500, 60000);
    bool lonOk = false, latOk = false, altOk = false;
    m_configOrigin.longitudeDeg = CF_INS.radarValue("longitude").toDouble(&lonOk);
    m_configOrigin.latitudeDeg = CF_INS.radarValue("latitude").toDouble(&latOk);
    m_configOrigin.altitudeM = CF_INS.radarValue("altitude").toDouble(&altOk);
    m_configOriginValid = lonOk && latOk && altOk && Wgs84Coordinate::isValid(m_configOrigin);
    if (m_configOriginValid)
        LOG_INFO("[电磁设备][ORIGIN_CONFIG] 已加载radar.longitude/latitude/altitude；真实点迹/航迹可使用，DD05到达后优先");
    else warn(QStringLiteral("[ORIGIN_CONFIG] radar经纬高缺失或无效，备用原点不可用；等待有效DD05"), Warning::Origin);
    m_receiveTimeout = numberValue("receive_timeout_ms", 15000, 1000, 600000);
    m_statusInterval = numberValue("status_interval_ms", 1000, 100, 60000);
    const int flushMs = numberValue("flush_interval_ms", 50, 20, 1000);
    const int pollMs = numberValue("network_poll_interval_ms", 5000, 1000, 60000);
    m_validConfig = QHostAddress(m_localIp).protocol() == QAbstractSocket::IPv4Protocol
        && QHostAddress(m_peerIp).protocol() == QAbstractSocket::IPv4Protocol
        && !QHostAddress(m_localIp).isNull() && !QHostAddress(m_peerIp).isNull()
        && !QHostAddress(m_localIp).isMulticast() && !QHostAddress(m_peerIp).isMulticast()
        && m_localId != m_peerId;
    if (!m_validConfig) {
        m_transportText = QStringLiteral("电磁设备配置无效：需指定单播 IPv4 和不同设备 ID");
        LOG_WARNING(m_transportText); return;
    }
    m_transport = new ElectromagneticTransport(m_localIp, m_localPort, m_peerIp, m_peerPort);
    m_transport->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_transport, &ElectromagneticTransport::start);
    connect(&m_thread, &QThread::finished, m_transport, &QObject::deleteLater);
    connect(this, &ElectromagneticDeviceModule::outbound, m_transport, &ElectromagneticTransport::sendBatch);
    connect(m_transport, &ElectromagneticTransport::received, this, &ElectromagneticDeviceModule::receive);
    connect(m_transport, &ElectromagneticTransport::ready, this, [this](bool ok, const QString& text) {
        m_ready = ok; m_transportText = text; emit statusChanged(text);
        LOG_INFO("[电磁设备][UDP] " + text);
    });
    connect(m_transport, &ElectromagneticTransport::completed, this,
            [this](int sent, int failed, const QString& error) {
        m_busy = false; m_sent += sent; m_failed += failed;
        const qint64 elapsed = m_clock.elapsed() - m_inFlightStarted;
        m_maxTxMs = qMax(m_maxTxMs, elapsed);
        if (m_clock.elapsed() - m_lastTxLog >= m_logSampleMs || failed) {
            // Failure detail is separately rate limited; do not write each failed 50ms batch.
            if (!failed) {
                m_lastTxLog = m_clock.elapsed();
                LOG_INFO(QStringLiteral("[电磁设备][TX_DONE] seq=%1..%2 packets=%3 socketSent=%4 failed=%5 elapsedMs=%6 (本机发送结果，非对端应答)")
                    .arg(m_inFlightFirstSequence).arg(quint32(m_inFlightFirstSequence + m_inFlightPackets - 1))
                    .arg(m_inFlightPackets).arg(sent).arg(failed).arg(elapsed));
            }
        }
        if (failed) warn(QStringLiteral("[TX_FAIL] seq=%1 packets=%2 failed=%3 elapsedMs=%4 error=%5")
            .arg(m_inFlightFirstSequence).arg(m_inFlightPackets).arg(failed).arg(elapsed).arg(error), Warning::Transmit);
    });
    m_thread.start();
    m_flushTimer.start(flushMs);
    if (m_networkEnabled) { pollNetwork(); m_networkTimer.start(pollMs); }
    LOG_INFO(QStringLiteral("[电磁设备][INIT] %1:%2 ID=0x%3 -> %4:%5 ID=0x%6 检测=%7 航迹=%8 控制=%9 UI=%10")
        .arg(m_localIp).arg(m_localPort).arg(m_localId, 4, 16, QLatin1Char('0'))
        .arg(m_peerIp).arg(m_peerPort).arg(m_peerId, 4, 16, QLatin1Char('0'))
        .arg(m_reportDetections).arg(m_reportTracks).arg(m_remoteControl).arg(m_uiEnabled));
    LOG_INFO(QStringLiteral("[电磁设备][CONFIG] file=%1 autoDet=%2 autoTrack=%3 status=%4 networkCheck=%5 flushMs=%6 summaryMs=%7 dataLog=%8 sampleMs=%9")
        .arg(CF_INS.getConfigFilePath()).arg(m_autoDetections).arg(m_autoTracks).arg(m_reportStatus)
        .arg(m_networkEnabled).arg(flushMs).arg(m_logSummaryMs).arg(m_dataLogEnabled).arg(m_logSampleMs));
}

void ElectromagneticDeviceModule::warn(const QString& message, Warning category)
{
    const size_t index = size_t(category);
    if (m_clock.elapsed() - m_warningTimes[index] < 5000) { ++m_warningSuppressed[index]; return; }
    m_warningTimes[index] = m_clock.elapsed();
    LOG_WARNING(QStringLiteral("[电磁设备][WARN:%1] %2 suppressedSinceLast=%3")
        .arg(index).arg(message).arg(m_warningSuppressed[index]));
    m_warningSuppressed[index] = 0;
    emit diagnosticLog(message);
}

bool ElectromagneticDeviceModule::isTestPoint(const PointInfo& point) const
{
    const QByteArray key = detectionKey(point);
    return point.type == PointType::Detection ? m_testDetectionKeys.contains(key)
        : point.type == PointType::Track && point.batch <= 65535
            && m_testTrackKeys.contains(quint16(point.batch)) && m_testTrackKeys.value(quint16(point.batch)) == key;
}

QString ElectromagneticDeviceModule::rejectionReason(const PointInfo& point) const
{
    Wgs84Coordinate::Lla origin;
    if (!originFor(point, origin)) return QStringLiteral("NO_VALID_ORIGIN");
    if (point.type == PointType::Detection) {
        if (!isTestPoint(point) && !m_detectionMetadata.contains(detectionKey(point)))
            return QStringLiteral("DETECTION_METADATA_EXPIRED_OR_MISSING");
    } else {
        const auto it = m_trackMetadata.constFind(quint16(point.batch));
        if (it == m_trackMetadata.cend()) return QStringLiteral("TRACK_METADATA_MISSING");
        if (it->dis != point.range || it->azi != point.azimuth || it->ele != point.elevation)
            return QStringLiteral("TRACK_SAMPLE_MISMATCH");
    }
    return QStringLiteral("INVALID_VALUE_OR_COORDINATE");
}

void ElectromagneticDeviceModule::logPoint(const char* action, const PointInfo& point, const QByteArray& record, bool manual)
{
    qint64& last = point.type == PointType::Detection ? m_lastDetectionLog : m_lastTrackLog;
    if (!manual && (!m_dataLogEnabled || m_clock.elapsed() - last < m_logSampleMs)) return;
    last = m_clock.elapsed();
    const bool detection = point.type == PointType::Detection;
    const auto* bytes = reinterpret_cast<const uchar*>(record.constData());
    const bool valid = record.size() == (detection ? ElectromagneticProtocol::DetectionBytes : ElectromagneticProtocol::TrackBytes);
    const QString coordinates = valid ? QStringLiteral("lonE7=%1 latE7=%2 heightM=%3")
        .arg(qFromLittleEndian<qint32>(bytes + (detection ? 56 : 53)))
        .arg(qFromLittleEndian<qint32>(bytes + (detection ? 60 : 57)))
        .arg(qFromLittleEndian<qint32>(bytes + (detection ? 16 : 33))) : QStringLiteral("notEncoded");
    QString fields;
    if (valid && detection) {
        fields = QStringLiteral("amp=%1 cfarSNR=%2 statSNR=%3 aziBeam=%4 eleBeam=%5 disChannel=%6 dopChannel=%7 confidence=%8")
            .arg(littleEndianFloat(bytes + 20)).arg(littleEndianFloat(bytes + 24)).arg(littleEndianFloat(bytes + 28))
            .arg(littleEndianFloat(bytes + 32)).arg(littleEndianFloat(bytes + 36))
            .arg(qFromLittleEndian<quint32>(bytes + 40)).arg(qFromLittleEndian<quint32>(bytes + 44))
            .arg(littleEndianFloat(bytes + 48));
    } else if (valid) {
        fields = QStringLiteral("CPI=%1 UTC=%2 nsecond=%3 amp=%4 SNR=%5 spaceVel=%6 accelerate=%7")
            .arg(qFromLittleEndian<quint16>(bytes + 2)).arg(qFromLittleEndian<quint32>(bytes + 4))
            .arg(qFromLittleEndian<quint32>(bytes + 8)).arg(littleEndianFloat(bytes + 13))
            .arg(littleEndianFloat(bytes + 17)).arg(littleEndianFloat(bytes + 41)).arg(littleEndianFloat(bytes + 45));
    }
    LOG_INFO(QStringLiteral("[电磁设备][%1] source=%2 type=%3 batch=%4 stat=%5 mode=%6 rangeM=%7 azDeg=%8 elDeg=%9 speedMps=%10 target=%11 origin=%12 %13 %14")
        .arg(QString::fromLatin1(action), isTestPoint(point) ? "TEST" : "REAL", detection ? "DET" : "TRACK")
        .arg(point.batch).arg(point.statMethod)
        .arg(manual ? "MANUAL" : (!detection && m_manualTracks.contains(point.batch)
            ? (m_autoTracks ? "AUTO+MANUAL" : "MANUAL_CONTINUOUS") : "AUTO"))
        .arg(point.range, 0, 'f', 2).arg(point.azimuth, 0, 'f', 3).arg(point.elevation, 0, 'f', 3)
        .arg(point.speed, 0, 'f', 2).arg(point.targetRecResult)
        .arg(originSource(point), coordinates, fields));
}

void ElectromagneticDeviceModule::logSummary()
{
    if (!m_enabled || m_clock.elapsed() - m_lastLogSummary < m_logSummaryMs) return;
    m_lastLogSummary = m_clock.elapsed();
    LOG_INFO(QStringLiteral("[电磁设备][SUMMARY] inputDet=%1 inputTrack=%2 testPrepared=%3 queuedDet=%4 trackUpserts=%5 removals=%6 socketSent=%7 failed=%8 rejected=%9 droppedDet=%10 pendingDet=%11 pendingTrack=%12 tracks=%13 manual=%14 autoDet=%15 autoTrack=%16 ready=%17 busy=%18 waitNotReady=%19 waitBusy=%20 maxTxMs=%21 msg5=%22 msg6=%23 msg7=%24 msg9=%25")
        .arg(m_detInputs).arg(m_trackInputs).arg(m_testInputs).arg(m_detQueued).arg(m_trackQueued).arg(m_removals)
        .arg(m_sent).arg(m_failed).arg(m_rejected).arg(m_dropped)
        .arg(m_detections.size() + m_manualDetections.size()).arg(m_pendingTracks.size()).arg(m_trackPoints.size())
        .arg(m_manualTracks.size()).arg(m_autoDetections).arg(m_autoTracks).arg(m_ready).arg(m_busy)
        .arg(m_flushNotReady).arg(m_flushBusy).arg(m_maxTxMs)
        .arg(m_messageCounts[5]).arg(m_messageCounts[6]).arg(m_messageCounts[7]).arg(m_messageCounts[9]));
}

void ElectromagneticDeviceModule::updateRadarPosition(double latitude, double longitude, double altitude)
{
    const Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (!Wgs84Coordinate::isValid(origin)) { warn(QStringLiteral("[ORIGIN] DD05 雷达原点无效"), Warning::Origin); return; }
    const bool changed = !m_originValid || m_origin.longitudeDeg != longitude
        || m_origin.latitudeDeg != latitude || m_origin.altitudeM != altitude;
    if (changed && (!m_originValid || m_clock.elapsed() - m_lastOriginLog >= 5000)) {
        m_lastOriginLog = m_clock.elapsed();
        LOG_INFO("[电磁设备][ORIGIN] DD05 原点首次有效/发生变化；优先于测试原点（变化日志限频5秒，原点仍逐次更新）");
    }
    m_origin = origin; m_originValid = true;
    if (changed) emit radarOriginChanged();
}

void ElectromagneticDeviceModule::processDetectionFrame(const QByteArray& frame)
{
    if (!m_enabled || !m_validConfig) return;
    int count = 0;
    constexpr int prefix = 14 + 2 + 512 + 1 + 2;
    if (!internalFrame(frame, 0xDD01, prefix, sizeof(detInfo), count)) {
        ++m_rejected; warn(QStringLiteral("[META_DET] 检测帧长度或数量错误"), Warning::DetectionFrame); return;
    }
    m_lastDetection = m_clock.elapsed();
    // Raw frames carry supplementary fields only; PointInfo is the sole reporting entry.
    for (int i = 0; i < count; ++i) {
        detInfo point;
        std::memcpy(&point, frame.constData() + prefix + i * sizeof(detInfo), sizeof(point));
        PointInfo info{};
        info.range = point.dis; info.azimuth = point.azi; info.elevation = point.ele;
        info.speed = point.vel; info.amp = point.amp;
        const QByteArray key = detectionKey(info);
        m_testDetectionKeys.remove(key);
        if (!m_detectionMetadata.contains(key)) m_detectionMetadataOrder.enqueue(key);
        m_detectionMetadata.insert(key, point);
        while (m_detectionMetadataOrder.size() > kMaxPendingDetections)
            m_detectionMetadata.remove(m_detectionMetadataOrder.dequeue());
    }
}

void ElectromagneticDeviceModule::processTrackFrame(const QByteArray& frame)
{
    if (!m_enabled || !m_validConfig) return;
    int count = 0;
    constexpr int prefix = 18;
    if (!internalFrame(frame, 0xEE01, prefix, sizeof(trackInfo), count)) {
        ++m_rejected; warn(QStringLiteral("[META_TRACK] 航迹帧长度或数量错误"), Warning::TrackFrame); return;
    }
    m_lastTrack = m_clock.elapsed();
    for (int i = 0; i < count; ++i) {
        trackInfo point;
        std::memcpy(&point, frame.constData() + prefix + i * sizeof(trackInfo), sizeof(point));
        m_testTrackKeys.remove(point.batch);
        if (point.statMethod != 2) m_trackMetadata.insert(point.batch, point);
    }
}

void ElectromagneticDeviceModule::setTestRadarPosition(double latitude, double longitude, double altitude)
{
    const Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (Wgs84Coordinate::isValid(origin)) {
        m_testOrigin = origin; m_testOriginValid = true;
        LOG_INFO(QStringLiteral("[电磁设备][ORIGIN_TEST] 生成器提供测试原点，DD05优先=%1；不作为真实设备位置").arg(m_originValid));
    } else warn(QStringLiteral("[ORIGIN_TEST] 测试原点无效"), Warning::Origin);
}

void ElectromagneticDeviceModule::prepareTestPoint(PointInfo point)
{
    if (!m_enabled || !m_validConfig) return;
    if (point.type == PointType::Track && point.batch <= 65535) {
        const quint16 batch = quint16(point.batch);
        if (point.statMethod == 2) return; // Common entry owns lifecycle cleanup.
        ++m_testInputs;
        m_testTrackKeys.insert(batch, detectionKey(point));
        trackInfo record{};
        record.batch = batch; record.dis = point.range; record.azi = point.azimuth; record.ele = point.elevation;
        const qint64 milliseconds = QDateTime::currentMSecsSinceEpoch();
        record.UTCtime = unsigned(milliseconds / 1000); record.nsecond = unsigned(milliseconds % 1000) * 1000000u;
        record.spaceVel = point.speed; // Fixed-ray test trajectory.
        m_trackMetadata.insert(batch, record);
    } else if (point.type == PointType::Detection) {
        ++m_testInputs;
        const QByteArray key = detectionKey(point);
        if (!m_testDetectionKeys.contains(key)) {
            m_testDetectionKeys.insert(key); m_testDetectionOrder.enqueue(key);
        }
        while (m_testDetectionOrder.size() > kMaxPendingDetections)
            m_testDetectionKeys.remove(m_testDetectionOrder.dequeue());
    }
}

bool ElectromagneticDeviceModule::originFor(const PointInfo& point, Wgs84Coordinate::Lla& origin) const
{
    if (m_manualOriginValid) { origin = m_manualOrigin; return true; }
    if (m_originValid) { origin = m_origin; return true; }
    if (m_configOriginValid) { origin = m_configOrigin; return true; }
    const bool testTrack = point.type == PointType::Track && point.batch <= 65535
        && m_testTrackKeys.contains(quint16(point.batch))
        && m_testTrackKeys.value(quint16(point.batch)) == detectionKey(point);
    const bool testDetection = point.type == PointType::Detection && m_testDetectionKeys.contains(detectionKey(point));
    if (m_testOriginValid && (testTrack || testDetection)) { origin = m_testOrigin; return true; }
    return false;
}

QString ElectromagneticDeviceModule::originSource(const PointInfo& point) const
{
    if (m_manualOriginValid) return QStringLiteral("MANUAL");
    if (m_originValid) return QStringLiteral("DD05");
    if (m_configOriginValid) return QStringLiteral("CONFIG_FALLBACK");
    if (m_testOriginValid && isTestPoint(point)) return QStringLiteral("TEST_FALLBACK");
    return QStringLiteral("NONE");
}

bool ElectromagneticDeviceModule::currentRadarOrigin(Wgs84Coordinate::Lla& origin, QString& source) const
{
    PointInfo point{};
    source = originSource(point);
    return originFor(point, origin);
}

void ElectromagneticDeviceModule::refreshTrackCoordinates()
{
    // UI operation only: re-encode cached latest tracks without inventing new source timestamps.
    m_detections.clear(); m_manualDetections.clear(); // Old-origin pending detections must not be mixed with new ones.
    for (auto it = m_trackPoints.cbegin(); it != m_trackPoints.cend(); ++it) {
        const QByteArray record = encodeTrack(it.value());
        if (record.isEmpty()) { m_pendingTracks.remove(it.key()); continue; }
        m_lastTracks.insert(it.key(), record);
        if (m_reportTracks && (m_autoTracks || m_manualTracks.contains(it.key()))) {
            m_pendingTracks.insert(it.key(), record); m_reportedTracks.insert(it.key()); ++m_trackQueued;
        } else if (m_pendingTracks.contains(it.key())) m_pendingTracks.remove(it.key());
    }
    // Already-dispatched UDP batches cannot be recalled; subsequent batches use the new origin.
    emit radarOriginChanged();
}

bool ElectromagneticDeviceModule::setManualRadarPosition(double latitude, double longitude, double altitude)
{
    const Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (!m_enabled || !Wgs84Coordinate::isValid(origin)) return false;
    m_manualOrigin = origin; m_manualOriginValid = true;
    refreshTrackCoordinates();
    LOG_INFO(QStringLiteral("[电磁设备][ORIGIN_MANUAL] lon=%1 lat=%2 altitudeM=%3；本次运行有效，优先于DD05，未修改旧链路/配置文件")
        .arg(longitude, 0, 'f', 7).arg(latitude, 0, 'f', 7).arg(altitude, 0, 'f', 3));
    emit diagnosticLog(QStringLiteral("手动原点已生效：经度%1 纬度%2 高度%3m")
        .arg(longitude, 0, 'f', 7).arg(latitude, 0, 'f', 7).arg(altitude, 0, 'f', 3));
    return true;
}

void ElectromagneticDeviceModule::restoreAutomaticRadarPosition()
{
    m_manualOriginValid = false; refreshTrackCoordinates();
    LOG_INFO("[电磁设备][ORIGIN_MANUAL] 已恢复自动原点，优先DD05，其次配置备用");
    emit diagnosticLog(QStringLiteral("已恢复自动解算原点：DD05优先，配置经纬高备用"));
}

void ElectromagneticDeviceModule::showOriginDialog(QWidget* parent)
{
    if (!m_enabled || !m_uiEnabled) return;
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("手动配置经纬高"));
    dialog.setStyleSheet("QDialog { background:#071b17; color:#66ffcc; } QLabel { color:#66ffcc; }"
        "QDoubleSpinBox { background:#102820; color:#d9ffff; padding:6px; border:1px solid #00bfa5; }"
        "QPushButton { background:#075a58; color:#d9ffff; padding:6px 12px; border:1px solid #00bfa5; }");
    auto* layout = new QVBoxLayout(&dialog);
    Wgs84Coordinate::Lla origin; QString source;
    currentRadarOrigin(origin, source);
    auto* hint = new QLabel(&dialog);
    hint->setWordWrap(true);
    hint->hide();
    layout->addWidget(hint);
    auto* form = new QFormLayout(); layout->addLayout(form);
    auto makeInput = [&dialog, form](const QString& name, double minimum, double maximum, int decimals, double value) {
        auto* input = new QDoubleSpinBox(&dialog); input->setLocale(QLocale::c());
        input->setDecimals(decimals); input->setRange(minimum, maximum); input->setValue(value);
        form->addRow(name, input); return input;
    };
    auto* longitude = makeInput(QStringLiteral("经度(°)"), -180, 180, 7, origin.longitudeDeg);
    auto* latitude = makeInput(QStringLiteral("纬度(°)"), -90, 90, 7, origin.latitudeDeg);
    auto* altitude = makeInput(QStringLiteral("海拔高度(m)"), -2147483648.0, 2147483647.0, 3, origin.altitudeM);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    auto* automatic = buttons->addButton(QStringLiteral("恢复自动"), QDialogButtonBox::ResetRole);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (!longitude->hasAcceptableInput() || !latitude->hasAcceptableInput() || !altitude->hasAcceptableInput()) {
            hint->setText(QStringLiteral("请输入有效的经纬度和高度。"));
            hint->show();
            return;
        }
        longitude->interpretText(); latitude->interpretText(); altitude->interpretText();
        if (setManualRadarPosition(latitude->value(), longitude->value(), altitude->value())) dialog.accept();
    });
    connect(automatic, &QPushButton::clicked, &dialog, [&] { restoreAutomaticRadarPosition(); dialog.accept(); });
    dialog.exec();
}

QByteArray ElectromagneticDeviceModule::detectionKey(const PointInfo& point) const
{
    const float values[]{point.range, point.azimuth, point.elevation, point.speed, point.amp};
    return QByteArray(reinterpret_cast<const char*>(values), sizeof(values));
}

QByteArray ElectromagneticDeviceModule::encodeDetection(const PointInfo& point) const
{
    Wgs84Coordinate::Lla origin;
    if (!originFor(point, origin)) return {};
    const QByteArray key = detectionKey(point);
    const bool test = m_testDetectionKeys.contains(key);
    if (!test && !m_detectionMetadata.contains(key)) return {};
    detInfo record = test ? detInfo{} : m_detectionMetadata.value(key, detInfo{});
    record.dis = point.range; record.azi = point.azimuth; record.ele = point.elevation;
    record.vel = point.speed; record.amp = point.amp; record.CFARSNR = point.SNR;
    record.targetConfidence = point.targetConfidence; record.targetRecResult = point.targetRecResult;
    return ElectromagneticProtocol::detection(record, origin);
}

QByteArray ElectromagneticDeviceModule::encodeTrack(const PointInfo& point) const
{
    Wgs84Coordinate::Lla origin;
    if (!originFor(point, origin)) return {};
    if (!m_trackMetadata.contains(quint16(point.batch))) return {};
    trackInfo record = m_trackMetadata.value(quint16(point.batch));
    if (record.dis != point.range || record.azi != point.azimuth || record.ele != point.elevation)
        return {}; // Do not attach the timestamp of a different sample to a synthetic/replayed point.
    record.batch = quint16(point.batch); record.statMethod = point.statMethod;
    record.dis = point.range; record.azi = point.azimuth; record.ele = point.elevation;
    record.vel = point.speed; record.amp = point.amp; record.SNR = point.SNR;
    record.targetRecResult = m_targetClasses.value(record.batch, quint8(point.targetRecResult));
    return ElectromagneticProtocol::track(record, origin);
}

bool ElectromagneticDeviceModule::sendDetection(const PointInfo& point)
{
    const bool ok = enqueueDetection(point, false);
    LOG_INFO(QStringLiteral("[电磁设备][MANUAL_DET] result=%1 enabled=%2 validConfig=%3 reportPermit=%4 ready=%5 pending=%6 reason=%7")
        .arg(ok ? "QUEUED" : "REJECTED").arg(m_enabled).arg(m_validConfig).arg(m_reportDetections)
        .arg(m_ready).arg(m_detections.size() + m_manualDetections.size())
        .arg(ok ? "NONE" : (!m_reportDetections ? QStringLiteral("REPORT_DISABLED")
            : (m_detections.size() + m_manualDetections.size() >= kMaxPendingDetections ? QStringLiteral("QUEUE_FULL") : rejectionReason(point)))));
    return ok;
}

bool ElectromagneticDeviceModule::enqueueDetection(const PointInfo& point, bool automatic)
{
    if (!m_enabled || !m_validConfig || !m_reportDetections || point.type != PointType::Detection) return false;
    const QByteArray record = encodeDetection(point);
    if (record.size() != ElectromagneticProtocol::DetectionBytes) {
        ++m_rejected; warn(QStringLiteral("[DET_REJECT] reason=%1").arg(rejectionReason(point)), Warning::Detection); return false;
    }
    if (m_detections.size() + m_manualDetections.size() >= kMaxPendingDetections) {
        ++m_dropped; warn(QStringLiteral("[QUEUE_FULL] 检测点待发队列已满（2048）"), Warning::Queue); return false;
    }
    (automatic ? m_detections : m_manualDetections).enqueue(record);
    ++m_detQueued; logPoint("DET_QUEUE", point, record, !automatic); return true;
}

void ElectromagneticDeviceModule::processDetectionPoint(PointInfo point)
{
    if (!m_enabled || point.type != PointType::Detection) return;
    ++m_detInputs;
    m_lastDetection = m_clock.elapsed();
    if (m_autoDetections) enqueueDetection(point, true);
}

void ElectromagneticDeviceModule::processTrackPoint(PointInfo point)
{
    if (!m_enabled || !m_validConfig) return;
    if (point.type != PointType::Track || point.batch > 65535) {
        ++m_rejected; warn(QStringLiteral("[TRACK_REJECT] 非普通航迹或批号超出uint16"), Warning::Track); return;
    }
    ++m_trackInputs;
    m_lastTrack = m_clock.elapsed();
    const quint16 batch = quint16(point.batch);
    if (point.statMethod == 2) {
        QByteArray record = m_lastTracks.take(batch);
        if (m_reportedTracks.remove(batch) && !record.isEmpty()) {
            record[12] = char(2); m_pendingTracks.insert(batch, record);
            ++m_removals; logPoint("TRACK_REMOVE", point, record);
        } else m_pendingTracks.remove(batch);
        m_trackPoints.remove(batch); m_trackMetadata.remove(batch);
        m_testTrackKeys.remove(batch);
        m_targetClasses.remove(batch); m_manualTracks.remove(batch); return;
    }
    if (point.statMethod > 1) {
        ++m_rejected; warn(QStringLiteral("[TRACK_REJECT] statMethod超出0..2"), Warning::Track); return;
    }
    m_trackPoints.insert(batch, point);
    const QByteArray record = encodeTrack(point);
    if (record.size() != ElectromagneticProtocol::TrackBytes) {
        ++m_rejected;
        warn(QStringLiteral("[TRACK_REJECT] reason=%1").arg(rejectionReason(point)), Warning::Track); return;
    }
    m_lastTracks.insert(batch, record);
    if (m_reportTracks && (m_autoTracks || m_manualTracks.contains(batch))) {
        m_pendingTracks.insert(batch, record); m_reportedTracks.insert(batch);
        ++m_trackQueued; logPoint("TRACK_QUEUE", point, record);
    }
}

bool ElectromagneticDeviceModule::toggleManualReport(quint32 batch)
{
    if (!m_enabled || !m_validConfig || batch > 65535 || !m_trackPoints.contains(quint16(batch))) {
        LOG_INFO(QStringLiteral("[电磁设备][MANUAL_TRACK] batch=%1 result=REJECTED reason=DISABLED_OR_NO_CURRENT_TRACK").arg(batch)); return false;
    }
    if (m_manualTracks.remove(batch)) {
        if (!m_autoTracks) m_pendingTracks.remove(quint16(batch));
        LOG_INFO(QStringLiteral("[电磁设备][MANUAL_TRACK] batch=%1 action=STOP autoStillActive=%2 ready=%3")
            .arg(batch).arg(m_autoTracks && m_reportTracks).arg(m_ready));
        emit reportingStateChanged();
        return true;
    }
    const QByteArray record = encodeTrack(m_trackPoints.value(quint16(batch)));
    if (!m_reportTracks || record.isEmpty()) {
        LOG_INFO(QStringLiteral("[电磁设备][MANUAL_TRACK] batch=%1 result=REJECTED reason=%2")
            .arg(batch).arg(!m_reportTracks ? QStringLiteral("REPORT_DISABLED") : rejectionReason(m_trackPoints.value(quint16(batch)))));
        warn(QStringLiteral("航迹无法下发：上报关闭或原点/数值无效"), Warning::Track); return false;
    }
    m_manualTracks.insert(batch); m_lastTracks.insert(quint16(batch), record);
    m_pendingTracks.insert(quint16(batch), record); m_reportedTracks.insert(batch);
    ++m_trackQueued; logPoint("MANUAL_TRACK_QUEUE", m_trackPoints.value(quint16(batch)), record, true);
    LOG_INFO(QStringLiteral("[电磁设备][MANUAL_TRACK] batch=%1 action=START result=QUEUED ready=%2")
        .arg(batch).arg(m_ready));
    emit reportingStateChanged(); return true;
}

void ElectromagneticDeviceModule::setAutoTracks(bool enabled)
{
    LOG_INFO(QStringLiteral("[电磁设备][MODE] autoTrack=%1 -> %2 permit=%3 latestTracks=%4 manual=%5")
        .arg(m_autoTracks).arg(enabled).arg(m_reportTracks).arg(m_trackPoints.size()).arg(m_manualTracks.size()));
    m_autoTracks = enabled;
    for (auto it = m_trackPoints.cbegin(); it != m_trackPoints.cend(); ++it) {
        if (!m_reportTracks || (!enabled && !m_manualTracks.contains(it.key()))) continue;
        const QByteArray record = encodeTrack(it.value());
        if (record.isEmpty()) continue;
        m_lastTracks.insert(it.key(), record); m_pendingTracks.insert(it.key(), record);
        m_reportedTracks.insert(it.key());
        ++m_trackQueued;
    }
    if (!enabled) {
        for (auto it = m_pendingTracks.begin(); it != m_pendingTracks.end();) {
            if (quint8(it.value()[12]) == 2 || m_manualTracks.contains(it.key())) ++it;
            else it = m_pendingTracks.erase(it);
        }
    }
    emit reportingStateChanged();
}

void ElectromagneticDeviceModule::setAutoDetections(bool enabled)
{
    LOG_INFO(QStringLiteral("[电磁设备][MODE] autoDet=%1 -> %2 permit=%3 pendingAuto=%4 pendingManual=%5")
        .arg(m_autoDetections).arg(enabled).arg(m_reportDetections).arg(m_detections.size()).arg(m_manualDetections.size()));
    m_autoDetections = enabled;
    if (!enabled) m_detections.clear();
}

void ElectromagneticDeviceModule::processBitReport(BITReport report)
{
    if (report.radarId != 0 || report.yaw > 36000 || report.scanAngle > 36000) return;
    m_bit = ElectromagneticProtocol::panel(report); m_lastBit = m_clock.elapsed();
}

void ElectromagneticDeviceModule::processSoftwareReport(MonitorParam report)
{
    const QByteArray encoded = ElectromagneticProtocol::software(report);
    if (encoded.isEmpty()) { warn(QStringLiteral("UI软件状态超出0..5，跳过更新；线上状态仍限定0..3"), Warning::Software); return; }
    QByteArray uiValues;
    uiValues.append(char(report.dataProSta)); uiValues.append(char(report.beamConSta));
    uiValues.append(char(report.sigProSta)); uiValues.append(char(report.targetRecSta));
    if (uiValues != m_lastUiSoftwareLog) {
        m_lastUiSoftwareLog = uiValues;
        LOG_INFO(QStringLiteral("[电磁设备][STATUS_UI] UI=[%1] WIRE=[%2]（数据/波束/信号/识别，UI的4/5映射为1）")
            .arg(QString::fromLatin1(uiValues.toHex(' ')), QString::fromLatin1(encoded.toHex(' '))));
    }
    m_software = encoded; m_lastSoftware = m_clock.elapsed();
}

void ElectromagneticDeviceModule::setInitialSoftwareStatus(MonitorParam report)
{
    const QByteArray encoded = ElectromagneticProtocol::software(report);
    if (encoded.isEmpty()) {
        warn(QStringLiteral("[STATUS_UI] UI软件状态超出0..5，初始异常值不变"), Warning::Software); return;
    }
    m_software = encoded;
    LOG_INFO(QStringLiteral("[电磁设备][STATUS_UI_INIT] data=%1 beam=%2 signal=%3 target=%4（与UI共用状态，非结构体默认值）")
        .arg(report.dataProSta).arg(report.beamConSta).arg(report.sigProSta).arg(report.targetRecSta));
}

void ElectromagneticDeviceModule::processTargetClassification(TargetClaRes report)
{
    if (report.claRes > 5) return;
    const quint16 batch = report.batchID;
    m_targetClasses.insert(batch, report.claRes);
    auto it = m_lastTracks.find(batch);
    if (it == m_lastTracks.end()) return;
    const quint32 category = report.claRes == 1 ? 1 : 0;
    qToLittleEndian<quint32>(category, reinterpret_cast<uchar*>(it.value().data()) + 49);
    if (m_reportTracks && (m_autoTracks || m_manualTracks.contains(batch))) {
        m_pendingTracks.insert(batch, it.value()); m_reportedTracks.insert(batch);
        ++m_trackQueued;
    }
}

QByteArray ElectromagneticDeviceModule::wrap(quint16 message, const QByteArray& payload)
{
    if (message < m_messageCounts.size()) ++m_messageCounts[message];
    return ElectromagneticProtocol::encode(m_localId, m_peerId, m_sequence++, message, payload);
}

void ElectromagneticDeviceModule::flush()
{
    if (!m_ready) { ++m_flushNotReady; return; }
    if (m_busy) { ++m_flushBusy; return; }
    const quint32 firstSequence = m_sequence;
    QList<QByteArray> packets;
    auto takeRecords = [this, &packets](quint16 message, int maximum, auto take, auto available) {
        while (available() && packets.size() < kMaxPacketsPerBatch - 2) {
            QByteArray records;
            quint16 count = 0;
            while (available() && count < maximum) { records.append(take()); ++count; }
            QByteArray payload;
            QDataStream out(&payload, QIODevice::WriteOnly); out.setByteOrder(QDataStream::LittleEndian);
            out << count; payload.append(records); packets.append(wrap(message, payload));
        }
    };
    // Tracks first: detection overload must not starve track lifecycle updates.
    takeRecords(6, 23, [this] {
        auto it = m_pendingTracks.lowerBound(m_trackCursor);
        if (it == m_pendingTracks.end()) it = m_pendingTracks.begin();
        m_trackCursor = quint16(it.key() + 1);
        const QByteArray value = it.value();
        m_pendingTracks.erase(it); return value;
    }, [this] { return !m_pendingTracks.isEmpty(); });
    takeRecords(5, 22, [this] {
        return m_manualDetections.isEmpty() ? m_detections.dequeue() : m_manualDetections.dequeue();
    }, [this] { return !m_manualDetections.isEmpty() || !m_detections.isEmpty(); });
    if (m_reportStatus && m_clock.elapsed() - m_lastStatus >= m_statusInterval) {
        m_lastStatus = m_clock.elapsed();
        packets.append(wrap(7, m_bit));
        packets.append(wrap(9, m_software));
        auto source = [this](qint64 updated) {
            return updated < 0 ? QStringLiteral("DEFAULT")
                : (m_clock.elapsed() - updated < m_receiveTimeout ? QStringLiteral("REAL_LATEST") : QStringLiteral("REAL_CACHED_STALE"));
        };
        const QString sources = QStringLiteral("panel=%1 software=%2").arg(source(m_lastBit), source(m_lastSoftware));
        if (sources != m_lastStatusSourceLog || m_clock.elapsed() - m_lastStatusLog >= m_logSummaryMs) {
            m_lastStatusSourceLog = sources; m_lastStatusLog = m_clock.elapsed();
            LOG_INFO(QStringLiteral("[电磁设备][STATUS_TX] %1 intervalMs=%2 bitAgeMs=%3 softwareAgeMs=%4；默认/旧缓存不代表实测在线")
                .arg(sources).arg(m_statusInterval)
                .arg(m_lastBit < 0 ? -1 : m_clock.elapsed() - m_lastBit)
                .arg(m_lastSoftware < 0 ? -1 : m_clock.elapsed() - m_lastSoftware));
        }
    }
    if (!packets.isEmpty()) {
        m_busy = true; m_inFlightStarted = m_clock.elapsed();
        m_inFlightFirstSequence = firstSequence; m_inFlightPackets = packets.size();
        emit outbound(packets);
    }
}

void ElectromagneticDeviceModule::receive(const QByteArray& packet)
{
    ElectromagneticProtocol::Frame frame;
    QString error;
    if (!ElectromagneticProtocol::decode(packet, frame, error)
        || frame.source != m_peerId || frame.destination != m_localId
        || !ElectromagneticProtocol::validCommand(frame.message, frame.payload)) {
        ++m_rejected; warn(QStringLiteral("[RX_REJECT] bytes=%1 source=0x%2 dest=0x%3 msg=%4 reason=%5")
            .arg(packet.size()).arg(frame.source, 4, 16, QLatin1Char('0')).arg(frame.destination, 4, 16, QLatin1Char('0'))
            .arg(frame.message).arg(error.isEmpty() ? "设备 ID/命令字段错误" : error), Warning::Receive); return;
    }
    const qint64 now = m_clock.elapsed();
    // Unsigned modular difference handles counter wrap. After inactivity accept a restarted peer.
    const quint32 delta = frame.sequence - m_lastSequence;
    if (m_receivedSequence && now - m_lastReceive <= m_receiveTimeout
        && (delta == 0 || delta >= 0x80000000u)) {
        ++m_rejected; warn(QStringLiteral("[RX_SEQUENCE] 重复/乱序 seq=%1 last=%2 delta=%3")
            .arg(frame.sequence).arg(m_lastSequence).arg(delta), Warning::Sequence); return;
    }
    m_lastSequence = frame.sequence; m_receivedSequence = true;
    m_lastReceive = now; ++m_received;
    m_lastCommand = QStringLiteral("编号 %1，参数 %2，计数 %3%4")
        .arg(frame.message).arg(QString::fromLatin1(frame.payload.toHex(' '))).arg(frame.sequence)
        .arg(m_remoteControl ? QString() : QStringLiteral("（控制已关闭，未执行）"));
    LOG_INFO("[电磁设备][RX] " + m_lastCommand);
    emit diagnosticLog(m_lastCommand);
    if (m_remoteControl) emit controlRequested(frame.message, frame.payload);
}

void ElectromagneticDeviceModule::pollNetwork()
{
    m_localOk = false;
    for (const auto& interface : QNetworkInterface::allInterfaces()) {
        if (!interface.flags().testFlag(QNetworkInterface::IsUp)
            || !interface.flags().testFlag(QNetworkInterface::IsRunning)) continue;
        for (const auto& entry : interface.addressEntries()) {
            if (entry.ip() == QHostAddress(m_localIp)) m_localOk = true;
        }
    }
    if (!m_localOk) { m_peerOk = false; m_peerText = QStringLiteral("本机网口不可用"); return; }
    if (m_pingRunning || m_ping.state() != QProcess::NotRunning) return;
    const QString program = QStandardPaths::findExecutable("ping");
    if (program.isEmpty()) { m_peerOk = false; m_peerText = QStringLiteral("未找到 ping"); return; }
    m_pingRunning = true;
#ifdef Q_OS_WIN
    const QStringList args{"-4", "-n", "1", "-w", "1000", "-S", m_localIp, m_peerIp};
#else
    const QStringList args{"-4", "-n", "-c", "1", "-W", "1", "-I", m_localIp, m_peerIp};
#endif
    m_pingTimeout.start(3000); m_ping.start(program, args);
}

void ElectromagneticDeviceModule::finishPing(bool ok)
{
    if (!m_pingRunning) return;
    m_pingRunning = false; m_pingTimeout.stop();
    if (ok) { m_failures = 0; m_peerOk = true; m_peerText = QStringLiteral("ICMP 可达"); }
    else {
        m_failures = qMin(3, m_failures + 1);
        if (m_failures == 3) m_peerOk = false;
        m_peerText = QStringLiteral("Ping 失败 %1/3").arg(m_failures);
    }
}

void ElectromagneticDeviceModule::showWindow(QWidget* parent)
{
    if (!m_enabled || !m_uiEnabled) return;
    if (!m_window) {
        auto* dialog = new QDialog(parent);
        dialog->setWindowTitle(QStringLiteral("电磁设备通信"));
        dialog->setObjectName("ElectromagneticDeviceWindow");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->resize(720, 430);
        dialog->setStyleSheet("QDialog#ElectromagneticDeviceWindow { background:#071b17; color:#66ffcc; }"
                              "QDialog#ElectromagneticDeviceWindow QLabel { color:#66ffcc; padding:6px; }"
                              "QDialog#ElectromagneticDeviceWindow QCheckBox { color:#66ffcc; padding:6px; }");
        auto* layout = new QVBoxLayout(dialog);
        layout->addWidget(new QLabel(QStringLiteral("显控 %1:%2 (0x%3) → 电磁设备 %4:%5 (0x%6)")
            .arg(m_localIp).arg(m_localPort).arg(m_localId, 4, 16, QLatin1Char('0'))
            .arg(m_peerIp).arg(m_peerPort).arg(m_peerId, 4, 16, QLatin1Char('0')), dialog));
        for (auto& label : m_labels) { label = new QLabel(dialog); label->setWordWrap(true); layout->addWidget(label); }
        auto toggle = [this, dialog, layout](const QString& text, bool value, auto handler) {
            auto* box = new QCheckBox(text, dialog); box->setChecked(value); layout->addWidget(box);
            QObject::connect(box, &QCheckBox::toggled, dialog, handler);
        };
        toggle(QStringLiteral("检测点上报（本次运行）"), m_reportDetections, [this](bool on) {
            LOG_INFO(QStringLiteral("[电磁设备][MODE] reportDet=%1 -> %2 pending=%3")
                .arg(m_reportDetections).arg(on).arg(m_detections.size() + m_manualDetections.size()));
            m_reportDetections = on;
            if (!on) { m_detections.clear(); m_manualDetections.clear(); }
        });
        toggle(QStringLiteral("自动下发检测点（全部有效点迹）"), m_autoDetections, [this](bool on) { setAutoDetections(on); });
        toggle(QStringLiteral("自动下发航迹（全部有效普通航迹）"), m_autoTracks, [this](bool on) { setAutoTracks(on); });
        toggle(QStringLiteral("航迹上报（本次运行）"), m_reportTracks, [this](bool on) {
            LOG_INFO(QStringLiteral("[电磁设备][MODE] reportTrack=%1 -> %2 pending=%3")
                .arg(m_reportTracks).arg(on).arg(m_pendingTracks.size()));
            m_reportTracks = on;
            if (!on) {
                for (auto it = m_pendingTracks.begin(); it != m_pendingTracks.end();) {
                    if (quint8(it.value()[12]) == 2) ++it;
                    else it = m_pendingTracks.erase(it);
                }
            } else {
                setAutoTracks(m_autoTracks);
            }
            emit reportingStateChanged();
        });
        toggle(QStringLiteral("允许电磁设备下发控制（本次运行）"), m_remoteControl, [this](bool on) {
            LOG_INFO(QStringLiteral("[电磁设备][MODE] remoteControl=%1 -> %2").arg(m_remoteControl).arg(on));
            m_remoteControl = on;
        });
        m_window = dialog;
        connect(dialog, &QObject::destroyed, this, [this] { m_labels.fill(nullptr); });
    }
    updateUi(); m_window->show(); m_window->raise(); m_window->activateWindow();
}

void ElectromagneticDeviceModule::updateUi()
{
    logSummary();
    const qint64 now = m_clock.elapsed();
    const bool receivedRecently = m_lastReceive >= 0 && now - m_lastReceive <= m_receiveTimeout;
    const QString summary = QStringLiteral("网络:%1 / UDP:%2 / 协议接收:%3")
        .arg(m_networkEnabled ? (m_peerOk ? "可达" : "检测中/不可达") : "未检测",
             m_ready ? "就绪" : "未就绪", receivedRecently ? "有效报文" : "未收到/超时");
    const QString linkLog = QStringLiteral("%1 localOk=%2 peerOk=%3 peerDetail=%4 DD05=%5 testOrigin=%6 detAlive=%7 trackAlive=%8 configOrigin=%9 manualOrigin=%10")
        .arg(summary).arg(m_localOk).arg(m_peerOk).arg(m_peerText).arg(m_originValid).arg(m_testOriginValid)
        .arg(m_lastDetection >= 0 && now - m_lastDetection <= m_receiveTimeout)
        .arg(m_lastTrack >= 0 && now - m_lastTrack <= m_receiveTimeout).arg(m_configOriginValid).arg(m_manualOriginValid);
    if (linkLog != m_lastLinkLog) { m_lastLinkLog = linkLog; LOG_INFO("[电磁设备][LINK] " + linkLog); }
    emit statusChanged(summary);
    emit linkStateChanged(m_ready && (!m_networkEnabled || m_peerOk), summary);
    if (!m_window || !m_window->isVisible()) return;
    auto show = [this](int index, bool good, const QString& text) {
        m_labels[index]->setText(text);
        m_labels[index]->setStyleSheet(QStringLiteral("color:%1; background:#102820; padding:6px;")
            .arg(good ? "#66ffcc" : "#ff9090"));
    };
    show(0, m_localOk, m_networkEnabled ? QStringLiteral("本机网口：%1").arg(m_localOk ? "正常" : "不可用") : QStringLiteral("网络检测已关闭"));
    show(1, m_peerOk, QStringLiteral("电磁设备网络：%1").arg(m_networkEnabled ? m_peerText : "未启用检测"));
    show(2, m_ready, m_transportText);
    show(3, receivedRecently, QStringLiteral("协议接收：%1；最近命令：%2")
        .arg(receivedRecently ? "有效报文" : "未收到/超时", m_lastCommand));
    const bool detAlive = m_lastDetection >= 0 && now - m_lastDetection <= m_receiveTimeout;
    const bool trackAlive = m_lastTrack >= 0 && now - m_lastTrack <= m_receiveTimeout;
    show(4, detAlive && trackAlive && (m_manualOriginValid || m_originValid || m_configOriginValid || m_testOriginValid), QStringLiteral("检测数据源：%1；航迹数据源：%2；原点：%3；手动航迹 %4")
        .arg(detAlive ? "有数据" : "未收到/超时", trackAlive ? "有数据" : "未收到/超时",
             m_manualOriginValid ? "手动原点" : (m_originValid ? "DD05 有效" : (m_configOriginValid ? "配置备用原点" : (m_testOriginValid ? "仅测试原点" : "未收到")))).arg(m_manualTracks.size()));
    show(5, m_failed == 0, QStringLiteral("发送成功 %1 / 失败 %2；有效接收 %3；拒绝 %4；检测丢弃 %5；待发 检测 %6 / 航迹 %7")
        .arg(m_sent).arg(m_failed).arg(m_received).arg(m_rejected).arg(m_dropped)
        .arg(m_detections.size() + m_manualDetections.size()).arg(m_pendingTracks.size()));
}
