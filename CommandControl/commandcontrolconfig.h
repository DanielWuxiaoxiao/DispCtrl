/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:44
 * @Description: 
 */
/* 指控通信模块独立配置读取。 */
#ifndef COMMANDCONTROL_CONFIG_H
#define COMMANDCONTROL_CONFIG_H

#include <QString>
#include <QtGlobal>

struct CommandControlSettings {
    bool enabled = true;
    bool autoReportEnabled = true;
    QString localIp = QStringLiteral("192.30.105.13");
    quint16 localPort = 21505;
    QString multicastGroup = QStringLiteral("224.0.1.2");
    quint16 multicastPort = 21505;
    QString expectedControlIp = QStringLiteral("192.30.105.10");
    quint32 deviceId = 0x11474202;
    int linkIntervalMs = 3000;
    int equipmentStatusIntervalMs = 3000;
    int loginRetryCount = 3;
    int loginRetryIntervalMs = 1000;
    bool recordEnabled = true;
    QString recordDirectory = QStringLiteral("CommandControlRecords");
    int replayIntervalMs = 200;
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
    int dda4LogIntervalMs = 1000;
    bool packetHexLogEnabled = false;
};

class CommandControlConfig final
{
public:
    static CommandControlSettings load();
};

#endif  // COMMANDCONTROL_CONFIG_H
