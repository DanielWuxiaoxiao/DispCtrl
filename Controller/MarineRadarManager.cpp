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
#include "Basic/ConfigManager.h"
#include "Basic/log.h"
#include <QNetworkDatagram>
#include <QElapsedTimer>
#include <QDebug>
#include <QStringList>
#include <algorithm>
#include <cstring>

namespace {

bool shouldLogSample(uint32_t count, uint32_t firstCount, uint32_t interval)
{
    return count <= firstCount || (interval > 0 && count % interval == 0);
}

QString controlSummary(const MarineControlFrame& f)
{
    return QString("cmd=%1 az=%2 range=%3 tx=%4 gain=%5 level=%6 sea=%7 rain=%8 interference=%9 servoGear=%10 rpm=%11 checksum=0x%12")
        .arg(marineControlCmdLabel(f.cmdNum))
        .arg(f.azimuthDegrees(), 0, 'f', 2)
        .arg(static_cast<int>(f.rangeVal))
        .arg(static_cast<int>(f.txCtrl))
        .arg(static_cast<int>(f.gain))
        .arg(static_cast<int>(f.level))
        .arg(static_cast<int>(f.seaVal))
        .arg(static_cast<int>(f.rainVal))
        .arg(static_cast<int>(f.ganRao))
        .arg(static_cast<int>(f.servo))
        .arg(f.servoRpm(), 0, 'f', 2)
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

QString cleanAddressText(QString text)
{
    text = text.trimmed();
    if ((text.startsWith('"') && text.endsWith('"')) ||
        (text.startsWith('\'') && text.endsWith('\''))) {
        text = text.mid(1, text.length() - 2).trimmed();
    }
    return text;
}

constexpr uint32_t kMaxLogicalEchoFrameBytes =
    static_cast<uint32_t>(sizeof(MarineEchoHeader)) +
    65535U * 4U +
    static_cast<uint32_t>(MARINE_ECHO_FRAME_TAIL_SIZE);

uint32_t crc32IsoHdlcUpdate(uint32_t crc, const char* bytes, int length)
{
    for (int i = 0; i < length; ++i) {
        crc ^= static_cast<uint8_t>(bytes[i]);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
        }
    }
    return crc;
}

uint32_t fragmentCrc32(const QByteArray& datagram, int fragmentLength)
{
    uint32_t crc = 0xFFFFFFFFU;
    crc = crc32IsoHdlcUpdate(crc, datagram.constData(), 20);
    crc = crc32IsoHdlcUpdate(crc,
                             datagram.constData() + MARINE_FRAGMENT_HEADER_SIZE,
                             fragmentLength);
    return crc ^ 0xFFFFFFFFU;
}

QString missingFragmentIndexes(const QBitArray& receivedFragments, int maxCount = 16)
{
    QStringList indexes;
    int missingCount = 0;
    for (int i = 0; i < receivedFragments.size(); ++i) {
        if (receivedFragments.testBit(i)) {
            continue;
        }
        ++missingCount;
        if (indexes.size() < maxCount) {
            indexes.append(QString::number(i));
        }
    }

    if (missingCount == 0) {
        return QStringLiteral("none");
    }
    QString result = indexes.join(QLatin1Char(','));
    if (missingCount > indexes.size()) {
        result += QStringLiteral("...(+%1)").arg(missingCount - indexes.size());
    }
    return result;
}

} // namespace

MarineRadarManager::MarineRadarManager(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<MarineEchoLine>("MarineEchoLine");
    qRegisterMetaType<MarineRadarStatus>("MarineRadarStatus");
    m_fragmentClock.start();
    m_sweepAzimuthBins.fill(false, MARINE_ECHO_AZI_BINS);

    m_rxSocket = new QUdpSocket(this);
    m_txSocket = new QUdpSocket(this);
    m_autoSendTimer = new QTimer(this);
    auto* rxPumpTimer = new QTimer(this);

    connect(m_rxSocket, &QUdpSocket::readyRead, this, &MarineRadarManager::onReadyRead);
    connect(m_autoSendTimer, &QTimer::timeout, this, &MarineRadarManager::onAutoSend);
    connect(rxPumpTimer, &QTimer::timeout, this, [this]() {
        cleanupExpiredFragments();
        if (m_rxSocket && m_rxSocket->hasPendingDatagrams()) {
            onReadyRead();
        }
    });
    rxPumpTimer->start(20);

    auto* rxDebugTimer = new QTimer(this);
    connect(rxDebugTimer, &QTimer::timeout, this, [this]() {
        const bool echoDebug = CF_INS.marineDisplayBool("echo_debug", false);
        if (!echoDebug || !m_rxSocket)
            return;

        LOG_DEBUG(QString("[MarineRadar][SOCKET-DEBUG] state=%1 local=%2:%3 rxDatagrams=%4 rxEcho=%5 invalid=%6 pending=%7 pendingSize=%8 error=%9")
                  .arg(static_cast<int>(m_rxSocket->state()))
                  .arg(m_rxSocket->localAddress().toString())
                  .arg(m_rxSocket->localPort())
                  .arg(m_rxDatagramCount)
                  .arg(m_rxEchoCount)
                  .arg(m_rxInvalidCount)
                  .arg(m_rxSocket->hasPendingDatagrams() ? 1 : 0)
                  .arg(m_rxSocket->hasPendingDatagrams() ? m_rxSocket->pendingDatagramSize() : 0)
                  .arg(m_rxSocket->errorString()));
    });
    rxDebugTimer->start(5000);

    auto* fragmentStatsTimer = new QTimer(this);
    connect(fragmentStatsTimer, &QTimer::timeout, this, &MarineRadarManager::logFragmentStats);
    fragmentStatsTimer->start(5000);
    LOG_INFO("[MarineRadar] manager created");
}

