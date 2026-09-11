/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:13:20
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:53
 * @Description: 
 */
#include "commandcontrolconfig.h"

#include "Basic/ConfigManager.h"
#include "Basic/log.h"

#include <QFile>
#include <QHash>
#include <QTextStream>

#include <limits>

namespace {

QHash<QString, QString> loadCommandControlValues()
{
    const QString configuredPath = CF_INS.getConfigFilePath();
    const QString configPath = configuredPath.isEmpty() ? QStringLiteral("config.toml") : configuredPath;
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARNING(QString("[CommandControl][CONFIG] 无法读取独立配置节：%1").arg(file.errorString()));
        return {};
    }

    QHash<QString, QString> values;
    bool inCommandControlSection = false;
    QTextStream input(&file);
    while (!input.atEnd()) {
        const QString line = input.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            inCommandControlSection = line.mid(1, line.size() - 2).trimmed()
                                      == QStringLiteral("command_control");
            continue;
        }
        if (!inCommandControlSection) {
            continue;
        }

        const int equalIndex = line.indexOf(QLatin1Char('='));
        if (equalIndex <= 0) {
            continue;
        }
        const QString key = line.left(equalIndex).trimmed();
        QString value = line.mid(equalIndex + 1).trimmed();
        const int commentIndex = value.indexOf(QLatin1Char('#'));
        if (commentIndex >= 0) {
            value = value.left(commentIndex).trimmed();
        }
        values.insert(key, value);
    }
    return values;
}

QString normalizedString(const QString& value, const QString& fallback)
{
    QString text = value.trimmed();
    while (text.size() >= 2
           && ((text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')))
               || (text.startsWith(QLatin1Char('\'')) && text.endsWith(QLatin1Char('\''))))) {
        text = text.mid(1, text.size() - 2).trimmed();
    }
    return text.isEmpty() ? fallback : text;
}

QString configuredText(const QHash<QString, QString>& values, const QString& key,
                       const QString& fallback)
{
    return normalizedString(values.value(key), fallback);
}

quint32 unsignedValue(const QHash<QString, QString>& values, const QString& key, quint32 fallback)
{
    bool ok = false;
    const QString text = configuredText(values, key, QString::number(fallback));
    const quint64 value = text.toULongLong(&ok, 0);
    return ok && value <= std::numeric_limits<quint32>::max()
        ? static_cast<quint32>(value) : fallback;
}

quint16 portValue(const QHash<QString, QString>& values, const QString& key, quint16 fallback)
{
    const quint32 value = unsignedValue(values, key, fallback);
    return value > 0 && value <= std::numeric_limits<quint16>::max()
        ? static_cast<quint16>(value) : fallback;
}

quint8 byteValue(const QHash<QString, QString>& values, const QString& key, quint8 fallback)
{
    const quint32 value = unsignedValue(values, key, fallback);
    return value <= std::numeric_limits<quint8>::max() ? static_cast<quint8>(value) : fallback;
}

quint16 ushortValue(const QHash<QString, QString>& values, const QString& key, quint16 fallback)
{
    const quint32 value = unsignedValue(values, key, fallback);
    return value <= std::numeric_limits<quint16>::max() ? static_cast<quint16>(value) : fallback;
}

int boundedInt(const QHash<QString, QString>& values, const QString& key, int fallback, int low, int high)
{
    bool ok = false;
    const int value = configuredText(values, key, QString::number(fallback)).toInt(&ok);
    return ok && value >= low && value <= high ? value : fallback;
}

bool boolValue(const QHash<QString, QString>& values, const QString& key, bool fallback)
{
    const QString text = configuredText(values, key, fallback ? QStringLiteral("true") : QStringLiteral("false")).toLower();
    if (text == QStringLiteral("true") || text == QStringLiteral("1")) {
        return true;
    }
    if (text == QStringLiteral("false") || text == QStringLiteral("0")) {
        return false;
    }
    return fallback;
}

}  // namespace

