/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-17 22:45:15
 * @Description: 
 */
/**
 * @file controller.cpp
 * @brief 系统主控制器实现
 * @details 雷达显示控制系统的核心控制器实现，负责协调各个子系统的数据流和消息传递
 *
 * 实现功能：
 * - 单例模式的控制器实例管理
 * - 各子系统管理器的创建和初始化
 * - 信号槽连接的统一配置
 * - 系统生命周期的管理
 *
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#include "controller.h"
#include "disp2resmanager.h"
#include "res2dispmanager.h"
#include "disp2sigmanager.h"
#include "disp2photomanager.h"
#include "sig2dispmanager.h"
#include "data2dispmanager.h"
#include "tbd2dispmanager.h"
#include "collabtrack2dispmanager.h"
#include "disp2datamanager.h"
#include "targetdispmanager.h"
#include "disp2monmanager.h"
#include "mon2dispmanager.h"
#include "ExternalCtrlManager.h"
#include "testtrackgenerator.h"
#include "Basic/log.h"
#include "Basic/radarpositioncontrol.h"
#include "Basic/radarpanelstore.h"
#include <QDir>
#include <cmath>
#include <QTimer>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

// 全局静态单例实例定义
Q_GLOBAL_STATIC(Controller, ControllerInstance)

/**
 * @brief 获取控制器单例实例
 * @return 控制器实例指针
 * @details 使用Qt的Q_GLOBAL_STATIC宏实现线程安全的单例模式
 */
Controller* Controller::getInstance() {
    return ControllerInstance();
}

/**
 * @brief 构造函数
 * @param parent 父对象指针
 * @details 初始化基本成员变量，实际的子系统创建在init()函数中
 */
Controller::Controller(QObject* parent)
    : QObject(parent)
    , m_radarReference(m_panelState.reference(0))
    , resMgr(nullptr)
    , resRecvMgr(nullptr)
    , sigMgr(nullptr)
    , photoMgr(nullptr)
    , sigRecvMgr(nullptr)
    , sigRecvMgr2(nullptr)
    , dataRecvMgr(nullptr)
    , tbdRecvMgr(nullptr)
    , collabTrackRecvMgr(nullptr)
    , dataMgr(nullptr)
    , tarMgr(nullptr)
    , monMgr(nullptr)
    , monRecvMgr(nullptr)
    , extCtrlMgr(nullptr) {
    std::array<double, RadarReference::Count> defaults{108.9138, 34.2311, 400.0, 0.0, 0.0, 0.0};
    const std::array<const char*, RadarReference::Count> keys{"longitude", "latitude", "altitude", "azimuth", "pitch", "roll"};
    for (int i = 0; i < RadarReference::Count; ++i) {
        bool ok = false;
        const double value = CF_INS.radarValue(QString::fromLatin1(keys[i]), defaults[i]).toDouble(&ok);
        if (ok && RadarReference::valid(static_cast<RadarReference::Field>(i), value)) defaults[i] = value;
        else LOG_WARNING(QStringLiteral("[RADAR_REFERENCE] invalid radar.%1; using built-in boot default %2")
                         .arg(QString::fromLatin1(keys[i])).arg(defaults[i], 0, 'g', 15));
    }
    m_panelState.initialize(defaults, CF_INS.gpsMaxHorizontalDeviationM(), CF_INS.gpsMaxAltitudeDeviationM(), CF_INS.defaultPanelCount() > 1);
    m_multiPanelReference = CF_INS.defaultPanelCount() > 1;
    m_referenceDebugEnabled = CF_INS.referenceDebugLogEnabled();
    m_referenceDebugSamples = CF_INS.referenceDebugLogSamples();
    m_referenceDebugIntervalMs = CF_INS.referenceDebugLogIntervalMs();
    m_savedPanelReferences = m_panelState.dialogValues();
    if (m_multiPanelReference) {
        m_panelReferencePath = QFileInfo(CF_INS.getConfigFilePath().isEmpty()
            ? QStringLiteral("config.toml") : CF_INS.getConfigFilePath()).absoluteDir().filePath("radar_panel_reference.toml");
        if (QFile::exists(m_panelReferencePath)) {
            RadarPanelValues restored{}; QString error;
            if (RadarPanelStore::load(m_panelReferencePath, restored, error)) {
                m_panelState.restoreLocked(restored); m_savedPanelReferences = restored;
                LOG_INFO("[PANEL_REFERENCE][LOAD] " + m_panelReferencePath);
            } else LOG_WARNING("[PANEL_REFERENCE][LOAD_FAILED] " + m_panelReferencePath + ": " + error);
        } else saveRadarPanelReferences(m_panelState);
    }
    m_gpsFilter.setLimits(CF_INS.gpsMaxHorizontalDeviationM(), CF_INS.gpsMaxAltitudeDeviationM());
    // 成员指针初始化在init()中进行
    m_bitLogClock.start();
    m_bitWarningLogIntervalMs = qMax(HealthStateWindow::IntervalMs, CF_INS.bitReportWarningLogIntervalMs());
    connect(this, &Controller::monitorParamSend, this, [this](MonitorParam report) {
        m_healthStateWindow.pushSoftware(report);
    });
    auto* healthTimer = new QTimer(this);
    healthTimer->setInterval(HealthStateWindow::IntervalMs);
    healthTimer->setTimerType(Qt::PreciseTimer);
    connect(healthTimer, &QTimer::timeout, this, &Controller::flushHealthStateWindow);
    healthTimer->start();
}

