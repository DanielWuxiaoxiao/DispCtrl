/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:45
 * @Description: 
 */
#include "commandcontrolrecordstore.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace {

QJsonObject toJson(const CommandControlRecord& record)
{
    const auto& track = record.track;
    QJsonObject object;
    object.insert(QStringLiteral("direction"), record.outbound ? QStringLiteral("outbound") : QStringLiteral("inbound"));
    object.insert(QStringLiteral("observed_utc_ms"), static_cast<double>(record.observedUtcMs));
    object.insert(QStringLiteral("packet_hex"), QString::fromLatin1(record.packet.toHex()));
    object.insert(QStringLiteral("replay_radar_origin_valid"), record.hasReplayRadarOrigin);
    object.insert(QStringLiteral("replay_radar_longitude_deg"), record.replayRadarLongitudeDeg);
    object.insert(QStringLiteral("replay_radar_latitude_deg"), record.replayRadarLatitudeDeg);
    object.insert(QStringLiteral("replay_radar_altitude_m"), record.replayRadarAltitudeM);
    object.insert(QStringLiteral("comprehensive_batch"), static_cast<double>(track.comprehensiveBatch));
    object.insert(QStringLiteral("local_batch"), static_cast<double>(track.localBatch));
    object.insert(QStringLiteral("device_id"), static_cast<double>(track.deviceId));
    object.insert(QStringLiteral("device_type"), track.deviceType);
    object.insert(QStringLiteral("device_number"), track.deviceNumber);
    object.insert(QStringLiteral("rate_centi_hz"), track.rateCentiHz);
    object.insert(QStringLiteral("target_attribute"), track.targetAttribute);
    object.insert(QStringLiteral("target_type"), track.targetType);
    object.insert(QStringLiteral("track_quality"), track.trackQuality);
    object.insert(QStringLiteral("longitude_e7"), track.longitudeE7);
    object.insert(QStringLiteral("latitude_e7"), track.latitudeE7);
    object.insert(QStringLiteral("altitude_m"), track.altitudeM);
    object.insert(QStringLiteral("velocity_east"), track.velocityEast);
    object.insert(QStringLiteral("velocity_north"), track.velocityNorth);
    object.insert(QStringLiteral("velocity_up"), track.velocityUp);
    object.insert(QStringLiteral("rcs_milli_square_m"), track.rcsMilliSquareM);
    object.insert(QStringLiteral("interference_status"), track.interferenceStatus);
    object.insert(QStringLiteral("update_method"), track.updateMethod);
    object.insert(QStringLiteral("relative_delay_ms"), track.relativeDelayMs);
    object.insert(QStringLiteral("month_timestamp_packed"),
                  static_cast<double>(CommandControlProtocol::packMonthDayTime(track.timestamp)));
    object.insert(QStringLiteral("month_day"), track.timestamp.day);
    object.insert(QStringLiteral("month_hour"), track.timestamp.hour);
    object.insert(QStringLiteral("month_minute"), track.timestamp.minute);
    object.insert(QStringLiteral("month_second"), track.timestamp.second);
    object.insert(QStringLiteral("month_millisecond"), track.timestamp.millisecond);
    return object;
}

