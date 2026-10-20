#include "module.h"
#include "protocol.h"
#include "Basic/ConfigManager.h"
#include "cusWidgets/cuswindow.h"
#include "cusWidgets/custommessagebox.h"
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QDateTime>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalBlocker>
#include <limits>

namespace PhotoelectricV2 {
void Worker::start()
{
    m_control = new QUdpSocket(this);
    m_target = new QUdpSocket(this);
    m_status = new QUdpSocket(this);
    m_target->setReadBufferSize(2 * 1024 * 1024);
    m_status->setReadBufferSize(2 * 1024 * 1024);
    const QHostAddress local(m_settings.localIp);
    if (!m_control->bind(local, m_settings.controlLocalPort, QUdpSocket::DontShareAddress)
        || !m_target->bind(local, m_settings.targetPort, QUdpSocket::DontShareAddress)
        || !m_status->bind(local, m_settings.statusPort, QUdpSocket::DontShareAddress)) {
        const QString error = QStringLiteral("绑定失败 control=%1 target=%2 status=%3；修复后重启")
            .arg(m_control->errorString(), m_target->errorString(), m_status->errorString());
        m_control->close(); m_target->close(); m_status->close();
        emit ready(false, error);
        return;
    }
    connect(m_target, &QUdpSocket::readyRead, this, [this] { receive(m_target, false); });
    connect(m_status, &QUdpSocket::readyRead, this, [this] { receive(m_status, true); });
    m_deliveryTimer = new QTimer(this);
    m_deliveryTimer->setInterval(100);
    connect(m_deliveryTimer, &QTimer::timeout, this, &Worker::deliver);
    m_deliveryTimer->start();
    emit ready(true, QStringLiteral("UDP就绪；本机发送成功不代表对端已接收"));
}

void Worker::send(const QList<QByteArray>& frames)
{
    int count = 0;
    QString error;
    if (!m_control || m_control->state() != QAbstractSocket::BoundState) {
        emit sent(0, QStringLiteral("UDP未绑定")); return;
    }
    for (const QByteArray& frame : frames) {
        if (m_control->writeDatagram(frame, QHostAddress(m_settings.peerIp), m_settings.controlPort) != frame.size()) {
            error = m_control->errorString(); break;
        }
        ++count;
    }
    emit sent(count, error);
}

void Worker::receive(QUdpSocket* socket, bool statusChannel)
{
    // A bounded drain lets transmit/acknowledgement events run under excessive input.
    int budget = 128;
    while (budget-- && socket->hasPendingDatagrams()) {
        if (socket->pendingDatagramSize() > 1472) {
            char discard[1]; socket->readDatagram(discard, 1);
            ++m_rejected; m_lastError = QStringLiteral("数据报超过1472字节"); continue;
        }
        const auto datagram = socket->receiveDatagram();
        QVariantMap fields;
        QString error;
        const quint16 sourcePort = statusChannel ? m_settings.peerStatusPort : m_settings.peerTargetPort;
        if (datagram.senderAddress() != QHostAddress(m_settings.peerIp)
            || (sourcePort && datagram.senderPort() != sourcePort)) {
            ++m_rejected; m_lastError = QStringLiteral("来源IP/端口不匹配"); continue;
        }
        if (!parse(datagram.data(), fields, error)) {
            ++m_rejected; m_lastError = error + " hex=" + datagram.data().left(80).toHex(' '); continue;
        }
        const int function = fields["function"].toInt();
        if ((statusChannel && function == Target) || (!statusChannel && function != Target)
            || (function != Heartbeat && fields["device"].toUInt() != m_settings.device)
            || (m_settings.terminal && fields["terminal"].toUInt() != m_settings.terminal)) {
            ++m_rejected; m_lastError = QStringLiteral("接收通道/设备ID/终端ID不匹配"); continue;
        }
        QElapsedTimer arrival; arrival.start();
        fields["receivedMs"] = arrival.msecsSinceReference() - m_settings.timeOriginMs;
        const QString key = QString::number(function) + ':' + fields.value("batch").toString();
        if (function != Target || m_latest.size() < 256 || m_latest.contains(key)) {
            m_latest.insert(key, fields); ++m_received;
        }
        else { ++m_rejected; m_lastError = QStringLiteral("目标接收缓存已满"); }
    }
    const int index = statusChannel ? 1 : 0;
    if (socket->hasPendingDatagrams() && !m_drainScheduled[index]) {
        m_drainScheduled[index] = true;
        QTimer::singleShot(1, this, [this, socket, statusChannel, index] {
            m_drainScheduled[index] = false; receive(socket, statusChannel);
        });
    }
}

void Worker::deliver()
{
    if (m_deliveryPending) return;
    QVariantList frames;
    frames.reserve(m_latest.size() + 1);
    if (m_rejected || m_received) {
        QVariantMap rejected;
        rejected["function"] = -1;
        rejected["count"] = QVariant::fromValue<qulonglong>(m_rejected);
        rejected["received"] = QVariant::fromValue<qulonglong>(m_received);
        rejected["reason"] = m_lastError;
        frames.append(rejected); m_rejected = 0; m_received = 0;
    }
    for (const auto& fields : m_latest) frames.append(fields);
    m_latest.clear();
    if (!frames.isEmpty()) {
        m_deliveryPending = true;
        emit received(frames);
    }
}

Module::Module(QObject* parent) : QObject(parent)
{
    m_clock.start();
    connect(&m_timer, &QTimer::timeout, this, &Module::flush);
}
Module::~Module()
{
    m_timer.stop();
    m_thread.quit(); m_thread.wait();
    delete m_window.data();
}
void Module::init()
{
    if (!CF_INS.photoelectricV2Enabled()) return;
    auto text = [](const QString& k, const QString& d) { return CF_INS.photoelectricV2Value(k, d).toString().trimmed(); };
    bool valid = true;
    auto number = [&](const QString& k, quint64 d, quint64 lo, quint64 hi) {
        bool ok = false;
        const QVariant value = CF_INS.photoelectricV2Value(k, QVariant::fromValue<qulonglong>(d));
        quint64 v = 0;
        if (value.type() == QVariant::Double) {
            // The existing TOML reader stores unsigned values above INT_MAX as double.
            const double raw = value.toDouble();
            ok = std::isfinite(raw) && raw >= double(lo) && raw <= double(hi) && std::floor(raw) == raw;
            if (ok) v = quint64(raw);
        } else v = value.toString().toULongLong(&ok, 0);
        if (!ok || v < lo || v > hi) { valid = false; emit logMessage("[PE-V2][CONFIG] 非法参数 " + k); return d; }
        return v;
    };
    m_settings.localIp = text("local_ip", "192.168.101.14");
    m_settings.peerIp = text("peer_ip", "192.168.101.10");
    m_settings.targetPort = quint16(number("target_port", 10100, 1, 65535));
    m_settings.statusPort = quint16(number("status_port", 11100, 1, 65535));
    m_settings.controlLocalPort = quint16(number("control_local_port", 10101, 1, 65535));
    m_settings.controlPort = quint16(number("control_port", 21001, 1, 65535));
    m_settings.peerTargetPort = quint16(number("peer_target_port", 0, 0, 65535));
    m_settings.peerStatusPort = quint16(number("peer_status_port", 0, 0, 65535));
    m_settings.device = quint32(number("device_id", 1, 0, 0xffffffffULL));
    m_settings.terminal = quint32(number("terminal_id", 0, 0, 0xffffffffULL));
    m_settings.intervalMs = int(number("guidance_interval_ms", 1000, 50, 60000));
    m_settings.timeoutMs = int(number("receive_timeout_ms", 5000, 1000, 60000));
    m_settings.trackTtlMs = int(number("track_timeout_ms", 10000, 1000, 120000));
    m_settings.maxTracks = int(number("max_tracks", 1024, 1, 4096));
    m_settings.logIntervalMs = int(number("log_interval_ms", 5000, 1000, 60000));
    const QHostAddress local(m_settings.localIp), peer(m_settings.peerIp);
    if (local.protocol() != QAbstractSocket::IPv4Protocol || local == QHostAddress::AnyIPv4 || local == QHostAddress::Broadcast
        || peer.protocol() != QAbstractSocket::IPv4Protocol || peer.isMulticast()
        || peer == QHostAddress::AnyIPv4 || peer == QHostAddress::Broadcast
        || m_settings.targetPort == m_settings.statusPort || m_settings.targetPort == m_settings.controlLocalPort
        || m_settings.statusPort == m_settings.controlLocalPort) valid = false;
    emit logMessage(QString("[PE-V2][CONFIG] path=%1 local=%2 target=%3 status=%4 tx=%5 -> %6:%7 device=%8 terminal=%9")
        .arg(CF_INS.getConfigFilePath(), m_settings.localIp).arg(m_settings.targetPort).arg(m_settings.statusPort)
        .arg(m_settings.controlLocalPort).arg(m_settings.peerIp).arg(m_settings.controlPort)
        .arg(m_settings.device).arg(m_settings.terminal));
    if (!valid) {
        m_socketStatus = QStringLiteral("配置错误，V2链路未启动");
        emit logMessage("[PE-V2][CONFIG][ERROR] " + m_socketStatus); publishStatus(); return;
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    m_origin = {CF_INS.longitude("longitude", nan), CF_INS.latitude("latitude", nan), CF_INS.altitude("altitude", nan)};
    m_originValid = Wgs84Coordinate::isValid(m_origin);
    emit logMessage(QString("[PE-V2][ORIGIN] CONFIG valid=%1 lon=%2 lat=%3 alt=%4；不额外叠加姿态")
        .arg(m_originValid).arg(m_origin.longitudeDeg, 0, 'f', 7).arg(m_origin.latitudeDeg, 0, 'f', 7).arg(m_origin.altitudeM));
    qRegisterMetaType<QList<QByteArray>>("QList<QByteArray>");
    m_settings.timeOriginMs = m_clock.msecsSinceReference();
    auto* worker = new Worker(m_settings);
    worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, worker, &Worker::start);
    connect(&m_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Module::sendFrames, worker, &Worker::send, Qt::QueuedConnection);
    connect(this, &Module::acknowledgeReceive, worker, &Worker::acknowledge, Qt::QueuedConnection);
    connect(worker, &Worker::ready, this, [this](bool ok, const QString& text) {
        m_ready = ok; m_socketStatus = text; emit logMessage("[PE-V2][UDP] " + text); publishStatus();
    });
    connect(worker, &Worker::sent, this, [this](int count, const QString& error) {
        m_busy = false;
        m_txCount += quint64(count);
        logLimited(error.isEmpty() ? "TX" : "TX_ERROR", QString("本机本批发送=%1 累计=%2 error=%3").arg(count).arg(m_txCount).arg(error));
    });
    connect(worker, &Worker::received, this, &Module::consume);
    m_thread.setObjectName(QStringLiteral("PhotoelectricV2Udp"));
    m_thread.start();
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(qMin(100, m_settings.intervalMs));
}

void Module::updateRadarPosition(double latitude, double longitude, double altitude)
{
    Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (!Wgs84Coordinate::isValid(origin)) { logLimited("ORIGIN_INVALID", "DD05经纬高非法，保留之前有效原点"); return; }
    m_origin = origin; m_originValid = true;
    if (!m_dd05) emit logMessage("[PE-V2][ORIGIN] 切换到真实DD05原点");
    m_dd05 = true;
}
void Module::processTrack(const PointInfo& point)
{
    if (point.type != PointType::Track || point.statMethod == 250) return;
    const int batch = int(point.batch);
    if (point.statMethod == 2) {
        m_tracks.remove(batch);
        m_reportingDirty = true;
        if (m_selected == batch) stopReport();
        return; // V2 has no cancel flag: never invent a cancel/control command.
    }
    if (!std::isfinite(point.range) || point.range < 0 || !std::isfinite(point.azimuth)
        || !std::isfinite(point.elevation) || std::abs(point.elevation) > 90 || !std::isfinite(point.speed)) {
        logLimited("TRACK_INVALID", "拒绝非法普通航迹"); return;
    }
    if (!m_tracks.contains(batch) && m_tracks.size() >= m_settings.maxTracks) {
        auto oldest = m_tracks.begin();
        for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) if (it->updated < oldest->updated) oldest = it;
        const int evicted = oldest.key(); m_tracks.erase(oldest);
        if (m_selected == evicted) stopReport();
        logLimited("TRACK_CAPACITY", "最新航迹缓存达到上限，淘汰最旧记录");
    }
    if (m_auto && !m_tracks.contains(batch)) m_reportingDirty = true;
    m_tracks.insert(batch, {point, m_clock.elapsed()});
}
bool Module::hasTrack(int batch) const
{
    const auto it = m_tracks.constFind(batch);
    return it != m_tracks.cend() && m_clock.elapsed() - it->updated <= m_settings.trackTtlMs;
}
bool Module::isReporting(int batch) const { return hasTrack(batch) && (m_auto || m_selected == batch); }
QByteArray Module::encode(const PointInfo& p) const
{
    double east, north, up;
    Wgs84Coordinate::Lla target;
    if (!m_originValid || !Wgs84Coordinate::polarToEnu(p.range, p.azimuth, p.elevation, east, north, up)
        || !Wgs84Coordinate::enuToLla(m_origin, east, north, up, target)) return {};
    // The V2 table specifies unsigned speed. Use the magnitude of the existing radial speed.
    return guidance(m_settings.device, m_settings.terminal, quint64(QDateTime::currentMSecsSinceEpoch()),
                    p.batch, target.longitudeDeg, target.latitudeDeg, target.altitudeM, std::abs(double(p.speed)), 0);
}
bool Module::startReport(int batch)
{
    if (!m_ready || !hasTrack(batch) || encode(m_tracks.value(batch).point).isEmpty()) {
        emit logMessage(QString("[PE-V2][SELECT][ERROR] batch=%1 UDP/当前航迹/原点/字段不满足发送条件").arg(batch)); return false;
    }
    m_selected = batch; m_lastTx = -1;
    emit logMessage(QString("[PE-V2][SELECT] 持续引导 batch=%1").arg(batch));
    emit reportingStateChanged(); flush(); return true;
}
void Module::stopReport()
{
    if (m_selected < 0) return;
    emit logMessage(QString("[PE-V2][STOP] batch=%1 停止引导（协议无消批指令）").arg(m_selected));
    m_selected = -1; emit reportingStateChanged();
}
void Module::setAutoReport(bool enabled)
{
    if (m_auto == enabled) return;
    if (enabled && (!m_ready || !m_originValid)) { emit logMessage("[PE-V2][AUTO][ERROR] UDP/原点未就绪"); return; }
    m_auto = enabled; m_lastTx = -1;
    emit logMessage(QString("[PE-V2][AUTO] %1").arg(enabled));
    emit reportingStateChanged(); flush();
}
void Module::flush()
{
    const qint64 now = m_clock.elapsed();
    bool expired = false;
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (now - it->updated > m_settings.trackTtlMs) {
            if (it.key() == m_selected) stopReport();
            it = m_tracks.erase(it); expired = true;
        } else ++it;
    }
    if (expired || m_reportingDirty) { m_reportingDirty = false; emit reportingStateChanged(); }
    publishStatus();
    if (m_window && m_window->isVisible() && (m_lastUi < 0 || now - m_lastUi >= 500)) {
        m_lastUi = now; refreshWindow();
    }
    if (!m_ready || m_busy) return;
    QList<QByteArray> frames;
    while (!m_controls.isEmpty()) frames.append(m_controls.dequeue());
    if (m_lastTx < 0 || now - m_lastTx >= m_settings.intervalMs) {
        for (auto it = m_tracks.cbegin(); it != m_tracks.cend(); ++it) {
            if (!m_auto && m_selected != it.key()) continue;
            const QByteArray frame = encode(it->point);
            if (!frame.isEmpty()) {
                frames.append(frame);
                logLimited("GUIDE", QString("batch=%1 hex=%2").arg(it.key()).arg(QString::fromLatin1(frame.toHex(' '))));
            }
            else logLimited("ENCODE", "原点/经纬高/速度超出V2无符号字段范围，拒绝引导");
        }
        m_lastTx = now;
    }
    if (!frames.isEmpty()) { m_busy = true; emit sendFrames(frames); }
}
void Module::consume(const QVariantList& frames)
{
    const qint64 now = m_clock.elapsed();
    for (const auto& value : frames) {
        const auto fields = value.toMap();
        const int f = fields.value("function").toInt();
        if (f == -1) {
            const quint64 count = fields.value("count").toULongLong(); m_rejectedCount += count;
            m_rxCount += fields.value("received").toULongLong();
            if (count) logLimited("RX_REJECT", QString("本批拒绝=%1 累计拒绝=%2 最近原因=%3")
                .arg(count).arg(m_rejectedCount).arg(fields.value("reason").toString()));
            continue;
        }
        // Timeout is measured at UDP receipt, not delayed delivery to a busy GUI.
        const qint64 receivedAt = fields.value("receivedMs", now).toLongLong();
        if (f == Target) {
            const quint32 batch = fields.value("batch").toUInt();
            if (!m_targets.contains(batch) && m_targets.size() >= 256) m_targets.erase(m_targets.begin());
            auto target = fields; target["receivedMs"] = receivedAt;
            m_targets.insert(batch, target); m_lastTarget = receivedAt;
        } else if (f == Status) { m_deviceState = fields; m_lastStatus = receivedAt; }
        else if (f == Heartbeat) { m_heartbeat = fields; m_lastHeartbeat = receivedAt; }
        auto loggedFields = fields;
        // Preserve the full uint64 identifier; QJsonValue otherwise rounds it to double.
        for (const QString& key : {QStringLiteral("guideTarget"), QStringLiteral("timestamp")})
            if (loggedFields.contains(key)) loggedFields[key] = loggedFields[key].toString();
        logLimited(QString("RX_%1").arg(f, 4, 16, QChar('0')),
            QString("累计接收=%1 ").arg(m_rxCount) + QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(loggedFields)).toJson(QJsonDocument::Compact)));
    }
    emit acknowledgeReceive(); publishStatus();
}
void Module::logLimited(const QString& category, const QString& text)
{
    const auto now = m_clock.elapsed();
    if (!m_lastLogs.contains(category) || now - m_lastLogs.value(category) >= m_settings.logIntervalMs) {
        m_lastLogs[category] = now;
        emit logMessage("[PE-V2][" + category + "] " + text);
    }
}
QString Module::statusText() const
{
    const auto now = m_clock.elapsed();
    auto state = [&](qint64 last) { return last < 0 ? QStringLiteral("未收到")
        : now - last > m_settings.timeoutMs ? QStringLiteral("超时") : QStringLiteral("已收到"); };
    return QString("光电V2：%1 / 目标:%2 / 设备状态:%3 / 终端心跳:%4")
        .arg(m_socketStatus, state(m_lastTarget), state(m_lastStatus), state(m_lastHeartbeat));
}
void Module::publishStatus()
{
    const QString text = statusText();
    if (m_publishedStatus == text) return;
    m_publishedStatus = text;
    const bool ok = m_ready && m_lastStatus >= 0 && m_clock.elapsed() - m_lastStatus <= m_settings.timeoutMs;
    emit statusChanged(ok, text); emit logMessage("[PE-V2][LINK] " + text);
}