/**
 * @brief 初始化系统
 * @details 创建所有子系统管理器实例并建立信号槽连接
 *
 * 子系统创建顺序：
 * 1. 资源管理器 - 硬件资源控制
 * 2. 信号管理器 - 信号处理控制
 * 3. 光电管理器 - 光电设备控制
 * 4. 数据管理器 - 数据流控制
 * 5. 目标管理器 - 目标处理控制
 * 6. 监控管理器 - 系统监控
 *
 * 信号连接策略：
 * - 控制信号从Controller向各管理器传递
 * - 状态信号从各管理器向Controller汇总
 * - 数据流信号在管理器间直接传递
 */
void Controller::init()
{
    LOG_INFO(QString("[Controller::init] displayConfig iftbd=%1 ifxietong=%2 detListen=%3 trackListen=%4 tbdListen=%5 collabListen=%6")
             .arg(CF_INS.iftbd(false))
             .arg(CF_INS.ifxietong(false))
             .arg(CF_INS.port("DISP_GET_SIG_PORT1", DISP_GET_SIG_PORT1))
             .arg(CF_INS.port("DISP_GET_DATA_PORT", DISP_GET_DATA_PORT))
             .arg(CF_INS.port("DISP_GET_DATA_PORT2", DISP_GET_DATA_PORT2))
             .arg(CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3)));

    // === 创建子系统管理器实例 ===
    resMgr = new Disp2ResManager(this);        // 显示到资源管理器
    resRecvMgr = new Res2DispManager(this);    // 资源到显示管理器
    sigMgr = new Disp2SigManager(this);        // 显示到信号管理器
    if (CF_INS.photoelectricTxEnabled(false)) {
        photoMgr = new Disp2PhotoManager(this);    // 显示到光电管理器
    }
    sigRecvMgr = new sig2dispmanager(this);    // 信号到显示管理器
    sigRecvMgr2 = new sig2dispmanager2(this);  // 信号到显示管理器2
    dataRecvMgr = new Data2DispManager(this);  // 数据到显示管理器
    if (CF_INS.iftbd(false)) {
        tbdRecvMgr = new Tbd2DispManager(this);    // TBD数据到显示管理器
    } else {
        LOG_INFO("[Controller::init] TBD receive/display path disabled by config iftbd=false");
    }
    if (CF_INS.ifxietong(false)) {
        collabTrackRecvMgr = new CollabTrack2DispManager(this); // 协同航迹到显示管理器
    } else {
        LOG_INFO("[Controller::init] Cooperative track receive/display path disabled by config ifxietong=false");
    }
    dataMgr = new Disp2DataManager(this);      // 显示到数据管理器
    tarMgr = new targetDispManager(this);      // 目标显示管理器
    monMgr = new Disp2MonManager(this);        // 显示到监控管理器
    monRecvMgr = new Mon2DispManager(this);    // 监控到显示管理器
    extCtrlMgr = new ExternalCtrlManager(this); // 外部雷控链路
    if (CF_INS.testTrackEnabled(false)) {
        // 生成器运行在 Controller 所在线程；定时产生的普通航迹直接进入既有下游信号。
        testTrackGenerator = new TestTrackGenerator(this);
        connect(testTrackGenerator, &TestTrackGenerator::trackGenerated, this, [this](PointInfo point) {
            emit testPointGenerated(point);
            emit traInfoProcess(point);
        });
        // 不把预存位置伪装成 DD05；总控模块显式识别这是测试联调原点。
        connect(testTrackGenerator, &TestTrackGenerator::fallbackRadarPositionReady,
                this, &Controller::testTrackFallbackRadarPositionReady);
        connect(testTrackGenerator, &TestTrackGenerator::generationStarted,
                this, [this](int, int) { emit testTrackGenerationStateChanged(true); });
        connect(testTrackGenerator, &TestTrackGenerator::generationFinished,
                this, [this]() { emit testTrackGenerationStateChanged(false); });
        LOG_INFO("[Controller::init] 本地测试航迹功能已启用；按钮可在参数设置区启动生成");
    }

    if (CF_INS.testDetectionEnabled(false)) {
        testDetectionGenerator = new TestDetectionGenerator(this);
        connect(testDetectionGenerator, &TestDetectionGenerator::detectionGenerated, this, [this](PointInfo point) {
            emit testPointGenerated(point);
            emit detInfoProcess(point);
        });
        connect(testDetectionGenerator, &TestDetectionGenerator::fallbackRadarPositionReady,
                this, &Controller::testDetectionFallbackRadarPositionReady);
        connect(testDetectionGenerator, &TestDetectionGenerator::stateChanged,
                this, &Controller::testDetectionGenerationStateChanged);
    }

    // === 建立信号槽连接 ===

    // 向资源系统发送控制参数
    connect(this, &Controller::sendBCParam, resMgr, &Disp2ResManager::sendBCParam);
    connect(this, &Controller::sendTRParam, resMgr, &Disp2ResManager::sendTRParam);
    connect(this, &Controller::sendServoControl, resMgr, &Disp2ResManager::sendServoControl);
    connect(this, &Controller::sendFCParam, resMgr, &Disp2ResManager::sendFCParam);
    connect(this, &Controller::sendSRParam, resMgr, &Disp2ResManager::sendSRParam);
    connect(this, &Controller::sendWCParam, resMgr, &Disp2ResManager::sendWCParam);
    connect(this, &Controller::sendSPParam, resMgr, &Disp2ResManager::sendSPParam);
    connect(this, &Controller::sendRadarPositionControl, resMgr, &Disp2ResManager::sendRadarPositionControl);
    connect(this, &Controller::sendDPParam, resMgr, &Disp2ResManager::sendDPParam);

    // 向光电系统发送控制参数
    if (photoMgr) {
        connect(this, &Controller::sendPEParam, photoMgr, &Disp2PhotoManager::sendPEParam);
        connect(this, &Controller::sendPEParam2, photoMgr, &Disp2PhotoManager::sendPEParam2);
    }

    // 向信号系统发送控制参数
    connect(this, &Controller::sendDSParam, sigMgr, &Disp2SigManager::sendDSParam);

    // 向数据系统发送控制指令
    connect(this, &Controller::setManual, dataMgr, &Disp2DataManager::setManual);

    // 向监控系统发送控制参数
    connect(this, &Controller::sendSysStart, monMgr, &Disp2MonManager::sendSysStart);

    // 外部雷控链路回送
    connect(extCtrlMgr, &ExternalCtrlManager::systemCtrlAck, this, &Controller::externalSystemCtrlAck);
    connect(extCtrlMgr, &ExternalCtrlManager::servoCtrlAck, this, &Controller::externalServoAck);
    connect(extCtrlMgr, &ExternalCtrlManager::externalLog, this, &Controller::externalCtrlLog);

    // 航向角（控制表解析）
    connect(sigRecvMgr, &sig2dispmanager::headingUpdated,
            this, &Controller::updateHeadingFromCtrlTable);
}

