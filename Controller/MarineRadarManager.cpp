/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 15:27:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-21 17:53:15
 * @Description: 
 */
/**
 * @file MarineRadarManager.cpp
 * @brief 船用雷达UDP通信管理器实现
 */
#include "MarineRadarManager.h"
#include "Basic/log.h"
#include <QNetworkDatagram>
#include <QElapsedTimer>
#include <QDebug>

namespace {

bool shouldLogSample(uint32_t count, uint32_t firstCount, uint32_t interval)
{
    return count <= firstCount || (interval > 0 && count % interval == 0);
}

QString controlSummary(const MarineControlFrame& f)
{
    return QString("range=%1 tx=%2 gain=%3 level=%4 sea=%5 rain=%6 interference=%7 servo=%8 checksum=0x%9")
        .arg(static_cast<int>(f.rangeVal))
        .arg(static_cast<int>(f.txCtrl))
        .arg(static_cast<int>(f.gain))
        .arg(static_cast<int>(f.level))
        .arg(static_cast<int>(f.seaVal))
        .arg(static_cast<int>(f.rainVal))
        .arg(static_cast<int>(f.ganRao))
        .arg(static_cast<int>(f.servo))
        .arg(static_cast<int>(f.checkSum), 2, 16, QLatin1Char('0'));
}

QString echoSummary(const MarineEchoHeader& h)
{
    return QString("aziRaw=%1 aziDeg=%2 aziIdx=%3 style=0x%4 packet=%5 fftWords=%6 echoBytes=%7 cells=%8 range=%9 tx=%10 gain=%11 level=%12 sea=%13 rain=%14 interference=%15 freq=%16")
        .arg(h.azimuthRaw())
        .arg(h.azimuthDeg(), 0, 'f', 2)
        .arg(h.azimuthRenderIndex())
        .arg(static_cast<int>(h.style), 2, 16, QLatin1Char('0'))
        .arg(h.packetNumber())
        .arg(h.fftWordCount())
        .arg(h.echoByteCount())
        .arg(h.rangeCellCount())
        .arg(static_cast<int>(h.rangeCode))
        .arg(static_cast<int>(h.txState))
        .arg(static_cast<int>(h.gain))
        .arg(static_cast<int>(h.level))
        .arg(static_cast<int>(h.seaVal))
        .arg(static_cast<int>(h.rainVal))
        .arg(static_cast<int>(h.ganRao))
        .arg(static_cast<int>(h.freqStatus));
}

QString firstBytesHex(const QByteArray& data, int maxBytes)
{
    return QString::fromLatin1(data.left(qMin(data.size(), maxBytes)).toHex());
}

} // namespace

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
    LOG_INFO("[MarineRadar] manager created");
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
    LOG_INFO(QString("[MarineRadar][INIT] bind=%1:%2 servo=%3:%4")
             .arg(localIp.isEmpty() ? QStringLiteral("0.0.0.0") : localIp)
             .arg(localPort)
             .arg(servoIp)
             .arg(servoPort));

    // 绑定接收端口
    if (m_rxSocket->state() == QAbstractSocket::BoundState)
        m_rxSocket->close();

    QHostAddress bindAddr = localIp.isEmpty() ? QHostAddress::AnyIPv4 : QHostAddress(localIp);
    if (!m_rxSocket->bind(bindAddr, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        QString msg = QString("[MarineRadar][INIT] failed to bind %1:%2 - %3")
                          .arg(localIp).arg(localPort).arg(m_rxSocket->errorString());
        LOG_ERROR(msg);
        emit logMessage(msg);
    } else {
        QString msg = QString("[MarineRadar][INIT] listening on %1:%2").arg(localIp).arg(localPort);
        LOG_INFO(msg);
        emit logMessage(msg);
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
    const qint64 written = m_txSocket->writeDatagram(data, m_servoAddr, m_servoPort);
    ++m_txCount;

    if (written != data.size()) {
        LOG_WARNING(QString("[MarineRadar][TX] send failed #%1 written=%2 expected=%3 target=%4:%5 error=%6 %7")
                    .arg(m_txCount)
                    .arg(written)
                    .arg(data.size())
                    .arg(m_servoAddr.toString())
                    .arg(m_servoPort)
                    .arg(m_txSocket->errorString())
                    .arg(controlSummary(f)));
    } else if (shouldLogSample(m_txCount, 5, 50)) {
        LOG_INFO(QString("[MarineRadar][TX] sent #%1 bytes=%2 target=%3:%4 %5")
                 .arg(m_txCount)
                 .arg(written)
                 .arg(m_servoAddr.toString())
                 .arg(m_servoPort)
                 .arg(controlSummary(f)));
    }
}

void MarineRadarManager::setCurrentControl(const MarineControlFrame& frame)
{
    m_ctrl = frame;
    m_ctrl.updateChecksum();
    LOG_INFO(QString("[MarineRadar][CTRL] loaded initial control %1").arg(controlSummary(m_ctrl)));
}

void MarineRadarManager::setRange(uint8_t rangeVal)
{
    if (rangeVal >= MARINE_RANGE_TABLE_SIZE) {
        LOG_WARNING(QString("[MarineRadar][CTRL] ignore invalid range=%1").arg(static_cast<int>(rangeVal)));
        return;
    }
    LOG_INFO(QString("[MarineRadar][CTRL] setRange=%1 (%2)").arg(static_cast<int>(rangeVal)).arg(marineRangeLabel(rangeVal)));
    m_ctrl.rangeVal = rangeVal;
    sendControl(m_ctrl);
}

void MarineRadarManager::setGain(uint8_t gain)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setGain=%1").arg(static_cast<int>(gain)));
    m_ctrl.gain = gain;
    sendControl(m_ctrl);
}

