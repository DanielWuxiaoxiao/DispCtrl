/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:13
 * @Description: 
 */
/**
 * @file threadudpsocket.h
 * @brief 多线程UDP Socket通信模块头文件
 * @details 提供雷达数据网络通信的核心功能：
 *          - 异步UDP数据收发
 *          - 自动重连和错误恢复机制
 *          - 雷达协议数据解析和分发
 *          - 连接状态监控和错误处理
 *          - 线程安全的网络操作
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef THREADUDPSOCKET_H
#define THREADUDPSOCKET_H

#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QTimer>
#include "Basic/Protocol.h"

/**
 * @class ThreadedUdpSocket
 * @brief 多线程UDP Socket通信类
 * @details 雷达数据网络通信的核心类，提供以下功能：
 *          - UDP数据包的异步收发
 *          - 雷达协议帧的解析和验证
 *          - 自动重连机制（最多5次重试）
 *          - 错误检测和状态监控
 *          - 数据分类和信号分发
 *
 * @example 基本使用方式：
 * @code
 * ThreadedUdpSocket* socket = new ThreadedUdpSocket("192.168.1.100", 8080);
 * socket->setSourceAndDestID(1, 2);
 * connect(socket, &ThreadedUdpSocket::detInfo, this, &MyClass::handleDetection);
 * socket->start();
 * @endcode
 */
