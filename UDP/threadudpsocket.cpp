/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 15:56:15
 * @Description: 
 */

/**
 * @file threadudpsocket.cpp
 * @brief 多线程UDP套接字实现文件
 * @details 实现了用于雷达显示控制系统的多线程UDP通信功能，支持异步数据收发、
 *          自动重连、错误处理和协议解析。该类负责处理与各个子系统间的UDP通信，
 *          包括检测点数据、航迹数据、目标分析结果等的接收和发送。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#include "threadudpsocket.h"
#include "Basic/Protocol.h"
#include "Controller/ErrorHandler.h"
#include <QNetworkDatagram>
#include <QDateTime>
#include <QDebug>
#include "Controller/controller.h"

/**
 * @brief ThreadedUdpSocket构造函数
 * @details 初始化UDP套接字对象，设置IP地址、端口号和重连参数。
 *          创建重连定时器并建立信号槽连接。
 * @param ip UDP套接字绑定的IP地址
 * @param port UDP套接字绑定的端口号
 * @param parent 父对象指针，用于Qt对象树管理
 *
 * 功能说明：
 * - 设置网络参数（IP地址和端口）
 * - 初始化重连计数器为0
 * - 创建单次触发的重连定时器
 * - 连接定时器超时信号到重连槽函数
 */
ThreadedUdpSocket::ThreadedUdpSocket(QString ip, quint16 port, QObject* parent)
    : QObject(parent), m_Ip(ip), m_Port(port), m_reconnectAttempts(0)
{
    // 初始化重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ThreadedUdpSocket::attemptReconnect);
}

/**
 * @brief 设置源ID和目标ID
 * @details 配置UDP通信中的源系统ID和目标系统ID，用于消息路由和系统识别
 * @param src 源系统ID（本系统的标识符）
 * @param dst 目标系统ID（通信对方的标识符）
 *
 * 使用场景：
 * - 在建立UDP连接前设置通信双方的系统标识
 * - 用于协议层的消息路由和验证
 */
void ThreadedUdpSocket::setSourceAndDestID(quint16 src, quint16 dst) {
    srcID = src;
    destID = dst;
}

/**
 * @brief 启动UDP套接字服务
 * @details 创建UDP套接字并绑定到指定端口，建立信号槽连接，启动数据接收服务。
 *          如果启动失败会自动尝试重连。
 *
 * 执行步骤：
 * 1. 清理现有套接字资源
 * 2. 创建新的QUdpSocket对象
 * 3. 连接错误处理、状态变化和数据就绪信号
 * 4. 绑定套接字到指定端口
 * 5. 重置重连计数器并发送连接状态信号
 *
 * 异常处理：
 * - 绑定失败时报告错误并尝试重连
 * - 捕获std::exception异常并记录
 *
 * @note 使用Qt5兼容的信号槽连接方式
 */
void ThreadedUdpSocket::start() {
    try {
        if (m_socket) {
            m_socket->close();
            delete m_socket;
        }

        m_socket = new QUdpSocket(this);

        // 连接错误处理信号 - Qt5兼容版本
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
                this, &ThreadedUdpSocket::onSocketError);
        connect(m_socket, &QUdpSocket::stateChanged,
                this, &ThreadedUdpSocket::onSocketStateChanged);
        connect(m_socket, &QUdpSocket::readyRead, this, &ThreadedUdpSocket::onReadyRead);

        if (!m_socket->bind(QHostAddress::Any, m_Port)) {
            reportError("UDP_BIND_FAILED",
                       QString("Failed to bind UDP socket to port %1: %2")
                       .arg(m_Port).arg(m_socket->errorString()));
            attemptReconnect();
            return;
        }

        m_reconnectAttempts = 0;
        emit connectionStatusChanged(true);
        qInfo() << "UDP socket successfully bound to port" << m_Port;

    } catch (const std::exception& e) {
        reportError("UDP_START_EXCEPTION",
                   QString("Exception in UDP start: %1").arg(e.what()));
    }
}

