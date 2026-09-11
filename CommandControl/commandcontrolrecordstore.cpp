/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:14:44
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
#include "commandcontrolrecordstore.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QJsonObject toJson(const CommandControlRecord& record)
{
    const auto& track = record.track;
    QJsonObject object;
    object.insert(QStringLiteral("direction"), record.outbound ? QStringLiteral("outbound") : QStringLiteral("inbound"));
    object.insert(QStringLiteral("observed_utc_ms"), static_cast<double>(record.observedUtcMs));
    object.insert(QStringLiteral("packet_hex"), QString::fromLatin1(record.packet.toHex()));
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
    object.insert(QStringLiteral("month_timestamp_ms"), static_cast<double>(track.monthTimestampMs));
    return object;
}

bool fromJson(const QJsonObject& object, CommandControlRecord& record)
{
    auto number = [&object](const char* key, qint64 fallback = 0) {
        const QJsonValue value = object.value(QLatin1String(key));
        return value.isDouble() ? static_cast<qint64>(value.toDouble()) : fallback;
    };

    record.outbound = object.value(QStringLiteral("direction")).toString() == QStringLiteral("outbound");
    record.observedUtcMs = number("observed_utc_ms");
    record.packet = QByteArray::fromHex(object.value(QStringLiteral("packet_hex")).toString().toLatin1());
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
    track.monthTimestampMs = static_cast<quint32>(number("month_timestamp_ms"));
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
