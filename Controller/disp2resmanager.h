/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:56
 * @Description: 
 */
/**
 * @file disp2resmanager.h
 * @brief 显示到资源管理器头文件
 * @details 负责将显示系统的控制参数发送到雷达资源系统
 * 
 * 主要功能：
 * - 电池控制参数传输
 * - 收发器控制参数传输
 * - 频率控制参数传输
 * - 扫描范围参数传输
 * - 波束控制参数传输
 * - 信号处理参数传输
 * - 数据处理参数传输
 * 
 * 设计特性：
 * - 基于UDP的可靠参数传输
 * - 线程化的网络通信
 * - 统一的参数发送接口
 * - 类型安全的参数封装
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef DISP2RESMANAGER_H
#define DISP2RESMANAGER_H

#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

// 前向声明
class ThreadedUdpSocket;  ///< 线程化UDP套接字
class QThread;            ///< Qt线程类

/**
 * @class Disp2ResManager
 * @brief 显示到资源管理器
 * @details 负责将显示控制系统的各类参数通过UDP网络发送到雷达资源控制系统
 * 
 * 该类封装了：
 * - 雷达硬件控制参数的网络传输
 * - 各类控制指令的统一发送接口
 * - 基于UDP协议的可靠通信机制
 * - 线程安全的参数发送操作
 * 
 * 支持的控制参数类型：
 * - 电池控制参数：电源管理和功率控制
 * - 收发器控制：射频发射和接收控制
 * - 频率控制：工作频率和频谱管理
 * - 扫描控制：扫描范围和模式设置
 * - 波束控制：天线波束形成和指向
 * - 信号处理：信号处理算法参数
 * - 数据处理：数据处理和滤波参数
 */
class Disp2ResManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     * @details 初始化UDP通信组件和网络连接
     */
    explicit Disp2ResManager(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 清理网络资源和线程
     */
    ~Disp2ResManager();

public slots:
    /**
     * @brief 发送电池控制参数
     * @param param 电池控制参数结构
     * @details 发送电池管理和功率控制参数到资源系统
     */
    void sendBCParam(BatteryControlM param);
    
    /**
     * @brief 发送收发器控制参数
     * @param param 收发器控制参数结构
     * @details 发送射频收发器的控制参数
     */
    void sendTRParam(TranRecControl param);
    
    /**
     * @brief 发送频率控制参数
     * @param param 方向图扫描参数结构
     * @details 发送频率控制和扫描参数
     */
    void sendFCParam(DirGramScan param);
    
    /**
     * @brief 发送扫描范围参数
     * @param param 扫描范围参数结构
     * @details 发送扫描范围和角度设置
     */
    void sendSRParam(ScanRange param);
    
    /**
     * @brief 发送波束控制参数
     * @param param 波束控制参数结构
     * @details 发送天线波束形成和指向控制参数
     */
    void sendWCParam(BeamControl param);
    
    /**
     * @brief 发送信号处理参数
     * @param param 信号处理参数结构
     * @details 发送信号处理算法和滤波参数
     */
    void sendSPParam(SigProParam param);
    
    /**
     * @brief 发送数据处理参数
     * @param param 数据处理参数结构
     * @details 发送数据处理和目标检测参数
     */
    void sendDPParam(DataProParam param);
    
    /**
     * @brief 发送通用参数
     * @param paramData 参数数据指针
     * @param paramSize 参数数据大小
     * @details 通用的参数发送接口，支持自定义格式参数
     */
    void sendParam(char* paramData, unsigned paramSize);

private:
    ThreadedUdpSocket* socket;  ///< UDP通信套接字
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // DISP2RESMANAGER_H
