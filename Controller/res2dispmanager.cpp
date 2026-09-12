/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:49
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-26
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-26
 * @Description: 资源管理到显控管理器实现
 */
#include "res2dispmanager.h"
#include "Basic/log.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include <QThread>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 * @details 初始化UDP接收组件，监听来自资源管理系统的消息
 */
Res2DispManager::Res2DispManager(QObject *parent) : QObject(parent)
{
    // 从配置管理器获取系统ID配置
    src = CF_INS.id("RES_DIS_ID", RES_DIS_ID);            // 资源管理系统ID
    dst = CF_INS.id("DISP_CTRL_ID", DISP_CTRL_ID);        // 显示控制系统ID

    // 创建UDP套接字，监听资源管理系统消息
    // 根据Wireshark实际抓包，使用8008端口
    socket = new ThreadedUdpSocket(
        CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP),          // 本地IP地址
        CF_INS.port("DATA_GET_DISP", 8008)                // 监听端口8008（实际端口）
    );

    // 设置通信双方的系统标识
    socket->setSourceAndDestID(src, dst);

    // 创建独立的网络通信线程
    thread = new QThread(this);
    socket->moveToThread(thread);

    // 建立线程生命周期信号连接
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);

    // 连接UDP接收信号到Controller
    bool conn1 = connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    bool conn2 = connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);

    LOG_DEBUG(QString("[Res2DispManager] Signal connections - servoCtrlRet: %1 bitReport: %2")
                  .arg(conn1)
                  .arg(conn2));

    thread->start();

    // 配置资源管理系统的网络地址
    host = QHostAddress(CF_INS.ip("RES_DIS_IP", RES_DIS_IP));     // 资源系统IP
    port = CF_INS.port("DATA_PRO_2_DISP", 6008);                   // 资源系统发送端口（实际端口）

    LOG_INFO("[Res2DispManager] 已启动，监听端口8008，接收来自192.168.64.3:6008的消息");
}

/**
 * @brief 析构函数
 */
Res2DispManager::~Res2DispManager()
{
    if (thread) {
        thread->quit();
        thread->wait();
    }
}