/**
 * @brief 析构函数
 * @details 清理资源，Qt的父子对象关系会自动清理子对象
 */
Controller::~Controller() {
    // Qt对象树会自动清理子对象，无需手动删除
}

bool Controller::startTestTrackGeneration()
{
    if (!testTrackGenerator) {
        LOG_WARNING("[TestTrack] 已请求生成，但 [test_track].enabled=false，未创建生成器");
        return false;
    }
    return testTrackGenerator->start();
}

bool Controller::stopTestTrackGeneration()
{
    if (!testTrackGenerator) {
        return false;
    }
    return testTrackGenerator->stop();
}

bool Controller::isTestTrackGenerationRunning() const
{
    return testTrackGenerator && testTrackGenerator->isRunning();
}

bool Controller::startTestDetectionGeneration()
{
    return testDetectionGenerator && testDetectionGenerator->start();
}
bool Controller::stopTestDetectionGeneration()
{
    return testDetectionGenerator && testDetectionGenerator->stop();
}
bool Controller::isTestDetectionGenerationRunning() const
{
    return testDetectionGenerator && testDetectionGenerator->isRunning();
}

bool Controller::sendExternalSystemControl(const QByteArray& frame512) {
    if (!extCtrlMgr) return false;
    return extCtrlMgr->sendSystemControl(frame512);
}