void MarineRadarManager::setSeaClutter(uint8_t val)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setSeaClutter=%1").arg(static_cast<int>(val)));
    m_ctrl.seaVal = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setRainClutter(uint8_t val)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setRainClutter=%1").arg(static_cast<int>(val)));
    m_ctrl.rainVal = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setInterference(uint8_t val)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setInterference=%1").arg(static_cast<int>(val)));
    m_ctrl.ganRao = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setLevel(uint8_t val)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setLevel=%1").arg(static_cast<int>(val)));
    m_ctrl.level = val;
    sendControl(m_ctrl);
}

void MarineRadarManager::setTxOn(bool on)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setTxOn=%1").arg(on ? 1 : 0));
    m_ctrl.txCtrl = on ? 1 : 0;
    sendControl(m_ctrl);
}

void MarineRadarManager::setServoSpeed(uint16_t speed)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setServoSpeed=%1").arg(speed));
    m_ctrl.servo = static_cast<uint8_t>(speed);
    sendControl(m_ctrl);
}

void MarineRadarManager::setAutoSendInterval(int ms)
{
    if (ms <= 0) {
        m_autoSendTimer->stop();
        LOG_INFO("[MarineRadar][TX] auto-send stopped");
    } else {
        m_autoSendTimer->start(ms);
        LOG_INFO(QString("[MarineRadar][TX] auto-send interval=%1ms").arg(ms));
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
    m_rxDrainScheduled = false;

    QElapsedTimer budget;
    budget.start();
    int processed = 0;
    constexpr int kMaxDatagramsPerDrain = 64;
    constexpr qint64 kMaxDrainMs = 4;

    while (m_rxSocket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_rxSocket->receiveDatagram();
        const QByteArray data = datagram.data();
        ++m_rxDatagramCount;

        if (shouldLogSample(m_rxDatagramCount, 5, 512)) {
            LOG_INFO(QString("[MarineRadar][RX] datagram #%1 bytes=%2 from=%3:%4")
                     .arg(m_rxDatagramCount)
                     .arg(data.size())
                     .arg(datagram.senderAddress().toString())
                     .arg(datagram.senderPort()));
        }

        parseEchoDatagram(data);

        ++processed;
        if (processed >= kMaxDatagramsPerDrain || budget.elapsed() >= kMaxDrainMs)
            break;
    }

    if (m_rxSocket->hasPendingDatagrams() && !m_rxDrainScheduled) {
        m_rxDrainScheduled = true;
        QTimer::singleShot(0, this, &MarineRadarManager::onReadyRead);
    }
}

void MarineRadarManager::parseEchoDatagram(const QByteArray& data)
{
    constexpr int HEADER_SIZE = static_cast<int>(sizeof(MarineEchoHeader));

    if (data.size() < HEADER_SIZE) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] short frame invalidCount=%1 bytes=%2 expectedHeader=%3 head=%4")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(HEADER_SIZE)
                        .arg(firstBytesHex(data, 16)));
        }
        return; // 数据太短
    }

    const auto* hdr = reinterpret_cast<const MarineEchoHeader*>(data.constData());

    // 校验帧头
    if (!hdr->isHeaderValid()) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] invalid header invalidCount=%1 bytes=%2 head=%3")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(firstBytesHex(data, 16)));
        }
        return;
    }

    if (!hdr->isStatusValid() && shouldLogSample(m_rxDatagramCount, 5, 512)) {
        LOG_WARNING(QString("[MarineRadar][RX] status checksum mismatch %1").arg(echoSummary(*hdr)));
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
                          st.gain      != m_status.gain ||
                          st.level     != m_status.level ||
                          st.seaVal    != m_status.seaVal ||
                          st.rainVal   != m_status.rainVal ||
                          st.ganRao    != m_status.ganRao ||
                          st.freqStatus != m_status.freqStatus);
    m_status = st;
    if (statusChanged) {
        LOG_INFO(QString("[MarineRadar][STATUS] updated range=%1 tx=%2 gain=%3 level=%4 sea=%5 rain=%6 interference=%7 freq=%8")
                 .arg(static_cast<int>(m_status.rangeCode))
                 .arg(m_status.txOn ? 1 : 0)
                 .arg(static_cast<int>(m_status.gain))
                 .arg(static_cast<int>(m_status.level))
                 .arg(static_cast<int>(m_status.seaVal))
                 .arg(static_cast<int>(m_status.rainVal))
                 .arg(static_cast<int>(m_status.ganRao))
                 .arg(static_cast<int>(m_status.freqStatus)));
        emit radarStatusUpdated(m_status);
    }

    // 提取回波数据
    const int echoBytes = hdr->echoByteCount();
    int expectedSize = HEADER_SIZE + echoBytes;
    if (data.size() < expectedSize) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] incomplete echo invalidCount=%1 bytes=%2 expected=%3 %4")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(expectedSize)
                        .arg(echoSummary(*hdr)));
        }
        return; // 数据不完整
    }

    MarineEchoLine line;
    line.azimuthRaw = hdr->azimuthRenderIndex();
    line.azimuthDeg = hdr->azimuthDeg();
    line.style      = hdr->style;
    line.packetNum  = hdr->packetNumber();

    // 拷贝幅值数据
    // Echo payload is temporarily defined as one big-endian uint16 per range cell.
    // The current renderer consumes 8-bit amplitudes, so clamp each magnitude.
    const int cellCount = echoBytes / 2;
    line.amplitudes.resize(cellCount);
    const uint8_t* echoData = reinterpret_cast<const uint8_t*>(data.constData()) + HEADER_SIZE;
    for (int i = 0; i < cellCount; ++i) {
        const uint16_t magnitude = (static_cast<uint16_t>(echoData[i * 2]) << 8) |
                                   static_cast<uint16_t>(echoData[i * 2 + 1]);
        line.amplitudes[i] = static_cast<uint8_t>(qMin<uint16_t>(magnitude, 255));
    }

    ++m_rxEchoCount;
    if (shouldLogSample(m_rxEchoCount, 5, 512)) {
        LOG_INFO(QString("[MarineRadar][RX] echo #%1 bytes=%2 %3")
                 .arg(m_rxEchoCount)
                 .arg(data.size())
                 .arg(echoSummary(*hdr)));
    }

    emit echoLineReceived(line);
}