/**
 * @brief 数据就绪事件处理函数
 * @details 当UDP套接字接收到数据时触发，循环读取所有待处理的数据报。
 *          对每个有效的数据报调用handleDatagram进行协议解析。
 *
 * 处理流程：
 * 1. 检查套接字状态和数据可用性
 * 2. 循环接收所有待处理的数据报
 * 3. 验证数据报的有效性
 * 4. 提取发送方端口信息
 * 5. 调用数据报处理函数进行协议解析
 *
 * 异常处理：
 * - 捕获数据接收过程中的异常
 * - 记录无效数据报的错误信息
 *
 * @note 使用while循环确保处理所有待接收的数据
 */
void ThreadedUdpSocket::onReadyRead() {
    try {
        while (m_socket && m_socket->hasPendingDatagrams()) {
            QNetworkDatagram datagram = m_socket->receiveDatagram();
            if (datagram.isValid()) {
                handleDatagram(datagram.data(), datagram.senderPort());
            } else {
                reportError("UDP_INVALID_DATAGRAM", "Received invalid UDP datagram");
            }
        }
    } catch (const std::exception& e) {
        reportError("UDP_READ_EXCEPTION",
                   QString("Exception in UDP read: %1").arg(e.what()));
    }
}

/**
 * @brief UDP数据报处理函数
 * @details 解析接收到的UDP数据报，根据消息ID和端口号进行协议分发。
 *          支持多种雷达系统消息类型的处理和转发。
 * @param data 接收到的数据报内容
 * @param senderPort 发送方的端口号，用于验证消息来源
 *
 * 支持的消息类型：
 * - 0xDD01: 检测点信息 (来自信号处理系统)
 * - 0xEE01: 航迹信息 (来自数据处理系统)
 * - 0xDD02: 数据保存确认 (来自信号处理系统)
 * - 0xDD03: 数据删除确认 (来自信号处理系统)
 * - 0xDD04: 离线状态信息 (来自信号处理系统)
 * - 0xDB01: 目标分类结果 (来自目标分析系统)
 * - 0xDC01: 监控参数 (来自监控系统)
 *
 * 安全检查：
 * 1. 帧格式验证（头部和尾部校验）
 * 2. 数据长度检查
 * 3. 发送端口验证
 * 4. 接收端口匹配验证
 *
 * @note 使用配置管理器获取端口配置，确保端口匹配的准确性
 */
void ThreadedUdpSocket::handleDatagram(const QByteArray& data, int senderPort) {
    try {
        if (!validateFrame(data)) {
            reportError("UDP_INVALID_FRAME",
                       QString("Invalid frame received from port %1").arg(senderPort));
            return;
        }

        if (data.size() < sizeof(ProtocolFrame) + sizeof(quint16)) {
            reportError("UDP_INSUFFICIENT_DATA",
                       QString("Datagram too small: %1 bytes").arg(data.size()));
            return;
        }

        quint16 msgID = *reinterpret_cast<const quint16*>(data.constData() + sizeof(ProtocolFrame));
        const char* payload = data.constData() + sizeof(ProtocolFrame);

        switch (msgID) {
        case 0xDD01: {  // 检测点信息消息
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT1",SIG_2_DISP_PORT1) &&
               m_Port == CF_INS.port("DISP_GET_SIG_PORT1",DISP_GET_SIG_PORT1)) {
                emit detInfo(data);
            }
            break;
        }
        case 0xEE01: {  // 航迹信息消息
            if(senderPort == CF_INS.port("DATA_PRO_2_DISP",DATA_PRO_2_DISP) &&
               m_Port == CF_INS.port("DISP_GET_DATA_PORT",DISP_GET_DATA_PORT)) {
                emit traInfo(data);
            }
            break;
        }
        case 0xDD02: {  // 数据保存确认消息
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) &&
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                DataSaveOK info;
                memcpy(&info, payload, sizeof(info));
                emit dataSaveOK(info);
            }
            break;
        }
        case 0xDD03: {  // 数据删除确认消息
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) &&
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                DataDelOK info;
                memcpy(&info, payload, sizeof(info));
                emit dataDelOK(info);
            }
            break;
        }
        case 0xDD04: {  // 离线状态信息消息
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) &&
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                OfflineStat info;
                memcpy(&info, payload, sizeof(info));
                emit offLineStat(info);
            }
            break;
        }
        case 0xDB01: {  // 目标分类结果消息
            if(m_Port == CF_INS.port("DISP_GET_TARGET_PORT",DISP_GET_TARGET_PORT)) {
                TargetClaRes info;
                memcpy(&info, payload, sizeof(info));
                emit targetClaRes(info);
            }
            break;
        }
        case 0xDC01: {  // 监控参数消息
            if(m_Port == CF_INS.port("DISP_GET_MONITOR_PORT",DISP_GET_MONITOR_PORT)) {
                MonitorParam info;
                memcpy(&info, payload, sizeof(info));
                emit monitorParamSend(info);
            }
            break;
        }
        default:
            qDebug() << "Unknown message ID:" << QString::number(msgID, 16);
            break;
        }
    } catch (const std::exception& e) {
        reportError("UDP_HANDLE_EXCEPTION",
                   QString("Exception handling datagram: %1").arg(e.what()));
    }
}

