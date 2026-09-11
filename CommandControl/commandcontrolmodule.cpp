/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:18:30
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
#include "commandcontrolmodule.h"

#include "commandcontrolrecordwriter.h"
#include "commandcontrolwindow.h"
#include "commandcontroltransport.h"

#include "Basic/ConfigManager.h"
#include "Basic/log.h"
#include "Basic/wgs84coordinate.h"

#include <QDateTime>
#include <QStringList>
#include <QTimeZone>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kDegToRad = M_PI / 180.0;
constexpr double kVelocityResolution = 0.15;
constexpr int kPeerOfflineIntervalMs = 15000;
constexpr int kUiRefreshIntervalMs = 200;

QDateTime beijingNow()
{
    return QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(QByteArrayLiteral("Asia/Shanghai")));
}

quint32 headerDayTicks10Ms(const QDateTime& now)
{
    const QDateTime start(now.date(), QTime(0, 0), now.timeZone());
    return static_cast<quint32>(start.msecsTo(now) / 10);
}

quint32 monthTimestampMs(const QDateTime& now)
{
    const QDate startDate(now.date().year(), now.date().month(), 1);
    const QDateTime start(startDate, QTime(0, 0), now.timeZone());
    return static_cast<quint32>(start.msecsTo(now));
}

QString ipv4ToString(quint32 lowFirst)
{
    return QStringLiteral("%1.%2.%3.%4")
        .arg(lowFirst & 0xFFU)
        .arg((lowFirst >> 8) & 0xFFU)
        .arg((lowFirst >> 16) & 0xFFU)
        .arg((lowFirst >> 24) & 0xFFU);
}

quint32 ipv4LowFirst(const QHostAddress& address)
{
    const QStringList parts = address.toString().split(QLatin1Char('.'));
    if (parts.size() != 4) {
        return 0;
    }
    quint32 value = 0;
    for (int index = 0; index < parts.size(); ++index) {
        bool ok = false;
        const uint octet = parts.at(index).toUInt(&ok);
        if (!ok || octet > 255U) {
            return 0;
        }
        value |= static_cast<quint32>(octet) << (index * 8);
    }
    return value;
}

qint16 quantizeVelocity(double value)
{
    if (!std::isfinite(value)) {
        return std::numeric_limits<qint16>::max();
    }
    const qint64 raw = qRound64(value / kVelocityResolution);
    if (raw < std::numeric_limits<qint16>::min() || raw >= std::numeric_limits<qint16>::max()) {
        return std::numeric_limits<qint16>::max();
    }
    return static_cast<qint16>(raw);
}

bool validIp(const QHostAddress& address)
{
    return address.protocol() == QAbstractSocket::IPv4Protocol
        && address != QHostAddress(QHostAddress::AnyIPv4)
        && address != QHostAddress(QHostAddress::Broadcast);
}

double normalizeNorthAngle(double angleDeg)
{
    double normalized = std::fmod(angleDeg, 360.0);
    return normalized < 0.0 ? normalized + 360.0 : normalized;
}

QString hexValue(quint32 value, int width)
{
    return QStringLiteral("0x%1").arg(value, width, 16, QChar('0'));
}

QString headerFields(const CommandControlProtocol::Header& header)
{
    return QStringLiteral("报头{标识=%1,版本=%2,发端ID=%3,收端ID=%4,标志=%5,类型=%6,流水=%7,当日10ms=%8}")
        .arg(hexValue(header.magic, 4))
        .arg(hexValue(header.version, 2))
        .arg(hexValue(header.senderId, 8))
        .arg(hexValue(header.receiverId, 8))
        .arg(hexValue(header.flags, 2))
        .arg(hexValue(header.messageType, 4))
        .arg(header.sequence)
        .arg(header.dayTicks10Ms);
}

QString loginIdentityText(CommandControlProtocol::LoginIdentity identity)
{
    switch (identity) {
    case CommandControlProtocol::LoginIdentity::ManagementNodeNotice: return QStringLiteral("管理节点通报(0x01)");
    case CommandControlProtocol::LoginIdentity::Relogin: return QStringLiteral("重新登陆(0x02)");
    case CommandControlProtocol::LoginIdentity::Logout: return QStringLiteral("退出登陆(0x03)");
    }
    return QStringLiteral("未知");
}

QString loginRequestTypeText(CommandControlProtocol::LoginRequestType type)
{
    switch (type) {
    case CommandControlProtocol::LoginRequestType::Default: return QStringLiteral("默认(0x00)");
    case CommandControlProtocol::LoginRequestType::Login: return QStringLiteral("登陆(0x01)");
    case CommandControlProtocol::LoginRequestType::Logout: return QStringLiteral("退出登陆(0x02)");
    }
    return QStringLiteral("未知");
}

QString managementNodeFields(const CommandControlProtocol::ManagementNode& node)
{
    return QStringLiteral("负载{总控ID=%1,登陆身份=%2,IP=%3,端口=%4,备份=%5}")
        .arg(hexValue(node.controlId, 8))
        .arg(loginIdentityText(node.identity))
        .arg(ipv4ToString(node.ipv4LowFirst))
        .arg(node.port)
        .arg(QString::fromLatin1(node.reserve.toHex(' ')));
}

QString loginRequestFields(const CommandControlProtocol::LoginRequestPayload& request)
{
    return QStringLiteral("负载{用户ID=%1,请求方IP=%2,端口=%3,类型=%4,时间=%5-%6-%7 %8:%9:%10.%11}")
        .arg(hexValue(request.userId, 8))
        .arg(ipv4ToString(request.ipv4LowFirst))
        .arg(request.port)
        .arg(loginRequestTypeText(request.type))
        .arg(request.year, 4, 10, QChar('0'))
        .arg(request.month, 2, 10, QChar('0'))
        .arg(request.day, 2, 10, QChar('0'))
        .arg(request.hour, 2, 10, QChar('0'))
        .arg(request.minute, 2, 10, QChar('0'))
        .arg(request.second, 2, 10, QChar('0'))
        .arg(request.millisecond, 3, 10, QChar('0'));
}

QString loginReplyFields(const CommandControlProtocol::LoginReplyPayload& reply)
{
    return QStringLiteral("负载{结果=%1,拒绝原因=%2,主控ID=%3}")
        .arg(hexValue(reply.result, 2))
        .arg(hexValue(reply.rejectReason, 2))
        .arg(hexValue(reply.controlId, 8));
}

QString linkCheckFields(const CommandControlProtocol::LinkCheckPayload& linkCheck)
{
    return QStringLiteral("负载{本月毫秒时间戳=%1,协同参与状态=%2}")
        .arg(linkCheck.monthTimestampMs)
        .arg(hexValue(linkCheck.cooperationStatus, 2));
}

QString dda4Fields(const CommandControlProtocol::Dda4Track& track)
{
    return QStringLiteral("负载{目标综合批号=%1,本机设置批号=%2,本机ID=%3,设备类型=%4,设备编号=%5,数据率厘Hz=%6,目标属性=%7,目标类型=%8,备份=%9,航迹质量=%10,经度E7=%11,纬度E7=%12,高度m=%13,X东向速率原始=%14,Y北向速率原始=%15,Z天向速率原始=%16,RCS毫平方米=%17,干扰状态=%18,更新方式=%19,相对延时ms=%20,本月毫秒时间戳=%21}")
        .arg(track.comprehensiveBatch).arg(track.localBatch).arg(hexValue(track.deviceId, 8))
        .arg(hexValue(track.deviceType, 2)).arg(track.deviceNumber).arg(track.rateCentiHz)
        .arg(hexValue(track.targetAttribute, 2)).arg(hexValue(track.targetType, 2))
        .arg(hexValue(track.reserve, 4)).arg(hexValue(track.trackQuality, 2))
        .arg(track.longitudeE7).arg(track.latitudeE7).arg(track.altitudeM)
        .arg(track.velocityEast).arg(track.velocityNorth).arg(track.velocityUp)
        .arg(track.rcsMilliSquareM).arg(hexValue(track.interferenceStatus, 2))
        .arg(hexValue(track.updateMethod, 2)).arg(track.relativeDelayMs).arg(track.monthTimestampMs);
}

