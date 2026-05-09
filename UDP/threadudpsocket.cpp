/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 17:16:08
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

#include <QDateTime>
#include <QDebug>
#include <QNetworkDatagram>
#include <QTimer>
#include <cstdlib>
#include <cstring>

#include "Basic/Protocol.h"
#include "Basic/log.h"
#include "Controller/ErrorHandler.h"
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
    : QObject(parent), m_Ip(ip), m_Port(port), m_reconnectAttempts(0), m_reconnectGiveUp(false) {
    // 初始化重连定时器：超时后执行实际重连（调用 start()）
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() { start(); });
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
            m_socket->deleteLater();  // 使用 deleteLater 而不是 delete，确保线程安全
            m_socket = nullptr;
        }

        // 不指定 parent，避免跨线程问题
        m_socket = new QUdpSocket();

        // 连接错误处理信号 - Qt5兼容版本
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error), this,
                &ThreadedUdpSocket::onSocketError);
        connect(m_socket, &QUdpSocket::stateChanged, this,
                &ThreadedUdpSocket::onSocketStateChanged);
        connect(m_socket, &QUdpSocket::readyRead, this, &ThreadedUdpSocket::onReadyRead);

        if (!m_socket->bind(QHostAddress::Any, m_Port)) {
            reportError("UDP_BIND_FAILED", QString("Failed to bind UDP socket to port %1: %2")
                                               .arg(m_Port)
                                               .arg(m_socket->errorString()));
            attemptReconnect();
            return;
        }

        m_reconnectAttempts = 0;
        emit connectionStatusChanged(true);
        qInfo() << "UDP socket successfully bound to port" << m_Port;

    } catch (const std::exception& e) {
        reportError("UDP_START_EXCEPTION", QString("Exception in UDP start: %1").arg(e.what()));
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
        reportError("UDP_READ_EXCEPTION", QString("Exception in UDP read: %1").arg(e.what()));
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
                if (senderPort == CF_INS.port("SIG_2_DISP_PORT1", SIG_2_DISP_PORT1) &&
                    m_Port == CF_INS.port("DISP_GET_SIG_PORT1", DISP_GET_SIG_PORT1)) {
                    emit detInfo(data);
                }
                break;
            }
            case 0xEE01: {  // 航迹信息消息
                if (senderPort == CF_INS.port("DATA_PRO_2_DISP", DATA_PRO_2_DISP) &&
                    m_Port == CF_INS.port("DISP_GET_DATA_PORT", DISP_GET_DATA_PORT)) {
                    LOG_DEBUG(QString("[ThreadedUdpSocket] route normal track mesID=0x%1 sender=%2 recv=%3 size=%4")
                              .arg(msgID, 4, 16, QChar('0'))
                              .arg(senderPort)
                              .arg(m_Port)
                              .arg(data.size()));
                    emit traInfo(data);
                }
                break;
            }
            case 0xEE02: {  // TBD 航迹信息消息
                if (senderPort == CF_INS.port("DATA_PRO_2_DISP2", DATA_PRO_2_DISP2) &&
                    m_Port == CF_INS.port("DISP_GET_DATA_PORT2", DISP_GET_DATA_PORT2)) {
                    LOG_DEBUG(QString("[ThreadedUdpSocket] route TBD track mesID=0x%1 sender=%2 recv=%3 size=%4")
                              .arg(msgID, 4, 16, QChar('0'))
                              .arg(senderPort)
                              .arg(m_Port)
                              .arg(data.size()));
                    emit tbdInfo(data);
                } else {
                    emit tbdInfo(data);  // 容错：若端口配置不同仍转发
                }
                break;
            }
            case 0xEE03: {  // 协同航迹信息消息
                if (senderPort == CF_INS.port("DATA_PRO_2_DISP3", DATA_PRO_2_DISP3) &&
                    m_Port == CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3)) {
                    LOG_DEBUG(QString("[ThreadedUdpSocket] route cooperative track mesID=0x%1 sender=%2 recv=%3 size=%4")
                              .arg(msgID, 4, 16, QChar('0'))
                              .arg(senderPort)
                              .arg(m_Port)
                              .arg(data.size()));
                    emit cooperativeTrackInfo(data);
                } else {
                    qWarning() << "[ThreadedUdpSocket] Cooperative track port mismatch, sender="
                               << senderPort << "expected=" << CF_INS.port("DATA_PRO_2_DISP3", DATA_PRO_2_DISP3)
                               << "recv=" << m_Port << "expectedRecv=" << CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3)
                               << "forwarding for debug";
                    emit cooperativeTrackInfo(data);  // 容错：若端口配置不同仍转发
                }
                break;
            }
            case 0xDD02: {  // 数据保存确认消息
                if (senderPort == CF_INS.port("SIG_2_DISP_PORT2", SIG_2_DISP_PORT2) &&
                    m_Port == CF_INS.port("DISP_GET_SIG_PORT2", DISP_GET_SIG_PORT2)) {
                    DataSaveOK info;
                    memcpy(&info, payload, sizeof(info));
                    emit dataSaveOK(info);
                }
                break;
            }
            case 0xDD03: {  // 数据删除确认消息
                if (senderPort == CF_INS.port("SIG_2_DISP_PORT2", SIG_2_DISP_PORT2) &&
                    m_Port == CF_INS.port("DISP_GET_SIG_PORT2", DISP_GET_SIG_PORT2)) {
                    DataDelOK info;
                    memcpy(&info, payload, sizeof(info));
                    emit dataDelOK(info);
                }
                break;
            }
            case 0xDD04: {  // 离线状态信息消息
                if (senderPort == CF_INS.port("SIG_2_DISP_PORT2", SIG_2_DISP_PORT2) &&
                    m_Port == CF_INS.port("DISP_GET_SIG_PORT2", DISP_GET_SIG_PORT2)) {
                    OfflineStat info;
                    memcpy(&info, payload, sizeof(info));
                    emit offLineStat(info);
                }
                break;
            }
            case 0xDD05: {  // 经纬高信息上报
                if (senderPort == CF_INS.port("SIG_2_DISP_PORT2", SIG_2_DISP_PORT2) &&
                    m_Port == CF_INS.port("DISP_GET_SIG_PORT2", DISP_GET_SIG_PORT2)) {
                    GeoLocationReport raw;
                    memcpy(&raw, payload, sizeof(raw));
                    double lat = raw.latitude / 1e9;
                    double lon = raw.longitude / 1e9;
                    double alt = raw.altitude / 100.0;
                    emit geoLocationReport(lat, lon, alt);
                }
                break;
            }
            case 0xDB01: {  // 目标分类结果消息
                if (m_Port == CF_INS.port("DISP_GET_TARGET_PORT", DISP_GET_TARGET_PORT)) {
                    TargetClaRes info;
                    memcpy(&info, payload, sizeof(info));
                    emit targetClaRes(info);
                }
                break;
            }
            case 0xCF01: {  // 监控参数消息
                if (m_Port == CF_INS.port("DISP_GET_MONITOR_PORT", DISP_GET_TARGET_PORT)) {
                    MonitorParam info;
                    memcpy(&info, payload, sizeof(info));
                    emit monitorParamSend(info);
                }
                break;
            }
            case 0xDE01: {  // 伺服控制回送
                ServoCtrlRet info;
                memcpy(&info, payload, sizeof(info));
                emit servoCtrlRet(info);
                break;
            }
            case 0xDE02: {  // BIT 上报
                BITReport info;
                memcpy(&info, payload, sizeof(info));

                emit bitReport(info);
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
bool ThreadedUdpSocket::validateFrame(const QByteArray& data) {
    if (data.size() < sizeof(ProtocolFrame) + sizeof(ProtocolEnd)) {
        return false;
    }
    auto frame = reinterpret_cast<const ProtocolFrame*>(data.constData());
    auto end =
        reinterpret_cast<const ProtocolEnd*>(data.constData() + data.size() - sizeof(ProtocolEnd));
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
void ThreadedUdpSocket::writeData(const QByteArray& datagram, const QHostAddress& host,
                                  quint16 port) {
    try {
        if (!m_socket) {
            reportError("UDP_WRITE_NO_SOCKET", "Attempted to write data but socket is null");
            return;
        }

        // 检查 socket 状态，只有在已绑定状态才能发送数据
        if (m_socket->state() != QAbstractSocket::BoundState) {
            reportError("UDP_WRITE_NOT_BOUND",
                        QString("Attempted to write data but socket is not bound (state: %1, port: %2)")
                        .arg(m_socket->state())
                        .arg(m_Port));
            return;
        }

        qint64 bytesWritten = m_socket->writeDatagram(datagram, host, port);
        if (bytesWritten == -1) {
            reportError("UDP_WRITE_FAILED",
                        QString("Failed to write UDP datagram: %1").arg(m_socket->errorString()));
        }
    } catch (const std::exception& e) {
        reportError("UDP_WRITE_EXCEPTION", QString("Exception writing UDP data: %1").arg(e.what()));
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
void ThreadedUdpSocket::onSocketError(QAbstractSocket::SocketError socketError) {
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
void ThreadedUdpSocket::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
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
void ThreadedUdpSocket::attemptReconnect() {
    // 已放弃重连（冷却期内），忽略本次调用
    if (m_reconnectGiveUp) {
        return;
    }

    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        reportError("UDP_RECONNECT_FAILED",
                    QString("Failed to reconnect after %1 attempts").arg(MAX_RECONNECT_ATTEMPTS));
        m_reconnectGiveUp = true;
        // 冷却 60 秒后重置，允许再次尝试
        QTimer::singleShot(RECONNECT_COOLDOWN_MS, this, [this]() {
            m_reconnectAttempts = 0;
            m_reconnectGiveUp = false;
            qInfo() << "UDP reconnect cooldown expired, will retry on next disconnect event";
        });
        return;
    }

    m_reconnectAttempts++;
    qInfo() << "Attempting UDP reconnect" << m_reconnectAttempts << "of" << MAX_RECONNECT_ATTEMPTS;

    // 延迟 RECONNECT_INTERVAL_MS 后执行实际重连
    m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
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
void ThreadedUdpSocket::reportError(const QString& code, const QString& message) {
    ERROR_HANDLER.reportError(code, message, ErrorSeverity::Error, ErrorCategory::Network,
                              {{"port", m_Port}, {"ip", m_Ip}});
    emit socketError(QString("%1: %2").arg(code, message));
}

/**
 * @brief 计算异或校验码
 * @details 计算数据的异或校验码，用于光电协议
 * @param data 待计算数据指针
 * @param len 数据长度
 * @return 校验码
 */
char ThreadedUdpSocket::checkAccusation(const char* data, int len) {
    char checksum = 0;
    for (int i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief 启用心跳机制
 * @details 创建并启动心跳定时器，定期向光电系统发送心跳包
 */
void ThreadedUdpSocket::enableHeartBeat() {
    // 构造心跳包
    char* sendData = (char*)malloc(sizeof(HeartbeatPacket));
    HeartbeatPacket param;
    param.dataLen = sizeof(HeartbeatPacket) - sizeof(param.versionNumber) - sizeof(param.head) -
                    sizeof(param.dataLen) - sizeof(param.checkCode);

    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    heartbeatPacket = QByteArray::fromRawData(sendData, sizeof(param));

    // 创建心跳定时器
    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, &ThreadedUdpSocket::sendHeartbeat);
    heartbeatTimer->start(HEARTBEAT_INTERVAL);

    qInfo() << "Heartbeat mechanism enabled, interval:" << HEARTBEAT_INTERVAL << "ms";
    free(sendData);
}

/**
 * @brief 安全发送数据报（内部使用）
 * @param data 要发送的数据
 * @param size 数据大小
 * @param host 目标主机地址
 * @param port 目标端口
 * @return true表示发送成功，false表示失败
 * @details 在发送前检查socket状态，确保socket已绑定
 */
bool ThreadedUdpSocket::safeWriteDatagram(const char* data, qint64 size, const QHostAddress& host, quint16 port) {
    if (!m_socket) {
        reportError("UDP_WRITE_NO_SOCKET", "Attempted to write data but socket is null");
        return false;
    }

    if (m_socket->state() != QAbstractSocket::BoundState) {
        reportError("UDP_WRITE_NOT_BOUND",
                    QString("Attempted to write data but socket is not bound (state: %1, port: %2)")
                    .arg(m_socket->state())
                    .arg(m_Port));
        return false;
    }

    qint64 bytesWritten = m_socket->writeDatagram(data, size, host, port);
    if (bytesWritten == -1) {
        reportError("UDP_WRITE_FAILED",
                    QString("Failed to write UDP datagram: %1").arg(m_socket->errorString()));
        return false;
    }

    return true;
}

/**
 * @brief 发送心跳包
 * @details 定时器触发，向光电系统发送心跳包
 */
void ThreadedUdpSocket::sendHeartbeat() {
    if (!heartbeatPacket.isEmpty()) {
        safeWriteDatagram(heartbeatPacket.constData(), heartbeatPacket.size(),
                         QHostAddress(PHOTO_ELE_IP), PHOTO_GET_DISP_PORT);
    }
}

/**
 * @brief 发送光电参数（经纬高模式）
 * @param param 光电参数结构体
 * @details 向光电系统发送目标引导信息（经纬高坐标）
 */
void ThreadedUdpSocket::sendPEParam(PhotoElectricParamSet param) {
    char* sendData = (char*)malloc(sizeof(PhotoElectricParamSet));
    param.dataLen = sizeof(PhotoElectricParamSet) - sizeof(param.versionNumber) -
                    sizeof(param.head) - sizeof(param.dataLen) - sizeof(param.checkCode);

    param.timeStamp = QDateTime::currentMSecsSinceEpoch();

    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    safeWriteDatagram(sendData, sizeof(param), QHostAddress(PHOTO_ELE_IP), PHOTO_GET_DISP_PORT);
    free(sendData);
}

/**
 * @brief 发送光电参数（方位俯仰模式）
 * @param param 光电参数结构体
 * @details 向光电系统发送目标引导信息（极坐标）
 */
void ThreadedUdpSocket::sendPEParam2(PhotoElectricParamSet2 param) {
    char* sendData = (char*)malloc(sizeof(PhotoElectricParamSet2));
    param.dataLen = sizeof(PhotoElectricParamSet2) - sizeof(param.versionNumber) -
                    sizeof(param.head) - sizeof(param.dataLen) - sizeof(param.checkCode);

    param.timeStamp = QDateTime::currentMSecsSinceEpoch();

    memcpy(sendData, &param, sizeof(param) - sizeof(param.checkCode));
    char checksum = checkAccusation(sendData, sizeof(param) - sizeof(param.checkCode));

    param.checkCode = checksum;
    memcpy(sendData, &param, sizeof(param));

    safeWriteDatagram(sendData, sizeof(param), QHostAddress(PHOTO_ELE_IP), PHOTO_GET_DISP_PORT);
    free(sendData);
}

/**
 * @brief 发送阵地控制参数
 * @param param 阵地控制参数
 * @details 控制雷达阵地各象限的开关状态
 */
void ThreadedUdpSocket::sendBCParam(BatteryControlM param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送收发控制参数
 * @param param 收发控制参数
 * @details 控制雷达的收发状态
 */
void ThreadedUdpSocket::sendTRParam(TranRecControl param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

void ThreadedUdpSocket::sendServoControl(ServoControlParam param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送频率控制参数
 * @param param 方向图扫描参数
 * @details 控制雷达的频率和扫描参数
 */
void ThreadedUdpSocket::sendFCParam(DirGramScan param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送扫描范围参数
 * @param param 扫描范围参数
 * @details 设置雷达的扫描范围和工作方式
 */
void ThreadedUdpSocket::sendSRParam(ScanRange param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送波控参数
 * @param param 波束控制参数
 * @details 控制雷达波束的参数
 */
void ThreadedUdpSocket::sendWCParam(BeamControl param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送信号处理参数
 * @param param 信号处理参数
 * @details 配置信号处理算法参数
 */
void ThreadedUdpSocket::sendSPParam(SigProParam param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送数据处理参数
 * @param param 数据处理参数
 * @details 配置数据处理算法参数
 */
void ThreadedUdpSocket::sendDPParam(DataProParam param) {
    auto data = packData(reinterpret_cast<char*>(&param), sizeof(param), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray =
        QByteArray::fromRawData(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(data, sizeof(param) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(RES_DIS_IP), RES_GET_DISP_PORT);
    free(data);
}

/**
 * @brief 发送数据存储设置
 * @param param 数据存储设置参数
 * @details 配置数据的保存、删除和离线处理
 */
void ThreadedUdpSocket::sendDSParam(DataSet param) {
    if (param.ifsave == 1) {
        auto data = packData(reinterpret_cast<char*>(&param.save), sizeof(param.save), srcID,
                             destID, commCount);
        commCount++;

        QByteArray byteArray = QByteArray::fromRawData(
            data, sizeof(param.save) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
        safeWriteDatagram(data,
                                sizeof(param.save) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                                QHostAddress(SIG_PRO_IP), SIG_GET_DISP_PORT);
        free(data);
    }

    if (param.ifdel == 1) {
        auto data = packData(reinterpret_cast<char*>(&param.del), sizeof(param.del), srcID, destID,
                             commCount);
        commCount++;

        QByteArray byteArray = QByteArray::fromRawData(
            data, sizeof(param.del) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
        safeWriteDatagram(data,
                                sizeof(param.del) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                                QHostAddress(SIG_PRO_IP), SIG_GET_DISP_PORT);
        free(data);
    }

    if (param.ifoffline == 1) {
        auto data = packData(reinterpret_cast<char*>(&param.off), sizeof(param.off), srcID, destID,
                             commCount);
        commCount++;

        QByteArray byteArray = QByteArray::fromRawData(
            data, sizeof(param.off) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
        safeWriteDatagram(data,
                                sizeof(param.off) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                                QHostAddress(SIG_PRO_IP), SIG_GET_DISP_PORT);
        free(data);
    }
}

/**
 * @brief 发送系统启动命令
 * @param data 系统启动参数
 * @details 向监控系统发送启动命令
 */
void ThreadedUdpSocket::sendSysStart(StartSysParam data) {
    auto sendData =
        packData(reinterpret_cast<char*>(&data), sizeof(data), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray = QByteArray::fromRawData(
        sendData, sizeof(data) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(sendData, sizeof(data) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(MONITOR_IP), MONITOR_GET_DISP_PORT);
    free(sendData);
}

/**
 * @brief 设置手动航迹
 * @param data 手动航迹参数
 * @details 手动设置目标航迹
 */
void ThreadedUdpSocket::setManual(SetTrackManual data) {
    auto sendData =
        packData(reinterpret_cast<char*>(&data), sizeof(data), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray = QByteArray::fromRawData(
        sendData, sizeof(data) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(sendData, sizeof(data) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(DATA_PRO_IP), DATA_GET_DISP);
    free(sendData);
}

/**
 * @brief 上报点迹信息
 * @param info 点迹信息
 * @details 向数据处理系统上报用户选中的点迹信息（检测点或航迹）
 *          用于目标确认、引导光电跟踪或数据分析
 */
void ThreadedUdpSocket::reportPointInfo(PointInfo info) {
    auto sendData =
        packData(reinterpret_cast<char*>(&info), sizeof(info), srcID, destID, commCount);
    commCount++;

    QByteArray byteArray = QByteArray::fromRawData(
        sendData, sizeof(info) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd));
    safeWriteDatagram(sendData, sizeof(info) + sizeof(ProtocolFrame) + sizeof(ProtocolEnd),
                            QHostAddress(DATA_PRO_IP), DATA_GET_DISP);

    qInfo() << "Reported point info - Type:" << info.type << "Range:" << info.range
            << "Azimuth:" << info.azimuth << "Elevation:" << info.elevation
            << "Batch:" << info.batch;

    free(sendData);
}

/**
 * @brief ThreadedUdpSocket析构函数
 * @details 安全清理UDP套接字资源，关闭连接并释放内存
 *
 * 清理步骤：
 * 1. 停止重连定时器
 * 2. 关闭并删除UDP套接字
 *
 * @note 析构函数会在对象删除时自动调用
 */
ThreadedUdpSocket::~ThreadedUdpSocket() {
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }

    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}