void Module::showWindow(QWidget* parent)
{
    if (m_window) { m_window->show(); m_window->raise(); m_window->activateWindow(); return; }
    auto* window = new CusWindow(QStringLiteral("光电 V2.0 控制与状态"), QIcon(":/resources/icon/radararray.png"), parent);
    window->setAttribute(Qt::WA_DeleteOnClose); m_window = window;
    auto* content = new QWidget(window);
    auto* layout = new QVBoxLayout(content);
    m_statusLabel = new QLabel(content); m_statusLabel->setWordWrap(true); layout->addWidget(m_statusLabel);
    m_deviceLabel = new QLabel(content); m_deviceLabel->setWordWrap(true); layout->addWidget(m_deviceLabel);
    auto* grid = new QGridLayout;
    const QStringList names = {"转台上电", "激光测距", "电视上电", "红外上电", "云台运动", "焦距控制类型", "可见光焦距", "红外焦距"};
    const QList<QStringList> options = {{"关闭", "上电"}, {"关闭", "开启"}, {"关闭", "上电"}, {"关闭", "上电"},
        {"停止", "向左", "向右", "向上", "向下"}, {"红外", "可见光"},
        {"停止", "长焦", "短焦", "聚焦+", "聚焦-", "透雾关", "透雾开", "自动聚焦"},
        {"停止", "长焦", "短焦", "聚焦+", "聚焦-", "内部矫正", "自动聚焦"}};
    QList<QComboBox*> controls;
    for (int i = 0; i < names.size(); ++i) {
        auto* combo = new QComboBox(content); combo->addItems(options[i]); controls.append(combo);
        grid->addWidget(new QLabel(names[i], content), i / 2, (i % 2) * 2);
        grid->addWidget(combo, i / 2, (i % 2) * 2 + 1);
    }
    layout->addLayout(grid);
    auto* sendButton = new QPushButton("确认下发设备控制 (0x3500)", content);
    layout->addWidget(sendButton);
    connect(sendButton, &QPushButton::clicked, window, [this, controls, window] {
        if (!m_ready || m_controls.size() >= 16) { CustomMessageBox::showWarning(window, "光电V2", "UDP未就绪或控制队列已满"); return; }
        if (!CustomMessageBox::showConfirm(window, "光电V2", "将按当前8项配置下发设备控制，是否继续？")) return;
        // The confirmation dialog runs a nested event loop; recheck queue/readiness afterwards.
        if (!m_ready || m_controls.size() >= 16) return;
        QByteArray commands;
        for (auto* combo : controls) commands.append(char(combo->currentIndex()));
        const auto frame = control(m_settings.device, m_settings.terminal, quint64(QDateTime::currentMSecsSinceEpoch()), commands);
        if (frame.isEmpty()) return;
        m_controls.enqueue(frame);
        emit logMessage("[PE-V2][CONTROL] 入队（非设备执行确认） hex=" + frame.toHex(' ')); flush();
    });
    auto* autoReport = new QCheckBox("自动引导全部当前普通航迹", content); autoReport->setChecked(m_auto); layout->addWidget(autoReport);
    connect(autoReport, &QCheckBox::toggled, window, [this, autoReport](bool on) {
        setAutoReport(on); const QSignalBlocker blocker(autoReport); autoReport->setChecked(m_auto);
    });
    auto* stop = new QPushButton("停止手动持续引导", content); layout->addWidget(stop);
    connect(stop, &QPushButton::clicked, this, &Module::stopReport);
    m_targetTable = new QTableWidget(0, 9, content);
    m_targetTable->setHorizontalHeaderLabels({"光电批号", "引导编号", "经度", "纬度", "海拔(m)", "方位/俯仰", "距离(m)", "属性", "时效"});
    m_targetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_targetTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(m_targetTable);
    window->setContentWidget(content); window->resize(960, 650);
    connect(window, &QObject::destroyed, this, [this] { m_statusLabel = nullptr; m_deviceLabel = nullptr; m_targetTable = nullptr; });
    refreshWindow(); window->show();
}
void Module::refreshWindow()
{
    if (!m_statusLabel || !m_targetTable) return;
    m_statusLabel->setText(statusText());
    if (m_deviceState.isEmpty()) m_deviceLabel->setText("设备状态未上报（不以默认正常代替）");
    else m_deviceLabel->setText(QString("设备经纬高 %1, %2, %3m；转台 %4° / %5°；模式=%6；故障位=0x%7")
        .arg(m_deviceState["longitude"].toDouble(), 0, 'f', 5).arg(m_deviceState["latitude"].toDouble(), 0, 'f', 5)
        .arg(m_deviceState["altitude"].toInt()).arg(m_deviceState["azimuth"].toDouble()).arg(m_deviceState["elevation"].toDouble())
        .arg(m_deviceState["mode"].toInt()).arg(m_deviceState["fault"].toUInt(), 2, 16, QChar('0')));
    m_targetTable->setRowCount(m_targets.size());
    int row = 0;
    for (const auto& target : m_targets) {
        const QStringList cells = {target["batch"].toString(), target["guideTarget"].toString(),
            QString::number(target["longitude"].toDouble(), 'f', 5), QString::number(target["latitude"].toDouble(), 'f', 5),
            target["altitude"].toString(), QString("%1 / %2").arg(target["azimuth"].toDouble()).arg(target["elevation"].toDouble()),
            target["range"].toString(), target["attribute"].toString(),
            m_clock.elapsed() - target["receivedMs"].toLongLong() > m_settings.timeoutMs ? QStringLiteral("超时") : QStringLiteral("最新")};
        for (int c = 0; c < cells.size(); ++c) {
            auto* item = m_targetTable->item(row, c);
            if (!item) { item = new QTableWidgetItem; m_targetTable->setItem(row, c, item); }
            if (item->text() != cells[c]) item->setText(cells[c]);
        }
        ++row;
    }
}
} // namespace PhotoelectricV2
