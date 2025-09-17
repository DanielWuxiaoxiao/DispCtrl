#include "threadudpsocket.h"
#include "Basic/Protocol.h"
#include <QNetworkDatagram>
#include <QDateTime>
#include <QDebug>
#include "Controller/controller.h"

ThreadedUdpSocket::ThreadedUdpSocket(QString ip, quint16 port, QObject* parent)
    : QObject(parent), m_Ip(ip), m_Port(port) {}

void ThreadedUdpSocket::setSourceAndDestID(quint16 src, quint16 dst) {
    srcID = src;
    destID = dst;
}

void ThreadedUdpSocket::start() {
    if (m_socket)
        delete m_socket;
    m_socket = new QUdpSocket(this);
    m_socket->bind(QHostAddress::Any, m_Port);
    connect(m_socket, &QUdpSocket::readyRead, this, &ThreadedUdpSocket::onReadyRead);
}

void ThreadedUdpSocket::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        handleDatagram(datagram.data(),datagram.senderPort());
    }
}

void ThreadedUdpSocket::handleDatagram(const QByteArray& data, int senderPort) {
    if (!validateFrame(data))
    {
        return;
    }

    quint16 msgID = *reinterpret_cast<const quint16*>(data.constData() + sizeof(ProtocolFrame));

    const char* payload = data.constData() + sizeof(ProtocolFrame);

    switch (msgID) {
    case 0xDD01: {
        if(senderPort ==CF_INS.port("SIG_2_DISP_PORT1",SIG_2_DISP_PORT1)&& m_Port == CF_INS.port("DISP_GET_SIG_PORT1",DISP_GET_SIG_PORT1))
            emit detInfo(data);

        break;
    }
    case 0xEE01:
        if(senderPort ==CF_INS.port("DATA_PRO_2_DISP",DATA_PRO_2_DISP)&& m_Port == CF_INS.port("DISP_GET_DATA_PORT",DISP_GET_DATA_PORT))
            emit traInfo(data);
        break;
    case 0xDD02: {
        if(senderPort ==CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2)&& m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2))
        {
            DataSaveOK info;
            memcpy(&info, payload, sizeof(info));
            emit dataSaveOK(info);
        }
        break;
    }
    case 0xDD03: {
        if(senderPort ==CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2)&& m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2))
        {
            DataDelOK info;
            memcpy(&info, payload, sizeof(info));
            emit dataDelOK(info);
        }
        break;
    }
    case 0xDD04: {
        if(senderPort ==CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2)&& m_Port == CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2))
        {
            OfflineStat info;
            memcpy(&info, payload, sizeof(info));
            emit offLineStat(info);
        }
        break;
    }
    case 0xDB01: {
        if(m_Port == CF_INS.port("DISP_GET_TARGET_PORT",DISP_GET_TARGET_PORT))
        {
            TargetClaRes res;
            memcpy(&res, payload, sizeof(res));
            emit targetClaRes(res);
        }
        break;
    }
    case 0xCF01: {
        if(senderPort ==CF_INS.port("MONITOR_2_DISP",MONITOR_2_DISP)&& m_Port == CF_INS.port("DISP_GET_MONITOR_PORT",DISP_GET_MONITOR_PORT))
        {
            MonitorParam param;
            memcpy(&param, payload, sizeof(param));
            emit monitorParamSend(param);
            break;
        }
    }
    default:
        break;
    }
}

bool ThreadedUdpSocket::validateFrame(const QByteArray& data) {
    if (data.size() < sizeof(ProtocolFrame) + sizeof(ProtocolEnd))
    {
        return false;
    }
    const ProtocolFrame* head = reinterpret_cast<const ProtocolFrame*>(data.constData());
    if (head->srcID != srcID || head->destID != destID)
    {
        return false;
    }
    if (memcmp(&HEADCODE, head, sizeof(HEADCODE)) != 0)
    {
        return false;
    }
    const ProtocolEnd* end = reinterpret_cast<const ProtocolEnd*>(data.constData() + head->dataLen);
    return (calculateXOR(data.constData(), head->dataLen) == end->checkCode &&
            memcmp(&ENDCODE, &end->end, sizeof(ENDCODE)) == 0);
}

void ThreadedUdpSocket::writeData(const QByteArray &datagram, const QHostAddress &host, quint16 port)
{
    m_socket->writeDatagram(datagram, host, port);
}