bool Controller::sendExternalServoControl(const QByteArray& frame32) {
    if (!extCtrlMgr) return false;
    return extCtrlMgr->sendServoControl(frame32);
}

void Controller::sendRoadPointsToDataPro(const RoadPointGeo* points, int count)
{
    if (!dataMgr || !points || count <= 0) return;

    int frameTotal = (count + ROAD_POINTS_PER_FRAME - 1) / ROAD_POINTS_PER_FRAME;

    for (int frameIdx = 0; frameIdx < frameTotal; ++frameIdx) {
        int startIdx = frameIdx * ROAD_POINTS_PER_FRAME;
        int endIdx = qMin(startIdx + ROAD_POINTS_PER_FRAME, count);
        int pointsInFrame = endIdx - startIdx;

        // 构建帧: [RoadPointFrame] + [pointsInFrame × RoadPointGeo]
        int dataSize = static_cast<int>(sizeof(RoadPointFrame))
                     + pointsInFrame * static_cast<int>(sizeof(RoadPointGeo));
        QByteArray buffer(dataSize, 0);

        auto* header = reinterpret_cast<RoadPointFrame*>(buffer.data());
        header->mesID = 0xDF02;
        header->totalPoints = static_cast<unsigned int>(count);
        header->frameIndex  = static_cast<unsigned short>(frameIdx);
        header->frameTotal  = static_cast<unsigned short>(frameTotal);
        header->pointsInFrame = static_cast<unsigned short>(pointsInFrame);

        auto* ptData = reinterpret_cast<RoadPointGeo*>(buffer.data() + sizeof(RoadPointFrame));
        memcpy(ptData, &points[startIdx], pointsInFrame * sizeof(RoadPointGeo));

        dataMgr->sendParam(buffer.data(), static_cast<unsigned>(dataSize));
    }

    LOG_DEBUG(QString("[Controller] Sent %1 road points to data processing in %2 frames")
                  .arg(count)
                  .arg(frameTotal));
}

void Controller::updateHeadingFromCtrlTable(double headingDeg) {
    if (!m_multiPanelReference && m_panelState.updateLegacyHeading(headingDeg)) emit radarReferenceChanged();
    emit scanHeadingChanged(headingDeg);
}

