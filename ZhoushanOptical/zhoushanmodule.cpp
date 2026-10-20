#include "zhoushanmodule.h"
#include "Basic/ConfigManager.h"
#include "cusWidgets/cuswindow.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QStyle>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>
#include <limits>

namespace ZhoushanOptical {
namespace {
enum ExchangeRow { BatchRow, KindRow, ResultRow, LongitudeRow, LatitudeRow, HeightRow,
                   DeviceTimeRow, LocalTimeRow, IdRow, MatchRow, ExchangeRows };
QString indicationName(quint8 kind)
{
    return QString("%1 · %2").arg(kind).arg(kind == 1 ? QStringLiteral("目标提示") : QStringLiteral("目标确认"));
}
QString frameTime(const QByteArray& packet)
{
    return QDateTime(QDate(2000 + int(read(packet, 8, 1)), int(read(packet, 9, 1)), int(read(packet, 10, 1))),
        QTime(int(read(packet, 11, 1)), int(read(packet, 12, 1)), int(read(packet, 13, 1)), int(read(packet, 14, 2))), Qt::UTC)
        .toString("yyyy-MM-dd HH:mm:ss.zzz");
}
void updateStatusCard(QLabel* label, const QString& text, const QString& state, const QString& detail = {})
{
    if (label->text() != text) label->setText(text);
    if (label->toolTip() != detail) label->setToolTip(detail);
    if (label->property("state").toString() != state) {
        label->setProperty("state", state);
        label->style()->unpolish(label); label->style()->polish(label); label->update();
    }
}
}
void Transport::start()
{
    m_server = new QTcpServer(this);
    m_server->setMaxPendingConnections(2);
    connect(m_server, &QTcpServer::newConnection, this, &Transport::accept);
    if (!m_server->listen(QHostAddress(m_settings.localIp), m_settings.port)) {
        emit connectionChanged(false, m_session, QStringLiteral("TCP监听失败：") + m_server->errorString()); return;
    }
    m_timer = new QTimer(this); m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &Transport::deliver); m_timer->start();
    emit connectionChanged(false, m_session, QStringLiteral("TCP已监听，等待光电主动连接"));
}
void Transport::reject(const QString& reason) { ++m_rejected; m_error = reason; }
void Transport::accept()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) continue;
        if (socket->peerAddress() != QHostAddress(m_settings.peerIp) || m_socket) {
            reject(QStringLiteral("拒绝非配置IP或重复TCP连接")); socket->abort(); socket->deleteLater(); continue;
        }
        socket->setParent(this); socket->setReadBufferSize(65536);
        m_socket = socket; ++m_session; m_decoder.clear(); m_partialTimer.invalidate();
        connect(socket, &QTcpSocket::readyRead, this, &Transport::drain);
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            if (m_socket == socket) {
                drain(); deliver();
                m_socket = nullptr; m_decoder.clear(); m_partialTimer.invalidate();
                emit connectionChanged(false, m_session, QStringLiteral("TCP已断开，等待光电重新连接"));
            }
            socket->deleteLater();
        });
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this,
            [this, socket](QAbstractSocket::SocketError) { reject(QStringLiteral("TCP错误：") + socket->errorString()); });
        emit connectionChanged(true, m_session, QStringLiteral("TCP已连接 ") + socket->peerAddress().toString());
        drain();
    }
}
void Transport::submit(quint64 session, quint64 token, const QByteArray& packet)
{
    // Cancel work still in the Qt queue; a write already in progress/TCP cannot be recalled.
    if (token != m_allowedToken->load(std::memory_order_relaxed)) {
        emit submitted(token, false, 0, QStringLiteral("目标已切换或停止，丢弃尚未写入TCP的旧请求")); return;
    }
    if (!m_socket || session != m_session || m_socket->state() != QAbstractSocket::ConnectedState) {
        emit submitted(token, false, 0, QStringLiteral("TCP未连接或会话已变化")); return;
    }
    if (packet.size() != FrameSize || m_socket->bytesToWrite() != 0) {
        emit submitted(token, false, 0, QStringLiteral("拒绝无效帧或TCP发送积压（不堆积旧坐标）")); return;
    }
    const qint64 accepted = m_socket->write(packet);
    if (accepted != packet.size()) {
        const QString error = m_socket->errorString();
        m_socket->abort();
        emit submitted(token, false, 0, QStringLiteral("TCP写入失败：") + error); return;
    }
    QElapsedTimer now; now.start();
    emit submitted(token, true, now.msecsSinceReference() - m_settings.timeOriginMs,
                   QStringLiteral("已进入本机TCP发送缓冲，尚非光电执行确认"));
}
void Transport::drain()
{
    if (!m_socket) return;
    int budget = 64;
    while (budget-- > 0 && m_socket) {
        quint64 discarded = 0;
        QByteArray frame = m_decoder.take(discarded);
        if (discarded) { m_discardedBytes += discarded; m_error = QStringLiteral("丢弃非法/非第6节数据字节"); }
        if (frame.isEmpty()) {
            if (m_socket->bytesAvailable() == 0) break;
            if (!m_decoder.appendData(m_socket->read(4096))) {
                reject(QStringLiteral("TCP接收缓冲超限")); m_socket->abort(); return;
            }
            continue;
        }
        Response response; QString error;
        if (!parseResponse(frame, m_settings.receiverId, m_settings.senderId, response, error)) {
            reject(error + " hex=" + QString::fromLatin1(frame.toHex(' '))); continue;
        }
        const quint64 key = (quint64(response.kind) << 32) | response.batch;
        if (m_latest.size() >= 32 && !m_latest.contains(key)) { reject(QStringLiteral("响应缓存达到32条上限")); continue; }
        QElapsedTimer now; now.start();
        QVariantMap record;
        record["packet"] = frame; record["session"] = QVariant::fromValue<qulonglong>(m_session);
        record["receivedMs"] = now.msecsSinceReference() - m_settings.timeOriginMs;
        record["order"] = QVariant::fromValue<qulonglong>(++m_responseOrder);
        record["receivedTime"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        m_latest.insert(key, record); ++m_accepted;
    }
    if (m_decoder.hasPartial()) { if (!m_partialTimer.isValid()) m_partialTimer.start(); }
    else m_partialTimer.invalidate();
    if (budget < 0 && !m_drainScheduled) {
        m_drainScheduled = true;
        QTimer::singleShot(1, this, [this] { m_drainScheduled = false; drain(); });
    }
}
void Transport::deliver()
{
    if (m_socket && m_partialTimer.isValid() && m_partialTimer.elapsed() > 5000) {
        reject(QStringLiteral("不完整TCP帧超过5秒，关闭连接防止流错位")); m_socket->abort();
    }
    if (m_deliveryPending || (m_latest.isEmpty() && !m_rejected && !m_accepted && !m_discardedBytes)) return;
    QVariantList packets;
    QVariantMap stats;
    stats["accepted"] = QVariant::fromValue<qulonglong>(m_accepted);
    stats["rejected"] = QVariant::fromValue<qulonglong>(m_rejected); stats["error"] = m_error;
    stats["discardedBytes"] = QVariant::fromValue<qulonglong>(m_discardedBytes);
    packets.append(stats); m_accepted = m_rejected = m_discardedBytes = 0;
    for (const auto& record : m_latest) packets.append(record);
    m_latest.clear(); m_deliveryPending = true; emit received(packets);
}

Module::Module(QObject* parent) : QObject(parent)
{
    m_clock.start(); connect(&m_timer, &QTimer::timeout, this, &Module::tick);
}
Module::~Module()
{
    nextToken(); m_timer.stop(); m_thread.quit(); m_thread.wait(); delete m_window.data();
}
quint64 Module::nextToken()
{
    const quint64 token = ++m_token;
    m_allowedToken->store(token, std::memory_order_relaxed);
    return token;
}
void Module::init()
{
    if (!CF_INS.zhoushanOpticalEnabled()) return;
    bool valid = true;
    auto number = [&](const QString& key, int fallback, int low, int high) {
        bool ok; const int value = CF_INS.zhoushanOpticalValue(key, fallback).toString().toInt(&ok, 0);
        if (!ok || value < low || value > high) { valid = false; emit logMessage("[ZS-OPT][CONFIG][ERROR] " + key); return fallback; }
        return value;
    };
    m_settings.localIp = CF_INS.zhoushanOpticalValue("local_ip", "192.168.101.14").toString().trimmed();
    m_settings.peerIp = CF_INS.zhoushanOpticalValue("peer_ip", "192.168.101.10").toString().trimmed();
    m_settings.port = quint16(number("listen_port", 9010, 1, 65535));
    m_settings.senderId = quint16(number("sender_id", 1100, 1, 65535));
    m_settings.receiverId = quint16(number("receiver_id", 3101, 1, 65535));
    m_settings.kind = quint8(number("indication_type", 1, 1, 2));
    m_settings.intervalMs = number("report_interval_ms", 1000, 100, 60000);
    m_settings.responseTimeoutMs = number("response_timeout_ms", 3000, 500, 60000);
    m_settings.trackTimeoutMs = number("track_timeout_ms", 10000, 1000, 120000);
    m_settings.maxTracks = number("max_tracks", 1024, 1, 4096);
    m_settings.logIntervalMs = number("log_interval_ms", 5000, 1000, 60000);
    const QString basis = CF_INS.zhoushanOpticalValue("time_basis", "local").toString().trimmed().toLower();
    if (basis != "local" && basis != "utc") valid = false;
    m_settings.utc = basis == "utc";
    const QHostAddress local(m_settings.localIp), peer(m_settings.peerIp);
    if (local.protocol() != QAbstractSocket::IPv4Protocol || local == QHostAddress::AnyIPv4 || local.isMulticast()
        || local == QHostAddress::Broadcast || peer.protocol() != QAbstractSocket::IPv4Protocol
        || peer.isMulticast() || peer == QHostAddress::AnyIPv4 || peer == QHostAddress::Broadcast) valid = false;
    emit logMessage(QString("[ZS-OPT][CONFIG] path=%1 TCP server=%2:%3 peer=%4 IDs=%5->%6 kind=%7 time=%8 section=6 only")
        .arg(CF_INS.getConfigFilePath(), m_settings.localIp).arg(m_settings.port).arg(m_settings.peerIp)
        .arg(m_settings.senderId).arg(m_settings.receiverId).arg(m_settings.kind).arg(basis));
    if (!valid) { m_connectionText = QStringLiteral("配置非法，TCP未启动"); publish(); return; }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    m_origin = {CF_INS.longitude("longitude", nan), CF_INS.latitude("latitude", nan), CF_INS.altitude("altitude", nan)};
    m_originValid = Wgs84Coordinate::isValid(m_origin);
    emit logMessage(QString("[ZS-OPT][ORIGIN] CONFIG valid=%1 lon=%2 lat=%3 alt=%4；不额外旋转姿态")
        .arg(m_originValid).arg(m_origin.longitudeDeg, 0, 'f', 7).arg(m_origin.latitudeDeg, 0, 'f', 7).arg(m_origin.altitudeM));
    m_settings.timeOriginMs = m_clock.msecsSinceReference();
    auto* worker = new Transport(m_settings, m_allowedToken); worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, worker, &Transport::start);
    connect(&m_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Module::submitPacket, worker, &Transport::submit, Qt::QueuedConnection);
    connect(this, &Module::acknowledgeReceive, worker, &Transport::acknowledge, Qt::QueuedConnection);
    connect(worker, &Transport::connectionChanged, this, [this](bool connected, quint64 session, const QString& text) {
        m_connected = connected; m_session = session; m_connectionText = text;
        m_pending = m_busy = false; m_writeToken = 0; nextToken(); m_request.clear(); m_nextSend = 0;
        m_timedOut = false; m_resultText = connected ? QStringLiteral("未请求") : QStringLiteral("连接未就绪");
        if (connected && m_settings.kind == 2 && m_confirmationSent)
            m_resultText = QStringLiteral("确认请求不自动重发，请重新人工下发");
        logLimited(connected ? "TCP_CONNECTED" : "TCP_OFFLINE", text + QString(" session=%1").arg(session));
        emit reportingStateChanged(); publish();
    });
    connect(worker, &Transport::submitted, this, [this](quint64 token, bool ok, qint64 acceptedAt, const QString& text) {
        if (token != m_writeToken) return;
        m_busy = false;
        if (ok) recordSentPacket(acceptedAt);
        if (token != m_token) { publish(); return; }
        if (ok) {
            m_sentAt = acceptedAt;
            ++m_sentCount; logLimited("TX", QString("本机入队累计=%1 hex=%2 %3").arg(m_sentCount).arg(QString::fromLatin1(m_request.toHex(' ')), text));
        }
        else {
            m_pending = false; m_confirmationSent = false; m_resultText = QStringLiteral("发送失败");
            m_nextSend = m_clock.elapsed() + m_settings.intervalMs; logLimited("TX_ERROR", text);
        }
        publish();
    });
    connect(worker, &Transport::received, this, &Module::consume);
    m_thread.setObjectName(QStringLiteral("ZhoushanOpticalTcp")); m_thread.start();
    m_timer.start(100);
}
void Module::updateRadarPosition(double latitude, double longitude, double altitude)
{
    const Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (!Wgs84Coordinate::isValid(origin)) { logLimited("ORIGIN_INVALID", "拒绝非法DD05原点"); return; }
    m_origin = origin; m_originValid = true;
    if (!m_dd05) emit logMessage("[ZS-OPT][ORIGIN] 使用真实DD05原点");
    m_dd05 = true;
}
bool Module::hasTrack(int batch) const
{
    auto it = m_tracks.constFind(batch);
    return it != m_tracks.cend() && m_clock.elapsed() - it->at <= m_settings.trackTimeoutMs;
}
bool Module::isReporting(int batch) const { return m_connected && batch == m_selected && hasTrack(batch); }
void Module::processTrack(const PointInfo& point)
{
    if (point.type != PointType::Track || point.statMethod == 250) return;
    const int batch = int(point.batch);
    if (point.statMethod == 2) { m_tracks.remove(batch); if (batch == m_selected) stopReport(); return; }
    if (!std::isfinite(point.range) || point.range < 0 || !std::isfinite(point.azimuth)
        || !std::isfinite(point.elevation) || std::abs(point.elevation) > 90) { logLimited("TRACK_INVALID", "拒绝非法RAE"); return; }
    if (!m_tracks.contains(batch) && m_tracks.size() >= m_settings.maxTracks) {
        auto oldest = m_tracks.begin();
        for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) if (it->at < oldest->at) oldest = it;
        const int evicted = oldest.key(); m_tracks.erase(oldest); if (evicted == m_selected) stopReport();
        logLimited("CAPACITY", "最新普通航迹缓存已满，淘汰最旧记录");
    }
    m_tracks.insert(batch, {point, m_clock.elapsed()});
}
QByteArray Module::encode(const PointInfo& point) const
{
    double east, north, up; Wgs84Coordinate::Lla target;
    if (!m_originValid || !Wgs84Coordinate::polarToEnu(point.range, point.azimuth, point.elevation, east, north, up)
        || !Wgs84Coordinate::enuToLla(m_origin, east, north, up, target)) return {};
    const QDateTime time = m_settings.utc ? QDateTime::currentDateTimeUtc() : QDateTime::currentDateTime();
    return indication(m_settings.senderId, m_settings.receiverId, m_settings.kind, point.batch, target, time);
}
bool Module::startReport(int batch)
{
    if (!m_connected || !hasTrack(batch) || encode(m_tracks.value(batch).point).isEmpty()) {
        emit logMessage(QString("[ZS-OPT][SELECT][ERROR] batch=%1 TCP/普通航迹/雷达原点未就绪").arg(batch)); return false;
    }
    // Switching is local only: section 6 has no cancellation command.
    if (m_currentResponse.batch != batch) {
        m_previousResponse = m_currentResponse;
        m_currentResponse = {batch, 0, {}};
    }
    m_selected = batch; m_pending = false; nextToken();
    m_request.clear(); m_confirmationSent = false; m_timedOut = false; m_nextSend = 0;
    m_resultText = QStringLiteral("等待发送目标指示");
    emit logMessage(QString("[ZS-OPT][SELECT] batch=%1 kind=%2").arg(batch).arg(m_settings.kind));
    emit reportingStateChanged(); tick(); return true;
}
void Module::stopReport()
{
    if (m_selected < 0) return;
    emit logMessage(QString("[ZS-OPT][STOP] batch=%1 仅停止本地下发，不伪造取消报文").arg(m_selected));
    m_selected = -1; m_pending = false; nextToken(); m_request.clear();
    m_timedOut = false; m_resultText = QStringLiteral("已停止本地下发"); emit reportingStateChanged(); publish();
}
void Module::tick()
{
    const auto now = m_clock.elapsed();
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (now - it->at > m_settings.trackTimeoutMs) {
            if (it.key() == m_selected) stopReport(); it = m_tracks.erase(it);
        } else ++it;
    }
    if (m_pending && !m_busy && now - m_sentAt > m_settings.responseTimeoutMs) {
        m_pending = false; m_timedOut = true; m_resultText = QStringLiteral("目标指示响应超时");
        logLimited("RESPONSE_TIMEOUT", QString("batch=%1 kind=%2").arg(m_selected).arg(m_settings.kind));
    }
    if (m_connected && m_selected >= 0 && hasTrack(m_selected) && !m_busy && !m_pending
        && now >= m_nextSend && !(m_settings.kind == 2 && m_confirmationSent)) {
        const QByteArray request = encode(m_tracks.value(m_selected).point);
        m_nextSend = now + m_settings.intervalMs;
        if (request.isEmpty()) logLimited("ENCODE", "无法解算目标经纬高/时标非法，拒绝下发");
        else {
            m_request = m_writePacket = request; m_pending = m_busy = true; m_sentAt = now;
            m_confirmationSent = m_settings.kind == 2; m_resultText = QStringLiteral("等待光电响应");
            m_writeToken = nextToken(); emit submitPacket(m_session, m_writeToken, request);
        }
    }
    publish();
}
void Module::consume(const QVariantList& packets)
{
    for (const auto& value : packets) {
        const auto record = value.toMap();
        if (!record.contains("packet")) {
            m_receivedCount += record["accepted"].toULongLong(); m_rejectedCount += record["rejected"].toULongLong();
            m_discardedBytes += record["discardedBytes"].toULongLong();
            if (record["rejected"].toULongLong() || record["discardedBytes"].toULongLong()) logLimited("RX_REJECT",
                QString("累计有效帧=%1 拒绝事件=%2 丢弃字节=%3 最近原因=%4")
                    .arg(m_receivedCount).arg(m_rejectedCount).arg(m_discardedBytes).arg(record["error"].toString()));
            continue;
        }
        Response response; QString error; const auto packet = record["packet"].toByteArray();
        if (!parseResponse(packet, m_settings.receiverId, m_settings.senderId, response, error)) { logLimited("RX_REJECT", error); continue; }
        const qint64 receivedAt = record["receivedMs"].toLongLong();
        const bool matches = m_pending && record["session"].toULongLong() == m_session
            && response.batch == quint32(m_selected) && response.kind == m_settings.kind
            && receivedAt >= m_sentAt && receivedAt - m_sentAt <= m_settings.responseTimeoutMs
            && (response.kind == 2 || packet.mid(24, 20) == m_request.mid(24, 20));
        const QString result = response.kind == 1
            ? (response.state == 1 ? QStringLiteral("已接收并处理") : QStringLiteral("已接收但忽略"))
            : response.state == 1 ? QStringLiteral("不能确认目标") : response.state == 2
                ? QStringLiteral("已确认是目标") : QStringLiteral("已确认不是目标");
        const QString lastResponse = QString("batch=%1 类型=%2（%3） 状态=%4（%5）\n经度=%6° 纬度=%7° 高度=%8\n%9")
            .arg(response.batch).arg(response.kind)
            .arg(response.kind == 1 ? QStringLiteral("目标提示") : QStringLiteral("目标确认"))
            .arg(response.state).arg(result)
            .arg(response.target.longitudeDeg, 0, 'f', 7).arg(response.target.latitudeDeg, 0, 'f', 7)
            .arg(response.altitudeValid ? QString::number(response.target.altitudeM, 'f', 1) + " m" : QStringLiteral("无效"))
            .arg(matches ? QStringLiteral("已匹配对应下发请求") : QStringLiteral("未匹配当前请求，仅记录"));
        logLimited(matches ? "RESPONSE" : "UNMATCHED_RESPONSE", lastResponse + " hex=" + QString::fromLatin1(packet.toHex(' ')));
        if (record["session"].toULongLong() == m_session) {
            ResponseDisplay* display = m_currentResponse.batch >= 0 && response.batch == quint32(m_currentResponse.batch) ? &m_currentResponse
                : m_previousResponse.batch >= 0 && response.batch == quint32(m_previousResponse.batch) ? &m_previousResponse : nullptr;
            const quint64 order = record["order"].toULongLong();
            // QHash delivery is unordered, including replies with identical millisecond timestamps.
            if (display && (!order || order > display->order)) {
                const QString deviceTime = frameTime(packet);
                display->order = order;
                display->text = lastResponse + QStringLiteral("\n对端时标：") + deviceTime
                    + (m_settings.utc ? QStringLiteral(" UTC") : QStringLiteral("（约定本地时间）"))
                    + QStringLiteral("\n本机接收：") + record["receivedTime"].toString()
                    + QString(" / ID %1 → %2").arg(m_settings.receiverId).arg(m_settings.senderId);
                display->fields = QStringList{QString::number(response.batch), indicationName(response.kind),
                    QString("%1 · %2").arg(response.state).arg(result),
                    QString::number(response.target.longitudeDeg, 'f', 7), QString::number(response.target.latitudeDeg, 'f', 7),
                    response.altitudeValid ? QString::number(response.target.altitudeM, 'f', 1) : QStringLiteral("无效"),
                    deviceTime, record["receivedTime"].toString(),
                    QString("%1 → %2").arg(m_settings.receiverId).arg(m_settings.senderId),
                    matches ? QStringLiteral("已匹配") : QStringLiteral("未匹配")};
            }
        }
        if (matches) {
            m_pending = false; m_timedOut = false;
            m_resultText = result;
        }
    }
    emit acknowledgeReceive(); publish();
}
void Module::logLimited(const QString& category, const QString& text)
{
    const auto now = m_clock.elapsed();
    if (!m_logs.contains(category) || now - m_logs[category] >= m_settings.logIntervalMs) {
        m_logs[category] = now; emit logMessage("[ZS-OPT][" + category + "] " + text);
    }
}
QString Module::statusText() const {
    return QStringLiteral("光电：%1 / 目指：%2%3").arg(m_connectionText, m_resultText,
        m_timedOut ? QStringLiteral("（最近请求超时，尚无有效响应）") : QString());
}
void Module::publish()
{
    const auto text = statusText();
    if (text != m_published) { m_published = text; emit statusChanged(m_connected && !m_timedOut, text); }
    if (m_window && m_window->isVisible()) refreshWindow();
}
void Module::recordSentPacket(qint64 acceptedAt)
{
    if (m_writePacket.size() != FrameSize) return;
    const auto& packet = m_writePacket;
    m_lastSentFields = QStringList{QString::number(read(packet, 19, 4)), indicationName(quint8(packet[18])),
        QStringLiteral("已提交发送"), QString::number(readDouble(packet, 24), 'f', 7),
        QString::number(readDouble(packet, 32), 'f', 7), QString::number(readFloat(packet, 40), 'f', 1),
        frameTime(packet), QDateTime::currentDateTime().addMSecs(acceptedAt - m_clock.elapsed()).toString("yyyy-MM-dd HH:mm:ss.zzz"),
        QString("%1 → %2").arg(m_settings.senderId).arg(m_settings.receiverId), QStringLiteral("—")};
}
void Module::refreshWindow()
{
    if (!m_exchangeTable) return;
    QString connection = QStringLiteral("未启动"), tone = QStringLiteral("idle");
    if (m_connected) { connection = QStringLiteral("已连接"); tone = QStringLiteral("good"); }
    else if (m_connectionText.contains(QStringLiteral("监听失败"))) { connection = QStringLiteral("监听失败"); tone = QStringLiteral("bad"); }
    else if (m_connectionText.contains(QStringLiteral("配置非法"))) { connection = QStringLiteral("配置错误"); tone = QStringLiteral("bad"); }
    else if (m_connectionText.contains(QStringLiteral("已断开"))) { connection = QStringLiteral("已断开"); tone = QStringLiteral("bad"); }
    else if (m_connectionText.contains(QStringLiteral("已监听"))) { connection = QStringLiteral("等待光电连接"); tone = QStringLiteral("waiting"); }
    updateStatusCard(m_tcpStatusLabel, QStringLiteral("TCP连接：") + connection, tone, m_connectionText);
    const QString report = m_selected < 0 ? QStringLiteral("当前目标：无\n") + (m_lastSentFields.isEmpty()
        ? QStringLiteral("尚未下发") : QStringLiteral("已停止下发"))
        : QString("当前目标：批号 %1\n%2").arg(m_selected).arg(m_busy
            ? QStringLiteral("正在发送") : m_resultText);
    updateStatusCard(m_reportStatusLabel, report, m_selected < 0 ? QStringLiteral("idle")
        : !m_connected || m_timedOut ? QStringLiteral("waiting") : QStringLiteral("active"));
    updateStatusCard(m_localEndpointLabel, QString("雷达地址\n%1:%2").arg(m_settings.localIp).arg(m_settings.port), "idle");
    updateStatusCard(m_peerEndpointLabel, QStringLiteral("光电地址\n") + m_settings.peerIp, "idle");
    QStringList columns[3] = {m_lastSentFields, m_currentResponse.fields, m_previousResponse.fields};
    for (int column = 0; column < 3; ++column) {
        if (columns[column].isEmpty()) {
            for (int row = 0; row < ExchangeRows; ++row) columns[column].append(QStringLiteral("—"));
            const int batch = column == 1 ? m_currentResponse.batch : column == 2 ? m_previousResponse.batch : -1;
            if (batch >= 0) columns[column][BatchRow] = QString::number(batch);
            columns[column][ResultRow] = column == 0 ? QStringLiteral("尚未下发")
                : batch >= 0 ? QStringLiteral("等待回传") : QStringLiteral("暂无回传");
        }
        for (int row = 0; row < ExchangeRows; ++row) {
            auto* item = m_exchangeTable->item(row, column + 1);
            const QString value = columns[column].value(row, QStringLiteral("—"));
            if (item->text() != value) { item->setText(value); item->setToolTip(value); }
        }
    }
    m_stopButton->setEnabled(m_selected >= 0);
}
void Module::showWindow(QWidget* parent)
{
    if (m_window) { m_window->show(); publish(); m_window->raise(); m_window->activateWindow(); return; }
    auto* window = new CusWindow(QStringLiteral("光电目标指示"), QIcon(":/resources/icon/radararray.png"), parent);
    window->setAttribute(Qt::WA_DeleteOnClose); m_window = window;
    auto* content = new QWidget(window); auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(14, 12, 14, 12); layout->setSpacing(12);
    content->setObjectName(QStringLiteral("OpticalIndicationContent"));
    content->setAttribute(Qt::WA_StyledBackground);
    // This detached window cannot rely on the main panel's label palette.
    content->setStyleSheet(QStringLiteral(
        "QWidget#OpticalIndicationContent { background-color:#101818; color:#D9FFFF; }"
        "QWidget#OpticalIndicationContent QLabel { color:#D9FFFF; background:transparent; font-size:22px; }"
        "QWidget#OpticalIndicationContent QLabel[section=\"true\"] { color:#66FFCC; font-size:24px; font-weight:bold; }"
        "QWidget#OpticalIndicationContent QLabel[card=\"true\"] { border:1px solid #426960; border-radius:6px; padding:10px; background:#123630; }"
        "QWidget#OpticalIndicationContent QLabel[state=\"good\"] { color:#FFFFFF; background:#008E55; border-color:#66FFCC; }"
        "QWidget#OpticalIndicationContent QLabel[state=\"bad\"] { color:#FFFFFF; background:#972B31; border-color:#FF7878; }"
        "QWidget#OpticalIndicationContent QLabel[state=\"waiting\"] { color:#FFE7A3; background:#514326; border-color:#D7B65D; }"
        "QWidget#OpticalIndicationContent QLabel[state=\"active\"] { color:#66FFCC; background:#124E46; border-color:#66FFCC; }"
        "QWidget#OpticalIndicationContent QTableWidget { color:#D9FFFF; background:#101E1E; alternate-background-color:#152D2A; "
        "font-size:22px; border:1px solid #426960; gridline-color:#31554E; selection-background-color:#19554C; selection-color:#FFFFFF; }"
        "QWidget#OpticalIndicationContent QTableWidget::item { padding:5px; }"
        "QWidget#OpticalIndicationContent QHeaderView::section { color:#66FFCC; background:#173D37; font-size:22px; "
        "font-weight:bold; border:1px solid #426960; padding:8px; }"
        "QWidget#OpticalIndicationContent QPushButton { color:#66FFCC; background-color:#123630; font-size:22px; "
        "border:1px solid #66FFCC; border-radius:5px; min-height:38px; padding:4px 12px; }"
        "QWidget#OpticalIndicationContent QPushButton:hover { background-color:#195046; }"
        "QWidget#OpticalIndicationContent QPushButton:disabled { color:#809A93; border-color:#426960; background:#172724; }"));
    auto heading = [content, layout](const QString& text) {
        auto* label = new QLabel(text, content); label->setProperty("section", true); layout->addWidget(label);
    };
    heading(QStringLiteral("链路与下发状态"));
    auto* grid = new QGridLayout; grid->setSpacing(10); grid->setColumnStretch(0, 1); grid->setColumnStretch(1, 1);
    auto card = [content, grid](const QString& name, int row, int column) {
        auto* label = new QLabel(content); label->setObjectName(name); label->setProperty("card", true);
        label->setTextFormat(Qt::PlainText); label->setWordWrap(true); label->setAlignment(Qt::AlignCenter);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse); label->setMinimumHeight(58);
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred); grid->addWidget(label, row, column); return label;
    };
    m_tcpStatusLabel = card(QStringLiteral("OpticalTcpStatus"), 0, 0);
    m_reportStatusLabel = card(QStringLiteral("OpticalReportStatus"), 0, 1);
    m_localEndpointLabel = card(QStringLiteral("OpticalLocalEndpoint"), 1, 0);
    m_peerEndpointLabel = card(QStringLiteral("OpticalPeerEndpoint"), 1, 1);
    layout->addLayout(grid);
    heading(QStringLiteral("目标下发与回传"));
    m_exchangeTable = new QTableWidget(ExchangeRows, 4, content);
    m_exchangeTable->setObjectName(QStringLiteral("OpticalExchangeTable"));
    QFont tableFont = m_exchangeTable->font(); tableFont.setPixelSize(22);
    m_exchangeTable->setFont(tableFont); m_exchangeTable->horizontalHeader()->setFont(tableFont);
    m_exchangeTable->setHorizontalHeaderLabels({QStringLiteral("信息项"), QStringLiteral("最近下发"),
        QStringLiteral("当前批次回传"), QStringLiteral("上一批次回传")});
    m_exchangeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_exchangeTable->setAlternatingRowColors(true); m_exchangeTable->setWordWrap(false);
    m_exchangeTable->verticalHeader()->hide(); m_exchangeTable->verticalHeader()->setDefaultSectionSize(44);
    m_exchangeTable->horizontalHeader()->setMinimumSectionSize(160);
    m_exchangeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_exchangeTable->horizontalHeader()->setStretchLastSection(true);
    m_exchangeTable->horizontalHeader()->setSectionsMovable(false);
    m_exchangeTable->setColumnWidth(0, 180);
    for (int column = 1; column < 4; ++column) m_exchangeTable->setColumnWidth(column, 310);
    m_exchangeTable->setMinimumHeight(240);
    const QStringList rows{QStringLiteral("目标批号"), QStringLiteral("指示类型"), QStringLiteral("处理状态"),
        QStringLiteral("经度 (°)"), QStringLiteral("纬度 (°)"), QStringLiteral("高度 (m)"),
        m_settings.utc ? QStringLiteral("报文时间 (UTC)") : QStringLiteral("报文时间"), QStringLiteral("本机时间"),
        QStringLiteral("设备ID"), QStringLiteral("应答关联")};
    for (int row = 0; row < ExchangeRows; ++row) {
        for (int column = 0; column < 4; ++column) {
            auto* item = new QTableWidgetItem(column == 0 ? rows[row] : QStringLiteral("—"));
            item->setTextAlignment(Qt::AlignCenter);
            if (column == 0) item->setForeground(QColor("#66FFCC"));
            m_exchangeTable->setItem(row, column, item);
        }
    }
    layout->addWidget(m_exchangeTable, 1);
    m_stopButton = new QPushButton(QStringLiteral("停止下发"), content);
    m_stopButton->setObjectName(QStringLiteral("OpticalStopReport")); layout->addWidget(m_stopButton);
    connect(m_stopButton, &QPushButton::clicked, this, &Module::stopReport);
    connect(window, &QObject::destroyed, this, [this] {
        m_tcpStatusLabel = m_reportStatusLabel = m_localEndpointLabel = m_peerEndpointLabel = nullptr;
        m_exchangeTable = nullptr; m_stopButton = nullptr;
    });
    auto* scroll = new QScrollArea(window); scroll->setObjectName(QStringLiteral("OpticalIndicationScroll"));
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea#OpticalIndicationScroll { background:#101818; border:0; }"));
    scroll->setWidget(content);
    window->setContentWidget(scroll); window->setMinimumSize(640, 480);
    refreshWindow();
    QScreen* screen = parent ? QGuiApplication::screenAt(parent->mapToGlobal(parent->rect().center())) : nullptr;
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 960);
    // CusWindow::showEvent calls adjustSize(); size afterwards to retain this operator layout.
    window->show(); window->resize(qMin(1240, area.width() - 32), qMin(920, area.height() - 32));
    window->move(area.center() - window->rect().center()); publish();
}
}
