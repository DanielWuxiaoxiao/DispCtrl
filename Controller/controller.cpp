/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-07 11:18:02
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
#include "disp2datamanager.h"
#include "targetdispmanager.h"
#include "disp2monmanager.h"
#include "mon2dispmanager.h"
#include "ExternalCtrlManager.h"
#include "MarineRadarManager.h"

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
    : QObject(parent) {
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
    // === 创建子系统管理器实例 ===
    resMgr = new Disp2ResManager(this);        // 显示到资源管理器
    resRecvMgr = new Res2DispManager(this);    // 资源到显示管理器
    sigMgr = new Disp2SigManager(this);        // 显示到信号管理器
    photoMgr = new Disp2PhotoManager(this);    // 显示到光电管理器
    sigRecvMgr = new sig2dispmanager(this);    // 信号到显示管理器
    sigRecvMgr2 = new sig2dispmanager2(this);  // 信号到显示管理器2
    dataRecvMgr = new Data2DispManager(this);  // 数据到显示管理器
    tbdRecvMgr = new Tbd2DispManager(this);    // TBD数据到显示管理器
    dataMgr = new Disp2DataManager(this);      // 显示到数据管理器
    tarMgr = new targetDispManager(this);      // 目标显示管理器
    monMgr = new Disp2MonManager(this);        // 显示到监控管理器
    monRecvMgr = new Mon2DispManager(this);    // 监控到显示管理器
    extCtrlMgr = new ExternalCtrlManager(this); // 外部雷控链路

    // === 船用雷达管理器 ===
    m_marineMgr = new MarineRadarManager(this);
    {
        QString localIp  = CF_INS.marineNetworkStr("local_ip", "192.168.1.100");
        int echoPort     = CF_INS.marineNetworkInt("echo_port", 5000);
        QString servoIp  = CF_INS.marineNetworkStr("servo_ip", "192.168.1.200");
        int servoPort    = CF_INS.marineNetworkInt("servo_port", 5001);
        int autoSendMs   = CF_INS.marineNetworkInt("auto_send_ms", 200);

        m_marineMgr->init(localIp, static_cast<uint16_t>(echoPort),
                          servoIp, static_cast<uint16_t>(servoPort));
        m_marineMgr->setAutoSendInterval(autoSendMs);

        // 加载默认控制参数
        MarineControlFrame ctrl;
        ctrl.rangeVal = static_cast<uint8_t>(CF_INS.marineControl("range", 7));
        ctrl.gain     = static_cast<uint8_t>(CF_INS.marineControl("gain", 0));
        ctrl.ganRao   = static_cast<uint8_t>(CF_INS.marineControl("interference", 0));
        ctrl.level    = static_cast<uint8_t>(CF_INS.marineControl("level", 0));
        ctrl.seaVal   = static_cast<uint8_t>(CF_INS.marineControl("sea_clutter", 0));
        ctrl.rainVal  = static_cast<uint8_t>(CF_INS.marineControl("rain_clutter", 0));
        ctrl.txCtrl   = CF_INS.marineControlBool("tx_on", false) ? 1 : 0;
        ctrl.servo    = static_cast<uint8_t>(CF_INS.marineControl("servo_speed", 0));
        // 初始控制帧不立即发送，等UI准备好
    }

    // 船用雷达信号转发
    connect(m_marineMgr, &MarineRadarManager::echoLineReceived,
            this, &Controller::marineEchoLine);
    connect(m_marineMgr, &MarineRadarManager::radarStatusUpdated,
            this, &Controller::marineStatusUpdated);

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
    connect(this, &Controller::sendPEParam, photoMgr, &Disp2PhotoManager::sendPEParam);
    connect(this, &Controller::sendPEParam2, photoMgr, &Disp2PhotoManager::sendPEParam2);

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
