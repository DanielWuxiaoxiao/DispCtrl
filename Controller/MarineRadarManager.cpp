/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-09 16:45:56
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-20 11:30:45
 * @Description: 
 */
/**
 * @file MarineRadarManager.cpp
 * @brief 船用雷达UDP通信管理器实现
 */
#include "MarineRadarManager.h"
#include <QNetworkDatagram>
#include <QDebug>

MarineRadarManager::MarineRadarManager(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<MarineEchoLine>("MarineEchoLine");
    qRegisterMetaType<MarineRadarStatus>("MarineRadarStatus");

    m_rxSocket = new QUdpSocket(this);
    m_txSocket = new QUdpSocket(this);
    m_autoSendTimer = new QTimer(this);

    connect(m_rxSocket, &QUdpSocket::readyRead, this, &MarineRadarManager::onReadyRead);
    connect(m_autoSendTimer, &QTimer::timeout, this, &MarineRadarManager::onAutoSend);
}

MarineRadarManager::~MarineRadarManager()
{
    m_autoSendTimer->stop();
}

void MarineRadarManager::init(const QString& localIp, uint16_t localPort,
                              const QString& servoIp, uint16_t servoPort)
{
    m_servoAddr = QHostAddress(servoIp);
    m_servoPort = servoPort;

    // 绑定接收端口
    if (m_rxSocket->state() == QAbstractSocket::BoundState)
        m_rxSocket->close();

    QHostAddress bindAddr = localIp.isEmpty() ? QHostAddress::AnyIPv4 : QHostAddress(localIp);
    if (!m_rxSocket->bind(bindAddr, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit logMessage(QString("[MarineRadar] Failed to bind %1:%2 - %3")
                        .arg(localIp).arg(localPort).arg(m_rxSocket->errorString()));
    } else {
        emit logMessage(QString("[MarineRadar] Listening on %1:%2").arg(localIp).arg(localPort));
    }
}

// ============================================================================
// 发送控制
// ============================================================================

void MarineRadarManager::sendControl(const MarineControlFrame& frame)
{
    MarineControlFrame f = frame;
    f.updateChecksum();

    QByteArray data(reinterpret_cast<const char*>(&f), sizeof(f));
    m_txSocket->writeDatagram(data, m_servoAddr, m_servoPort);
    ++m_txCount;
}

void MarineRadarManager::setRange(uint8_t rangeVal)
{
    if (rangeVal >= MARINE_RANGE_TABLE_SIZE) return;
    m_ctrl.rangeVal = rangeVal;
    sendControl(m_ctrl);
}

void MarineRadarManager::setGain(uint8_t gain)
{
    m_ctrl.gain = gain;
    sendControl(m_ctrl);
}

void MarineRadarManager::setSeaClutter(uint8_t val)
{
    m_ctrl.seaVal = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setRainClutter(uint8_t val)
{
    m_ctrl.rainVal = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setInterference(uint8_t val)
{
    m_ctrl.ganRao = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setLevel(uint8_t val)
{
    m_ctrl.level = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setTxOn(bool on)
{
    m_ctrl.txCtrl = on ? 1 : 0;
    sendControl(m_ctrl);
}

void MarineRadarManager::setServoSpeed(uint16_t speed)
{
    m_ctrl.servo = static_cast<uint8_t>(speed);
    sendControl(m_ctrl);
}

void MarineRadarManager::setAutoSendInterval(int ms)
{
    if (ms <= 0) {
        m_autoSendTimer->stop();
    } else {
        m_autoSendTimer->start(ms);
    }
}

void MarineRadarManager::onAutoSend()
{
    sendControl(m_ctrl);
}

// ============================================================================
// 接收与解析
// ============================================================================

void MarineRadarManager::onReadyRead()
{
    while (m_rxSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(static_cast<int>(m_rxSocket->pendingDatagramSize()));
        m_rxSocket->readDatagram(data.data(), data.size());

        parseEchoDatagram(data);
    }
}

void MarineRadarManager::parseEchoDatagram(const QByteArray& data)
{
    constexpr int HEADER_SIZE = static_cast<int>(sizeof(MarineEchoHeader));

    if (data.size() < HEADER_SIZE) {
        return; // 数据太短
    }

    const auto* hdr = reinterpret_cast<const MarineEchoHeader*>(data.constData());

    // 校验帧头
    if (!hdr->isHeaderValid()) {
        return;
    }

    // 提取状态
    MarineRadarStatus st;
    st.rangeCode  = hdr->rangeCode;
    st.txOn       = (hdr->txState != 0);
    st.gain       = hdr->gain;
    st.level      = hdr->level;
    st.seaVal     = hdr->seaVal;
    st.rainVal    = hdr->rainVal;
    st.ganRao     = hdr->ganRao;
    st.freqStatus = hdr->freqStatus;

    bool statusChanged = (st.rangeCode != m_status.rangeCode ||
                          st.txOn      != m_status.txOn ||
                          st.gain      != m_status.gain);
    m_status = st;
    if (statusChanged) {
        emit radarStatusUpdated(m_status);
    }

    // 提取回波数据
    uint16_t fftLen = hdr->fftDataLen;
    int expectedSize = HEADER_SIZE + fftLen;
    if (data.size() < expectedSize) {
        return; // 数据不完整
    }

    MarineEchoLine line;
    line.azimuthRaw = hdr->azimuthRaw();
    line.azimuthDeg = hdr->azimuthDeg();
    line.style      = hdr->style;
    line.packetNum  = hdr->packetNum;

    // 拷贝幅值数据
    line.amplitudes.resize(fftLen);
    const uint8_t* echoData = reinterpret_cast<const uint8_t*>(data.constData()) + HEADER_SIZE;
    memcpy(line.amplitudes.data(), echoData, fftLen);

    emit echoLineReceived(line);
}
