/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:56
 * @Description: 
 */
/**
 * @file disp2resmanager.cpp
 * @brief 显示到资源管理器实现
 * @details 实现显示系统到雷达资源系统的UDP通信和参数传输
 * 
 * 实现功能：
 * - UDP网络通信的初始化和管理
 * - 各类控制参数的序列化和发送
 * - 协议帧的封装和打包
 * - 线程化的异步网络通信
 * - 通信计数和错误处理
 * 
 * 通信协议：
 * - 基于UDP的可靠传输协议
 * - 标准化的协议帧格式
 * - 源和目标ID的标识管理
 * - 通信序号的自动递增
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#include "disp2resmanager.h"
#include "UDP/threadudpsocket.h"
#include <QThread>
#include "controller.h"

/**
 * @brief 构造函数
 * @param parent 父对象指针
 * @details 初始化UDP通信组件，包括：
 *          - 从配置管理器获取网络参数
 *          - 创建线程化UDP套接字
 *          - 设置源和目标ID
 *          - 启动独立的网络通信线程
 *          - 初始化通信计数器
 */
Disp2ResManager::Disp2ResManager(QObject* parent)
    : QObject(parent) {
    // 从配置管理器获取系统ID配置
    src = CF_INS.id("DISP_CTRL_ID", DISP_CTRL_ID);        // 显示控制系统ID
    dst = CF_INS.id("RES_DIS_ID", RES_DIS_ID);            // 资源显示系统ID
    
    // 创建UDP套接字，配置本地IP和端口
    socket = new ThreadedUdpSocket(
        CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP),          // 本地IP地址
        CF_INS.port("DISP_2_RES_PORT", DISP_2_RES_PORT)   // 本地发送端口
    );
    
    // 设置通信双方的系统标识
    socket->setSourceAndDestID(src, dst);

    // 创建独立的网络通信线程
    thread = new QThread(this);
    socket->moveToThread(thread);
    
    // 建立线程生命周期信号连接
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    
    // 初始化通信序号计数器
    commCount = 1;

    // 配置目标主机的网络地址
    host = QHostAddress(CF_INS.ip("RES_DIS_IP", RES_DIS_IP));     // 资源系统IP
    port = CF_INS.port("RES_GET_DISP_PORT", RES_GET_DISP_PORT);   // 资源系统接收端口
}

/**
 * @brief 发送通用参数
 * @param paramData 参数数据指针
 * @param paramSize 参数数据大小（字节）
 * @details 通用的参数发送实现：
 *          - 封装数据为标准协议帧格式
 *          - 添加协议头和尾部校验
 *          - 更新通信序号
 *          - 异步发送到目标主机
 *          - 释放临时内存资源
 */
void Disp2ResManager::sendParam(char* paramData, unsigned paramSize)
{
    // 封装数据为标准协议帧（包含头部、数据、尾部）
    auto data = packData(paramData, paramSize, src, dst, commCount);
    commCount++;  // 通信序号递增
    
    // 转换为Qt字节数组进行网络传输
    QByteArray byteArray = QByteArray::fromRawData(
        data, 
        paramSize + sizeof(ProtocolFrame) + sizeof(ProtocolEnd)
    );
    
    // 异步发送数据到目标主机
    socket->writeData(byteArray, host, port);
    
    // 释放协议帧内存
    free(data);
}

/**
 * @brief 发送电池控制参数
 * @param param 电池控制参数结构
 * @details 将电池控制参数转换为字节流并发送
 */
void Disp2ResManager::sendBCParam(BatteryControlM param)
{
    sendParam(reinterpret_cast<char *>(&param), sizeof(param));
}

/**
 * @brief 发送收发器控制参数
 * @param param 收发器控制参数结构
 * @details 将收发器控制参数转换为字节流并发送
 */
void Disp2ResManager::sendTRParam(TranRecControl param)
{
    sendParam(reinterpret_cast<char *>(&param), sizeof(param));
}

void Disp2ResManager::sendFCParam(DirGramScan param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendSRParam(ScanRange param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendWCParam(BeamControl param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendSPParam(SigProParam param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

void Disp2ResManager::sendDPParam(DataProParam param)
{
    sendParam(reinterpret_cast<char *>(&param),sizeof(param));
}

Disp2ResManager::~Disp2ResManager() {
    thread->quit();
    thread->wait();
}