MarineRadarManager::~MarineRadarManager()
{
    m_autoSendTimer->stop();
}

void MarineRadarManager::init(const QString& localIp, uint16_t localPort,
                              const QString& servoIp, uint16_t servoPort)
{
    const QString cleanLocalIp = cleanAddressText(localIp);
    const QString cleanServoIp = cleanAddressText(servoIp);
    const int configuredFragmentTimeoutMs =
        CF_INS.marineNetworkInt("fragment_timeout_ms", MARINE_FRAGMENT_REASSEMBLY_TIMEOUT_MS);
    m_fragmentReassemblyTimeoutMs = std::clamp(configuredFragmentTimeoutMs, 100, 5000);

    m_servoAddr = QHostAddress(cleanServoIp);
    m_servoPort = servoPort;
    LOG_INFO(QString("[MarineRadar][INIT] bind=%1:%2 servo=%3:%4")
             .arg(cleanLocalIp.isEmpty() ? QStringLiteral("0.0.0.0") : cleanLocalIp)
             .arg(localPort)
             .arg(cleanServoIp)
             .arg(servoPort));
    LOG_INFO(QString("[MarineRadar][INIT] echo_debug=%1 sweep_history_rounds=%2 fragment_timeout_ms=%3")
             .arg(CF_INS.marineDisplayBool("echo_debug", false) ? 1 : 0)
             .arg(CF_INS.marineDisplayInt("sweep_history_rounds", 1))
             .arg(m_fragmentReassemblyTimeoutMs));
    if (m_servoAddr.isNull()) {
        QString msg = QString("[MarineRadar][INIT] invalid servo_ip='%1'; UDP control send target is not usable")
                          .arg(cleanServoIp);
        LOG_ERROR(msg);
        emit logMessage(msg);
    }

    // 绑定接收端口
    if (m_rxSocket->state() == QAbstractSocket::BoundState)
        m_rxSocket->close();

    QHostAddress bindAddr = cleanLocalIp.isEmpty() ? QHostAddress::AnyIPv4 : QHostAddress(cleanLocalIp);
    if (!cleanLocalIp.isEmpty() && bindAddr.isNull()) {
        QString msg = QString("[MarineRadar][INIT] invalid local_ip='%1'; fallback bind address to 0.0.0.0")
                          .arg(cleanLocalIp);
        LOG_ERROR(msg);
        emit logMessage(msg);
        bindAddr = QHostAddress::AnyIPv4;
    }
    if (!m_rxSocket->bind(bindAddr, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        QString msg = QString("[MarineRadar][INIT] failed to bind %1:%2 - %3")
                          .arg(cleanLocalIp).arg(localPort).arg(m_rxSocket->errorString());
        LOG_ERROR(msg);
        emit logMessage(msg);
    } else {
        constexpr int kRequestedReceiveBufferBytes = 4 * 1024 * 1024;
        m_rxSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,
                                    kRequestedReceiveBufferBytes);
        const qint64 effectiveReceiveBufferBytes =
            m_rxSocket->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toLongLong();
        QString msg = QString("[MarineRadar][INIT] listening on %1:%2")
                          .arg(cleanLocalIp.isEmpty() ? QStringLiteral("0.0.0.0") : cleanLocalIp)
                          .arg(localPort);
        LOG_INFO(msg);
        emit logMessage(msg);
        LOG_INFO(QString("[MarineRadar][INIT] UDP receive buffer requested=%1 effective=%2")
                 .arg(kRequestedReceiveBufferBytes)
                 .arg(effectiveReceiveBufferBytes));
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
    // Use the bound receive socket for TX whenever possible so the radar sees
    // the configured echo_port as the UDP source port. Some devices reply to
    // the control packet source port; an unbound TX socket would use an
    // ephemeral port and move echo data away from the socket we actually read.
    QUdpSocket* sendSocket = (m_rxSocket && m_rxSocket->state() == QAbstractSocket::BoundState)
                             ? m_rxSocket
                             : m_txSocket;
    const qint64 written = sendSocket->writeDatagram(data, m_servoAddr, m_servoPort);
    ++m_txCount;

    if (written != data.size()) {
        LOG_WARNING(QString("[MarineRadar][TX] send failed #%1 written=%2 expected=%3 target=%4:%5 error=%6 %7")
                    .arg(m_txCount)
                    .arg(written)
                    .arg(data.size())
                    .arg(m_servoAddr.toString())
                    .arg(m_servoPort)
                    .arg(sendSocket->errorString())
                    .arg(controlSummary(f)));
    } else if (shouldLogSample(m_txCount, 5, 50)) {
        LOG_INFO(QString("[MarineRadar][TX] sent #%1 bytes=%2 src=%3:%4 target=%5:%6 %7")
                 .arg(m_txCount)
                 .arg(written)
                 .arg(sendSocket->localAddress().toString())
                 .arg(sendSocket->localPort())
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
    m_ctrl.servo = static_cast<uint8_t>(qMin<uint16_t>(speed, MARINE_SERVO_MAX_GEAR));
    sendControl(m_ctrl);
}

void MarineRadarManager::setCommand(uint8_t cmdNum)
{
    const uint8_t boundedCmd = qMin<uint8_t>(cmdNum, static_cast<uint8_t>(MarineCmdStart));
    LOG_INFO(QString("[MarineRadar][CTRL] setCommand=%1").arg(marineControlCmdLabel(boundedCmd)));
    m_ctrl.cmdNum = boundedCmd;
    sendControl(m_ctrl);
}

void MarineRadarManager::setAzimuthDegrees(double degrees)
{
    LOG_INFO(QString("[MarineRadar][CTRL] setAzimuth=%1").arg(degrees, 0, 'f', 2));
    m_ctrl.azimuth = marineEncodeAzimuth(degrees);
    sendControl(m_ctrl);
}

void MarineRadarManager::setServoGear(uint8_t gear)
{
    const uint8_t boundedGear = qMin<uint8_t>(gear, MARINE_SERVO_MAX_GEAR);
    LOG_INFO(QString("[MarineRadar][CTRL] setServoGear=%1 rpm=%2")
             .arg(static_cast<int>(boundedGear))
             .arg(marineServoGearRpm(boundedGear), 0, 'f', 2));
    m_ctrl.servo = boundedGear;
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

void MarineRadarManager::logRxSnapshot(const QString& reason) const
{
    const bool hasPending = m_rxSocket && m_rxSocket->hasPendingDatagrams();
    const qint64 pendingSize = hasPending ? m_rxSocket->pendingDatagramSize() : 0;
    LOG_DEBUG(QString("[MarineRadar][RX-SNAPSHOT] %1 state=%2 boundLocal=%3:%4 rx=%5 echo=%6 fragments=%7 reassembled=%8 inflight=%9 invalid=%10 pending=%11 pendingSize=%12 drainScheduled=%13 error=%14")
              .arg(reason)
              .arg(m_rxSocket ? static_cast<int>(m_rxSocket->state()) : -1)
              .arg(m_rxSocket ? m_rxSocket->localAddress().toString() : QStringLiteral("<null>"))
              .arg(m_rxSocket ? m_rxSocket->localPort() : 0)
              .arg(m_rxDatagramCount)
              .arg(m_rxEchoCount)
              .arg(m_rxFragmentCount)
              .arg(m_rxReassembledCount)
              .arg(m_fragmentAssemblies.size())
              .arg(m_rxInvalidCount)
              .arg(hasPending ? 1 : 0)
              .arg(pendingSize)
              .arg(m_rxDrainScheduled ? 1 : 0)
              .arg(m_rxSocket ? m_rxSocket->errorString() : QStringLiteral("<null>")));
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
    constexpr int kMaxDatagramsPerDrain = 256;
    constexpr qint64 kMaxDrainMs = 8;

    while (m_rxSocket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_rxSocket->receiveDatagram();
        const QByteArray data = datagram.data();
        ++m_rxDatagramCount;

        if (shouldLogSample(m_rxDatagramCount, 5, 512)) {
            LOG_DEBUG(QString("[MarineRadar][RX] datagram #%1 bytes=%2 from=%3:%4")
                      .arg(m_rxDatagramCount)
                      .arg(data.size())
                      .arg(datagram.senderAddress().toString())
                      .arg(datagram.senderPort()));
        }
        if (CF_INS.marineDisplayBool("echo_debug", false) && shouldLogSample(m_rxDatagramCount, 20, 128)) {
            LOG_DEBUG(QString("[MarineRadar][DATAGRAM-DEBUG] rxDatagram=%1 bytes=%2 from=%3:%4 first32=%5")
                      .arg(m_rxDatagramCount)
                      .arg(data.size())
                      .arg(datagram.senderAddress().toString())
                      .arg(datagram.senderPort())
                      .arg(firstBytesHex(data, 32)));
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
    cleanupExpiredFragments();
    if (data.size() >= 2 &&
        static_cast<uint8_t>(data[0]) == MARINE_FRAGMENT_MAGIC_0 &&
        static_cast<uint8_t>(data[1]) == MARINE_FRAGMENT_MAGIC_1) {
        parseFragmentDatagram(data);
        return;
    }

    parseLogicalEchoFrame(data);
}

void MarineRadarManager::parseFragmentDatagram(const QByteArray& data)
{
    if (data.size() < MARINE_FRAGMENT_HEADER_SIZE) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][FRAGMENT] short header invalidCount=%1 bytes=%2")
                        .arg(m_rxInvalidCount)
                        .arg(data.size()));
        }
        return;
    }

    const auto* hdr = reinterpret_cast<const MarineEchoFragmentHeader*>(data.constData());
    const uint16_t frameSeq = hdr->frameSequence();
    const uint16_t fragmentIndex = hdr->fragmentIndex();
    const uint16_t fragmentCount = hdr->fragmentCount();
    const uint32_t totalDataLen = hdr->totalDataLength();
    const uint32_t dataOffset = hdr->dataOffset();
    const uint16_t fragmentLen = hdr->fragmentLength();
    const bool isFirst = (hdr->flags & MARINE_FRAGMENT_FLAG_FIRST) != 0;
    const bool isLast = (hdr->flags & MARINE_FRAGMENT_FLAG_LAST) != 0;

    const bool basicValid = hdr->hasMagic() &&
                            hdr->version == MARINE_FRAGMENT_VERSION &&
                            hdr->isFlagsValid() &&
                            fragmentCount > 0 &&
                            fragmentIndex < fragmentCount &&
                            fragmentLen <= MARINE_FRAGMENT_MAX_DATA_SIZE &&
                            totalDataLen >= sizeof(MarineEchoHeader) + MARINE_ECHO_FRAME_TAIL_SIZE &&
                            totalDataLen <= kMaxLogicalEchoFrameBytes &&
                            dataOffset <= totalDataLen &&
                            fragmentLen <= totalDataLen - dataOffset &&
                            data.size() == MARINE_FRAGMENT_HEADER_SIZE + fragmentLen &&
                            ((fragmentIndex == 0) == isFirst) &&
                            ((fragmentIndex + 1 == fragmentCount) == isLast);
    if (!basicValid) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][FRAGMENT] invalid metadata invalidCount=%1 bytes=%2 seq=%3 index=%4/%5 total=%6 offset=%7 len=%8 flags=0x%9")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(frameSeq)
                        .arg(fragmentIndex)
                        .arg(fragmentCount)
                        .arg(totalDataLen)
                        .arg(dataOffset)
                        .arg(fragmentLen)
                        .arg(hdr->flags, 2, 16, QLatin1Char('0')));
        }
        return;
    }

    const uint32_t calculatedCrc = fragmentCrc32(data, fragmentLen);
    if (calculatedCrc != hdr->crc32()) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][FRAGMENT] CRC mismatch invalidCount=%1 seq=%2 index=%3 expected=0x%4 actual=0x%5")
                        .arg(m_rxInvalidCount)
                        .arg(frameSeq)
                        .arg(fragmentIndex)
                        .arg(hdr->crc32(), 8, 16, QLatin1Char('0'))
                        .arg(calculatedCrc, 8, 16, QLatin1Char('0')));
        }
        return;
    }

    ++m_rxFragmentCount;
    const qint64 nowMs = m_fragmentClock.elapsed();
    auto it = m_fragmentAssemblies.find(frameSeq);
    if (it == m_fragmentAssemblies.end()) {
        if (m_fragmentAssemblies.size() >= MARINE_FRAGMENT_MAX_INFLIGHT_FRAMES) {
            auto oldestIt = m_fragmentAssemblies.begin();
            for (auto candidate = m_fragmentAssemblies.begin(); candidate != m_fragmentAssemblies.end(); ++candidate) {
                if (candidate->lastUpdateMs < oldestIt->lastUpdateMs) {
                    oldestIt = candidate;
                }
            }
            discardFragmentAssembly(oldestIt.key(), QStringLiteral("in-flight limit"));
        }

        FragmentAssembly assembly;
        assembly.totalDataLen = totalDataLen;
        assembly.fragmentCount = fragmentCount;
        assembly.data.resize(static_cast<int>(totalDataLen));
        assembly.receivedFragments.fill(false, fragmentCount);
        assembly.receivedBytes.fill(false, static_cast<int>(totalDataLen));
        assembly.lastUpdateMs = nowMs;
        it = m_fragmentAssemblies.insert(frameSeq, assembly);
        ++m_fragmentFramesStarted;
    } else if (it->totalDataLen != totalDataLen || it->fragmentCount != fragmentCount) {
        discardFragmentAssembly(frameSeq, QStringLiteral("inconsistent totalDataLen or fragmentCount"));
        ++m_rxInvalidCount;
        return;
    }

    FragmentAssembly& assembly = it.value();
    assembly.lastUpdateMs = nowMs;
    if (assembly.receivedFragments.testBit(fragmentIndex)) {
        return; // Duplicate fragment: keep the first verified copy.
    }

    const int offset = static_cast<int>(dataOffset);
    for (int i = 0; i < fragmentLen; ++i) {
        if (assembly.receivedBytes.testBit(offset + i)) {
            discardFragmentAssembly(frameSeq, QStringLiteral("overlapping fragment data"));
            ++m_rxInvalidCount;
            return;
        }
    }

    if (fragmentLen > 0) {
        std::memcpy(assembly.data.data() + offset,
                    data.constData() + MARINE_FRAGMENT_HEADER_SIZE,
                    fragmentLen);
        for (int i = 0; i < fragmentLen; ++i) {
            assembly.receivedBytes.setBit(offset + i, true);
        }
    }
    assembly.receivedFragments.setBit(fragmentIndex, true);
    ++assembly.receivedFragmentCount;
    assembly.receivedByteCount += fragmentLen;

    const bool echoDebug = CF_INS.marineDisplayBool("echo_debug", false);
    if (echoDebug && shouldLogSample(m_rxFragmentCount, 20, 256)) {
        LOG_DEBUG(QString("[MarineRadar][FRAGMENT] seq=%1 index=%2/%3 offset=%4 len=%5 received=%6/%7 bytes=%8/%9")
                  .arg(frameSeq)
                  .arg(fragmentIndex)
                  .arg(fragmentCount)
                  .arg(dataOffset)
                  .arg(fragmentLen)
                  .arg(assembly.receivedFragmentCount)
                  .arg(assembly.fragmentCount)
                  .arg(assembly.receivedByteCount)
                  .arg(assembly.totalDataLen));
    }

    if (assembly.receivedFragmentCount != assembly.fragmentCount) {
        return;
    }
    if (assembly.receivedByteCount != static_cast<int>(assembly.totalDataLen)) {
        discardFragmentAssembly(frameSeq, QStringLiteral("fragments complete but logical data has gaps"));
        ++m_rxInvalidCount;
        return;
    }

    QByteArray logicalFrame = assembly.data;
    m_fragmentAssemblies.erase(it);
    ++m_rxReassembledCount;
    ++m_fragmentFramesCompleted;
    m_fragmentExpectedSettled += fragmentCount;
    m_fragmentReceivedSettled += fragmentCount;
    if (echoDebug) {
        LOG_DEBUG(QString("[MarineRadar][FRAGMENT] reassembled seq=%1 bytes=%2 fragments=%3")
                  .arg(frameSeq)
                  .arg(logicalFrame.size())
                  .arg(fragmentCount));
    }
    parseLogicalEchoFrame(logicalFrame);
}