QString dda1Fields(const CommandControlProtocol::Dda1Status& status)
{
    return QStringLiteral("负载{经度E7=%1,纬度E7=%2,高度m=%3,工作状态=%4,健康状态=%5,设备个数=%6,类型=%7,设备编号=%8,设备状态=%9,工作模式=%10,辐射状态=%11,方位起始=%12,方位终止=%13,俯仰起始=%14,俯仰终止=%15,备份=%16}")
        .arg(status.longitudeE7).arg(status.latitudeE7).arg(status.altitudeM)
        .arg(hexValue(status.workStatus, 2)).arg(hexValue(status.healthStatus, 2))
        .arg(status.deviceCount).arg(hexValue(status.deviceType, 2)).arg(status.deviceNumber)
        .arg(hexValue(status.deviceStatus, 2)).arg(hexValue(status.workMode, 2))
        .arg(status.radiationStatus).arg(status.azimuthStartDeg).arg(status.azimuthEndDeg)
        .arg(status.elevationStartDeg).arg(status.elevationEndDeg).arg(hexValue(status.reserve, 4));
}

}  // namespace

CommandControlModule::CommandControlModule(QObject* parent)
    : QObject(parent)
{
    m_linkTimer.setSingleShot(false);
    m_equipmentStatusTimer.setSingleShot(false);
    m_loginRetryTimer.setSingleShot(true);
    m_peerLivenessTimer.setSingleShot(false);
    m_replayTimer.setSingleShot(false);
    m_peerRefreshTimer.setSingleShot(true);
    m_recordRefreshTimer.setSingleShot(true);

    connect(&m_linkTimer, &QTimer::timeout, this, &CommandControlModule::sendLinkCheck);
    connect(&m_equipmentStatusTimer, &QTimer::timeout, this, &CommandControlModule::sendEquipmentStatus);
    connect(&m_loginRetryTimer, &QTimer::timeout, this, &CommandControlModule::retryLogin);
    connect(&m_peerLivenessTimer, &QTimer::timeout, this, &CommandControlModule::updatePeerLiveness);
    connect(&m_replayTimer, &QTimer::timeout, this, &CommandControlModule::emitNextReplayPoint);
    connect(&m_peerRefreshTimer, &QTimer::timeout, this, &CommandControlModule::peersChanged);
    connect(&m_recordRefreshTimer, &QTimer::timeout, this, &CommandControlModule::recordsChanged);
}

CommandControlModule::~CommandControlModule()
{
    stopNetwork();
    delete m_window;
    m_window = nullptr;
}

bool CommandControlModule::init()
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    qRegisterMetaType<CommandControlRecord>("CommandControlRecord");
    qRegisterMetaType<QVector<CommandControlRecord>>("QVector<CommandControlRecord>");
    m_settings = CommandControlConfig::load();
    m_autoReportEnabled = m_settings.autoReportEnabled;
    m_radiationStatus = m_settings.radiationStatus;
    if (!m_settings.enabled) {
        setStatus(QStringLiteral("总控通信已由配置关闭"));
        return true;
    }
    if (m_settings.recordEnabled) {
        startRecordWriter();
    }
    startNetwork();
    return true;
}

bool CommandControlModule::isManualReportActive(quint32 sourceBatch) const
{
    return m_manualBatches.contains(sourceBatch);
}

QList<CommandControlPeer> CommandControlModule::peers() const
{
    return m_peers.values();
}

bool CommandControlModule::startNetwork()
{
    m_transport = new CommandControlTransport();
    m_transport->moveToThread(&m_transportThread);
    connect(&m_transportThread, &QThread::finished, m_transport, &QObject::deleteLater);
    connect(m_transport, &CommandControlTransport::started, this, &CommandControlModule::onTransportStarted);
    connect(m_transport, &CommandControlTransport::datagramReceived, this, &CommandControlModule::handleDatagram);
    connect(m_transport, &CommandControlTransport::datagramSent, this, &CommandControlModule::onTransportSendResult);
    m_transportThread.start();
    CommandControlTransport* transport = m_transport;
    const CommandControlSettings settings = m_settings;
    QMetaObject::invokeMethod(transport, [transport, settings]() { transport->start(settings); }, Qt::QueuedConnection);
    return true;
}

void CommandControlModule::startRecordWriter()
{
    if (m_recordWriter) {
        return;
    }

    m_recordWriter = new CommandControlRecordWriter();
    m_recordWriter->moveToThread(&m_recordThread);
    connect(&m_recordThread, &QThread::finished, m_recordWriter, &QObject::deleteLater);
    connect(m_recordWriter, &CommandControlRecordWriter::sessionOpened,
            this, &CommandControlModule::onRecordSessionOpened);
    connect(m_recordWriter, &CommandControlRecordWriter::writeFailed,
            this, &CommandControlModule::onRecordWriteFailed);
    connect(m_recordWriter, &CommandControlRecordWriter::recordsLoaded,
            this, &CommandControlModule::onReplayRecordsLoaded);

    m_recordThread.start();
    CommandControlRecordWriter* writer = m_recordWriter;
    const QString directory = m_settings.recordDirectory;
    QMetaObject::invokeMethod(writer, [writer, directory]() { writer->startSession(directory); },
                              Qt::QueuedConnection);
}

void CommandControlModule::stopRecordWriter()
{
    if (!m_recordWriter) {
        return;
    }

    if (m_recordThread.isRunning()) {
        CommandControlRecordWriter* writer = m_recordWriter;
        QMetaObject::invokeMethod(writer, [writer]() { writer->stop(); }, Qt::BlockingQueuedConnection);
        m_recordThread.quit();
        m_recordThread.wait();
    }
    m_recordWriter = nullptr;
    m_sessionRecordPath.clear();
}

void CommandControlModule::onTransportStarted(bool success, const QString& detail)
{
    if (!m_transport) {
        return;
    }
    if (!success) {
        setStatus(detail);
        return;
    }
    LOG_INFO(QString("[CommandControl][NET] %1 bind=0.0.0.0:%2 multicast=%3:%4 localIp=%5")
             .arg(detail)
             .arg(m_settings.localPort)
             .arg(m_settings.multicastGroup)
             .arg(m_settings.multicastPort)
             .arg(m_settings.localIp));
    m_ready = true;
    m_linkTimer.start(m_settings.linkIntervalMs);
    m_equipmentStatusTimer.start(m_settings.equipmentStatusIntervalMs);
    m_peerLivenessTimer.start(1000);
    sendLinkCheck();
    sendEquipmentStatus();
    setStatus(QStringLiteral("总控通信就绪，等待 DD31 管理节点通报"));
    emit readyChanged(true);
}

void CommandControlModule::onRecordSessionOpened(bool success, const QString& filePath, const QString& detail)
{
    if (!success) {
        LOG_WARNING(QString("[CommandControl][RECORD] %1").arg(detail));
        return;
    }
    m_sessionRecordPath = filePath;
    LOG_INFO(QString("[CommandControl][RECORD] 独立记录线程已打开会话：%1").arg(filePath));
}

void CommandControlModule::onRecordWriteFailed(const QString& detail)
{
    LOG_WARNING(QString("[CommandControl][RECORD] %1").arg(detail));
}

