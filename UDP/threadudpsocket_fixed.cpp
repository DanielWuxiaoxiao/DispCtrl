#include "threadudpsocket.h"
#include "Basic/Protocol.h"
#include "Controller/ErrorHandler.h"
#include <QNetworkDatagram>
#include <QDateTime>
#include <QDebug>
#include "Controller/controller.h"

ThreadedUdpSocket::ThreadedUdpSocket(QString ip, quint16 port, QObject* parent)
    : QObject(parent), m_Ip(ip), m_Port(port), m_reconnectAttempts(0) 
{
    // 初始化重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ThreadedUdpSocket::attemptReconnect);
}

void ThreadedUdpSocket::setSourceAndDestID(quint16 src, quint16 dst) {
    srcID = src;
    destID = dst;
}

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
        case 0xDD01: {
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT1",SIG_2_DISP_PORT1) && 
               m_Port == CF_INS.port("DISP_GET_SIG_PORT1",DISP_GET_SIG_PORT1)) {
                emit detInfo(data);
            }
            break;
        }
        case 0xEE01: {
            if(senderPort == CF_INS.port("DATA_PRO_2_DISP",DATA_PRO_2_DISP) && 
               m_Port == CF_INS.port("DISP_GET_DATA_PORT",DISP_GET_DATA_PORT)) {
                emit traInfo(data);
            }
            break;
        }
        case 0xDD02: {
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) && 
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                DataSaveOK info;
                memcpy(&info, payload, sizeof(info));
                emit dataSaveOK(info);
            }
            break;
        }
        case 0xDD03: {
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) && 
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                DataDelOK info;
                memcpy(&info, payload, sizeof(info));
                emit dataDelOK(info);
            }
            break;
        }
        case 0xDD04: {
            if(senderPort == CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2) && 
               m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2)) {
                OfflineStat info;
                memcpy(&info, payload, sizeof(info));
                emit offLineStat(info);
            }
            break;
        }
        case 0xDB01: {
            if(m_Port == CF_INS.port("DISP_GET_TARGET_PORT",DISP_GET_TARGET_PORT)) {
                TargetClaRes info;
                memcpy(&info, payload, sizeof(info));
                emit targetClaRes(info);
            }
            break;
        }
        case 0xDC01: {
            if(m_Port == CF_INS.port("DISP_GET_MON_PORT",DISP_GET_MON_PORT)) {
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

bool ThreadedUdpSocket::validateFrame(const QByteArray& data)
{
    if (data.size() < sizeof(ProtocolFrame) + sizeof(ProtocolEnd)) {
        return false;
    }
    auto frame = reinterpret_cast<const ProtocolFrame*>(data.constData());
    auto end = reinterpret_cast<const ProtocolEnd*>(data.constData() + data.size() - sizeof(ProtocolEnd));
    return (memcmp(&STARTCODE, &frame->start, sizeof(STARTCODE)) == 0 &&
            memcmp(&ENDCODE, &end->end, sizeof(ENDCODE)) == 0);
}

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

void ThreadedUdpSocket::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    bool isConnected = (socketState == QAbstractSocket::BoundState);
    emit connectionStatusChanged(isConnected);
    
    if (!isConnected && socketState == QAbstractSocket::UnconnectedState) {
        qWarning() << "UDP socket disconnected, attempting reconnect...";
        attemptReconnect();
    }
}

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

void ThreadedUdpSocket::reportError(const QString& code, const QString& message)
{
    ERROR_HANDLER.reportError(code, message, ErrorSeverity::Error, ErrorCategory::Network,
                             {{"port", m_Port}, {"ip", m_Ip}});
    emit socketError(QString("%1: %2").arg(code, message));
}