void MarineRadarManager::cleanupExpiredFragments()
{
    if (!m_fragmentClock.isValid() || m_fragmentAssemblies.isEmpty()) {
        return;
    }

    const qint64 nowMs = m_fragmentClock.elapsed();
    for (auto it = m_fragmentAssemblies.begin(); it != m_fragmentAssemblies.end();) {
        if (nowMs - it->lastUpdateMs > m_fragmentReassemblyTimeoutMs) {
            const uint16_t frameSeq = it.key();
            const int received = it->receivedFragmentCount;
            const int total = it->fragmentCount;
            const int bytes = it->receivedByteCount;
            const int expected = static_cast<int>(it->totalDataLen);
            const int expectedFragments = it->fragmentCount;
            const QString missingIndexes = missingFragmentIndexes(it->receivedFragments);
            m_fragmentExpectedSettled += static_cast<uint64_t>(expectedFragments);
            m_fragmentReceivedSettled += static_cast<uint64_t>(received);
            ++m_fragmentFramesTimedOut;
            noteDroppedEchoForSweep();
            it = m_fragmentAssemblies.erase(it);
            ++m_rxInvalidCount;
            LOG_WARNING(QString("[MarineRadar][FRAGMENT] timeout seq=%1 received=%2/%3 bytes=%4/%5 missingIndexes=%6")
                        .arg(frameSeq)
                        .arg(received)
                        .arg(total)
                        .arg(bytes)
                        .arg(expected)
                        .arg(missingIndexes));
        } else {
            ++it;
        }
    }
}

