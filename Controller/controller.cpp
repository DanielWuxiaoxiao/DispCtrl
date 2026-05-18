/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:18
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
#include "Basic/log.h"

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
    // 成员指针初始化在init()中进行
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

    // === 建立信号槽连接 ===

    // 向资源系统发送控制参数
    connect(this, &Controller::sendBCParam, resMgr, &Disp2ResManager::sendBCParam);
    connect(this, &Controller::sendTRParam, resMgr, &Disp2ResManager::sendTRParam);
    connect(this, &Controller::sendServoControl, resMgr, &Disp2ResManager::sendServoControl);
    connect(this, &Controller::sendFCParam, resMgr, &Disp2ResManager::sendFCParam);
    connect(this, &Controller::sendSRParam, resMgr, &Disp2ResManager::sendSRParam);
    connect(this, &Controller::sendWCParam, resMgr, &Disp2ResManager::sendWCParam);
    connect(this, &Controller::sendSPParam, resMgr, &Disp2ResManager::sendSPParam);
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
    emit scanHeadingChanged(headingDeg);
}

void Controller::onBITReport(BITReport res) {
    // qDebug() << "[Controller] onBITReport called - yaw:" << res.yaw
    //          << "scanAngle:" << res.scanAngle;

    // 转发BIT上报信号
    emit bitReport(res);

    //qDebug() << "[Controller] bitReport signal emitted";

    // 解析扫描角度（量化单位0.01度）
    double scanAngle = res.scanAngle * 0.01;  // 转换为度

    // 发送扫描角度变化信号，用于更新显控扫描线位置
    emit scanAngleChanged(scanAngle);
}
