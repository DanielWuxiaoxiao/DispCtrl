/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-29 09:49:42
 * @Description: 
 */
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QTextStream>
#include "Basic/log.h"

#define CF_INS ConfigManager::instance()

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    bool load(const QString& path = "config.toml") {
        // 保存配置文件的完整路径，用于后续save操作
        QFileInfo fileInfo(path);
        configFilePath = fileInfo.absoluteFilePath();
        LOG_INFO(QString("Config file path resolved to: %1").arg(configFilePath));
        return loadToml(path);
    }

    // 获取当前配置文件路径
    QString getConfigFilePath() const {
        return configFilePath;
    }

    QString ip(const QString& key, const QString& def = "127.0.0.1") const {
        return getValue("network.ips." + key, def).toString();
    }

    int id(const QString& key, int def = -1) const {
        return getValue("network.ids." + key, def).toInt();
    }

    int port(const QString& key, int def = 0) const {
        return getValue("network.ports." + key, def).toInt();
    }

    QString gcsIp() const {
        return getValue("network.ips.GCS_IP", "192.168.144.77").toString();
    }

    QString gcsLocalIp() const {
        return getValue("network.ips.GCS_LOCAL_IP", "192.168.144.66").toString();
    }

    quint16 gcsDstPort() const {
        return static_cast<quint16>(getValue("network.ports.GCS_DST_PORT", 19801).toUInt());
    }

    quint16 gcsSrcPort() const {
        return static_cast<quint16>(getValue("network.ports.GCS_SRC_PORT", 19800).toUInt());
    }

    bool gcsTargetReportEnabled(bool def = false) const {
        return getValue("network.gcs.target_report_enabled", def).toBool();
    }

    bool edgeRadarReportEnabled(bool def = false) const {
        return getValue("network.edge_radar_report.enabled", def).toBool();
    }

    QString edgeRadarReportIp(const QString& def = "192.168.1.100") const {
        return getValue("network.edge_radar_report.target_ip", def).toString();
    }

    quint16 edgeRadarReportPort(quint16 def = 9001) const {
        return static_cast<quint16>(getValue("network.edge_radar_report.target_port", def).toUInt());
    }

    QString edgeRadarReportLocalIp(const QString& def = "0.0.0.0") const {
        return getValue("network.edge_radar_report.local_ip", def).toString();
    }

    quint16 edgeRadarReportLocalPort(quint16 def = 0) const {
        return static_cast<quint16>(getValue("network.edge_radar_report.local_port", def).toUInt());
    }

    int edgeRadarHeartbeatIntervalMs(int def = 5000) const {
        return getValue("network.edge_radar_report.heartbeat_interval_ms", def).toInt();
    }

    int edgeRadarTargetReportIntervalMs(int def = 4000) const {
        return getValue("network.edge_radar_report.target_report_interval_ms", def).toInt();
    }

    double edgeRadarMaxTargetDistanceM(double def = 2000.0) const {
        return getValue("network.edge_radar_report.max_target_distance_m", def).toDouble();
    }

    bool edgeRadarResultReceiveEnabled(bool def = true) const {
        return getValue("network.edge_radar_result.enabled", def).toBool();
    }

    QString edgeRadarResultLocalIp(const QString& def = "0.0.0.0") const {
        return getValue("network.edge_radar_result.local_ip", def).toString();
    }

    quint16 edgeRadarResultLocalPort(quint16 def = 9002) const {
        return static_cast<quint16>(getValue("network.edge_radar_result.local_port", def).toUInt());
    }

    bool totalControlMqttEnabled(bool def = false) const {
        return getValue("network.total_control_mqtt.enabled", def).toBool();
    }

    QString totalControlMqttHost(const QString& def = "192.168.1.30") const {
        return getValue("network.total_control_mqtt.host", def).toString();
    }

    quint16 totalControlMqttPort(quint16 def = 1883) const {
        return static_cast<quint16>(getValue("network.total_control_mqtt.port", def).toUInt());
    }

    QString totalControlMqttClientId(const QString& def = "DispCtrl-X576") const {
        return getValue("network.total_control_mqtt.client_id", def).toString();
    }

    QString totalControlMqttTopic(const QString& def = "x576/target/result") const {
        return getValue("network.total_control_mqtt.topic", def).toString();
    }

    int totalControlMqttKeepAliveSec(int def = 30) const {
        return getValue("network.total_control_mqtt.keep_alive_sec", def).toInt();
    }

    int totalControlMqttPublishIntervalMs(int def = 4000) const {
        return getValue("network.total_control_mqtt.publish_interval_ms", def).toInt();
    }

    int range(const QString& key, int def = 0) const {
        return getValue("polarDisp.range." + key, def).toInt();
    }

    int mapType(const QString& key, int def = 1) const {
        return getValue("map." + key, def).toInt();
    }

    int mapEngine(int def = 0) const {
        return getValue("map.engine", def).toInt();  // 0=OSM, 1=AMap
    }

    // 扇形显示相关配置
    double sectorAngle(const QString& key, double def = 0.0) const {
        return getValue("sectorDisp.angle." + key, def).toDouble();
    }

    double sectorRange(const QString& key, double def = 0.0) const {
        return getValue("sectorDisp.range." + key, def).toDouble();
    }

    // 距离-方位显示相关配置
    double rangeAzimuthAngle(const QString& key, double def = 0.0) const {
        return getValue("rangeAzimuthDisp.angle." + key, def).toDouble();
    }

    void setRangeAzimuthAngle(const QString& key, double value) {
        saveValue("rangeAzimuthDisp.angle." + key, value);
    }

    // 雷达位置相关配置
    double latitude(const QString& key = "latitude", double def = 34.2311) const {
        return getValue("radar." + key, def).toDouble();
    }

    double longitude(const QString& key = "longitude", double def = 108.9138) const {
        return getValue("radar." + key, def).toDouble();
    }

    double altitude(const QString& key = "altitude", double def = 400.0) const {
        return getValue("radar." + key, def).toDouble();
    }

    // 雷达姿态相关配置
    double azimuth(const QString& key = "azimuth", double def = 0.0) const {
        return getValue("radar." + key, def).toDouble();
    }

    double pitch(const QString& key = "pitch", double def = 0.0) const {
        return getValue("radar." + key, def).toDouble();
    }

    double roll(const QString& key = "roll", double def = 0.0) const {
        return getValue("radar." + key, def).toDouble();
    }

    // OSM道路数据配置
    QString osmFile(const QString& def = "bailuyuan.osm") const {
        return getValue("osm.file", def).toString();
    }
    bool osmGcj02(bool def = true) const {
        return getValue("osm.gcj02", def).toBool();
    }
    double osmInterpolationStep(double def = 30.0) const {
        return getValue("osm.interpolation_step", def).toDouble();
    }

    // 新增的配置访问方法
    int azimuthRange(const QString& key, int def = 0) const {
        return getValue("polarDisp.azimuthRange." + key, def).toInt();
    }

    int elevationRange(const QString& key, int def = 0) const {
        return getValue("polarDisp.elevationRange." + key, def).toInt();
    }

    int pointSize(const QString& key, int def = 1) const {
        return getValue("targetDisplay.pointSizes." + key, def).toInt();
    }

    QString targetLabel(const QString& key, const QString& def = "") const {
        return getValue("targetDisplay.labels." + key, def).toString();
    }

    int zValue(const QString& key, int def = 0) const {
        return getValue("targetDisplay.zValues." + key, def).toInt();
    }

    int fontSize(const QString& key, int def = 9) const {
        return getValue("ui.fonts." + key, def).toInt();
    }

    // UI DPI策略（必须在 QApplication 创建前读取）
    // fixed：显控固定按物理像素布局，不跟随 Windows 文本/显示缩放
    // system：跟随系统DPI缩放（Qt高DPI），用于现场兼容回退
    QString uiDpiPolicy(const QString& def = "fixed") const {
        return getValue("ui.dpi_policy", def).toString();
    }

    double uiScale(double def = 1.0) const {
        return getValue("ui.ui_scale", def).toDouble();
    }

    bool offlineRaeEnabled(bool def = false) const {
        return getValue("offline_rae.enabled", def).toBool();
    }

    QString offlineRaeFile(const QString& key, const QString& def = "") const {
        return getValue("offline_rae." + key, def).toString();
    }

    int offlineRaeMaxRecords(int def = 200000) const {
        return getValue("offline_rae.max_records_per_click", def).toInt();
    }

    bool offlineRaeClearBeforeDraw(bool def = true) const {
        return getValue("offline_rae.clear_before_draw", def).toBool();
    }

    int windowProperty(const QString& key, int def = 0) const {
        return getValue("ui.window." + key, def).toInt();
    }

    double mapCenter(const QString& key, double def = 0.0) const {
        return getValue("mapDisplay.center_" + key, def).toDouble();
    }

    QString mapProperty(const QString& key, const QString& def = "") const {
        return getValue("mapDisplay." + key, def).toString();
    }

    int systemProperty(const QString& key, int def = 0) const {
        return getValue("system." + key, def).toInt();
    }

    bool systemFlag(const QString& key, bool def = false) const {
        return getValue("system." + key, def).toBool();
    }

    QString systemString(const QString& key, const QString& def = "") const {
        return getValue("system." + key, def).toString();
    }

    bool photoelectricTxEnabled(bool def = false) const {
        return systemFlag("enable_photoelectric_tx", def);
    }

    bool photoelectricHeartbeatEnabled(bool def = false) const {
        return systemFlag("enable_photoelectric_heartbeat", def);
    }

    unsigned int protocolCode(const QString& key, unsigned int def = 0) const {
        return getValue("network.protocol." + key, def).toUInt();
    }

    // WebEngine调试配置
    bool webEngineDebugEnabled(bool def = false) const {
        return getValue("webengine.enable_debug", def).toBool();
    }

    int webEngineDebugPort(int def = 6669) const {
        return getValue("webengine.debug_port", def).toInt();
    }

    // WebEngine 渲染后端配置（必须在 QApplication 创建前读取）
    // 用于解决黑屏/黑屏闪烁（GPU 加速与显卡/双显卡切换/桌面合成冲突）
    // 取值：angle/gles（ANGLE→D3D，默认，Windows/双显卡最稳，WebGL地图正常）| desktop（原生OpenGL）| software（软件渲染）
    QString webEngineGlBackend(const QString& def = "angle") const {
        return getValue("webengine.gl_backend", def).toString();
    }

    // true 时给 Chromium 加 --disable-gpu --disable-gpu-compositing，关闭 WebEngine 的 GPU 加速/合成
    // （地图为2D瓦片，软件渲染足够；可消除录屏黑屏与部分显卡的黑屏闪烁）
    bool webEngineDisableGpu(bool def = false) const {
        return getValue("webengine.disable_gpu", def).toBool();
    }

    // 追加自定义 Chromium 启动参数（空格分隔），用于高级调优，一般留空
    QString webEngineExtraChromiumFlags(const QString& def = "") const {
        return getValue("webengine.extra_chromium_flags", def).toString();
    }

    // ===== 激光侦察上报配置（独立模块，默认关闭；关闭时右键菜单无此项）=====
    bool laserReportEnabled(bool def = false) const {
        return getValue("laser.enabled", def).toBool();
    }
    QString laserRadarIp(const QString& def = "192.168.101.9") const {
        return getValue("laser.radar_ip", def).toString();
    }
    QString laserCtrlIp(const QString& def = "192.168.101.10") const {
        return getValue("laser.laser_ctrl_ip", def).toString();
    }
    quint16 laserUdpPort(quint16 def = 9009) const {
        return static_cast<quint16>(getValue("laser.udp_port", def).toUInt());
    }
    int laserReportIntervalMs(int def = 1000) const {
        return getValue("laser.report_interval_ms", def).toInt();
    }
    bool laserSaveTxt(bool def = true) const {
        return getValue("laser.save_txt", def).toBool();
    }
    QString laserSaveDir(const QString& def = "LaserReportLog") const {
        return getValue("laser.save_dir", def).toString();
    }
    // 阵面对端地址不能由数据处理IP推断，必须由现场配置明确给出。
    bool healthNetworkEnabled(bool def = true) const {
        return getValue("health_network.enabled", def).toBool();
    }
    int healthNetworkPollIntervalMs(int def = 5000) const {
        return getValue("health_network.poll_interval_ms", def).toInt();
    }
    QString healthRadarLocalIp(const QString& def = "192.168.64.4") const {
        return getValue("health_network.radar_local_ip", ip("DISP_CTRL_IP", def)).toString();
    }
    QString healthRadarPeerIp(const QString& def = QString()) const {
        return getValue("health_network.radar_peer_ip", def).toString();
    }

    // 显示配置相关
    int displayConfig(const QString& key, int def = 1000) const {
        return getValue("displayConfig." + key, def).toInt();
    }

    int defaultPanelCount() const {
        const QVariant value = getValue("displayConfig.default_panel_count", 1);
        bool ok = false;
        const int count = value.toInt(&ok);
        if (!ok || count < 1 || count > 4) {
            LOG_WARNING(QString("Invalid displayConfig.default_panel_count=%1; using panel 1 only")
                        .arg(value.toString()));
            return 1;
        }
        return count;
    }

    bool displayFlag(const QString& key, bool def = false) const {
        return getValue("displayConfig." + key, def).toBool();
    }

    bool iftbd(bool def = false) const {
        return displayFlag("iftbd", def);
    }

    bool ifxietong(bool def = false) const {
        return displayFlag("ifxietong", def);
    }

    // 保存显示配置参数
    void saveDisplayConfig(const QString& key, int value) {
        saveValue("displayConfig." + key, value);
    }

    // ========== 参数保存功能 ==========

    // 保存单个参数值到内存
    void saveValue(const QString& key, const QVariant& value) {
        configData[key] = value;
    }

    // 保存整个配置到TOML文件
    // 如果不指定路径，使用load()时记录的配置文件路径
    bool save(const QString& path = "") {
        QString savePath = path.isEmpty() ? configFilePath : path;
        if (savePath.isEmpty()) {
            // 如果还没有配置文件路径，使用默认路径
            QFileInfo fileInfo("config.toml");
            savePath = fileInfo.absoluteFilePath();
        }
        LOG_INFO(QString("Saving config to: %1").arg(savePath));
        return saveToml(savePath);
    }

    // 伺服控制参数保存/读取
    void saveServoParam(unsigned char cmd, unsigned char speed, unsigned short az) {
        saveValue("params.servo.cmd", cmd);
        saveValue("params.servo.speed", speed);
        saveValue("params.servo.az", az);
    }

    unsigned char servoCmd(unsigned char def = 0) const {
        return getValue("params.servo.cmd", def).toUInt();
    }

    unsigned char servoSpeed(unsigned char def = 3) const {
        return getValue("params.servo.speed", def).toUInt();
    }

    unsigned short servoAz(unsigned short def = 0) const {
        return getValue("params.servo.az", def).toUInt();
    }

    // 扫描范围参数保存/读取
    void saveScanRangeParam(unsigned char workMode) {
        saveValue("params.scanrange.workMode", workMode);
    }

    unsigned char scanRangeWorkMode(unsigned char def = 0) const {
        return getValue("params.scanrange.workMode", def).toUInt();
    }

    // 波形控制参数保存/读取（支持完整的BeamControl结构）
    void saveBeamControlParam(unsigned char freqID, unsigned char type,
                            short aziStart, short aziEnd, short aziStep,
                            unsigned char scene,
                            unsigned char flagNum, unsigned short pulseNum,
                            unsigned char beam1Flag, unsigned char beam1Code,
                            unsigned short sampleStart1, unsigned short sampleEnd1,
                            short elestart1, short eleend1, short elestep1,
                            unsigned char beam2Flag, unsigned char beam2Code,
                            unsigned short sampleStart2, unsigned short sampleEnd2,
                            short elestart2, short eleend2, short elestep2,
                            unsigned char beam3Flag, unsigned char beam3Code,
                            unsigned short sampleStart3, unsigned short sampleEnd3,
                            short elestart3, short eleend3, short elestep3) {
        saveValue("params.beamcontrol.freqID", freqID);
        saveValue("params.beamcontrol.type", type);
        saveValue("params.beamcontrol.aziStart", aziStart);
        saveValue("params.beamcontrol.aziEnd", aziEnd);
        saveValue("params.beamcontrol.aziStep", aziStep);
        saveValue("params.beamcontrol.scene", scene);
        saveValue("params.beamcontrol.flagNum", flagNum);
        saveValue("params.beamcontrol.pulseNum", pulseNum);

        saveValue("params.beamcontrol.beam1Flag", beam1Flag);
        saveValue("params.beamcontrol.beam1Code", beam1Code);
        saveValue("params.beamcontrol.sampleStart1", sampleStart1);
        saveValue("params.beamcontrol.sampleEnd1", sampleEnd1);
        saveValue("params.beamcontrol.elestart1", elestart1);
        saveValue("params.beamcontrol.eleend1", eleend1);
        saveValue("params.beamcontrol.elestep1", elestep1);

        saveValue("params.beamcontrol.beam2Flag", beam2Flag);
        saveValue("params.beamcontrol.beam2Code", beam2Code);
        saveValue("params.beamcontrol.sampleStart2", sampleStart2);
        saveValue("params.beamcontrol.sampleEnd2", sampleEnd2);
        saveValue("params.beamcontrol.elestart2", elestart2);
        saveValue("params.beamcontrol.eleend2", eleend2);
        saveValue("params.beamcontrol.elestep2", elestep2);

        saveValue("params.beamcontrol.beam3Flag", beam3Flag);
        saveValue("params.beamcontrol.beam3Code", beam3Code);
        saveValue("params.beamcontrol.sampleStart3", sampleStart3);
        saveValue("params.beamcontrol.sampleEnd3", sampleEnd3);
        saveValue("params.beamcontrol.elestart3", elestart3);
        saveValue("params.beamcontrol.eleend3", eleend3);
        saveValue("params.beamcontrol.elestep3", elestep3);
    }

    // BeamControl参数读取辅助函数
    unsigned char beamFreqID(unsigned char def = 4) const {
        return getValue("params.beamcontrol.freqID", def).toUInt();
    }

    unsigned char beamType(unsigned char def = 2) const {
        return getValue("params.beamcontrol.type", def).toUInt();
    }

    short beamAziStart(short def = -4500) const {
        return getValue("params.beamcontrol.aziStart", def).toInt();
    }

    short beamAziEnd(short def = 4500) const {
        return getValue("params.beamcontrol.aziEnd", def).toInt();
    }

    short beamAziStep(short def = 400) const {
        return getValue("params.beamcontrol.aziStep", def).toInt();
    }

    unsigned char beamFlagNum(unsigned char def = 2) const {
        return getValue("params.beamcontrol.flagNum", def).toUInt();
    }

    unsigned char beamScene(unsigned char def = 0) const {
        return getValue("params.beamcontrol.scene", def).toUInt();
    }

    unsigned short beamPulseNum(unsigned short def = 128) const {
        return getValue("params.beamcontrol.pulseNum", def).toUInt();
    }

    // 波形1参数
    unsigned char beamBeam1Flag(unsigned char def = 1) const {
        return getValue("params.beamcontrol.beam1Flag", def).toUInt();
    }
    unsigned char beamBeam1Code(unsigned char def = 6) const {
        return getValue("params.beamcontrol.beam1Code", def).toUInt();
    }
    unsigned short beamSampleStart1(unsigned short def = 30) const {
        return getValue("params.beamcontrol.sampleStart1", def).toUInt();
    }
    unsigned short beamSampleEnd1(unsigned short def = 130) const {
        return getValue("params.beamcontrol.sampleEnd1", def).toUInt();
    }
    short beamElestart1(short def = 0) const {
        return getValue("params.beamcontrol.elestart1", def).toInt();
    }
    short beamEleend1(short def = 3000) const {
        return getValue("params.beamcontrol.eleend1", def).toInt();
    }
    short beamElestep1(short def = 400) const {
        return getValue("params.beamcontrol.elestep1", def).toInt();
    }

    // 波形2参数
    unsigned char beamBeam2Flag(unsigned char def = 1) const {
        return getValue("params.beamcontrol.beam2Flag", def).toUInt();
    }
    unsigned char beamBeam2Code(unsigned char def = 9) const {
        return getValue("params.beamcontrol.beam2Code", def).toUInt();
    }
    unsigned short beamSampleStart2(unsigned short def = 120) const {
        return getValue("params.beamcontrol.sampleStart2", def).toUInt();
    }
    unsigned short beamSampleEnd2(unsigned short def = 490) const {
        return getValue("params.beamcontrol.sampleEnd2", def).toUInt();
    }
    short beamElestart2(short def = 0) const {
        return getValue("params.beamcontrol.elestart2", def).toInt();
    }
    short beamEleend2(short def = 1500) const {
        return getValue("params.beamcontrol.eleend2", def).toInt();
    }
    short beamElestep2(short def = 400) const {
        return getValue("params.beamcontrol.elestep2", def).toInt();
    }

    // 波形3参数
    unsigned char beamBeam3Flag(unsigned char def = 0) const {
        return getValue("params.beamcontrol.beam3Flag", def).toUInt();
    }
    unsigned char beamBeam3Code(unsigned char def = 11) const {
        return getValue("params.beamcontrol.beam3Code", def).toUInt();
    }
    unsigned short beamSampleStart3(unsigned short def = 270) const {
        return getValue("params.beamcontrol.sampleStart3", def).toUInt();
    }
    unsigned short beamSampleEnd3(unsigned short def = 1730) const {
        return getValue("params.beamcontrol.sampleEnd3", def).toUInt();
    }
    short beamElestart3(short def = 0) const {
        return getValue("params.beamcontrol.elestart3", def).toInt();
    }
    short beamEleend3(short def = 400) const {
        return getValue("params.beamcontrol.eleend3", def).toInt();
    }
    short beamElestep3(short def = 400) const {
        return getValue("params.beamcontrol.elestep3", def).toInt();
    }

    // 信号处理参数保存/读取
    void saveSigProParam(unsigned short noise, unsigned short thresh1, unsigned short thresh2,
                        unsigned short clutterThresh, unsigned char clutterMapFlaseRate,
                        unsigned char CFARType, unsigned char disProWin, unsigned char disRefWin,
                        unsigned char dopProWin, unsigned char dopRefWin, unsigned char MTDWinType,
                        unsigned char clutterMode, unsigned char clutterChannelWidth,
                        unsigned char clutterUnitWin, unsigned char clutterIter, unsigned char algorithmSwitch) {
        saveValue("params.sigpro.noise", noise);
        saveValue("params.sigpro.thresh1", thresh1);
        saveValue("params.sigpro.thresh2", thresh2);
        saveValue("params.sigpro.clutterThresh", clutterThresh);
        saveValue("params.sigpro.clutterMapFlaseRate", clutterMapFlaseRate);
        saveValue("params.sigpro.CFARType", CFARType);
        saveValue("params.sigpro.disProWin", disProWin);
        saveValue("params.sigpro.disRefWin", disRefWin);
        saveValue("params.sigpro.dopProWin", dopProWin);
        saveValue("params.sigpro.dopRefWin", dopRefWin);
        saveValue("params.sigpro.MTDWinType", MTDWinType);
        saveValue("params.sigpro.clutterMode", clutterMode);
        saveValue("params.sigpro.clutterChannelWidth", clutterChannelWidth);
        saveValue("params.sigpro.clutterUnitWin", clutterUnitWin);
        saveValue("params.sigpro.clutterIter", clutterIter);
        saveValue("params.sigpro.algorithmSwitch", algorithmSwitch);
    }

    unsigned short sigProNoise(unsigned short def = 350) const {
        return getValue("params.sigpro.noise", def).toUInt();
    }
    unsigned short sigProThresh1(unsigned short def = 90) const {
        return getValue("params.sigpro.thresh1", def).toUInt();
    }
    unsigned short sigProThresh2(unsigned short def = 150) const {
        return getValue("params.sigpro.thresh2", def).toUInt();
    }
    unsigned short sigProClutterThresh(unsigned short def = 170) const {
        return getValue("params.sigpro.clutterThresh", def).toUInt();
    }
    unsigned char sigProClutterMapFlaseRate(unsigned char def = 4) const {
        return getValue("params.sigpro.clutterMapFlaseRate", def).toUInt();
    }
    unsigned char sigProCFARType(unsigned char def = 0) const {
        return getValue("params.sigpro.CFARType", def).toUInt();
    }
    unsigned char sigProDisProWin(unsigned char def = 2) const {
        return getValue("params.sigpro.disProWin", def).toUInt();
    }
    unsigned char sigProDisRefWin(unsigned char def = 16) const {
        return getValue("params.sigpro.disRefWin", def).toUInt();
    }
    unsigned char sigProDopProWin(unsigned char def = 2) const {
        return getValue("params.sigpro.dopProWin", def).toUInt();
    }
    unsigned char sigProDopRefWin(unsigned char def = 16) const {
        return getValue("params.sigpro.dopRefWin", def).toUInt();
    }
    unsigned char sigProMTDWinType(unsigned char def = 1) const {
        return getValue("params.sigpro.MTDWinType", def).toUInt();
    }
    unsigned char sigProClutterMode(unsigned char def = 1) const {
        return getValue("params.sigpro.clutterMode", def).toUInt();
    }
    unsigned char sigProClutterChannelWidth(unsigned char def = 5) const {
        return getValue("params.sigpro.clutterChannelWidth", def).toUInt();
    }
    unsigned char sigProClutterUnitWin(unsigned char def = 1) const {
        return getValue("params.sigpro.clutterUnitWin", def).toUInt();
    }
    unsigned char sigProClutterIter(unsigned char def = 19) const {
        return getValue("params.sigpro.clutterIter", def).toUInt();
    }
    unsigned char sigProAlgorithmSwitch(unsigned char def = 7) const {
        return getValue("params.sigpro.algorithmSwitch", def).toUInt();
    }

    // 数据处理参数保存/读取
    void saveDataProParam(unsigned char startWinLen, unsigned char startPoint,
                         unsigned char endWinLen, unsigned char endPoint,
                         unsigned short noiseVar, unsigned short trackDisLower,
                         unsigned short trackDisUpper, unsigned short trackAziThresh,
                         unsigned short trackEleThresh, unsigned short trackVelThresh,
                         unsigned short trackStatThresh, unsigned char accuDisGate,
                         unsigned char accuAziGate, unsigned char accuEleGate, unsigned char accuVelGate) {
        saveValue("params.datapro.startWinLen", startWinLen);
        saveValue("params.datapro.startPoint", startPoint);
        saveValue("params.datapro.endWinLen", endWinLen);
        saveValue("params.datapro.endPoint", endPoint);
        saveValue("params.datapro.noiseVar", noiseVar);
        saveValue("params.datapro.trackDisLower", trackDisLower);
        saveValue("params.datapro.trackDisUpper", trackDisUpper);
        saveValue("params.datapro.trackAziThresh", trackAziThresh);
        saveValue("params.datapro.trackEleThresh", trackEleThresh);
        saveValue("params.datapro.trackVelThresh", trackVelThresh);
        saveValue("params.datapro.trackStatThresh", trackStatThresh);
        saveValue("params.datapro.accuDisGate", accuDisGate);
        saveValue("params.datapro.accuAziGate", accuAziGate);
        saveValue("params.datapro.accuEleGate", accuEleGate);
        saveValue("params.datapro.accuVelGate", accuVelGate);
    }

    unsigned char dataProStartWinLen(unsigned char def = 4) const {
        return getValue("params.datapro.startWinLen", def).toUInt();
    }
    unsigned char dataProStartPoint(unsigned char def = 3) const {
        return getValue("params.datapro.startPoint", def).toUInt();
    }
    unsigned char dataProEndWinLen(unsigned char def = 3) const {
        return getValue("params.datapro.endWinLen", def).toUInt();
    }
    unsigned char dataProEndPoint(unsigned char def = 3) const {
        return getValue("params.datapro.endPoint", def).toUInt();
    }
    unsigned short dataProNoiseVar(unsigned short def = 1) const {
        return getValue("params.datapro.noiseVar", def).toUInt();
    }
    unsigned short dataProTrackDisLower(unsigned short def = 10) const {
        return getValue("params.datapro.trackDisLower", def).toUInt();
    }
    unsigned short dataProTrackDisUpper(unsigned short def = 300) const {
        return getValue("params.datapro.trackDisUpper", def).toUInt();
    }
    unsigned short dataProTrackAziThresh(unsigned short def = 15) const {
        return getValue("params.datapro.trackAziThresh", def).toUInt();
    }
    unsigned short dataProTrackEleThresh(unsigned short def = 20) const {
        return getValue("params.datapro.trackEleThresh", def).toUInt();
    }
    unsigned short dataProTrackVelThresh(unsigned short def = 200) const {
        return getValue("params.datapro.trackVelThresh", def).toUInt();
    }
    unsigned short dataProTrackStatThresh(unsigned short def = 16) const {
        return getValue("params.datapro.trackStatThresh", def).toUInt();
    }
    unsigned char dataProAccuDisGate(unsigned char def = 20) const {
        return getValue("params.datapro.accuDisGate", def).toUInt();
    }
    unsigned char dataProAccuAziGate(unsigned char def = 15) const {
        return getValue("params.datapro.accuAziGate", def).toUInt();
    }
    unsigned char dataProAccuEleGate(unsigned char def = 20) const {
        return getValue("params.datapro.accuEleGate", def).toUInt();
    }
    unsigned char dataProAccuVelGate(unsigned char def = 3) const {
        return getValue("params.datapro.accuVelGate", def).toUInt();
    }

    // 数据保存参数保存/读取
    void saveDataSaveParam(unsigned char saveSwitch, unsigned char dataID) {
        saveValue("params.datasave.saveSwitch", saveSwitch);
        saveValue("params.datasave.dataID", dataID);
    }

    unsigned char dataSaveSaveSwitch(unsigned char def = 0) const {
        return getValue("params.datasave.saveSwitch", def).toUInt();
    }

    unsigned char dataSaveDataID(unsigned char def = 0) const {
        return getValue("params.datasave.dataID", def).toUInt();
    }