void CommandControlModule::onReplayRecordsLoaded(const QVector<CommandControlRecord>& records,
                                                  const QString& detail)
{
    if (records.isEmpty()) {
        setStatus(detail.isEmpty() ? QStringLiteral("未加载到可回放记录") : detail);
        return;
    }
    m_replayRecords = records;
    m_replayIndex = 0;
    m_replayBatches.clear();
    m_nextReplayBatch = 0x80000000U;
    m_replayTimer.start(m_settings.replayIntervalMs);
    setStatus(QStringLiteral("开始回放 %1 条 DDA4 记录").arg(m_replayRecords.size()));
}

void CommandControlModule::onTransportSendResult(quint16 messageType, bool success, const QString& detail)
{
    if (success) {
        return;
    }
    const QString message = QStringLiteral("%1 发送失败：%2")
        .arg(QString::fromLatin1(CommandControlProtocol::messageTypeName(messageType)), detail);
    LOG_WARNING(QString("[CommandControl][TX] %1").arg(message));
    if (messageType == CommandControlProtocol::LoginRequest) {
        setStatus(message);
    }
}

void CommandControlModule::stopNetwork()
{
    m_linkTimer.stop();
    m_equipmentStatusTimer.stop();
    m_loginRetryTimer.stop();
    m_peerLivenessTimer.stop();
    m_replayTimer.stop();
    m_peerRefreshTimer.stop();
    m_recordRefreshTimer.stop();
    stopRecordWriter();
    if (m_transport) {
        if (m_transportThread.isRunning()) {
            CommandControlTransport* transport = m_transport;
            QMetaObject::invokeMethod(transport, [transport]() { transport->stop(); }, Qt::BlockingQueuedConnection);
            m_transportThread.quit();
            m_transportThread.wait();
        }
        m_transport = nullptr;
    }
    m_ready = false;
    emit readyChanged(false);
}

void CommandControlModule::setStatus(const QString& text)
{
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    LOG_INFO(QString("[CommandControl][STATUS] %1").arg(text));
    emit statusChanged(text);
}

CommandControlProtocol::Header CommandControlModule::nextHeader(quint16 type, quint32 receiverId,
                                                                 bool requestReceipt, quint8 priority)
{
    const QDateTime now = beijingNow();
    CommandControlProtocol::Header header;
    header.senderId = m_settings.deviceId;
    header.receiverId = receiverId;
    header.flags = static_cast<quint8>((requestReceipt ? 0x80U : 0U) | (priority & 0x1FU));
    header.messageType = type;
    header.sequence = m_sequence++;
    header.dayTicks10Ms = headerDayTicks10Ms(now);
    return header;
}

bool CommandControlModule::isCurrentControl(quint32 deviceId) const
{
    return m_controlEndpoint.isValid() && deviceId == m_controlEndpoint.id;
}

void CommandControlModule::logControlPacket(const QString& direction,
                                            const CommandControlProtocol::Header& header,
                                            const QString& endpoint, const QString& fields,
                                            const QString& uiSummary)
{
    const QString typeName = QString::fromLatin1(CommandControlProtocol::messageTypeName(header.messageType));
    LOG_INFO(QString("[CommandControl][%1][%2] endpoint=%3 %4 %5")
             .arg(direction)
             .arg(typeName)
             .arg(endpoint)
             .arg(headerFields(header))
             .arg(fields));
    // UI 仅展示登录、端点变更、DDA1 状态等排障要点；DD25 仍保留完整落盘日志但不刷屏。
    if (!uiSummary.isEmpty()) {
        emit diagnosticLog(QStringLiteral("[%1][%2] %3")
                           .arg(direction)
                           .arg(typeName)
                           .arg(uiSummary));
    }
}

void CommandControlModule::sendMulticast(const QByteArray& packet, quint16 type)
{
    if (!m_transport) {
        return;
    }
    CommandControlTransport* transport = m_transport;
    const QHostAddress address(m_settings.multicastGroup);
    const quint16 port = m_settings.multicastPort;
    QMetaObject::invokeMethod(transport, [transport, packet, address, port, type]() {
        transport->sendDatagram(packet, address, port, type);
    }, Qt::QueuedConnection);
    if (type != CommandControlProtocol::SituationIntelligence) {
        logPacketHex(QStringLiteral("TX"), type, packet,
                     QStringLiteral("%1:%2").arg(m_settings.multicastGroup).arg(m_settings.multicastPort));
    }
}

void CommandControlModule::sendUnicast(const QByteArray& packet, quint16 type)
{
    if (!m_controlEndpoint.isValid()) {
        setStatus(QStringLiteral("未发现有效 DD31 管理节点，不能发送单播报文"));
        return;
    }
    if (!m_transport) {
        return;
    }
    const QHostAddress address = m_controlEndpoint.address;
    const quint16 port = m_controlEndpoint.port;
    CommandControlTransport* transport = m_transport;
    QMetaObject::invokeMethod(transport, [transport, packet, address, port, type]() {
        transport->sendDatagram(packet, address, port, type);
    }, Qt::QueuedConnection);
    logPacketHex(QStringLiteral("TX"), type, packet,
                 QStringLiteral("%1:%2").arg(m_controlEndpoint.address.toString()).arg(m_controlEndpoint.port));
}

void CommandControlModule::handleDatagram(const QByteArray& packet, const QHostAddress& sender, quint16 senderPort)
{
    CommandControlProtocol::Header header;
    if (!CommandControlProtocol::parseHeader(packet, header)) {
        LOG_WARNING(QString("[CommandControl][RX] 丢弃非法报头：from=%1:%2 bytes=%3")
                    .arg(sender.toString()).arg(senderPort).arg(packet.size()));
        return;
    }
    const int expectedSize = CommandControlProtocol::expectedPacketSize(header.messageType);
    if (expectedSize > 0 && packet.size() != expectedSize) {
        LOG_WARNING(QString("[CommandControl][RX] 丢弃长度异常 %1：from=%2:%3 got=%4 expected=%5")
                    .arg(QString::fromLatin1(CommandControlProtocol::messageTypeName(header.messageType)))
                    .arg(sender.toString()).arg(senderPort).arg(packet.size()).arg(expectedSize));
        return;
    }
    switch (header.messageType) {
    case CommandControlProtocol::ManagementNodeNotice:
        handleManagementNode(packet, header, sender, senderPort);
        break;
    case CommandControlProtocol::LoginRequest:
        handleLoginRequest(packet, header, sender, senderPort);
        break;
    case CommandControlProtocol::LoginReply:
        handleLoginReply(packet, header, sender, senderPort);
        break;
    case CommandControlProtocol::LinkCheck:
        handleLinkCheck(packet, header, sender, senderPort);
        break;
    case CommandControlProtocol::SituationIntelligence:
        handleIncomingDda4(packet, header, sender, senderPort);
        break;
    case CommandControlProtocol::EquipmentStatus:
        handleIncomingDda1(packet, header, sender, senderPort);
        break;
    default:
        LOG_INFO(QString("[CommandControl][RX] %1 from=%2:%3 bytes=%4")
                 .arg(QString::fromLatin1(CommandControlProtocol::messageTypeName(header.messageType)))
                 .arg(sender.toString()).arg(senderPort).arg(packet.size()));
        break;
    }
}