void MarineRadarManager::discardFragmentAssembly(uint16_t frameSeq, const QString& reason)
{
    auto it = m_fragmentAssemblies.find(frameSeq);
    if (it == m_fragmentAssemblies.end()) {
        return;
    }

    LOG_WARNING(QString("[MarineRadar][FRAGMENT] discard seq=%1 reason=%2 received=%3/%4 bytes=%5/%6")
                .arg(frameSeq)
                .arg(reason)
                .arg(it->receivedFragmentCount)
                .arg(it->fragmentCount)
                .arg(it->receivedByteCount)
                .arg(it->totalDataLen));
    m_fragmentExpectedSettled += static_cast<uint64_t>(it->fragmentCount);
    m_fragmentReceivedSettled += static_cast<uint64_t>(it->receivedFragmentCount);
    ++m_fragmentFramesDiscarded;
    noteDroppedEchoForSweep();
    m_fragmentAssemblies.erase(it);
}

void MarineRadarManager::logFragmentStats() const
{
    const uint64_t settledFrames = m_fragmentFramesCompleted +
                                   m_fragmentFramesTimedOut +
                                   m_fragmentFramesDiscarded;
    if (m_fragmentFramesStarted == 0 && settledFrames == 0) {
        return;
    }

    const uint64_t missingFragments = m_fragmentExpectedSettled - m_fragmentReceivedSettled;
    const double frameCompleteRate = settledFrames > 0
        ? (100.0 * static_cast<double>(m_fragmentFramesCompleted) / static_cast<double>(settledFrames))
        : 0.0;
    const double fragmentMissingRate = m_fragmentExpectedSettled > 0
        ? (100.0 * static_cast<double>(missingFragments) / static_cast<double>(m_fragmentExpectedSettled))
        : 0.0;

    LOG_INFO(QString("[MarineRadar][FRAGMENT-STATS] started=%1 settled=%2 completed=%3 timeout=%4 discarded=%5 completeRate=%6%% expectedFragments=%7 receivedFragments=%8 missingFragments=%9 missingRate=%10%% inflight=%11 rxFragments=%12")
             .arg(m_fragmentFramesStarted)
             .arg(settledFrames)
             .arg(m_fragmentFramesCompleted)
             .arg(m_fragmentFramesTimedOut)
             .arg(m_fragmentFramesDiscarded)
             .arg(frameCompleteRate, 0, 'f', 2)
             .arg(m_fragmentExpectedSettled)
             .arg(m_fragmentReceivedSettled)
             .arg(missingFragments)
             .arg(fragmentMissingRate, 0, 'f', 2)
             .arg(m_fragmentAssemblies.size())
             .arg(m_rxFragmentCount));
}