/**
 * @brief 数据帧格式验证函数
 * @details 验证接收到的UDP数据帧是否符合协议规范，检查帧头和帧尾标识。
 * @param data 待验证的数据帧
 * @return bool 返回true表示帧格式正确，false表示格式错误
 *
 * 验证内容：
 * 1. 数据长度检查：必须包含完整的协议帧头和协议帧尾
 * 2. 帧头验证：检查HEADCODE标识是否正确
 * 3. 帧尾验证：检查ENDCODE标识是否正确
 *
 * @note 使用memcmp进行字节级别的精确比较
 */
bool ThreadedUdpSocket::validateFrame(const QByteArray& data)
{
    if (data.size() < sizeof(ProtocolFrame) + sizeof(ProtocolEnd)) {
        return false;
    }
    auto frame = reinterpret_cast<const ProtocolFrame*>(data.constData());
    auto end = reinterpret_cast<const ProtocolEnd*>(data.constData() + data.size() - sizeof(ProtocolEnd));
    return (memcmp(&HEADCODE, &frame->head, sizeof(HEADCODE)) == 0 &&
            memcmp(&ENDCODE, &end->end, sizeof(ENDCODE)) == 0);
}

/**
 * @brief UDP数据发送函数
 * @details 通过UDP套接字向指定的主机和端口发送数据报
 * @param datagram 要发送的数据内容
 * @param host 目标主机地址
 * @param port 目标端口号
 *
 * 执行流程：
 * 1. 检查套接字状态是否有效
 * 2. 调用QUdpSocket::writeDatagram发送数据
 * 3. 检查发送结果并处理错误
 *
 * 错误处理：
 * - 套接字为空时报告错误
 * - 发送失败时记录错误信息
 * - 捕获发送过程中的异常
 *
 * @note 函数是线程安全的，可在多线程环境中调用
 */
void ThreadedUdpSocket::writeData(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    try {
        if (!m_socket) {
            reportError("UDP_WRITE_NO_SOCKET", "Attempted to write data but socket is null");
            return;
        }

        qint64 bytesWritten = m_socket->writeDatagram(datagram, host, port);
        if (bytesWritten == -1) {
            reportError("UDP_WRITE_FAILED",
                       QString("Failed to write UDP datagram: %1").arg(m_socket->errorString()));
        }
    } catch (const std::exception& e) {
        reportError("UDP_WRITE_EXCEPTION",
                   QString("Exception writing UDP data: %1").arg(e.what()));
    }
}

/**
 * @brief UDP套接字错误事件处理函数
 * @details 当UDP套接字发生错误时触发，记录错误信息并根据错误类型决定是否重连
 * @param socketError 套接字错误类型枚举值
 *
 * 处理的错误类型：
 * - NetworkError: 网络错误，触发重连
 * - ConnectionRefusedError: 连接被拒绝，触发重连
 * - 其他错误: 仅记录错误信息
 *
 * 错误处理流程：
 * 1. 获取详细的错误描述信息
 * 2. 通过错误处理器报告错误
 * 3. 对严重错误启动自动重连机制
 *
 * @note 自动重连机制有最大尝试次数限制
 */