void Controller::flushHealthStateWindow()
{
    for (int id = 0; id < 4; ++id) {
        HealthStateWindow::BitSnapshot snapshot;
        if (!m_healthStateWindow.takeBit(id, snapshot)) continue;
        emit healthBitSnapshot(snapshot.report, snapshot.powerConfirmed, snapshot.consecutiveZeroFrames);
        const auto bits = snapshot.report.bitGroup;
        quint64 faultKey = (bits & 0x60) | (~bits & 0x0F);
        faultKey |= quint64(~snapshot.report.powerState & 0x03) << 8;
        if (snapshot.powerConfirmed) for (int i = 0; i < 5; ++i)
            faultKey |= quint64(~snapshot.report.subArrayPower[i] & (i == 4 ? 0x0F : 0xFF)) << (16 + i * 8);
        if (!m_healthFaultLogGate.sample(id, faultKey, m_bitLogClock.elapsed())) continue;
        QStringList faults;
        if (bits & 0x40) faults << QStringLiteral("占空比报警");
        if (bits & 0x20) faults << QStringLiteral("脉宽报警");
        const std::array<QString, 4> names{{QStringLiteral("北斗异常"), QStringLiteral("伺服异常"),
                                          QStringLiteral("收发板断链"), QStringLiteral("频率源异常")}};
        for (int bit = 0; bit < 4; ++bit) if (!(bits & (1 << bit))) faults << names[bit];
        if (!(snapshot.report.powerState & 0x02)) faults << QStringLiteral("蓝牙异常");
        if (!(snapshot.report.powerState & 0x01)) faults << QStringLiteral("波控板电源异常");
        if (snapshot.powerConfirmed) {
            QStringList powers;
            for (int i = 0; i < 36; ++i)
                if (!(snapshot.report.subArrayPower[i / 8] & (1 << (i % 8)))) powers << QString::number(i + 1);
            if (!powers.isEmpty()) faults << QStringLiteral("子阵电源故障[%1]").arg(powers.join(','));
        }
        const QString summary = QStringLiteral("5秒窗口 阵面%1：%2").arg(id + 1).arg(faults.join(QStringLiteral("；")));
        LOG_WARNING("[HEALTH][FAULT] " + summary);
        emit healthFaultSummary(summary);
    }
    MonitorParam software;
    if (m_healthStateWindow.takeSoftware(software)) {
        emit healthSoftwareSnapshot(software);
        const std::array<unsigned char, 4> states{{software.sigProSta, software.dataProSta, software.beamConSta, software.targetRecSta}};
        const std::array<QString, 4> names{{QStringLiteral("信号处理"), QStringLiteral("数据处理"),
                                          QStringLiteral("波束调度"), QStringLiteral("目标识别")}};
        QStringList faults;
        quint64 faultKey = 0;
        for (std::size_t i = 0; i < states.size(); ++i) if (HealthStateWindow::softwareFault(states[i]))
        {
            faults << names[i] + QStringLiteral("异常(状态=%1)").arg(states[i]);
            faultKey |= quint64(states[i] + 1) << (8 * i);
        }
        if (m_healthFaultLogGate.sample(4, faultKey, m_bitLogClock.elapsed())) {
            const QString summary = QStringLiteral("5秒窗口 软件：") + faults.join(QStringLiteral("；"));
            LOG_WARNING("[HEALTH][FAULT] " + summary);
            emit healthFaultSummary(summary);
        }
    }
    emit healthWindowFlushed();
}

void Controller::updateRadarGeoLocation(double latitude, double longitude, double altitude)
{
    // Multi-panel installations take position from ID0 DE02, never mix in DD05.
    if (!m_multiPanelReference) applyRadarGeoLocation(latitude, longitude, altitude);
}

void Controller::applyRadarGeoLocation(double latitude, double longitude, double altitude)
{
    GpsPositionFilter::Fix average;
    const auto result = m_gpsFilter.push({latitude, longitude, altitude}, average);
    if (result != GpsPositionFilter::Result::Accepted) {
        const auto log = m_gpsRejectLogs.sample(m_bitLogClock.elapsed(), 5000);
        if (log.write) LOG_WARNING(QStringLiteral("[GPS][REJECT] reason=%1 lat=%2 lon=%3 alt=%4 suppressed=%5")
            .arg(static_cast<int>(result)).arg(latitude, 0, 'g', 15).arg(longitude, 0, 'g', 15)
            .arg(altitude, 0, 'g', 15).arg(qulonglong(log.suppressed)));
        return;
    }
    // EM keeps its independent manual override, but never receives invalid/outlier GPS fixes.
    if (!m_radarReference.locked()) emit filteredGeoLocationReported(average.latitude, average.longitude, average.altitude);
    bool changed = m_radarReference.updateReport(RadarReference::Latitude, average.latitude);
    changed = m_radarReference.updateReport(RadarReference::Longitude, average.longitude) || changed;
    changed = m_radarReference.updateReport(RadarReference::Altitude, average.altitude) || changed;
    if (changed) publishRadarReference();
}

void Controller::publishRadarReference()
{
    emit radarReferenceChanged();
    emit geoLocationUpdated(m_radarReference.value(RadarReference::Latitude),
                            m_radarReference.value(RadarReference::Longitude),
                            m_radarReference.value(RadarReference::Altitude));
}