private:
    QMap<QString, QVariant> configData;
    QString configFilePath;  // 配置文件的完整路径，用于save()操作

    // TOML文件加载
    bool loadToml(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            LOG_WARNING(QString("TOML config file not found: %1, using defaults.").arg(path));
            return false;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        bool success = parseToml(content);
        if (success) {
            LOG_INFO(QString("Successfully loaded TOML config: %1 with %2 entries")
                         .arg(path)
                         .arg(configData.size()));

            // 调试：打印params段的值
            LOG_DEBUG("=== Loaded params values ===");
            LOG_DEBUG(QString("params.servo.cmd: %1").arg(configData.value("params.servo.cmd", "NOT FOUND").toString()));
            LOG_DEBUG(QString("params.servo.speed: %1").arg(configData.value("params.servo.speed", "NOT FOUND").toString()));
            LOG_DEBUG(QString("params.servo.az: %1").arg(configData.value("params.servo.az", "NOT FOUND").toString()));
            LOG_DEBUG(QString("params.scanrange.workMode: %1").arg(configData.value("params.scanrange.workMode", "NOT FOUND").toString()));
            LOG_DEBUG(QString("params.beamcontrol.freqID: %1").arg(configData.value("params.beamcontrol.freqID", "NOT FOUND").toString()));
            LOG_DEBUG("============================");
        }
        return success;
    }

    // 简单的TOML解析器（基础实现）
    bool parseToml(const QString& content) {
        QStringList lines = content.split('\n');
        QString currentSection = "";

        LOG_DEBUG(QString("parseToml: Total lines: %1").arg(lines.size()));

        for (const QString& line : lines) {
            QString trimmed = line.trimmed();

            // 跳过注释和空行
            if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                continue;
            }

            // 处理节（section）
            if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
                currentSection = trimmed.mid(1, trimmed.length() - 2);
                LOG_DEBUG(QString("parseToml: Found section: %1").arg(currentSection));
                continue;
            }

            // 处理键值对
            int equalPos = trimmed.indexOf('=');
            if (equalPos > 0) {
                QString key = trimmed.left(equalPos).trimmed();
                QString value = trimmed.mid(equalPos + 1).trimmed();

                // 移除行内注释
                int commentPos = value.indexOf('#');
                if (commentPos >= 0) {
                    value = value.left(commentPos).trimmed();
                }

                // 移除引号。必须在行内注释之后处理，否则 "192.168.64.3" # 注释 会保留引号。
                if ((value.startsWith('"') && value.endsWith('"')) ||
                    (value.startsWith('\'') && value.endsWith('\''))) {
                    value = value.mid(1, value.length() - 2);
                }

                // 构建完整的键路径
                QString fullKey = currentSection.isEmpty() ? key : currentSection + "." + key;

                // 尝试转换为适当的类型
                QVariant varValue;
                bool ok;
                int intVal = value.toInt(&ok);
                if (ok) {
                    varValue = intVal;
                } else {
                    double doubleVal = value.toDouble(&ok);
                    if (ok) {
                        varValue = doubleVal;
                    } else if (value.toLower() == "true") {
                        varValue = true;
                    } else if (value.toLower() == "false") {
                        varValue = false;
                    } else {
                        varValue = value;
                    }
                }

                configData[fullKey] = varValue;

                // 调试：打印params段的键值对
                if (fullKey.startsWith("params.")) {
                    LOG_DEBUG(QString("parseToml: Parsed %1 = %2")
                                  .arg(fullKey)
                                  .arg(varValue.toString()));
                }
            }
        }

        return true;
    }

    // 统一的值获取方法
    QVariant getValue(const QString& key, const QVariant& defaultValue) const {
        return configData.value(key, defaultValue);
    }

    // 保存配置到TOML文件（保持格式和注释）
    bool saveToml(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            LOG_WARNING(QString("Cannot open TOML config file for writing: %1").arg(path));
            return false;
        }

        // 读取现有内容
        QTextStream in(&file);
        QString content = in.readAll();
        file.seek(0);

        // 更新参数配置段（如果不存在则添加）
        if (!content.contains("[params.servo]")) {
            content += "\n# =============================================================================\n";
            content += "# 参数配置（运行时保存） Parameter Configuration (Runtime Save)\n";
            content += "# =============================================================================\n\n";
            content += "[params.servo]\n";
            content += "# 伺服控制参数 Servo Control Parameters\n";
            content += "cmd = 0\n";
            content += "speed = 3\n";
            content += "az = 0\n\n";

            content += "[params.scanrange]\n";
            content += "# 扫描范围参数 Scan Range Parameters\n";
            content += "workMode = 0\n\n";

            content += "[params.beamcontrol]\n";
            content += "# 波形控制参数 Beam Control Parameters\n";
            content += "freqID = 4\n";
            content += "type = 2\n";
            content += "aziStart = -4500\n";
            content += "aziEnd = 4500\n";
            content += "aziStep = 400\n";
            content += "pulseNum = 128\n";
            content += "flagNum = 2\n";
            content += "beam1Flag = 1\n";
            content += "beam1Code = 6\n";
            content += "sampleStart1 = 0\n";
            content += "sampleEnd1 = 0\n";
            content += "elestart1 = 0\n";
            content += "eleend1 = 3000\n";
            content += "elestep1 = 400\n";
            content += "beam2Flag = 1\n";
            content += "beam2Code = 9\n";
            content += "sampleStart2 = 0\n";
            content += "sampleEnd2 = 0\n";
            content += "elestart2 = 0\n";
            content += "eleend2 = 1500\n";
            content += "elestep2 = 400\n";
            content += "beam3Flag = 0\n";
            content += "beam3Code = 11\n";
            content += "sampleStart3 = 0\n";
            content += "sampleEnd3 = 0\n";
            content += "elestart3 = 0\n";
            content += "eleend3 = 400\n";
            content += "elestep3 = 400\n\n";

            content += "[params.sigpro]\n";
            content += "# 信号处理参数 Signal Processing Parameters\n";
            content += "noise = 1000\n";
            content += "thresh1 = 0\n";
            content += "thresh2 = 0\n";
            content += "clutterThresh = 0\n";
            content += "clutterMapFlaseRate = 0\n";
            content += "CFARType = 0\n";
            content += "disProWin = 0\n";
            content += "disRefWin = 0\n";
            content += "dopProWin = 0\n";
            content += "dopRefWin = 0\n";
            content += "MTDWinType = 0\n";
            content += "clutterMode = 0\n";
            content += "clutterChannelWidth = 0\n";
            content += "clutterUnitWin = 0\n";
            content += "clutterIter = 0\n";
            content += "algorithmSwitch = 0\n\n";

            content += "[params.datapro]\n";
            content += "# 数据处理参数 Data Processing Parameters\n";
            content += "startWinLen = 3\n";
            content += "startPoint = 0\n";
            content += "endWinLen = 0\n";
            content += "endPoint = 0\n";
            content += "noiseVar = 0\n";
            content += "trackDisLower = 0\n";
            content += "trackDisUpper = 0\n";
            content += "trackAziThresh = 0\n";
            content += "trackEleThresh = 0\n";
            content += "trackVelThresh = 0\n";
            content += "trackStatThresh = 0\n";
            content += "accuDisGate = 0\n";
            content += "accuAziGate = 0\n";
            content += "accuEleGate = 0\n";
            content += "accuVelGate = 0\n\n";

            content += "[params.datasave]\n";
            content += "# 数据保存参数 Data Save Parameters\n";
            content += "saveSwitch = 0\n";
            content += "dataID = 0\n\n";
        }

        // 如果displayConfig段不存在，添加它
        if (!content.contains("[displayConfig]")) {
            content += "\n# =============================================================================\n";
            content += "# 显示配置 Display Configuration\n";
            content += "# =============================================================================\n\n";
            content += "[displayConfig]\n";
            content += "# 检测点显示配置 Detection Point Display Configuration\n";
            content += "max_points = 1000\n";
            content += "max_track_points = 200\n\n";
        }

        // 更新内存中的值到文本内容
        QStringList lines = content.split('\n');
        QString currentSection = "";

        for (int i = 0; i < lines.size(); ++i) {
            QString trimmed = lines[i].trimmed();

            // 跟踪当前段
            if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
                currentSection = trimmed.mid(1, trimmed.length() - 2);
                continue;
            }

            // 更新参数值（支持 params.* 和 displayConfig 段）
            int equalPos = trimmed.indexOf('=');
            if (equalPos > 0 && (currentSection.startsWith("params.") || currentSection == "displayConfig")) {
                QString key = trimmed.left(equalPos).trimmed();
                QString fullKey = currentSection + "." + key;

                if (configData.contains(fullKey)) {
                    QVariant value = configData[fullKey];
                    QString valueStr;

                    if (value.type() == QVariant::Bool) {
                        valueStr = value.toBool() ? "true" : "false";
                    } else if (value.type() == QVariant::String) {
                        valueStr = "\"" + value.toString() + "\"";
                    } else {
                        valueStr = value.toString();
                    }

                    // 保留注释（如果有）
                    int commentPos = lines[i].indexOf('#', equalPos);
                    QString comment = commentPos >= 0 ? "  " + lines[i].mid(commentPos) : "";

                    lines[i] = key + " = " + valueStr + comment;
                }
            }
        }

        // 写回文件
        file.resize(0);
        QTextStream out(&file);
        out << lines.join('\n');
        file.close();

        LOG_INFO(QString("Successfully saved config to: %1").arg(path));
        return true;
    }


    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

#endif // CONFIGMANAGER_H