void MarineRadarManager::noteValidEchoForSweep(uint16_t azimuthBin)
{
    const uint16_t normalizedBin = static_cast<uint16_t>(azimuthBin % MARINE_ECHO_AZI_BINS);
    if (!m_sweepActive) {
        m_sweepActive = true;
        m_sweepNumber = 1;
        resetSweepStats();
    } else {
        const int signedDelta = static_cast<int>(normalizedBin) - static_cast<int>(m_lastSweepAzimuth);
        if (signedDelta < -static_cast<int>(MARINE_ECHO_AZI_BINS / 2)) {
            logSweepStats(QStringLiteral("increasing"));
            ++m_sweepNumber;
            m_sweepSynchronized = true;
            resetSweepStats();
        } else if (signedDelta > static_cast<int>(MARINE_ECHO_AZI_BINS / 2)) {
            logSweepStats(QStringLiteral("decreasing"));
            ++m_sweepNumber;
            m_sweepSynchronized = true;
            resetSweepStats();
        }
    }

    ++m_sweepValidLines;
    m_sweepAzimuthBins.setBit(normalizedBin, true);
    m_lastSweepAzimuth = normalizedBin;
}

void MarineRadarManager::noteDroppedEchoForSweep()
{
    if (m_sweepActive) {
        ++m_sweepDroppedLines;
    } else {
        ++m_pendingDroppedLines;
    }
}