bool Controller::setManualRadarReference(const RadarReference::Values& values)
{
    if (!m_radarReference.setManual(values)) return false;
    LOG_INFO(QStringLiteral("[RADAR_REFERENCE] latest UI applied lon=%1 lat=%2 alt=%3 yaw=%4 pitch=%5 roll=%6")
        .arg(m_radarReference.value(RadarReference::Longitude), 0, 'f', 7)
        .arg(m_radarReference.value(RadarReference::Latitude), 0, 'f', 7)
        .arg(m_radarReference.value(RadarReference::Altitude), 0, 'f', 3)
        .arg(m_radarReference.value(RadarReference::Yaw), 0, 'f', 3)
        .arg(m_radarReference.value(RadarReference::Pitch), 0, 'f', 3)
        .arg(m_radarReference.value(RadarReference::Roll), 0, 'f', 2));
    publishRadarReference();
    return true;
}

bool Controller::sendCurrentRadarPanelPosition()
{
    return sendRadarPanelPositions(m_panelState.dialogValues());
}

bool Controller::sendRadarPanelPositions(const RadarPanelValues& values)
{
    RadarPositionControl command;
    if (!resMgr || !m_panelState.buildCommand(values, command)) {
        LOG_WARNING("[AA08][REJECT] reference invalid or resource manager not initialized");
        return false;
    }
    auto proposed = m_panelState;
    proposed.rememberSubmitted(command);
    if (!saveRadarPanelReferences(proposed)) return false;
    QStringList panels;
    for (const auto& panel : command.panels) panels << QStringLiteral("ID=%1 lat=%2 lon=%3 alt=%4m heading=%5° pitch=%6° roll=%7°")
        .arg(panel.radarId).arg(panel.latitudeE9 / 1e9, 0, 'f', 9).arg(panel.longitudeE9 / 1e9, 0, 'f', 9)
        .arg(panel.altitudeCm * 0.01, 0, 'f', 2).arg(panel.headingMilliDeg * 0.001, 0, 'f', 3)
        .arg(panel.pitchMilliDeg * 0.001, 0, 'f', 3).arg(panel.rollCentiDeg * 0.01, 0, 'f', 2);
    const QString details = QStringLiteral("AA08 下发请求(142B)：") + panels.join(QStringLiteral("；"));
    LOG_INFO("[AA08][SUBMIT] " + details);
    emit radarPanelPositionRequested(details);
    emit sendRadarPositionControl(command);
    m_panelState.rememberSubmitted(command); // Commit edited locked rows after whole-command validation.
    if (m_radarReference.locked()) {
        publishRadarReference();
        emit filteredGeoLocationReported(m_radarReference.value(RadarReference::Latitude),
            m_radarReference.value(RadarReference::Longitude), m_radarReference.value(RadarReference::Altitude));
    }
    return true;
}

bool Controller::lockRadarPanelReference(int id, const RadarReference::Pose& values)
{
    auto proposed = m_panelState;
    if (!proposed.lock(id, values) || !saveRadarPanelReferences(proposed)) return false;
    if (!m_panelState.lock(id, values)) return false;
    LOG_INFO(QStringLiteral("[PANEL_REFERENCE][LOCK] id=%1 lon=%2 lat=%3 alt=%4 yaw=%5 pitch=%6 roll=%7")
        .arg(id).arg(values[0], 0, 'f', 9).arg(values[1], 0, 'f', 9).arg(values[2], 0, 'f', 2)
        .arg(values[3], 0, 'f', 3).arg(values[4], 0, 'f', 3).arg(values[5], 0, 'f', 2));
    if (id == 0) {
        publishRadarReference();
        emit filteredGeoLocationReported(values[1], values[0], values[2]);
    }
    return true;
}

bool Controller::unlockRadarPanelReference(int id)
{
    auto proposed = m_panelState;
    if (!proposed.unlock(id) || !saveRadarPanelReferences(proposed)) return false;
    if (!m_panelState.unlock(id)) return false;
    LOG_INFO(QStringLiteral("[PANEL_REFERENCE][UNLOCK] id=%1; other panels frozen; accept new BIT; curve/mean continue").arg(id));
    if (id == 0) {
        publishRadarReference();
        emit filteredGeoLocationReported(m_radarReference.value(RadarReference::Latitude),
            m_radarReference.value(RadarReference::Longitude), m_radarReference.value(RadarReference::Altitude));
    } else emit radarReferenceChanged(); // ID0 may have been automatically frozen by this selection.
    return true;
}

