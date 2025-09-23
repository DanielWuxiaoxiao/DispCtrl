/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:15
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
     * @brief 报告错误到错误处理框架
     * @param code 错误代码
     * @param message 错误消息
     * @details 集成ErrorHandler框架，统一错误处理和记录
     */
    void reportError(const QString& code, const QString& message);
};

#endif // THREADUDPSOCKET_H