void MarineRadarManager::logSweepStats(const QString& direction)
{
    int uniqueAzimuthBins = 0;
    for (int i = 0; i < m_sweepAzimuthBins.size(); ++i) {
        if (m_sweepAzimuthBins.testBit(i)) {
            ++uniqueAzimuthBins;
        }
    }

    const uint32_t observedLines = m_sweepValidLines + m_sweepDroppedLines;
    const double completeRate = observedLines > 0
        ? (100.0 * static_cast<double>(m_sweepValidLines) / static_cast<double>(observedLines))
        : 0.0;
    const double azimuthCoverage = 100.0 * static_cast<double>(uniqueAzimuthBins) /
                                   static_cast<double>(MARINE_ECHO_AZI_BINS);
    const qint64 durationMs = m_sweepClock.isValid() ? m_sweepClock.elapsed() : 0;
    const double lineRateHz = durationMs > 0
        ? (1000.0 * static_cast<double>(observedLines) / static_cast<double>(durationMs))
        : 0.0;

    // 8192 is the azimuth-number address space, not a protocol promise of 8192 FFT lines per turn.
    LOG_INFO(QString("[MarineRadar][SWEEP-STATS] sweep=%1 direction=%2 partial=%3 durationMs=%4 sourceExpectedLines=not-declared validLines=%5 droppedLines=%6 observedLines=%7 lineRateHz=%8 completeRate=%9%% uniqueAzimuthBins=%10 azimuthBinCapacity=%11 azimuthCoverage=%12%%")
             .arg(m_sweepNumber)
             .arg(direction)
             .arg(m_sweepSynchronized ? 0 : 1)
             .arg(durationMs)
             .arg(m_sweepValidLines)
             .arg(m_sweepDroppedLines)
             .arg(observedLines)
             .arg(lineRateHz, 0, 'f', 2)
             .arg(completeRate, 0, 'f', 2)
             .arg(uniqueAzimuthBins)
             .arg(MARINE_ECHO_AZI_BINS)
             .arg(azimuthCoverage, 0, 'f', 2));
}

