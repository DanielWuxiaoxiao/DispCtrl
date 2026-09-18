/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-18 23:22:44
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-18 23:42:18
 * @Description: 
 */
#include "commandcontroltrackreportstore.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

namespace {

QString targetClassText(quint32 targetRecognition)
{
    return targetRecognition == 1 ? QStringLiteral("无人机") : QStringLiteral("其它");
}

QString reportModeText(const QString& mode)
{
    return mode == QStringLiteral("manual") ? QStringLiteral("手动上报")
                                             : QStringLiteral("自动上报");
}

QString eventText(const QString& event)
{
    if (event == QStringLiteral("session_start")) {
        return QStringLiteral("上报会话开始");
    }
    if (event == QStringLiteral("session_stop")) {
        return QStringLiteral("上报会话结束");
    }
    return QStringLiteral("源航迹点");
}

QJsonObject toJson(const CommandControlTrackReportRecord& record)
{
    QJsonObject object;
    object.insert(QStringLiteral("event"), record.event);
    object.insert(QStringLiteral("report_session_id"), record.reportSessionId);
    object.insert(QStringLiteral("report_mode"), record.reportMode);
    object.insert(QStringLiteral("report_mode_name"), reportModeText(record.reportMode));
    object.insert(QStringLiteral("event_name"), eventText(record.event));
    object.insert(QStringLiteral("stop_reason"), record.stopReason);
    object.insert(QStringLiteral("source_batch"), static_cast<double>(record.sourceBatch));
    object.insert(QStringLiteral("point_index"), static_cast<double>(record.pointIndex));
    object.insert(QStringLiteral("observed_utc_ms"), static_cast<double>(record.observedUtcMs));
    object.insert(QStringLiteral("observed_beijing"),
                  QDateTime::fromMSecsSinceEpoch(record.observedUtcMs, Qt::UTC)
                      .toTimeZone(QTimeZone(QByteArrayLiteral("Asia/Shanghai")))
                      .toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("source_track_started_utc_ms"),
                  static_cast<double>(record.sourceTrackStartedUtcMs));
    object.insert(QStringLiteral("reporting_started_utc_ms"),
                  static_cast<double>(record.reportingStartedUtcMs));
    object.insert(QStringLiteral("source_type"), static_cast<int>(record.sourceType));
    object.insert(QStringLiteral("source_type_name"), QStringLiteral("普通DBT航迹"));
    object.insert(QStringLiteral("stat_method"), record.statMethod);
    object.insert(QStringLiteral("target_recognition"), static_cast<double>(record.targetRecognition));
    object.insert(QStringLiteral("target_class"), targetClassText(record.targetRecognition));
    object.insert(QStringLiteral("target_confidence"), record.targetConfidence);
    object.insert(QStringLiteral("radar_id"), record.radarId);
    object.insert(QStringLiteral("range_m"), record.rangeM);
    object.insert(QStringLiteral("azimuth_deg"), record.azimuthDeg);
    object.insert(QStringLiteral("elevation_deg"), record.elevationDeg);
    object.insert(QStringLiteral("snr"), record.snr);
    object.insert(QStringLiteral("speed_mps"), record.speedMps);
    object.insert(QStringLiteral("relative_altitude_m"), record.relativeAltitudeM);
    object.insert(QStringLiteral("amplitude"), record.amplitude);
    object.insert(QStringLiteral("radar_origin_valid"), record.radarOriginValid);
    object.insert(QStringLiteral("radar_longitude_deg"), record.radarLongitudeDeg);
    object.insert(QStringLiteral("radar_latitude_deg"), record.radarLatitudeDeg);
    object.insert(QStringLiteral("radar_altitude_m"), record.radarAltitudeM);
    object.insert(QStringLiteral("target_lla_valid"), record.targetLlaValid);
    object.insert(QStringLiteral("target_longitude_deg"), record.targetLongitudeDeg);
    object.insert(QStringLiteral("target_latitude_deg"), record.targetLatitudeDeg);
    object.insert(QStringLiteral("target_altitude_m"), record.targetAltitudeM);
    return object;
}

}  // namespace

CommandControlTrackReportStore::~CommandControlTrackReportStore()
{
    close();
}

bool CommandControlTrackReportStore::startSession(const QString& directoryPath, QString* errorMessage)
{
    close();
    QDir directory;
    if (!directory.mkpath(directoryPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建航迹上报记录目录：%1").arg(directoryPath);
        }
        return false;
    }

    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
    m_sessionFile.setFileName(QDir(directoryPath).filePath(
        QStringLiteral("command_control_track_report_%1.jsonl").arg(stamp)));
    if (!m_sessionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法打开航迹上报记录文件：%1").arg(m_sessionFile.errorString());
        }
        return false;
    }
    return true;
}

void CommandControlTrackReportStore::close()
{
    if (m_sessionFile.isOpen()) {
        m_sessionFile.close();
    }
}

bool CommandControlTrackReportStore::append(const CommandControlTrackReportRecord& record,
                                            QString* errorMessage)
{
    if (!m_sessionFile.isOpen()) {
        return true;
    }
    QByteArray line = QJsonDocument(toJson(record)).toJson(QJsonDocument::Compact);
    line.append('\n');
    if (m_sessionFile.write(line) != line.size() || !m_sessionFile.flush()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("航迹上报记录写入失败：%1").arg(m_sessionFile.errorString());
        }
        return false;
    }
    return true;
}