// 错误处理方法
void ThreadedUdpSocket::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorString = m_socket ? m_socket->errorString() : "Unknown error";
    reportError("UDP_SOCKET_ERROR",
               QString("Socket error %1: %2").arg(socketError).arg(errorString));

    // 对于严重错误，尝试重连
    if (socketError == QAbstractSocket::NetworkError ||
        socketError == QAbstractSocket::ConnectionRefusedError) {
        attemptReconnect();
    }
}

/**
 * @brief UDP套接字状态变化事件处理函数
 * @details 监控UDP套接字的连接状态变化，发送连接状态信号并处理断线重连
 * @param socketState 套接字当前状态
 *
 * 状态处理：
 * - BoundState: 套接字已绑定，视为连接成功
 * - UnconnectedState: 套接字未连接，触发重连机制
 * - 其他状态: 视为未连接状态
 *
 * 功能：
 * 1. 根据套接字状态判断连接是否正常
 * 2. 发送connectionStatusChanged信号通知上层
 * 3. 在断线时自动启动重连流程
 *
 * @note 使用BoundState作为UDP连接成功的标志
 */
void ThreadedUdpSocket::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    bool isConnected = (socketState == QAbstractSocket::BoundState);
    emit connectionStatusChanged(isConnected);

    if (!isConnected && socketState == QAbstractSocket::UnconnectedState) {
        qWarning() << "UDP socket disconnected, attempting reconnect...";
        attemptReconnect();
    }
}

/**
 * @brief 自动重连尝试函数
 * @details 当UDP连接失败或断开时，自动尝试重新建立连接。具有重连次数限制和延时机制。
 *
 * 重连机制：
 * 1. 检查重连尝试次数是否超过最大限制
 * 2. 增加重连计数器
 * 3. 启动重连定时器提供延时
 * 4. 延时后调用start()函数重新启动服务
 *
 * 安全措施：
 * - 最大重连次数限制 (MAX_RECONNECT_ATTEMPTS)
 * - 重连间隔延时 (RECONNECT_INTERVAL_MS)
 * - 短延时后执行实际重连操作 (100ms)
 *
 * 失败处理：
 * - 超过最大重连次数时报告错误并停止重连
 * - 记录每次重连尝试的进度信息
 *
 * @note 使用QTimer::singleShot避免阻塞主线程
 */
void ThreadedUdpSocket::attemptReconnect()
{
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        reportError("UDP_RECONNECT_FAILED",
                   QString("Failed to reconnect after %1 attempts").arg(MAX_RECONNECT_ATTEMPTS));
        return;
    }

    m_reconnectAttempts++;
    qInfo() << "Attempting UDP reconnect" << m_reconnectAttempts << "of" << MAX_RECONNECT_ATTEMPTS;

    m_reconnectTimer->start(RECONNECT_INTERVAL_MS);

    // 延迟执行重连
    QTimer::singleShot(100, this, [this]() {
        start();
    });
}

/**
 * @brief 错误报告函数
 * @details 统一的错误报告接口，通过错误处理器记录错误信息并发送错误信号
 * @param code 错误代码，用于错误分类和识别
 * @param message 详细的错误描述信息
 *
 * 错误报告功能：
 * 1. 通过全局错误处理器记录错误
 * 2. 设置错误严重程度为Error级别
 * 3. 归类为网络类别错误
 * 4. 附加端口和IP地址等上下文信息
 * 5. 发送socketError信号通知上层应用
 *
 * 上下文信息：
 * - port: 当前UDP端口号
 * - ip: 当前IP地址
 *
 * @note 错误信息会被记录到系统日志中，便于问题诊断
 */
void ThreadedUdpSocket::reportError(const QString& code, const QString& message)
{
    ERROR_HANDLER.reportError(code, message, ErrorSeverity::Error, ErrorCategory::Network,
                             {{"port", m_Port}, {"ip", m_Ip}});
    emit socketError(QString("%1: %2").arg(code, message));
}