void MarineRadarManager::resetSweepStats()
{
    m_sweepValidLines = 0;
    m_sweepDroppedLines = m_pendingDroppedLines;
    m_pendingDroppedLines = 0;
    m_sweepAzimuthBins.fill(false, MARINE_ECHO_AZI_BINS);
    m_sweepClock.restart();
}

void MarineRadarManager::parseLogicalEchoFrame(const QByteArray& data)
{
    constexpr int HEADER_SIZE = static_cast<int>(sizeof(MarineEchoHeader));

    if (data.size() < HEADER_SIZE + MARINE_ECHO_FRAME_TAIL_SIZE) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] short logical frame invalidCount=%1 bytes=%2 expectedAtLeast=%3 head=%4")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(HEADER_SIZE + MARINE_ECHO_FRAME_TAIL_SIZE)
                        .arg(firstBytesHex(data, 16)));
        }
        return;
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

    if (!hdr->isStatusValid()) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] status checksum mismatch invalidCount=%1 %2")
                        .arg(m_rxInvalidCount)
                        .arg(echoSummary(*hdr)));
        }
        return;
    }

    if (!hdr->isStyleValid() || !hdr->isRangeValid() || !hdr->isEchoLengthValid()) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] invalid echo metadata invalidCount=%1 bytes=%2 %3")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(echoSummary(*hdr)));
        }
        return;
    }

    const int echoBytes = hdr->echoByteCount();
    const int expectedSize = HEADER_SIZE + echoBytes + MARINE_ECHO_FRAME_TAIL_SIZE;
    const bool hasFrameTail = data.size() >= MARINE_ECHO_FRAME_TAIL_SIZE &&
                              static_cast<uint8_t>(data[data.size() - 2]) == MARINE_ECHO_FRAME_TAIL_HIGH &&
                              static_cast<uint8_t>(data[data.size() - 1]) == MARINE_ECHO_FRAME_TAIL_LOW;
    if (data.size() != expectedSize || !hasFrameTail) {
        ++m_rxInvalidCount;
        if (shouldLogSample(m_rxInvalidCount, 5, 100)) {
            LOG_WARNING(QString("[MarineRadar][RX] invalid logical frame length/tail invalidCount=%1 bytes=%2 expected=%3 tail=%4 %5")
                        .arg(m_rxInvalidCount)
                        .arg(data.size())
                        .arg(expectedSize)
                        .arg(data.size() >= 2 ? firstBytesHex(data.right(2), 2) : QStringLiteral("<short>"))
                        .arg(echoSummary(*hdr)));
        }
        return;
    }

    noteValidEchoForSweep(hdr->azimuthRaw());

    // Only publish device state after the complete logical frame is verified.
    MarineRadarStatus st;
    st.rangeCode  = hdr->rangeCode;
    st.txOn       = (hdr->txState != 0);
    st.gain       = hdr->gain;
    st.level      = hdr->level;
    st.seaVal     = hdr->seaVal;
    st.rainVal    = hdr->rainVal;
    st.ganRao     = hdr->ganRao;
    st.freqStatus = hdr->freqStatus;

    const bool statusChanged = (st.rangeCode != m_status.rangeCode ||
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

    const uint16_t renderIdx = hdr->azimuthRenderIndex();
    const bool azimuthChanged = renderIdx != m_lastDebugRenderIdx;
    if (azimuthChanged) {
        m_lastDebugRenderIdx = renderIdx;
        ++m_debugAziChangeCount;
    }

    MarineEchoLine line;
    line.azimuthRaw = renderIdx;
    line.azimuthDeg = hdr->azimuthDeg();
    line.style      = hdr->style;
    line.packetNum  = hdr->packetNumber();
    line.rangeCode  = hdr->rangeCode;
    line.sourceRangeMeters = marineRangeMeters(hdr->rangeCode);

    // Copy amplitudes. The echo payload is one little-endian uint32 magnitude
    // per range cell; the renderer consumes 8-bit amplitudes.
    const int cellCount = hdr->rangeCellCount();
    line.amplitudes.resize(cellCount);
    const uint8_t* echoData = reinterpret_cast<const uint8_t*>(data.constData()) + HEADER_SIZE;
    int activeCount = 0;
    int maxAmp = 0;
    uint32_t ampHash = 2166136261u;
    for (int i = 0; i < cellCount; ++i) {
        const int offset = i * 4;
        const uint32_t magnitude = static_cast<uint32_t>(echoData[offset]) |
                                   (static_cast<uint32_t>(echoData[offset + 1]) << 8) |
                                   (static_cast<uint32_t>(echoData[offset + 2]) << 16) |
                                   (static_cast<uint32_t>(echoData[offset + 3]) << 24);
        const uint8_t amp = static_cast<uint8_t>(qMin<uint32_t>(magnitude, 255U));
        line.amplitudes[i] = amp;
        if (amp >= 16)
            ++activeCount;
        maxAmp = qMax(maxAmp, static_cast<int>(amp));
        ampHash ^= amp;
        ampHash *= 16777619u;
    }

    ++m_rxEchoCount;
    const bool echoDebug = CF_INS.marineDisplayBool("echo_debug", false);
    const bool packetSample = shouldLogSample(m_rxEchoCount, 20, 128);
    const bool forcedSample = (m_rxEchoCount <= 20) || ((m_rxEchoCount % 8192) == 0);
    const bool angleSample = azimuthChanged &&
                             (m_debugAziChangeCount <= 256 || (m_debugAziChangeCount % 64) == 0);
    if (echoDebug && (packetSample || forcedSample || angleSample)) {
        LOG_DEBUG(QString("[MarineRadar][ECHO-DEBUG] rxEcho=%1 protoBin=%2 angle=%3 renderIdx=%4 angleChanged=%5 angleChanges=%6 packet=%7 cells=%8 active=%9 maxAmp=%10 ampHash=0x%11 range=%12 firstAmp=%13 lastAmp=%14")
                  .arg(m_rxEchoCount)
                  .arg(hdr->azimuthRaw())
                  .arg(hdr->azimuthDeg(), 0, 'f', 3)
                  .arg(renderIdx)
                  .arg(azimuthChanged ? 1 : 0)
                  .arg(m_debugAziChangeCount)
                  .arg(hdr->packetNumber())
                  .arg(cellCount)
                  .arg(activeCount)
                  .arg(maxAmp)
                  .arg(ampHash, 8, 16, QLatin1Char('0'))
                  .arg(static_cast<int>(hdr->rangeCode))
                  .arg(cellCount > 0 ? static_cast<int>(line.amplitudes.first()) : -1)
                  .arg(cellCount > 0 ? static_cast<int>(line.amplitudes.last()) : -1));
    } else if (shouldLogSample(m_rxEchoCount, 5, 512)) {
        LOG_DEBUG(QString("[MarineRadar][RX] echo #%1 bytes=%2 %3")
                  .arg(m_rxEchoCount)
                  .arg(data.size())
                  .arg(echoSummary(*hdr)));
    }

    emit echoLineReceived(line);
}