CommandControlSettings CommandControlConfig::load()
{
    CommandControlSettings settings;
    const QHash<QString, QString> values = loadCommandControlValues();
    settings.enabled = boolValue(values, QStringLiteral("enabled"), settings.enabled);
    settings.autoReportEnabled = boolValue(values, QStringLiteral("auto_report_enabled"), settings.autoReportEnabled);
    settings.localIp = configuredText(values, QStringLiteral("local_ip"), settings.localIp);
    settings.localPort = portValue(values, QStringLiteral("local_port"), settings.localPort);
    settings.multicastGroup = configuredText(values, QStringLiteral("multicast_group"), settings.multicastGroup);
    settings.multicastPort = portValue(values, QStringLiteral("multicast_port"), settings.multicastPort);
    settings.expectedControlIp = configuredText(values, QStringLiteral("expected_control_ip"), settings.expectedControlIp);
    settings.deviceId = unsignedValue(values, QStringLiteral("device_id"), settings.deviceId);
    if (settings.deviceId == 0) {
        LOG_WARNING(QString("[CommandControl][CONFIG] device_id 不能为 0，已回退到 0x11474202"));
        settings.deviceId = 0x11474202;
    }
    settings.linkIntervalMs = boundedInt(values, QStringLiteral("link_interval_ms"), settings.linkIntervalMs, 250, 60000);
    settings.equipmentStatusIntervalMs = boundedInt(values, QStringLiteral("equipment_status_interval_ms"), settings.equipmentStatusIntervalMs, 250, 60000);
    settings.loginRetryCount = boundedInt(values, QStringLiteral("login_retry_count"), settings.loginRetryCount, 0, 10);
    settings.loginRetryIntervalMs = boundedInt(values, QStringLiteral("login_retry_interval_ms"), settings.loginRetryIntervalMs, 250, 60000);
    settings.recordEnabled = boolValue(values, QStringLiteral("record_enabled"), settings.recordEnabled);
    settings.recordDirectory = configuredText(values, QStringLiteral("record_directory"), settings.recordDirectory);
    settings.replayIntervalMs = boundedInt(values, QStringLiteral("replay_interval_ms"), settings.replayIntervalMs, 10, 10000);
    settings.visibleRecordLimit = boundedInt(values, QStringLiteral("visible_record_limit"), settings.visibleRecordLimit, 10, 10000);
    settings.dd25CooperationStatus = byteValue(values, QStringLiteral("dd25_cooperation_status"), settings.dd25CooperationStatus);
    settings.dda4ComprehensiveBatch = unsignedValue(values, QStringLiteral("dda4_comprehensive_batch"), settings.dda4ComprehensiveBatch);
    settings.dda4DeviceType = byteValue(values, QStringLiteral("dda4_device_type"), settings.dda4DeviceType);
    settings.dda4DeviceNumber = byteValue(values, QStringLiteral("dda4_device_number"), settings.dda4DeviceNumber);
    settings.dda4RateCentiHz = ushortValue(values, QStringLiteral("dda4_rate_centi_hz"), settings.dda4RateCentiHz);
    settings.dda4TargetAttribute = byteValue(values, QStringLiteral("dda4_target_attribute"), settings.dda4TargetAttribute);
    settings.dda4TargetType = byteValue(values, QStringLiteral("dda4_target_type"), settings.dda4TargetType);
    settings.dda4TrackQuality = byteValue(values, QStringLiteral("dda4_track_quality"), settings.dda4TrackQuality);
    settings.dda4RcsMilliSquareM = ushortValue(values, QStringLiteral("dda4_rcs_milli_square_m"), settings.dda4RcsMilliSquareM);
    settings.dda4InterferenceStatus = byteValue(values, QStringLiteral("dda4_interference_status"), settings.dda4InterferenceStatus);
    settings.dda4UpdateStateExtrapolated = static_cast<quint8>(boundedInt(
        values, QStringLiteral("dda4_update_state_extrapolated"), settings.dda4UpdateStateExtrapolated, 0, 15));
    settings.dda4UpdateStateFiltered = static_cast<quint8>(boundedInt(
        values, QStringLiteral("dda4_update_state_filtered"), settings.dda4UpdateStateFiltered, 0, 15));
    settings.dda4UpdateModeAuto = static_cast<quint8>(boundedInt(
        values, QStringLiteral("dda4_update_mode_auto"), settings.dda4UpdateModeAuto, 0, 15));
    settings.dda4UpdateModeManual = static_cast<quint8>(boundedInt(
        values, QStringLiteral("dda4_update_mode_manual"), settings.dda4UpdateModeManual, 0, 15));
    settings.dda4RelativeDelayMs = ushortValue(values, QStringLiteral("dda4_relative_delay_ms"), settings.dda4RelativeDelayMs);
    settings.dda1WorkStatus = byteValue(values, QStringLiteral("dda1_work_status"), settings.dda1WorkStatus);
    settings.dda1HealthStatus = byteValue(values, QStringLiteral("dda1_health_status"), settings.dda1HealthStatus);
    settings.dda1DeviceCount = byteValue(values, QStringLiteral("dda1_device_count"), settings.dda1DeviceCount);
    settings.dda1DeviceType = byteValue(values, QStringLiteral("dda1_device_type"), settings.dda1DeviceType);
    settings.dda1DeviceNumber = byteValue(values, QStringLiteral("dda1_device_number"), settings.dda1DeviceNumber);
    settings.dda1DeviceStatus = byteValue(values, QStringLiteral("dda1_device_status"), settings.dda1DeviceStatus);
    settings.dda1WorkMode = byteValue(values, QStringLiteral("dda1_work_mode"), settings.dda1WorkMode);
    settings.dda1AzimuthStartDeg = static_cast<quint16>(boundedInt(
        values, QStringLiteral("dda1_azimuth_start_deg"), settings.dda1AzimuthStartDeg, 0, 360));
    settings.dda1AzimuthEndDeg = static_cast<quint16>(boundedInt(
        values, QStringLiteral("dda1_azimuth_end_deg"), settings.dda1AzimuthEndDeg, 0, 360));
    settings.dda1ElevationStartDeg = static_cast<qint8>(boundedInt(
        values, QStringLiteral("dda1_elevation_start_deg"), settings.dda1ElevationStartDeg, -10, 70));
    settings.dda1ElevationEndDeg = static_cast<qint8>(boundedInt(
        values, QStringLiteral("dda1_elevation_end_deg"), settings.dda1ElevationEndDeg, -10, 70));
    settings.dda1RadarId = static_cast<quint8>(boundedInt(
        values, QStringLiteral("dda1_radar_id"), settings.dda1RadarId, 0, 3));
    settings.radiationStatus = byteValue(values, QStringLiteral("radiation_status"), settings.radiationStatus);
    settings.dda4LogIntervalMs = boundedInt(values, QStringLiteral("dda4_log_interval_ms"),
                                             settings.dda4LogIntervalMs, 0, 60000);
    settings.packetHexLogEnabled = boolValue(values, QStringLiteral("packet_hex_log_enabled"),
                                              settings.packetHexLogEnabled);
    if (settings.radiationStatus > 1) {
        LOG_WARNING(QString("[CommandControl][CONFIG] radiation_status 必须为 0 或 1，已回退到 0"));
        settings.radiationStatus = 0;
    }

    LOG_INFO(QString("[CommandControl][CONFIG] enabled=%1 local=%2:%3 group=%4:%5 device=0x%6 autoReport=%7 record=%8 dda1Type=0x%9 dda4Log=%10 packetHex=%11")
             .arg(settings.enabled)
             .arg(settings.localIp)
             .arg(settings.localPort)
             .arg(settings.multicastGroup)
             .arg(settings.multicastPort)
             .arg(settings.deviceId, 8, 16, QChar('0'))
             .arg(settings.autoReportEnabled)
             .arg(settings.recordEnabled)
             .arg(settings.dda1DeviceType, 2, 16, QChar('0'))
             .arg(settings.dda4LogIntervalMs)
             .arg(settings.packetHexLogEnabled));
    LOG_INFO(QString("[CommandControl][CONFIG][DEFAULT] dd25State=0x%1 dda4(type=%2,no=%3,rate=%4,quality=0x%5,update=%6/%7) dda1(work=0x%8,health=0x%9,type=0x%10,no=%11,mode=0x%12,radarId=%13,az=%14~%15,el=%16~%17)")
             .arg(settings.dd25CooperationStatus, 2, 16, QChar('0'))
             .arg(settings.dda4DeviceType)
             .arg(settings.dda4DeviceNumber)
             .arg(settings.dda4RateCentiHz)
             .arg(settings.dda4TrackQuality, 2, 16, QChar('0'))
             .arg(settings.dda4UpdateModeAuto)
             .arg(settings.dda4UpdateModeManual)
             .arg(settings.dda1WorkStatus, 2, 16, QChar('0'))
             .arg(settings.dda1HealthStatus, 2, 16, QChar('0'))
             .arg(settings.dda1DeviceType, 2, 16, QChar('0'))
             .arg(settings.dda1DeviceNumber)
             .arg(settings.dda1WorkMode, 2, 16, QChar('0'))
             .arg(settings.dda1RadarId)
             .arg(settings.dda1AzimuthStartDeg)
             .arg(settings.dda1AzimuthEndDeg)
             .arg(settings.dda1ElevationStartDeg)
             .arg(settings.dda1ElevationEndDeg));
    return settings;
}