class ThreadedUdpSocket : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param ip 目标IP地址
     * @param port 目标端口号
     * @param parent 父对象指针
     * @details 初始化UDP Socket参数，设置错误处理和重连机制
     */
    ThreadedUdpSocket(QString ip, quint16 port, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     * @details 清理UDP Socket资源，关闭连接
     */
    ~ThreadedUdpSocket();

    /**
     * @brief 设置源和目标ID
     * @param src 源设备ID
     * @param dst 目标设备ID
     * @details 用于雷达协议中的设备标识，确保数据路由正确
     */
    void setSourceAndDestID(quint16 src, quint16 dst);

    /**
     * @brief 发送UDP数据
     * @param datagram 要发送的数据包
     * @param host 目标主机地址
     * @param port 目标端口
     * @details 异步发送UDP数据包，支持错误检测和重试机制
     */
    void writeData(const QByteArray &datagram, const QHostAddress &host, quint16 port);

    /**
     * @brief 启用心跳机制
     * @details 启动定时心跳包发送，用于保持与光电系统的连接
     *          心跳间隔默认为5秒
     */
    void enableHeartBeat();

    /**
     * @brief 发送光电参数（经纬高模式）
     * @param param 光电参数结构体（包含经纬高和速度信息）
     * @details 向光电系统发送目标引导信息
     */
    void sendPEParam(PhotoElectricParamSet param);

    /**
     * @brief 发送光电参数（方位俯仰模式）
     * @param param 光电参数结构体（包含方位角、俯仰角和距离信息）
     * @details 向光电系统发送目标引导信息（使用极坐标）
     */
    void sendPEParam2(PhotoElectricParamSet2 param);

    /**
     * @brief 发送阵地控制参数
     * @param param 阵地控制参数
     * @details 控制雷达阵地的开关状态
     */
    void sendBCParam(BatteryControlM param);

    /**
     * @brief 发送收发控制参数
     * @param param 收发控制参数
     * @details 控制雷达的收发状态
     */
    void sendTRParam(TranRecControl param);

    /**
     * @brief 发送伺服控制参数
     * @param param 伺服控制（0xAA03 指令/速度/方位角）
     */
    void sendServoControl(ServoControlParam param);

    /**
     * @brief 发送频率控制参数
     * @param param 方向图扫描参数
     * @details 控制雷达的频率和扫描参数
     */
    void sendFCParam(DirGramScan param);

    /**
     * @brief 发送扫描范围参数
     * @param param 扫描范围参数
     * @details 设置雷达的扫描范围和工作方式
     */
    void sendSRParam(ScanRange param);

    /**
     * @brief 发送波控参数
     * @param param 波束控制参数
     * @details 控制雷达波束的参数
     */
    void sendWCParam(BeamControl param);

    /**
     * @brief 发送信号处理参数
     * @param param 信号处理参数
     * @details 配置信号处理算法参数
     */
    void sendSPParam(SigProParam param);

    /**
     * @brief 发送数据处理参数
     * @param param 数据处理参数
     * @details 配置数据处理算法参数
     */
    void sendDPParam(DataProParam param);

    /**
     * @brief 发送数据存储设置
     * @param param 数据存储设置参数
     * @details 配置数据的保存、删除和离线处理
     */
    void sendDSParam(DataSet param);

    /**
     * @brief 发送系统启动命令
     * @param data 系统启动参数
     * @details 向监控系统发送启动命令
     */
    void sendSysStart(StartSysParam data);

    /**
     * @brief 设置手动航迹
     * @param data 手动航迹参数
     * @details 手动设置目标航迹
     */
    void setManual(SetTrackManual data);

    /**
     * @brief 上报点迹信息
     * @param info 点迹信息（包含检测点或航迹）
     * @details 向数据处理系统上报用户选中的点迹信息，用于目标确认或跟踪
     */
    void reportPointInfo(PointInfo info);

signals:
    /**
     * @defgroup DataSignals 雷达数据信号
     * @brief 不同类型雷达数据的信号定义
     * @{
     */

    /// 检测点信息信号 - 包含原始雷达检测数据
    void detInfo(QByteArray);

    /// 航迹信息信号 - 包含处理后的目标航迹数据
    void traInfo(QByteArray);

    /// TBD 航迹信息信号
    void tbdInfo(QByteArray);

    /// 数据保存确认信号 - 数据存储操作的确认
    void dataSaveOK(DataSaveOK);

    /// 数据删除确认信号 - 数据删除操作的确认
    void dataDelOK(DataDelOK);

    /// 离线状态信号 - 设备离线状态通知
    void offLineStat(OfflineStat);

    /// 目标分类结果信号 - 目标识别和分类结果
    void targetClaRes(TargetClaRes);

    /// 监控参数信号 - 系统监控参数数据
    void monitorParamSend(MonitorParam);

    /// 伺服回送信号
    void servoCtrlRet(ServoCtrlRet);

    /// BIT 上报信号
    void bitReport(BITReport);

    /** @} */ // end of DataSignals group

    /**
     * @defgroup ErrorSignals 错误处理信号
     * @brief 网络错误和状态变化信号
     * @{
     */

    /// Socket错误信号 - 网络错误详细信息
    void socketError(const QString& error);

    /// 连接状态变化信号 - 连接建立/断开通知
    void connectionStatusChanged(bool connected);

    /** @} */ // end of ErrorSignals group

public slots:
    /**
     * @brief 启动UDP Socket服务
     * @details 绑定指定端口，开始监听UDP数据包
     *          如果绑定失败，会自动触发重连机制
     */
    void start();

private slots:
    /**
     * @brief 处理接收到的UDP数据
     * @details 当Socket接收到数据时自动调用：
     *          1. 读取所有可用数据包
     *          2. 验证数据帧格式
     *          3. 解析协议类型
     *          4. 分发到相应的信号
     */
    void onReadyRead();

    /**
     * @brief 处理Socket错误
     * @param socketError Qt Socket错误类型
     * @details 处理各种网络错误：
     *          - 绑定失败
     *          - 网络不可达
     *          - 端口被占用
     *          - 自动触发重连机制
     */
    void onSocketError(QAbstractSocket::SocketError socketError);

    /**
     * @brief 处理Socket状态变化
     * @param socketState Qt Socket状态
     * @details 监控Socket连接状态变化，更新内部状态
     */
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

private:
    // 网络配置参数
    QString m_Ip;                    ///< 目标IP地址
    quint16 m_Port;                  ///< 目标端口号
    QUdpSocket* m_socket = nullptr;  ///< UDP Socket对象指针
    quint16 srcID;                   ///< 源设备ID
    quint16 destID;                  ///< 目标设备ID
    quint32 commCount = 1;           ///< 通信计数器

    // 错误处理和重连机制
    QTimer* m_reconnectTimer;                    ///< 重连定时器
    int m_reconnectAttempts;                     ///< 当前重连尝试次数
    static const int MAX_RECONNECT_ATTEMPTS = 5; ///< 最大重连尝试次数
    static const int RECONNECT_INTERVAL_MS = 3000; ///< 重连间隔时间(毫秒)

    // 心跳机制
    QTimer* heartbeatTimer = nullptr;            ///< 心跳定时器
    QByteArray heartbeatPacket;                  ///< 心跳数据包缓存
    static const int HEARTBEAT_INTERVAL = 5000;  ///< 心跳间隔时间(毫秒)

    /**
     * @brief 发送心跳包
     * @details 定时器触发，向光电系统发送心跳包
     */
    void sendHeartbeat();

    /**
     * @brief 计算校验码
     * @param data 待计算的数据
     * @param len 数据长度
     * @return 校验码
     * @details 计算光电协议的异或校验码
     */
    char checkAccusation(const char *data, int len);

    /**
     * @brief 处理接收到的数据报
     * @param datagram 数据报内容
     * @param senderPort 发送方端口
     * @details 解析雷达协议数据，根据帧类型分发到相应信号
     */
    void handleDatagram(const QByteArray& datagram, int senderPort);

    /**
     * @brief 验证数据帧格式
     * @param data 待验证的数据
     * @return true表示数据帧格式正确
     * @details 检查帧头、帧尾、长度等协议要素
     */
    bool validateFrame(const QByteArray& data);

    /**
     * @brief 尝试重新连接
     * @details 使用指数退避算法进行重连：
     *          - 检查重连次数限制
     *          - 递增延迟时间
     *          - 记录重连状态和错误
     */
    void attemptReconnect();

    /**
     * @brief 安全发送数据报（内部使用）
     * @param data 要发送的数据
     * @param size 数据大小
     * @param host 目标主机地址
     * @param port 目标端口
     * @return true表示发送成功，false表示失败
     * @details 在发送前检查socket状态，确保socket已绑定
     */
    bool safeWriteDatagram(const char* data, qint64 size, const QHostAddress& host, quint16 port);

    /**
     * @brief 报告错误到错误处理框架
     * @param code 错误代码
     * @param message 错误消息
     * @details 集成ErrorHandler框架，统一错误处理和记录
     */
    void reportError(const QString& code, const QString& message);
};

#endif // THREADUDPSOCKET_H