bool Controller::saveRadarPanelReferences(const RadarPanelState& state)
{
    if (!m_multiPanelReference) return true;
    auto values = m_savedPanelReferences;
    for (int id = 0; id < 4; ++id)
        if (state.reference(id).locked()) values[id] = state.reference(id).pose();
    QString error;
    if (!RadarPanelStore::save(m_panelReferencePath, values, error)) {
        LOG_WARNING("[PANEL_REFERENCE][SAVE_FAILED] " + m_panelReferencePath + ": " + error);
        return false;
    }
    m_savedPanelReferences = values;
    LOG_INFO("[PANEL_REFERENCE][SAVED] " + m_panelReferencePath);
    return true;
}

void Controller::logRadarReferenceDebug(const BITReportV2& report, const RadarPanelState::Update& update)
{
    if (!m_referenceDebugEnabled) return;
    const int id = report.radarId;
    const auto decision = m_referenceDebugLogs[id].sample(m_bitLogClock.elapsed(), m_referenceDebugIntervalMs);
    if (!decision.write) return; // No formatting/window copies on suppressed frames.
    const auto d = m_panelState.debugSnapshot(id);
    const auto plot = m_panelState.snapshot();
    const auto& reference = m_panelState.reference(id);
    const QStringList reasons{"accepted", "invalid", "all_zero", "deviation"};
    const QStringList sources{"boot", "report", "manual", "locked"};
    QJsonObject raw{{"yaw_uint16_offset8", int(report.yaw)}, {"yaw_deg", report.yaw * 0.01},
        {"heading_uint32_offset59", double(report.headingMilliDeg)}, {"heading_deg", report.headingMilliDeg * 0.001},
        {"pitch_deg", report.pitchMilliDeg * 0.001}, {"roll_deg", report.rollCentiDeg * 0.01},
        {"latitude_e9", double(report.latitudeE9)}, {"longitude_e9", double(report.longitudeE9)},
        {"altitude_cm", double(report.altitudeCm)}};
    QJsonObject means{{"gps_count", int(d.gpsCount)}, {"latitude_deg", d.gpsMean.latitude},
        {"longitude_deg", d.gpsMean.longitude}, {"altitude_m", d.gpsMean.altitude},
        {"heading_count_this_id", int(d.attitudeCount)}, {"heading_mean_deg", d.attitudeMean[0]},
        {"pitch_mean_deg", d.attitudeMean[1]}, {"roll_mean_deg", d.attitudeMean[2]},
        {"table_heading_effective_deg", reference.value(RadarReference::Yaw)},
        {"table_heading_source", sources[int(reference.source(RadarReference::Yaw))]},
        {"curve_count_across_unlock_switches", int(d.curveCount)}, {"curve_yaw_mean_deg", d.curveMean[0]}};
    if (plot.historyCount) {
        const auto& latest = plot.history[plot.historyCount - 1];
        means.insert("curve_last_id", latest.id); means.insert("curve_frame", double(latest.frame));
    }
    QJsonObject line{{"id", id}, {"locked", reference.locked()}, {"window_capacity", int(GpsPositionFilter::SampleCount)},
        {"gps_result", reference.locked() ? QString("ignored_locked") : reasons[int(update.position)]},
        {"heading_sample_accepted", update.attitudeValid}, {"curve_sample_accepted", !reference.locked()},
        {"suppressed_rx", double(decision.suppressed)}, {"raw_current_packet", raw}, {"means_after_packet", means}};
    if (m_referenceDebugSamples) {
        QJsonArray gps, attitude, curve, curveIds;
        for (std::size_t i = 0; i < d.gpsCount; ++i)
            gps.append(QJsonArray{d.gps[i].latitude, d.gps[i].longitude, d.gps[i].altitude});
        for (std::size_t i = 0; i < d.attitudeCount; ++i)
            attitude.append(QJsonArray{d.attitude[i][0], d.attitude[i][1], d.attitude[i][2]});
        for (std::size_t i = 0; i < d.curveCount; ++i) {
            curve.append(d.curve[i][0]);
            curveIds.append(plot.history[plot.historyCount - d.curveCount + i].id);
        }
        line.insert("inputs_oldest_to_newest", QJsonObject{{"gps_lat_lon_alt", gps},
            {"heading_pitch_roll_this_id", attitude}, {"curve_yaw_deg", curve}, {"curve_ids", curveIds}});
    }
    LOG_INFO("[RADAR_REFERENCE][DEBUG] " + QString::fromUtf8(QJsonDocument(line).toJson(QJsonDocument::Compact)));
}

