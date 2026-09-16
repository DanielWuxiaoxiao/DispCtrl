/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:29
 * @Description: 
 */
/* 指控通信模块独立配置读取。 */
#ifndef COMMANDCONTROL_CONFIG_H
#define COMMANDCONTROL_CONFIG_H

#include <QString>
#include <QtGlobal>

struct CommandControlSettings {
    bool enabled = true;
    bool autoReportEnabled = false;
    QString localIp = QStringLiteral("192.30.106.13");
    quint16 localPort = 21505;
    QString multicastGroup = QStringLiteral("224.0.1.2");
    quint16 multicastPort = 21505;
    QString expectedControlIp = QStringLiteral("192.30.106.10");
    int networkStatusPollIntervalMs = 5000;
    // 标准 SNTP/NTP 服务；启动总控通信前以该服务器授时。
    bool timeSyncEnabled = true;
    QString timeServerIp = QStringLiteral("192.30.1.1");
    quint16 timeServerPort = 123;
    int timeSyncTimeoutMs = 2000;
    int timeSyncRetryCount = 2;
    int timeSyncIntervalMs = 3600000;
    bool timeSyncOnDd31 = true;
    quint32 deviceId = 0x11474202;
    int linkIntervalMs = 3000;
    int equipmentStatusIntervalMs = 3000;
    int loginRetryCount = 3;
    int loginRetryIntervalMs = 1000;
    bool recordEnabled = true;
    QString recordDirectory = QStringLiteral("CommandControlRecords");
    int replayIntervalMs = 200;
    int replayTrackStaleMs = 3000;
    int visibleRecordLimit = 1000;
    quint8 dd25CooperationStatus = 0x00;
    quint32 dda4ComprehensiveBatch = 0;
    quint8 dda4DeviceType = 0x02;
    quint8 dda4DeviceNumber = 0x01;
    quint16 dda4RateCentiHz = 200;
    quint8 dda4TargetAttribute = 0x00;
    quint8 dda4TargetType = 0x00;
    quint8 dda4TrackQuality = 0xFF;
    quint16 dda4RcsMilliSquareM = 0;
    quint8 dda4InterferenceStatus = 0x00;
    quint8 dda4UpdateStateExtrapolated = 0x01;
    quint8 dda4UpdateStateFiltered = 0x02;
    quint8 dda4UpdateModeAuto = 0x01;
    quint8 dda4UpdateModeManual = 0x02;
    quint16 dda4RelativeDelayMs = 0;
    quint8 dda1WorkStatus = 0x01;
    quint8 dda1HealthStatus = 0x03;
    quint8 dda1DeviceCount = 0x01;
    quint8 dda1DeviceType = 0x02;
    quint8 dda1DeviceNumber = 0x01;
    quint8 dda1DeviceStatus = 0x03;
    quint8 dda1WorkMode = 0x22;
    quint16 dda1AzimuthStartDeg = 0;
    quint16 dda1AzimuthEndDeg = 360;
    qint8 dda1ElevationStartDeg = -10;
    qint8 dda1ElevationEndDeg = 70;
    quint8 dda1RadarId = 0;
    quint8 radiationStatus = 0;
    // 已完成联调时默认仅输出异常帧；打开后才写 DDA1 正常帧的完整字段。
    bool dda1NormalLogEnabled = false;
    // DDA4 的 JSONL 独立逐条落盘不受此项影响；本项只控制项目日志中的正常字段打印。
    bool dda4NormalLogEnabled = false;
    int dda4LogIntervalMs = 1000;
    bool packetHexLogEnabled = false;
};

class CommandControlConfig final
{
public:
    static CommandControlSettings load();
};

#endif  // COMMANDCONTROL_CONFIG_H
