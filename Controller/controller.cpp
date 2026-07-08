/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-21 17:53:15
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
#include "MarineRadarManager.h"
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
    LOG_INFO("[Controller::init] ship-radar init start");

    // === 创建子系统管理器实例 ===
    // === 船用雷达管理器 ===
    m_marineMgr = new MarineRadarManager(this);
    {
        QString localIp  = CF_INS.marineNetworkStr("local_ip", "192.168.1.100");
        int echoPort     = CF_INS.marineNetworkInt("echo_port", 9000);
        QString servoIp  = CF_INS.marineNetworkStr("servo_ip", "192.168.1.30");
        int servoPort    = CF_INS.marineNetworkInt("servo_port", 9000);
        int autoSendMs   = CF_INS.marineNetworkInt("auto_send_ms", 0);

        LOG_INFO(QString("[Controller::init] marine network local=%1:%2 servo=%3:%4 autoSendMs=%5")
                 .arg(localIp).arg(echoPort).arg(servoIp).arg(servoPort).arg(autoSendMs));

        m_marineMgr->init(localIp, static_cast<uint16_t>(echoPort),
                          servoIp, static_cast<uint16_t>(servoPort));

        // 加载默认控制参数
        MarineControlFrame ctrl;
        ctrl.cmdNum   = static_cast<uint8_t>(qBound(0, CF_INS.marineControl("cmd_num", MarineCmdParamsOnly),
                                                    static_cast<int>(MarineCmdStart)));
        ctrl.azimuth  = marineEncodeAzimuth(CF_INS.marineControlDouble("azimuth_deg", 0.0));
        ctrl.rangeVal = static_cast<uint8_t>(CF_INS.marineControl("range", 7));
        ctrl.gain     = static_cast<uint8_t>(CF_INS.marineControl("gain", 0));
        ctrl.ganRao   = static_cast<uint8_t>(CF_INS.marineControl("interference", 0));
        ctrl.level    = static_cast<uint8_t>(CF_INS.marineControl("level", 0));
        ctrl.seaVal   = static_cast<uint8_t>(CF_INS.marineControl("sea_clutter", 0));
        ctrl.rainVal  = static_cast<uint8_t>(CF_INS.marineControl("rain_clutter", 0));
        ctrl.txCtrl   = CF_INS.marineControlBool("tx_on", false) ? 1 : 0;
        ctrl.servo    = static_cast<uint8_t>(qBound(0,
                                                    CF_INS.marineControl("servo_gear",
                                                                         CF_INS.marineControl("servo_speed", 0)),
                                                    static_cast<int>(MARINE_SERVO_MAX_GEAR)));
        m_marineMgr->setCurrentControl(ctrl);
        m_marineMgr->setAutoSendInterval(autoSendMs);
        // Initial frame is cached; auto-send uses it when the timer fires.
    }

    // 船用雷达信号转发
    connect(m_marineMgr, &MarineRadarManager::echoLineReceived,
            this, &Controller::marineEchoLine);
    connect(m_marineMgr, &MarineRadarManager::radarStatusUpdated,
            this, &Controller::marineStatusUpdated);

    // === 建立信号槽连接 ===
}

/**
 * @brief 析构函数
 * @details 清理资源，Qt的父子对象关系会自动清理子对象
 */
Controller::~Controller() {
    // Qt对象树会自动清理子对象，无需手动删除
}

bool Controller::sendExternalSystemControl(const QByteArray& frame512) {
    Q_UNUSED(frame512);
    return false;
}

bool Controller::sendExternalServoControl(const QByteArray& frame32) {
    Q_UNUSED(frame32);
    return false;
}

void Controller::logMarineRxSnapshot(const QString& reason) const {
    if (m_marineMgr) {
        m_marineMgr->logRxSnapshot(reason);
    }
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