void Controller::onBITReferenceReport(BITReportV2 report)
{
    if (!m_multiPanelReference || report.mesID != 0xDE02 || report.radarId > RADAR_ID_MAX) return;
    const auto update = m_panelState.ingest(report, QDateTime::currentMSecsSinceEpoch());
    logRadarReferenceDebug(report, update);
    if (!m_panelState.reference(report.radarId).locked()
        && (!update.attitudeValid || update.position != GpsPositionFilter::Result::Accepted)) {
        const auto log = m_panelReferenceRejectLogs[report.radarId].sample(m_bitLogClock.elapsed(), m_bitWarningLogIntervalMs);
        if (log.write) LOG_WARNING(QStringLiteral("[BIT_REFERENCE][REJECT] id=%1 position=%2 attitudeValid=%3 suppressed=%4")
            .arg(report.radarId).arg(int(update.position)).arg(update.attitudeValid).arg(qulonglong(log.suppressed)));
    }
    if (report.radarId != RADAR_ID_MIN) return;
    if (update.attitudeValid) m_hasExtendedBitAttitude = true;
    if (!m_radarReference.locked() && update.position == GpsPositionFilter::Result::Accepted)
        emit filteredGeoLocationReported(m_radarReference.value(RadarReference::Latitude),
            m_radarReference.value(RadarReference::Longitude), m_radarReference.value(RadarReference::Altitude));
    if (update.positionChanged) publishRadarReference();
    else if (update.changed) emit radarReferenceChanged(); // Attitude-only changes must not replay position consumers.
}

void Controller::onBITReport(BITReport res) {
    const bool validRadarId = res.radarId >= RADAR_ID_MIN && res.radarId <= RADAR_ID_MAX;
    const std::size_t slot = validRadarId ? res.radarId - RADAR_ID_MIN : kBitLogSlots - 1;
    const qint64 nowMs = m_bitLogClock.elapsed();
    if (res.mesID != 0xDE02) {
        const auto decision = m_bitMessageLogs[slot].sample(nowMs, m_bitWarningLogIntervalMs);
        if (decision.write) {
            LOG_WARNING(QString("[Controller] BIT report mesID mismatch: 0x%1 radarId=%2 suppressed=%3")
                        .arg(res.mesID, 4, 16, QLatin1Char('0'))
                        .arg(res.radarId).arg(qulonglong(decision.suppressed)));
        }
    }
    if (!validRadarId) {
        const auto decision = m_bitRadarIdLogs[slot].sample(nowMs, m_bitWarningLogIntervalMs);
        if (decision.write) {
            LOG_WARNING(QString("[Controller] Invalid radarId=%1 in BIT report suppressed=%2")
                        .arg(res.radarId).arg(qulonglong(decision.suppressed)));
        }
    }
    if ((!m_multiPanelReference && res.yaw > 36000) || res.scanAngle > 36000) {
        const auto decision = m_bitAngleLogs[slot].sample(nowMs, m_bitWarningLogIntervalMs);
        if (decision.write) {
            LOG_WARNING(QString("[Controller] BIT angle exceeds protocol range: radarId=%1 yawRaw=%2 scanAngleRaw=%3 suppressed=%4")
                        .arg(res.radarId).arg(res.yaw).arg(res.scanAngle)
                        .arg(qulonglong(decision.suppressed)));
        }
    }

    // Normal BIT frames are never logged; only bounded validation/fault summaries remain.

    // qDebug() << "[Controller] onBITReport called - yaw:" << res.yaw
    //          << "scanAngle:" << res.scanAngle;

    // 转发BIT上报信号
    // Panel 1 defines platform yaw; other panels must not overwrite it every BIT frame.
    if (res.radarId == RADAR_ID_MIN && !m_hasExtendedBitAttitude
        && m_panelState.updateLegacyHeading(res.yaw * 0.01))
        emit radarReferenceChanged();
    m_healthStateWindow.pushBit(res);
    emit bitReport(res); // Real-time scan, attitude and non-health protocol consumers stay unchanged.

    //qDebug() << "[Controller] bitReport signal emitted";

    // 解析扫描角度（量化单位0.01度）
    double scanAngle = res.scanAngle * 0.01;  // 转换为度

    // 发送扫描角度变化信号，用于更新显控扫描线位置
    emit scanAngleChanged(scanAngle);
}