void CommandControlModule::handleManagementNode(const QByteArray& packet,
                                                const CommandControlProtocol::Header& header,
                                                const QHostAddress& sender, quint16 senderPort)
{
    CommandControlProtocol::ManagementNode node;
    if (!CommandControlProtocol::parseManagementNode(packet, node)) {
        LOG_WARNING(QString("[CommandControl][DD31] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    const QString nodeIp = ipv4ToString(node.ipv4LowFirst);
    const QHostAddress nodeAddress(nodeIp);
    if (node.controlId == 0 || !validIp(nodeAddress) || node.port == 0) {
        LOG_WARNING(QString("[CommandControl][DD31] 无效节点 id=0x%1 ip=%2 port=%3")
                    .arg(node.controlId, 8, 16, QChar('0')).arg(nodeIp).arg(node.port));
        return;
    }

    const bool changed = m_controlEndpoint.id != node.controlId
                      || m_controlEndpoint.address != nodeAddress
                      || m_controlEndpoint.port != node.port;
    if (changed) {
        m_loggedIn = false;
        m_logoutRequested = false;
    }
    m_controlEndpoint = {node.controlId, nodeAddress, node.port};
    updatePeer(node.controlId, nodeAddress, node.port, QStringLiteral("管理节点"), true);
    if (!m_settings.expectedControlIp.isEmpty() && sender.toString() != m_settings.expectedControlIp) {
        LOG_WARNING(QString("[CommandControl][DD31] 来源 IP=%1 与配置期望总控 IP=%2 不同，已按有效报文更新")
                    .arg(sender.toString(), m_settings.expectedControlIp));
    }

    logControlPacket(QStringLiteral("RX"), header,
                     QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                     managementNodeFields(node),
                     changed ? QStringLiteral("DD31 更新总控端点：%1:%2，%3")
                                   .arg(nodeIp).arg(node.port).arg(loginIdentityText(node.identity))
                             : QString());
    logPacketHex(QStringLiteral("RX"), header.messageType, packet,
                 QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort));
    if (node.identity == CommandControlProtocol::LoginIdentity::Logout) {
        if (changed || !m_logoutRequested) {
            beginLogin(CommandControlProtocol::LoginRequestType::Logout, false);
            m_logoutRequested = true;
        }
    } else if (node.identity == CommandControlProtocol::LoginIdentity::Relogin
               || changed || (!m_loggedIn && !m_waitingForLoginReply && !m_loginRetryExhausted)) {
        m_logoutRequested = false;
        beginLogin(CommandControlProtocol::LoginRequestType::Login, true);
    }
}

void CommandControlModule::handleLoginRequest(const QByteArray& packet,
                                              const CommandControlProtocol::Header& header,
                                              const QHostAddress& sender, quint16 senderPort)
{
    CommandControlProtocol::LoginRequestPayload request;
    if (!CommandControlProtocol::parseLoginRequest(packet, request)) {
        LOG_WARNING(QString("[CommandControl][DD33] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    // DD33 是本机向总控的单播请求；收到同类报文只做监听，且仅记录当前总控来源。
    if (isCurrentControl(header.senderId)) {
        logControlPacket(QStringLiteral("RX"), header,
                         QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                         loginRequestFields(request),
                         QStringLiteral("收到主控 DD33 %1 请求").arg(loginRequestTypeText(request.type)));
        logPacketHex(QStringLiteral("RX"), header.messageType, packet,
                     QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort));
    }
}

void CommandControlModule::handleLoginReply(const QByteArray& packet,
                                            const CommandControlProtocol::Header& header,
                                            const QHostAddress& sender, quint16 senderPort)
{
    CommandControlProtocol::LoginReplyPayload reply;
    if (!CommandControlProtocol::parseLoginReply(packet, reply)) {
        LOG_WARNING(QString("[CommandControl][DD34] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    if (!m_controlEndpoint.isValid() || header.senderId != m_controlEndpoint.id
        || reply.controlId != m_controlEndpoint.id) {
        LOG_WARNING(QString("[CommandControl][DD34] 主控 ID 不匹配：header=0x%1 payload=0x%2 expected=0x%3")
                    .arg(header.senderId, 8, 16, QChar('0'))
                    .arg(reply.controlId, 8, 16, QChar('0'))
                    .arg(m_controlEndpoint.id, 8, 16, QChar('0')));
        return;
    }
    logControlPacket(QStringLiteral("RX"), header,
                     QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                     loginReplyFields(reply),
                     QStringLiteral("DD34 登陆结果=%1，拒绝原因=%2")
                         .arg(hexValue(reply.result, 2)).arg(hexValue(reply.rejectReason, 2)));
    logPacketHex(QStringLiteral("RX"), header.messageType, packet,
                 QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort));
    m_loginRetryTimer.stop();
    m_waitingForLoginReply = false;
    m_loginRetryExhausted = false;
    updatePeer(reply.controlId, sender, senderPort, QStringLiteral("管理节点"), false);
    switch (reply.result) {
    case 0x01:
        m_loggedIn = true;
        m_logoutRequested = false;
        setStatus(QStringLiteral("登陆成功：主控 0x%1").arg(reply.controlId, 8, 16, QChar('0')));
        break;
    case 0x02:
        m_loggedIn = false;
        setStatus(QStringLiteral("登陆失败：拒绝原因 0x%1").arg(reply.rejectReason, 2, 16, QChar('0')));
        break;
    case 0x03:
        m_loggedIn = false;
        m_logoutRequested = true;
        setStatus(QStringLiteral("退出登陆成功：主控 0x%1").arg(reply.controlId, 8, 16, QChar('0')));
        break;
    default: setStatus(QStringLiteral("收到未知登陆结果：0x%1").arg(reply.result, 2, 16, QChar('0'))); break;
    }
}

void CommandControlModule::handleLinkCheck(const QByteArray& packet,
                                           const CommandControlProtocol::Header& header,
                                           const QHostAddress& sender, quint16 senderPort)
{
    CommandControlProtocol::LinkCheckPayload linkCheck;
    if (!CommandControlProtocol::parseLinkCheck(packet, linkCheck)) {
        LOG_WARNING(QString("[CommandControl][DD25] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    updatePeer(header.senderId, sender, senderPort, QStringLiteral("链路设备"), false);
    auto it = m_peers.find(header.senderId);
    if (it != m_peers.end()) {
        it->lastLinkCheck = QDateTime::currentDateTimeUtc();
        it->online = true;
        emit peersChanged();
    }
    if (isCurrentControl(header.senderId)) {
        logControlPacket(QStringLiteral("RX"), header,
                         QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                         linkCheckFields(linkCheck));
        logPacketHex(QStringLiteral("RX"), header.messageType, packet,
                     QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort));
    }
}

void CommandControlModule::handleIncomingDda4(const QByteArray& packet, const CommandControlProtocol::Header& header,
                                              const QHostAddress& sender, quint16 senderPort)
{
    if (header.senderId == m_settings.deviceId) {
        return;
    }
    CommandControlProtocol::Dda4Track track;
    if (!CommandControlProtocol::parseDda4Track(packet, track)) {
        LOG_WARNING(QString("[CommandControl][DDA4] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    updatePeer(header.senderId, sender, senderPort, QStringLiteral("态势设备"), false);
    storeDda4(false, track, packet);
    if (isCurrentControl(header.senderId) && shouldLogDda4(false)) {
        logControlPacket(QStringLiteral("RX"), header,
                         QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                         dda4Fields(track));
    }
}

void CommandControlModule::handleIncomingDda1(const QByteArray& packet, const CommandControlProtocol::Header& header,
                                              const QHostAddress& sender, quint16 senderPort)
{
    if (header.senderId == m_settings.deviceId) {
        return;
    }
    CommandControlProtocol::Dda1Status status;
    if (!CommandControlProtocol::parseDda1Status(packet, status)) {
        LOG_WARNING(QString("[CommandControl][DDA1] 解析失败：from=%1:%2").arg(sender.toString()).arg(senderPort));
        return;
    }
    updatePeer(header.senderId, sender, senderPort, QStringLiteral("装备状态"), false);
    auto it = m_peers.find(header.senderId);
    if (it != m_peers.end()) {
        it->lastEquipmentStatus = QDateTime::currentDateTimeUtc();
        it->healthStatus = status.healthStatus;
        it->radiationStatus = status.radiationStatus;
        emit peersChanged();
    }
    if (isCurrentControl(header.senderId)) {
        logControlPacket(QStringLiteral("RX"), header,
                         QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort),
                         dda1Fields(status));
        logPacketHex(QStringLiteral("RX"), header.messageType, packet,
                     QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort));
    }
}

void CommandControlModule::beginLogin(CommandControlProtocol::LoginRequestType type, bool allowRetries)
{
    if (!m_controlEndpoint.isValid()) {
        setStatus(QStringLiteral("尚未从 DD31 得到总控点播地址"));
        return;
    }
    const QHostAddress localAddress(m_settings.localIp);
    if (!validIp(localAddress)) {
        setStatus(QStringLiteral("本机 IP 配置无效：%1").arg(m_settings.localIp));
        return;
    }
    const auto header = nextHeader(CommandControlProtocol::LoginRequest, m_controlEndpoint.id,
                                   true, 0x02);
    const QByteArray packet = CommandControlProtocol::makeLoginRequest(
        header, m_settings.deviceId, ipv4LowFirst(localAddress), m_settings.localPort, type, beijingNow());
    CommandControlProtocol::LoginRequestPayload loggedRequest;
    if (CommandControlProtocol::parseLoginRequest(packet, loggedRequest)) {
        logControlPacket(QStringLiteral("TX"), header,
                         QStringLiteral("%1:%2").arg(m_controlEndpoint.address.toString()).arg(m_controlEndpoint.port),
                         loginRequestFields(loggedRequest),
                         QStringLiteral("发送 DD33 %1 请求").arg(loginRequestTypeText(type)));
    }
    sendUnicast(packet, CommandControlProtocol::LoginRequest);
    m_waitingForLoginReply = type == CommandControlProtocol::LoginRequestType::Login;
    if (allowRetries) {
        m_loginRetryExhausted = false;
    }
    m_loginRetriesRemaining = allowRetries ? m_settings.loginRetryCount : 0;
    if (m_waitingForLoginReply && m_loginRetriesRemaining > 0) {
        m_loginRetryTimer.start(m_settings.loginRetryIntervalMs);
    }
    setStatus(type == CommandControlProtocol::LoginRequestType::Login
                  ? QStringLiteral("已发送登陆请求，等待 DD34 回复")
                  : QStringLiteral("已发送退出登陆请求，等待 DD34 回复"));
}

void CommandControlModule::retryLogin()
{
    if (!m_waitingForLoginReply || m_loginRetriesRemaining <= 0) {
        if (m_waitingForLoginReply) {
            m_waitingForLoginReply = false;
            m_loginRetryExhausted = true;
            setStatus(QStringLiteral("未收到 DD34 回复，自动登陆重试已停止"));
        }
        return;
    }
    const int retriesAfterSend = --m_loginRetriesRemaining;
    LOG_WARNING(QString("[CommandControl][DD33] 未收到 DD34，执行剩余 %1 次重试")
                .arg(retriesAfterSend));
    beginLogin(CommandControlProtocol::LoginRequestType::Login, false);
    m_loginRetriesRemaining = retriesAfterSend;
    if (m_waitingForLoginReply) {
        m_loginRetryTimer.start(m_settings.loginRetryIntervalMs);
    }
}

void CommandControlModule::sendLinkCheck()
{
    if (!m_ready) {
        return;
    }
    const QDateTime now = beijingNow();
    const auto header = nextHeader(CommandControlProtocol::LinkCheck, 0, false, 0x01);
    const QByteArray packet = CommandControlProtocol::makeLinkCheck(header, monthTimestampMs(now),
                                                                       m_settings.dd25CooperationStatus);
    CommandControlProtocol::LinkCheckPayload linkCheck;
    if (CommandControlProtocol::parseLinkCheck(packet, linkCheck)) {
        logControlPacket(QStringLiteral("TX"), header,
                         QStringLiteral("%1:%2").arg(m_settings.multicastGroup).arg(m_settings.multicastPort),
                         linkCheckFields(linkCheck));
    }
    sendMulticast(packet, CommandControlProtocol::LinkCheck);
}

void CommandControlModule::sendEquipmentStatus()
{
    if (!m_ready) {
        return;
    }
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    if (!currentRadarPosition(longitude, latitude, altitude)) {
        if (!m_missingPositionLogged) {
            LOG_WARNING(QStringLiteral("[CommandControl][DDA1] 尚未收到 DD05 阵面经纬高真值，暂停发送装备状态，避免上报配置中的静态坐标"));
            m_missingPositionLogged = true;
        }
        return;
    }
    CommandControlProtocol::Dda1Status status;
    status.longitudeE7 = static_cast<qint32>(qRound64(longitude * 1e7));
    status.latitudeE7 = static_cast<qint32>(qRound64(latitude * 1e7));
    status.altitudeM = static_cast<qint32>(qRound64(altitude));
    status.workStatus = m_settings.dda1WorkStatus;
    status.healthStatus = m_settings.dda1HealthStatus;
    status.deviceCount = m_settings.dda1DeviceCount;
    status.deviceType = m_settings.dda1DeviceType;
    status.deviceNumber = m_settings.dda1DeviceNumber;
    status.deviceStatus = m_settings.dda1DeviceStatus;
    status.workMode = m_settings.dda1WorkMode;
    status.radiationStatus = m_radiationStatus;
    status.azimuthStartDeg = m_settings.dda1AzimuthStartDeg;
    status.azimuthEndDeg = m_settings.dda1AzimuthEndDeg;
    status.elevationStartDeg = m_settings.dda1ElevationStartDeg;
    status.elevationEndDeg = m_settings.dda1ElevationEndDeg;
    const bool usesConfirmedScan = confirmedScanRange(
        status.azimuthStartDeg, status.azimuthEndDeg,
        status.elevationStartDeg, status.elevationEndDeg);
    const auto header = nextHeader(CommandControlProtocol::EquipmentStatus, 0, false, 0x03);
    const QByteArray packet = CommandControlProtocol::makeDda1Status(header, status);
    CommandControlProtocol::Dda1Status loggedStatus;
    if (CommandControlProtocol::parseDda1Status(packet, loggedStatus)) {
        logControlPacket(QStringLiteral("TX"), header,
                         QStringLiteral("%1:%2").arg(m_settings.multicastGroup).arg(m_settings.multicastPort),
                         dda1Fields(loggedStatus),
                         QStringLiteral("DDA1 经纬高=%1,%2,%3；扫描=%4~%5/%6~%7（%8）")
                             .arg(longitude, 0, 'f', 7).arg(latitude, 0, 'f', 7).arg(altitude, 0, 'f', 1)
                             .arg(status.azimuthStartDeg).arg(status.azimuthEndDeg)
                             .arg(status.elevationStartDeg).arg(status.elevationEndDeg)
                             .arg(usesConfirmedScan ? QStringLiteral("DE01+BIT") : QStringLiteral("配置默认")));
    }
    sendMulticast(packet, CommandControlProtocol::EquipmentStatus);
}

void CommandControlModule::processTrackPoint(const PointInfo& info)
{
    if (!m_ready) {
        return;
    }
    if (info.type != PointType::Track) {
        return;
    }
    if (info.statMethod == 2) {
        m_latestTracks.remove(info.batch);
        m_targetClasses.remove(info.batch);
        m_manualBatches.remove(info.batch);
        m_localBatches.remove(info.batch);
        LOG_INFO(QString("[CommandControl][TRACK] 航迹消批 sourceBatch=%1，已停止手动上报").arg(info.batch));
        return;
    }
    m_latestTracks.insert(info.batch, info);
    const bool manual = m_manualBatches.contains(info.batch);
    const bool automatic = m_autoReportEnabled && isDrone(info.batch, info);
    if (manual || automatic) {
        reportTrack(info, manual);
    }
}

void CommandControlModule::processTargetClassification(TargetClaRes result)
{
    if (!m_ready) {
        return;
    }
    m_targetClasses.insert(result.batchID, result.claRes);
    if (!m_autoReportEnabled || result.claRes != 1 || !m_latestTracks.contains(result.batchID)) {
        return;
    }
    const PointInfo info = m_latestTracks.value(result.batchID);
    reportTrack(info, m_manualBatches.contains(result.batchID));
}

void CommandControlModule::toggleManualReport(quint32 sourceBatch)
{
    if (!m_ready) {
        return;
    }
    if (m_manualBatches.contains(sourceBatch)) {
        m_manualBatches.remove(sourceBatch);
        LOG_INFO(QString("[CommandControl][MANUAL] 关闭手动上报 sourceBatch=%1").arg(sourceBatch));
        return;
    }
    if (!m_latestTracks.contains(sourceBatch)) {
        LOG_WARNING(QString("[CommandControl][MANUAL] 未找到当前航迹，不能开启 sourceBatch=%1").arg(sourceBatch));
        return;
    }
    m_manualBatches.insert(sourceBatch);
    LOG_INFO(QString("[CommandControl][MANUAL] 开启手动上报 sourceBatch=%1").arg(sourceBatch));
    reportTrack(m_latestTracks.value(sourceBatch), true);
}

void CommandControlModule::setAutoReportEnabled(bool enabled)
{
    if (m_autoReportEnabled == enabled) {
        return;
    }
    m_autoReportEnabled = enabled;
    LOG_INFO(QString("[CommandControl][AUTO] 自动无人机航迹上报=%1").arg(enabled));
    emit autoReportChanged(enabled);
}

void CommandControlModule::requestLogin()
{
    m_logoutRequested = false;
    beginLogin(CommandControlProtocol::LoginRequestType::Login, true);
}

void CommandControlModule::requestLogout()
{
    beginLogin(CommandControlProtocol::LoginRequestType::Logout, false);
    m_logoutRequested = true;
}

void CommandControlModule::showControlWindow()
{
    if (!m_window) {
        m_window = new CommandControlWindow(this);
        connect(m_window, &QObject::destroyed, this, [this]() { m_window = nullptr; });
    }
    m_window->show();
    m_window->raise();
    m_window->activateWindow();
}

bool CommandControlModule::isDrone(quint32 sourceBatch, const PointInfo& info) const
{
    return info.targetRecResult == 1 || m_targetClasses.value(sourceBatch, 0) == 1;
}

quint32 CommandControlModule::localBatchFor(quint32 sourceBatch)
{
    if (m_localBatches.contains(sourceBatch)) {
        return m_localBatches.value(sourceBatch);
    }
    if (m_nextLocalBatch == 0) {
        ++m_nextLocalBatch;
    }
    const quint32 localBatch = m_nextLocalBatch++;
    m_localBatches.insert(sourceBatch, localBatch);
    return localBatch;
}

bool CommandControlModule::targetLla(const PointInfo& info, double& longitudeDeg,
                                     double& latitudeDeg, double& altitudeM) const
{
    if (!std::isfinite(info.range) || !std::isfinite(info.azimuth) || !std::isfinite(info.elevation)
        || info.range < 0.0f) {
        return false;
    }
    double radarLongitude = 0.0;
    double radarLatitude = 0.0;
    double radarAltitude = 0.0;
    if (!currentRadarPosition(radarLongitude, radarLatitude, radarAltitude)) {
        return false;
    }
    double eastM = 0.0;
    double northM = 0.0;
    double upM = 0.0;
    if (!Wgs84Coordinate::polarToEnu(info.range, info.azimuth, info.elevation,
                                     eastM, northM, upM)) {
        return false;
    }
    Wgs84Coordinate::Lla target;
    const Wgs84Coordinate::Lla radar{radarLongitude, radarLatitude, radarAltitude};
    if (!Wgs84Coordinate::enuToLla(radar, eastM, northM, upM, target)) {
        return false;
    }
    longitudeDeg = target.longitudeDeg;
    latitudeDeg = target.latitudeDeg;
    altitudeM = target.altitudeM;
    return true;
}

bool CommandControlModule::currentRadarPosition(double& longitudeDeg, double& latitudeDeg,
                                                 double& altitudeM) const
{
    if (!m_hasRadarPosition || !std::isfinite(m_radarLongitudeDeg)
        || !std::isfinite(m_radarLatitudeDeg) || !std::isfinite(m_radarAltitudeM)) {
        return false;
    }
    longitudeDeg = m_radarLongitudeDeg;
    latitudeDeg = m_radarLatitudeDeg;
    altitudeM = m_radarAltitudeM;
    return true;
}

bool CommandControlModule::confirmedScanRange(quint16& azimuthStartDeg, quint16& azimuthEndDeg,
                                               qint8& elevationStartDeg, qint8& elevationEndDeg) const
{
    if (!m_hasConfirmedBeamControl || !m_hasSelectedRadarYaw) {
        return false;
    }

    int elevationStart = 0;
    int elevationEnd = 0;
    bool hasEnabledBeam = false;
    const auto includeBeam = [&hasEnabledBeam, &elevationStart, &elevationEnd](quint8 enabled,
                                                                                 qint16 start,
                                                                                 qint16 end) {
        if (enabled == 0) {
            return;
        }
        const int startDeg = static_cast<int>(std::lround(start * 0.01));
        const int endDeg = static_cast<int>(std::lround(end * 0.01));
        if (!hasEnabledBeam) {
            elevationStart = startDeg;
            elevationEnd = endDeg;
            hasEnabledBeam = true;
            return;
        }
        elevationStart = std::min(elevationStart, startDeg);
        elevationEnd = std::max(elevationEnd, endDeg);
    };
    includeBeam(m_confirmedBeamControl.beam1Flag, m_confirmedBeamControl.elestart1,
                m_confirmedBeamControl.eleend1);
    includeBeam(m_confirmedBeamControl.beam2Flag, m_confirmedBeamControl.elestart2,
                m_confirmedBeamControl.eleend2);
    includeBeam(m_confirmedBeamControl.beam3Flag, m_confirmedBeamControl.elestart3,
                m_confirmedBeamControl.eleend3);
    if (!hasEnabledBeam) {
        return false;
    }

    const double relativeStart = m_confirmedBeamControl.aziStart * 0.01;
    const double relativeEnd = m_confirmedBeamControl.aziEnd * 0.01;
    if (std::abs(relativeEnd - relativeStart) >= 359.5) {
        azimuthStartDeg = 0;
        azimuthEndDeg = 360;
    } else {
        azimuthStartDeg = static_cast<quint16>(qRound(normalizeNorthAngle(
            m_selectedRadarYawDeg + relativeStart)));
        azimuthEndDeg = static_cast<quint16>(qRound(normalizeNorthAngle(
            m_selectedRadarYawDeg + relativeEnd)));
    }
    elevationStartDeg = static_cast<qint8>(std::clamp(elevationStart, -10, 70));
    elevationEndDeg = static_cast<qint8>(std::clamp(elevationEnd, -10, 70));
    return true;
}

CommandControlProtocol::Dda4Track CommandControlModule::makeDda4Track(const PointInfo& info, bool manualMode) const
{
    CommandControlProtocol::Dda4Track track;
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    targetLla(info, longitude, latitude, altitude);
    track.comprehensiveBatch = m_settings.dda4ComprehensiveBatch;
    track.localBatch = m_localBatches.value(info.batch);
    track.deviceId = m_settings.deviceId;
    track.deviceType = m_settings.dda4DeviceType;
    track.deviceNumber = m_settings.dda4DeviceNumber;
    track.rateCentiHz = m_settings.dda4RateCentiHz;
    track.targetAttribute = m_settings.dda4TargetAttribute;
    track.targetType = m_settings.dda4TargetType;
    track.trackQuality = m_settings.dda4TrackQuality;
    track.rcsMilliSquareM = m_settings.dda4RcsMilliSquareM;
    track.interferenceStatus = m_settings.dda4InterferenceStatus;
    track.relativeDelayMs = m_settings.dda4RelativeDelayMs;
    track.longitudeE7 = static_cast<qint32>(qRound64(longitude * 1e7));
    track.latitudeE7 = static_cast<qint32>(qRound64(latitude * 1e7));
    track.altitudeM = static_cast<qint32>(qRound64(altitude));
    const double elevationRad = static_cast<double>(info.elevation) * kDegToRad;
    const double azimuthRad = static_cast<double>(info.azimuth) * kDegToRad;
    const double speed = static_cast<double>(info.speed);
    track.velocityEast = quantizeVelocity(speed * std::cos(elevationRad) * std::sin(azimuthRad));
    track.velocityNorth = quantizeVelocity(speed * std::cos(elevationRad) * std::cos(azimuthRad));
    track.velocityUp = quantizeVelocity(speed * std::sin(elevationRad));
    const quint8 state = info.statMethod == 1 ? m_settings.dda4UpdateStateExtrapolated
                                              : m_settings.dda4UpdateStateFiltered;
    const quint8 mode = manualMode ? m_settings.dda4UpdateModeManual : m_settings.dda4UpdateModeAuto;
    track.updateMethod = static_cast<quint8>((state << 4) | mode);
    track.monthTimestampMs = monthTimestampMs(beijingNow());
    return track;
}

void CommandControlModule::reportTrack(const PointInfo& info, bool manualMode)
{
    double radarLongitude = 0.0;
    double radarLatitude = 0.0;
    double radarAltitude = 0.0;
    if (!currentRadarPosition(radarLongitude, radarLatitude, radarAltitude)) {
        if (!m_missingPositionLogged) {
            LOG_WARNING(QStringLiteral("[CommandControl][DDA4] 尚未收到 DD05 阵面经纬高真值，暂停航迹经纬高上报"));
            m_missingPositionLogged = true;
        }
        return;
    }
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    if (!targetLla(info, longitude, latitude, altitude)) {
        LOG_WARNING(QString("[CommandControl][DDA4] 跳过无效 RAE sourceBatch=%1").arg(info.batch));
        return;
    }
    localBatchFor(info.batch);
    const auto track = makeDda4Track(info, manualMode);
    const auto header = nextHeader(CommandControlProtocol::SituationIntelligence, 0, false, 0x03);
    const QByteArray packet = CommandControlProtocol::makeDda4Track(header, track);
    sendMulticast(packet, CommandControlProtocol::SituationIntelligence);
    storeDda4(true, track, packet);
    if (shouldLogDda4(true)) {
        CommandControlProtocol::Dda4Track loggedTrack;
        if (CommandControlProtocol::parseDda4Track(packet, loggedTrack)) {
            logControlPacket(QStringLiteral("TX"), header,
                             QStringLiteral("%1:%2").arg(m_settings.multicastGroup).arg(m_settings.multicastPort),
                             dda4Fields(loggedTrack));
        }
    }
}

void CommandControlModule::storeDda4(bool outbound, const CommandControlProtocol::Dda4Track& track,
                                     const QByteArray& packet)
{
    CommandControlRecord record;
    record.outbound = outbound;
    record.observedUtcMs = QDateTime::currentMSecsSinceEpoch();
    record.track = track;
    record.packet = packet;
    if (m_recordWriter) {
        CommandControlRecordWriter* writer = m_recordWriter;
        // 仅投递值对象；JSON 序列化、flush 和磁盘写入全在记录线程执行。
        QMetaObject::invokeMethod(writer, [writer, record]() { writer->appendRecord(record); },
                                  Qt::QueuedConnection);
    }
    m_recentRecords.append(record);
    while (m_recentRecords.size() > m_settings.visibleRecordLimit) {
        m_recentRecords.removeFirst();
    }
    scheduleRecordRefresh();
}

void CommandControlModule::updatePeer(quint32 deviceId, const QHostAddress& sender, quint16 senderPort,
                                      const QString& role, bool managementNotice)
{
    if (deviceId == 0 || deviceId == m_settings.deviceId) {
        return;
    }
    CommandControlPeer peer = m_peers.value(deviceId);
    peer.deviceId = deviceId;
    peer.ip = sender.toString();
    peer.port = senderPort;
    peer.role = role;
    if (managementNotice) {
        peer.lastManagementNotice = QDateTime::currentDateTimeUtc();
    }
    m_peers.insert(deviceId, peer);
    schedulePeerRefresh();
}

void CommandControlModule::schedulePeerRefresh()
{
    if (!m_peerRefreshTimer.isActive()) {
        m_peerRefreshTimer.start(kUiRefreshIntervalMs);
    }
}

void CommandControlModule::scheduleRecordRefresh()
{
    if (!m_recordRefreshTimer.isActive()) {
        m_recordRefreshTimer.start(kUiRefreshIntervalMs);
    }
}

void CommandControlModule::updatePeerLiveness()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    bool changed = false;
    for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
        const QDateTime lastSeen = it->lastLinkCheck.isValid() ? it->lastLinkCheck : it->lastManagementNotice;
        const bool online = lastSeen.isValid() && lastSeen.msecsTo(now) <= kPeerOfflineIntervalMs;
        if (it->online != online) {
            it->online = online;
            changed = true;
            LOG_INFO(QString("[CommandControl][PEER] device=0x%1 %2")
                     .arg(it->deviceId, 8, 16, QChar('0'))
                     .arg(online ? QStringLiteral("online") : QStringLiteral("offline")));
        }
    }
    if (changed) {
        emit peersChanged();
    }
}

PointInfo CommandControlModule::replayPointFor(const CommandControlRecord& record)
{
    const auto& track = record.track;
    double radarLongitude = 0.0;
    double radarLatitude = 0.0;
    double radarAltitude = 0.0;
    if (!currentRadarPosition(radarLongitude, radarLatitude, radarAltitude)) {
        return PointInfo{};
    }
    const double longitude = static_cast<double>(track.longitudeE7) / 1e7;
    const double latitude = static_cast<double>(track.latitudeE7) / 1e7;
    const Wgs84Coordinate::Lla radar{radarLongitude, radarLatitude, radarAltitude};
    const Wgs84Coordinate::Lla target{longitude, latitude, static_cast<double>(track.altitudeM)};
    double east = 0.0;
    double north = 0.0;
    double up = 0.0;
    if (!Wgs84Coordinate::llaToEnu(radar, target, east, north, up)) {
        return PointInfo{};
    }
    const double horizontal = std::hypot(east, north);

    PointInfo point;
    point.type = PointType::Track;
    const quint64 sourceKey = (static_cast<quint64>(track.deviceId) << 32) | track.localBatch;
    quint32 replayBatch = m_replayBatches.value(sourceKey);
    if (replayBatch == 0) {
        replayBatch = m_nextReplayBatch++;
        if (replayBatch == 0) {
            replayBatch = m_nextReplayBatch++;
        }
        m_replayBatches.insert(sourceKey, replayBatch);
    }
    point.batch = replayBatch;
    point.range = static_cast<float>(std::hypot(horizontal, up));
    point.azimuth = static_cast<float>(std::fmod(std::atan2(east, north) / kDegToRad + 360.0, 360.0));
    point.elevation = static_cast<float>(std::atan2(up, horizontal) / kDegToRad);
    point.altitute = static_cast<float>(track.altitudeM);
    if (track.velocityEast != std::numeric_limits<qint16>::max()
        && track.velocityNorth != std::numeric_limits<qint16>::max()
        && track.velocityUp != std::numeric_limits<qint16>::max()) {
        const double eastMps = track.velocityEast * kVelocityResolution;
        const double northMps = track.velocityNorth * kVelocityResolution;
        const double upMps = track.velocityUp * kVelocityResolution;
        point.speed = static_cast<float>(std::sqrt(eastMps * eastMps + northMps * northMps + upMps * upMps));
    }
    point.statMethod = 0;
    return point;
}

void CommandControlModule::startReplay(const QString& recordFilePath)
{
    if (!m_recordWriter) {
        setStatus(QStringLiteral("DDA4 记录功能未开启，不能读取回放文件"));
        return;
    }
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
    if (!currentRadarPosition(longitude, latitude, altitude)) {
        setStatus(QStringLiteral("等待 DD05 阵面经纬高真值，暂不启动 DDA4 回放"));
        return;
    }
    CommandControlRecordWriter* writer = m_recordWriter;
    QMetaObject::invokeMethod(writer, [writer, recordFilePath]() { writer->loadRecords(recordFilePath); },
                              Qt::QueuedConnection);
    setStatus(QStringLiteral("正在独立读取 DDA4 回放文件"));
}

bool CommandControlModule::shouldLogDda4(bool outbound)
{
    if (m_settings.dda4LogIntervalMs == 0) {
        return true;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qint64& lastLogMs = outbound ? m_lastDda4TxLogMs : m_lastDda4RxLogMs;
    if (lastLogMs != 0 && nowMs - lastLogMs < m_settings.dda4LogIntervalMs) {
        return false;
    }
    lastLogMs = nowMs;
    return true;
}

void CommandControlModule::logPacketHex(const QString& direction, quint16 type, const QByteArray& packet,
                                        const QString& endpoint) const
{
    if (!m_settings.packetHexLogEnabled) {
        return;
    }
    LOG_INFO(QString("[CommandControl][%1][HEX] %2 endpoint=%3 bytes=%4 data=%5")
             .arg(direction)
             .arg(QString::fromLatin1(CommandControlProtocol::messageTypeName(type)))
             .arg(endpoint)
             .arg(packet.size())
             .arg(QString::fromLatin1(packet.toHex(' '))));
}

void CommandControlModule::stopReplay()
{
    m_replayTimer.stop();
    m_replayRecords.clear();
    m_replayBatches.clear();
    m_replayIndex = 0;
    setStatus(QStringLiteral("DDA4 回放已停止"));
}

void CommandControlModule::emitNextReplayPoint()
{
    if (m_replayIndex >= m_replayRecords.size()) {
        stopReplay();
        return;
    }
    emit replayPointReady(replayPointFor(m_replayRecords.at(m_replayIndex++)));
}

void CommandControlModule::setRadiationStatus(bool transmitting)
{
    const quint8 value = transmitting ? 1 : 0;
    if (m_radiationStatus == value) {
        return;
    }
    m_radiationStatus = value;
    LOG_INFO(QString("[CommandControl][DDA1] 辐射状态更新为 %1").arg(value));
    sendEquipmentStatus();
}

void CommandControlModule::updateRadarPosition(double latitude, double longitude, double altitude)
{
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || !std::isfinite(altitude)
        || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        LOG_WARNING(QString("[CommandControl][DD05] 丢弃无效阵面经纬高 lon=%1 lat=%2 alt=%3")
                    .arg(longitude, 0, 'f', 8)
                    .arg(latitude, 0, 'f', 8)
                    .arg(altitude, 0, 'f', 2));
        return;
    }

    m_radarLongitudeDeg = longitude;
    m_radarLatitudeDeg = latitude;
    m_radarAltitudeM = altitude;
    m_hasRadarPosition = true;
    m_missingPositionLogged = false;
    LOG_INFO(QString("[CommandControl][DD05] 阵面经纬高真值已更新 lon=%1 lat=%2 alt=%3")
             .arg(longitude, 0, 'f', 8)
             .arg(latitude, 0, 'f', 8)
             .arg(altitude, 0, 'f', 2));
    sendEquipmentStatus();
}

void CommandControlModule::setPendingBeamControl(BeamControl beamControl)
{
    m_pendingBeamControl = beamControl;
    m_hasPendingBeamControl = true;
    m_waitingForServoReply = false;
    LOG_INFO(QString("[CommandControl][AA05] 已记录待确认 TAS/TWS 波束参数 azi=%1~%2 ele1=%3~%4")
             .arg(beamControl.aziStart * 0.01, 0, 'f', 2)
             .arg(beamControl.aziEnd * 0.01, 0, 'f', 2)
             .arg(beamControl.elestart1 * 0.01, 0, 'f', 2)
             .arg(beamControl.eleend1 * 0.01, 0, 'f', 2));
}

void CommandControlModule::setPendingServoControl(ServoControlParam servoControl)
{
    if (!m_hasPendingBeamControl) {
        return;
    }
    m_pendingServoControl = servoControl;
    m_waitingForServoReply = true;
    LOG_INFO(QString("[CommandControl][AA03] 等待伺服回送确认 cmd=%1 az=%2")
             .arg(servoControl.cmd)
             .arg(servoControl.az * 0.01, 0, 'f', 2));
}

void CommandControlModule::processBitReport(BITReport report)
{
    if (report.radarId != m_settings.dda1RadarId || report.yaw > 36000) {
        return;
    }
    m_selectedRadarYawDeg = report.yaw * 0.01;
    m_hasSelectedRadarYaw = true;
}

void CommandControlModule::processServoControlReply(ServoCtrlRet reply)
{
    if (!m_waitingForServoReply || reply.cmd != m_pendingServoControl.cmd) {
        return;
    }
    if (reply.result == 2) {
        return;
    }
    m_waitingForServoReply = false;
    if (reply.result != 1) {
        LOG_WARNING(QString("[CommandControl][DE01] 伺服确认失败，保留上一组已确认扫描范围 cmd=%1 result=%2")
                    .arg(reply.cmd).arg(reply.result));
        return;
    }

    m_confirmedBeamControl = m_pendingBeamControl;
    m_hasConfirmedBeamControl = true;
    m_hasPendingBeamControl = false;
    LOG_INFO(QString("[CommandControl][DE01] TAS/TWS 伺服确认成功，DDA1 扫描范围将按 BIT 偏航角换算为正北参考 cmd=%1")
             .arg(reply.cmd));
    if (m_hasSelectedRadarYaw) {
        quint16 azimuthStart = 0;
        quint16 azimuthEnd = 0;
        qint8 elevationStart = 0;
        qint8 elevationEnd = 0;
        if (confirmedScanRange(azimuthStart, azimuthEnd, elevationStart, elevationEnd)) {
            LOG_INFO(QString("[CommandControl][DDA1][SCAN] radarId=%1 yaw=%2 az=%3~%4 el=%5~%6")
                     .arg(m_settings.dda1RadarId)
                     .arg(m_selectedRadarYawDeg, 0, 'f', 2)
                     .arg(azimuthStart)
                     .arg(azimuthEnd)
                     .arg(elevationStart)
                     .arg(elevationEnd));
        }
        sendEquipmentStatus();
    }
}