bool fromJson(const QJsonObject& object, CommandControlRecord& record)
{
    auto number = [&object](const char* key, qint64 fallback = 0) {
        const QJsonValue value = object.value(QLatin1String(key));
        return value.isDouble() ? static_cast<qint64>(value.toDouble()) : fallback;
    };
    auto decimal = [&object](const char* key) {
        return object.value(QLatin1String(key)).toDouble();
    };

    record.outbound = object.value(QStringLiteral("direction")).toString() == QStringLiteral("outbound");
    record.observedUtcMs = number("observed_utc_ms");
    record.packet = QByteArray::fromHex(object.value(QStringLiteral("packet_hex")).toString().toLatin1());
    record.hasReplayRadarOrigin = object.value(QStringLiteral("replay_radar_origin_valid")).toBool(false);
    if (record.hasReplayRadarOrigin) {
        if (!object.contains(QStringLiteral("replay_radar_longitude_deg"))
            || !object.contains(QStringLiteral("replay_radar_latitude_deg"))
            || !object.contains(QStringLiteral("replay_radar_altitude_m"))) {
            return false;
        }
        record.replayRadarLongitudeDeg = decimal("replay_radar_longitude_deg");
        record.replayRadarLatitudeDeg = decimal("replay_radar_latitude_deg");
        record.replayRadarAltitudeM = decimal("replay_radar_altitude_m");
        if (!std::isfinite(record.replayRadarLongitudeDeg) || !std::isfinite(record.replayRadarLatitudeDeg)
            || !std::isfinite(record.replayRadarAltitudeM)
            || record.replayRadarLongitudeDeg < -180.0 || record.replayRadarLongitudeDeg > 180.0
            || record.replayRadarLatitudeDeg < -90.0 || record.replayRadarLatitudeDeg > 90.0) {
            return false;
        }
    }
    auto& track = record.track;
    track.comprehensiveBatch = static_cast<quint32>(number("comprehensive_batch"));
    track.localBatch = static_cast<quint32>(number("local_batch"));
    track.deviceId = static_cast<quint32>(number("device_id"));
    track.deviceType = static_cast<quint8>(number("device_type"));
    track.deviceNumber = static_cast<quint8>(number("device_number"));
    track.rateCentiHz = static_cast<quint16>(number("rate_centi_hz"));
    track.targetAttribute = static_cast<quint8>(number("target_attribute"));
    track.targetType = static_cast<quint8>(number("target_type"));
    track.trackQuality = static_cast<quint8>(number("track_quality"));
    track.longitudeE7 = static_cast<qint32>(number("longitude_e7"));
    track.latitudeE7 = static_cast<qint32>(number("latitude_e7"));
    track.altitudeM = static_cast<qint32>(number("altitude_m"));
    track.velocityEast = static_cast<qint16>(number("velocity_east"));
    track.velocityNorth = static_cast<qint16>(number("velocity_north"));
    track.velocityUp = static_cast<qint16>(number("velocity_up"));
    track.rcsMilliSquareM = static_cast<quint16>(number("rcs_milli_square_m"));
    track.interferenceStatus = static_cast<quint8>(number("interference_status"));
    track.updateMethod = static_cast<quint8>(number("update_method"));
    track.relativeDelayMs = static_cast<quint16>(number("relative_delay_ms"));
    const bool hasTimestampComponents = object.contains(QStringLiteral("month_day"));
    if (hasTimestampComponents) {
        const qint64 day = number("month_day");
        const qint64 hour = number("month_hour");
        const qint64 minute = number("month_minute");
        const qint64 second = number("month_second");
        const qint64 millisecond = number("month_millisecond");
        if (day < 1 || day > 31 || hour < 0 || hour > 23
            || minute < 0 || minute > 59 || second < 0 || second > 59
            || millisecond < 0 || millisecond > 999) {
            return false;
        }
        track.timestamp.day = static_cast<quint8>(day);
        track.timestamp.hour = static_cast<quint8>(hour);
        track.timestamp.minute = static_cast<quint8>(minute);
        track.timestamp.second = static_cast<quint8>(second);
        track.timestamp.millisecond = static_cast<quint16>(millisecond);
    } else if (object.contains(QStringLiteral("month_timestamp_packed"))) {
        if (!CommandControlProtocol::unpackMonthDayTime(
                static_cast<quint32>(number("month_timestamp_packed")), track.timestamp)) {
            return false;
        }
    } else if (object.contains(QStringLiteral("month_timestamp_ms"))) {
        // 兼容修正前 JSONL：旧版本错误记录的是本月首日以来累计毫秒，
        // 仅在读取历史文件时转换为当前的 day/h/min/sec/ms 表达。
        const qint64 legacyElapsedMs = number("month_timestamp_ms");
        constexpr qint64 kDayMs = 24LL * 60LL * 60LL * 1000LL;
        constexpr qint64 kMaxMonthMs = 31LL * kDayMs;
        if (legacyElapsedMs < 0 || legacyElapsedMs >= kMaxMonthMs) {
            return false;
        }
        qint64 remainingMs = legacyElapsedMs;
        track.timestamp.day = static_cast<quint8>(remainingMs / kDayMs + 1);
        remainingMs %= kDayMs;
        track.timestamp.hour = static_cast<quint8>(remainingMs / (60LL * 60LL * 1000LL));
        remainingMs %= 60LL * 60LL * 1000LL;
        track.timestamp.minute = static_cast<quint8>(remainingMs / (60LL * 1000LL));
        remainingMs %= 60LL * 1000LL;
        track.timestamp.second = static_cast<quint8>(remainingMs / 1000LL);
        track.timestamp.millisecond = static_cast<quint16>(remainingMs % 1000LL);
    }
    return record.observedUtcMs > 0 && track.localBatch != 0;
}

}  // namespace

CommandControlRecordStore::~CommandControlRecordStore()
{
    close();
}

bool CommandControlRecordStore::startSession(const QString& directoryPath, QString* errorMessage)
{
    close();
    QDir directory;
    if (!directory.mkpath(directoryPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建记录目录：%1").arg(directoryPath);
        }
        return false;
    }

    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
    m_sessionFile.setFileName(QDir(directoryPath).filePath(QStringLiteral("command_control_%1.jsonl").arg(stamp)));
    if (!m_sessionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法打开记录文件：%1").arg(m_sessionFile.errorString());
        }
        return false;
    }
    return true;
}

void CommandControlRecordStore::close()
{
    if (m_sessionFile.isOpen()) {
        m_sessionFile.close();
    }
}

bool CommandControlRecordStore::append(const CommandControlRecord& record, QString* errorMessage)
{
    if (!m_sessionFile.isOpen()) {
        return true;
    }
    QByteArray line = QJsonDocument(toJson(record)).toJson(QJsonDocument::Compact);
    line.append('\n');
    if (m_sessionFile.write(line) != line.size() || !m_sessionFile.flush()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("记录文件写入失败：%1").arg(m_sessionFile.errorString());
        }
        return false;
    }
    return true;
}

QVector<CommandControlRecord> CommandControlRecordStore::load(const QString& filePath, QString* errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法读取记录文件：%1").arg(file.errorString());
        }
        return {};
    }

    QVector<CommandControlRecord> records;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);
        CommandControlRecord record;
        if (error.error == QJsonParseError::NoError && document.isObject() && fromJson(document.object(), record)) {
            records.append(record);
        }
    }
    if (records.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("文件中没有可回放的 DDA4 记录");
    }
    return records;
}
