/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
/**
 * @file controller.h
 * @brief 系统主控制器
 * @details 雷达显示控制系统的核心控制器，负责协调各个子系统的数据流和消息传递
 * 
 * 功能架构：
 * - 集中管理各个子系统的数据交互
 * - 统一的信号处理和消息路由
 * - 系统参数配置和状态管理
 * - 多模块间的协调控制
 * - 数据流向的统一调度
 * 
 * 子系统管理：
 * - 资源控制管理 (Disp2ResManager)
 * - 信号处理管理 (Disp2SigManager)
 * - 数据处理管理 (Data2DispManager)
 * - 光电设备管理 (Disp2PhotoManager)
 * - 目标分析管理 (targetDispManager)
 * - 监控系统管理 (Disp2MonManager)
 * 
 * 设计模式：
 * - 单例模式：确保系统唯一控制入口
 * - 中介者模式：协调各子系统交互
 * - 观察者模式：基于信号槽的事件驱动
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H
// Controller.h
#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include "Basic/ConfigManager.h"

// 前向声明 - 各子系统管理器
class Disp2ResManager;    ///< 显示到资源管理器
class Disp2SigManager;    ///< 显示到信号管理器
class sig2dispmanager;    ///< 信号到显示管理器
class sig2dispmanager2;   ///< 信号到显示管理器2
class Data2DispManager;   ///< 数据到显示管理器
class Disp2DataManager;   ///< 显示到数据管理器
class Disp2PhotoManager;  ///< 显示到光电管理器
class targetDispManager;  ///< 目标显示管理器
class Disp2MonManager;    ///< 显示到监控管理器
class Mon2DispManager;    ///< 监控到显示管理器

// 便捷宏定义
#define CON_INS Controller::getInstance()  ///< 控制器单例访问宏
#define CF_INS ConfigManager::instance()   ///< 配置管理器单例访问宏

/**
 * @class Controller
 * @brief 系统主控制器
 * @details 雷达显示控制系统的核心调度中心，采用单例模式管理整个系统
 * 
 * 该类作为系统的中央控制器：
 * - 统一管理所有子系统的生命周期
 * - 协调各模块间的数据流和消息传递
 * - 提供系统级的参数配置和状态控制
 * - 实现模块间的解耦和松散连接
 * - 集中处理系统级的错误和异常
 * 
 * 数据流管理：
 * - 向下游发送控制参数和指令
 * - 向上游接收状态信息和数据
 * - 跨模块的信息路由和转发
 * - 系统状态的统一监控和管理
 * 
 * @example
 * ```cpp
 * // 获取控制器实例
 * Controller* controller = Controller::getInstance();
 * 
 * // 初始化系统
 * controller->init();
 * 
 * // 连接信号处理
 * connect(controller, &Controller::detInfoProcess,
 *         this, &MainWindow::onDetectionInfo);
 * 
 * // 发送控制参数
 * BatteryControlM param;
 * // 设置参数...
 * controller->sendBCParam(param);
 * ```
 */
class Controller : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     * @details 创建控制器实例，初始化基本成员变量
     */
    explicit Controller(QObject *parent = nullptr);
    
    /**
     * @brief 获取单例实例
     * @return 控制器单例指针
     * @details 线程安全的单例模式实现，确保全局唯一控制器
     */
    static Controller* getInstance();
    
    /**
     * @brief 析构函数
     * @details 清理资源，释放所有子系统管理器
     */
    ~Controller();
    
    /**
     * @brief 初始化系统
     * @details 创建并初始化所有子系统管理器，建立信号槽连接
     */
    void init();

signals:
    // === 向资源系统发送控制参数 ===
    
    /**
     * @brief 发送电池控制参数
     * @param param 电池控制参数结构
     */
    void sendBCParam(BatteryControlM param);
    
    /**
     * @brief 发送收发控制参数
     * @param param 收发器控制参数结构
     */
    void sendTRParam(TranRecControl param);
    
    /**
     * @brief 发送频率控制参数
     * @param param 方向图扫描参数结构
     */
    void sendFCParam(DirGramScan param);
    
    /**
     * @brief 发送扫描范围参数
     * @param param 扫描范围参数结构
     */
    void sendSRParam(ScanRange param);
    
    /**
     * @brief 发送波束控制参数
     * @param param 波束控制参数结构
     */
    void sendWCParam(BeamControl param);
    
    /**
     * @brief 发送信号处理参数
     * @param param 信号处理参数结构
     */
    void sendSPParam(SigProParam param);
    
    /**
     * @brief 发送数据处理参数
     * @param param 数据处理参数结构
     */
    void sendDPParam(DataProParam param);
    
    /**
     * @brief 发送光电参数设置
     * @param param 光电参数设置结构
     */
    void sendPEParam(PhotoElectricParamSet param);
    
    /**
     * @brief 发送光电参数设置2
     * @param param 光电参数设置2结构
     */
    void sendPEParam2(PhotoElectricParamSet2 param);

    // === 向信号系统发送参数 ===
    
    /**
     * @brief 发送数据设置参数
     * @param param 数据设置参数结构
     */
    void sendDSParam(DataSet param);

    // === 从信号系统接收状态 ===
    
    /**
     * @brief 数据保存完成信号
     * @param datasaveok 数据保存完成状态
     */
    void dataSaveOK(DataSaveOK datasaveok);
    
    /**
     * @brief 数据删除完成信号
     * @param datadelok 数据删除完成状态
     */
    void dataDelOK(DataDelOK datadelok);
    
    /**
     * @brief 离线状态信号
     * @param offlinestat 离线状态信息
     */
    void offLineStat(OfflineStat offlinestat);

    // === 目标信息处理 ===
    
    /**
     * @brief 检测点信息处理信号
     * @param info 检测点信息结构
     */
    void detInfoProcess(PointInfo info);
    
    /**
     * @brief 航迹信息处理信号
     * @param info 航迹点信息结构
     */
    void traInfoProcess(PointInfo info);

    // === 向数据系统发送控制 ===
    
    /**
     * @brief 设置手动航迹
     * @param data 手动航迹设置数据
     */
    void setManual(SetTrackManual data);

    // === 向监控系统发送参数 ===
    
    /**
     * @brief 发送系统启动参数
     * @param data 系统启动参数
     */
    void sendSysStart(StartSysParam data);

    // === 从目标系统接收结果 ===
    
    /**
     * @brief 目标分类结果信号
     * @param res 目标分类结果
     */
    void targetClaRes(TargetClaRes res);

    // === 从监控系统接收参数 ===
    
    /**
     * @brief 监控参数发送信号
     * @param res 监控参数结构
     */
    void monitorParamSend(MonitorParam res);
    
    /**
     * @brief 最小化窗口信号
     * @param checked 是否选中状态
     */
    void minimizeWindow(bool checked = false);

private:
    // === 子系统管理器实例 ===
    Disp2ResManager* resMgr;       ///< 显示到资源管理器
    Disp2SigManager* sigMgr;       ///< 显示到信号管理器
    Disp2PhotoManager* photoMgr;   ///< 显示到光电管理器
    sig2dispmanager* sigRecvMgr;   ///< 信号到显示管理器
    sig2dispmanager2* sigRecvMgr2; ///< 信号到显示管理器2
    Data2DispManager* dataRecvMgr; ///< 数据到显示管理器
    Disp2DataManager* dataMgr;     ///< 显示到数据管理器
    targetDispManager* tarMgr;     ///< 目标显示管理器
    Disp2MonManager* monMgr;       ///< 显示到监控管理器
    Mon2DispManager* monRecvMgr;   ///< 监控到显示管理器
};

#endif // CONTROLLER_H
