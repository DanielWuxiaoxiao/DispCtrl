#include "radartrackapi.h"
#include "Basic/ConfigManager.h"
#include "Basic/uifontscale.h"
#include "Basic/uithemeruntime.h"
#include "cusWidgets/cuswindow.h"
#include <QCheckBox>
#include <QDateTime>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QUuid>
#include <QVBoxLayout>
#include <QtEndian>
#include <cstring>
#include <limits>
#include <algorithm>

namespace RadarTrackApi {
namespace {
qint64 monotonicMs() { QElapsedTimer clock; clock.start(); return clock.msecsSinceReference(); }
bool validEndpoint(const QUrl& url)
{
    return url.isValid() && (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
        && !url.host().isEmpty() && url.userInfo().isEmpty() && !url.hasQuery() && !url.hasFragment();
}
Settings bounded(Settings s)
{
    s.reportIntervalMs = qBound(100, s.reportIntervalMs, 60000);
    s.requestIntervalMs = qBound(50, s.requestIntervalMs, 60000);
    s.timeoutMs = qBound(100, s.timeoutMs, 30000);
    s.trackTimeoutMs = qBound(500, s.trackTimeoutMs, 120000);
    s.maxTracks = qBound(1, s.maxTracks, 4096);
    s.logIntervalMs = qBound(1000, s.logIntervalMs, 60000);
    s.model = s.model.trimmed().left(128);
    if (s.model.isEmpty()) s.model = QStringLiteral("RADAR");
    s.idPrefix = s.idPrefix.trimmed().left(64);
    return s;
}
}

Settings readSettings()
{
    Settings s;
    const auto value = [](const QString& key, const QVariant& fallback) { return CF_INS.radarTrackApiValue(key, fallback); };
    s.endpoint = QUrl(value("endpoint", s.endpoint.toString()).toString());
    const QByteArray envName = value("token_env", "DISPCTRL_RADAR_TRACK_API_TOKEN").toString().toLocal8Bit();
    s.token = envName.isEmpty() ? QByteArray() : qgetenv(envName.constData()).trimmed();
    if (s.token.isEmpty()) s.token = value("api_token", "").toString().trimmed().toUtf8();
    s.model = value("model", s.model).toString(); s.idPrefix = value("track_id_prefix", s.idPrefix).toString();
    s.autoReport = value("auto_report", false).toBool();
    s.logEnabled = value("log_enabled", true).toBool();
    s.logPayloadEnabled = value("log_payload_enabled", false).toBool();
    s.logIntervalMs = value("log_interval_ms", s.logIntervalMs).toInt();
    s.reportIntervalMs = value("report_interval_ms", s.reportIntervalMs).toInt();
    s.requestIntervalMs = value("request_interval_ms", s.requestIntervalMs).toInt();
    s.timeoutMs = value("response_timeout_ms", s.timeoutMs).toInt();
    s.trackTimeoutMs = value("track_timeout_ms", s.trackTimeoutMs).toInt();
    s.maxTracks = value("max_tracks", s.maxTracks).toInt();
    return bounded(s);
}

QByteArray payload(const QString& trackId, const PointInfo& point,
                   const Wgs84Coordinate::Lla& origin, qint64 receivedUtcMs, const QString& model, const Motion& motion)
{
    double east, north, up; Wgs84Coordinate::Lla target;
    if (trackId.trimmed().isEmpty() || receivedUtcMs <= 0 || point.type != PointType::Track
        || point.statMethod == 2 || point.statMethod == 250
        || !Wgs84Coordinate::polarToEnu(point.range, point.azimuth, point.elevation, east, north, up)
        || !Wgs84Coordinate::enuToLla(origin, east, north, up, target)) return {};
    QJsonObject json;
    json["trackId"] = trackId;
    json["longitude"] = target.longitudeDeg; json["latitude"] = target.latitudeDeg;
    json["altitude"] = target.altitudeM;
    json["timestamp"] = double(receivedUtcMs); json["model"] = model;
    if (std::isfinite(motion.speedMps) && motion.speedMps >= 0.0) json["speed"] = motion.speedMps;
    if (std::isfinite(motion.headingDeg) && motion.headingDeg >= 0.0 && motion.headingDeg < 360.0)
        json["heading"] = motion.headingDeg;
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

Transport::Transport(const Settings& settings, std::shared_ptr<std::atomic<quint64>> generation)
    : m_settings(settings), m_generation(std::move(generation)) {}
Transport::~Transport()
{
    if (m_reply) { m_reply->disconnect(this); m_reply->abort(); }
}
void Transport::start()
{
    m_manager = new QNetworkAccessManager(this);
    m_timeout = new QTimer(this); m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        if (m_reply) { m_timedOut = true; m_reply->abort(); }
    });
}
void Transport::send(const Request& request)
{
    if (!m_manager || m_reply || request.generation != m_generation->load(std::memory_order_relaxed)
        || monotonicMs() >= request.expiresAt) {
        emit completed(request.generation, request.batch, false, QStringLiteral("请求已取消或样本已过期")); return;
    }
    m_request = request; m_response.clear(); m_timedOut = false; m_tooLarge = false;
    QNetworkRequest http(m_settings.endpoint);
    http.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    http.setRawHeader("X-Api-Token", m_settings.token);
    // 禁止自动重定向，避免认证头随重定向发送到其他地址。
    http.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    m_reply = m_manager->post(http, request.body);
    m_reply->setReadBufferSize(65537);
    connect(m_reply, &QIODevice::readyRead, this, &Transport::drain);
    connect(m_reply, &QNetworkReply::finished, this, &Transport::finish);
    m_timeout->start(m_settings.timeoutMs);
}
void Transport::drain()
{
    if (!m_reply || m_tooLarge) return;
    m_response += m_reply->read(65537 - m_response.size());
    if (m_response.size() > 65536) { m_tooLarge = true; m_reply->abort(); }
}
void Transport::finish()
{
    // drain() 的 abort 可能同步触发 finished，故只在 finished 中完成一次收尾。
    if (!m_reply) return;
    if (!m_tooLarge && m_reply->isOpen()) m_response += m_reply->read(65537 - m_response.size());
    m_tooLarge = m_tooLarge || m_response.size() > 65536;
    m_timeout->stop();
    const int http = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto error = m_reply->error();
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(m_response, &parse);
    const auto code = document.object().value(QStringLiteral("code"));
    const bool ok = !m_timedOut && !m_tooLarge && error == QNetworkReply::NoError
        && http >= 200 && http < 300 && parse.error == QJsonParseError::NoError
        && document.isObject() && code.isDouble() && code.toDouble() == 200.0;
    // 不把服务端 message/errorString 原文写进日志，防止其中回显认证信息。
    const QString result = m_timedOut ? QStringLiteral("请求超时，平台结果未知")
        : m_tooLarge ? QStringLiteral("响应超出64KiB限制")
        : QStringLiteral("HTTP %1 / API %2：%3").arg(http)
            .arg(code.isDouble() ? QString::number(code.toDouble()) : QStringLiteral("无有效code"))
            .arg(ok ? QStringLiteral("平台接收成功") : QStringLiteral("未确认接收成功"));
    m_reply->deleteLater(); m_reply = nullptr;
    emit completed(m_request.generation, m_request.batch, ok, result);
}

Module::Module(QObject* parent) : Module(readSettings(), parent) {}
Module::Module(const Settings& settings, QObject* parent) : QObject(parent), m_settings(bounded(settings))
{
    m_clock.start();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    m_origin = {CF_INS.longitude("longitude", nan), CF_INS.latitude("latitude", nan), CF_INS.altitude("altitude", nan)};
    m_originValid = Wgs84Coordinate::isValid(m_origin);
    connect(&m_timer, &QTimer::timeout, this, &Module::tick);
}
Module::~Module()
{
    m_timer.stop(); invalidate();
    m_thread.quit(); m_thread.wait();
    if (m_window) delete m_window.data();
}
void Module::init()
{
    if (m_started) return;
    m_started = true;
    if (!validEndpoint(m_settings.endpoint)) m_problem = QStringLiteral("接口地址无效（须为无认证参数的HTTP/HTTPS地址）");
    else if (m_settings.token.isEmpty() || m_settings.token.contains('\r') || m_settings.token.contains('\n'))
        m_problem = QStringLiteral("未配置有效Token，请设置环境变量或本机api_token");
    else m_ready = true;
    if (m_ready) {
        qRegisterMetaType<RadarTrackApi::Request>();
        auto* transport = new Transport(m_settings, m_generation);
        transport->moveToThread(&m_thread);
        connect(&m_thread, &QThread::started, transport, &Transport::start);
        connect(&m_thread, &QThread::finished, transport, &QObject::deleteLater);
        connect(this, &Module::submit, transport, &Transport::send, Qt::QueuedConnection);
        connect(transport, &Transport::completed, this, [this](quint64 generation, int batch, bool ok, const QString& result) {
            m_busy = false; m_inFlightBatch = -1;
            if (generation != m_generation->load(std::memory_order_relaxed)) return;
            m_lastOk = ok; if (ok) ++m_success; else ++m_failure;
            m_lastResult = QStringLiteral("批号 %1：%2").arg(batch).arg(result);
            if (!ok) m_nextRequest = qMax(m_nextRequest, m_clock.elapsed() + m_settings.reportIntervalMs);
            if (allowLog(ok ? QStringLiteral("RESPONSE") : QStringLiteral("ERROR")))
                writeLog(QStringLiteral("[TRACK-API][%1] %2 success=%3 failure=%4")
                    .arg(ok ? QStringLiteral("RESPONSE") : QStringLiteral("ERROR"), m_lastResult).arg(m_success).arg(m_failure));
        });
        m_thread.start(); m_auto = m_settings.autoReport;
    }
    m_timer.start(100);
    writeLog(QStringLiteral("[TRACK-API][CONFIG] %1 endpoint=%2 reportMs=%3 timeoutMs=%4；独立于TCP光电目指")
        .arg(m_ready ? QStringLiteral("已启用，等待选批或自动上报") : m_problem,
             validEndpoint(m_settings.endpoint) ? m_settings.endpoint.toString() : QStringLiteral("无效地址"))
        .arg(m_settings.reportIntervalMs).arg(m_settings.timeoutMs));
}
void Module::invalidate() { m_generation->fetch_add(1, std::memory_order_relaxed); }
void Module::writeLog(const QString& text) { if (m_settings.logEnabled) emit logMessage(text); }
bool Module::allowLog(const QString& category)
{
    if (!m_settings.logEnabled) return false;
    const auto now = m_clock.elapsed();
    if (m_logs.contains(category) && now - m_logs.value(category) < m_settings.logIntervalMs) return false;
    m_logs[category] = now; return true;
}
void Module::logLimited(const QString& text)
{
    if (allowLog(QStringLiteral("STATUS"))) writeLog(QStringLiteral("[TRACK-API][STATUS] ") + text);
}
void Module::updateRadarPosition(double latitude, double longitude, double altitude)
{
    const Wgs84Coordinate::Lla origin{longitude, latitude, altitude};
    if (!Wgs84Coordinate::isValid(origin)) return;
    if (!m_originValid || origin.longitudeDeg != m_origin.longitudeDeg || origin.latitudeDeg != m_origin.latitudeDeg
        || origin.altitudeM != m_origin.altitudeM) {
        invalidate();
        // 参考原点变更不能被当成目标运动；下一有效样本重新建立方向基点。
        for (auto& track : m_tracks) {
            track.directionAnchor = false; track.motion.headingDeg = std::numeric_limits<double>::quiet_NaN();
        }
    }
    const bool wasValid = m_originValid;
    m_origin = origin; m_originValid = true;
    if (!wasValid) emit reportingStateChanged();
}
void Module::processTrackFrame(const QByteArray& frame)
{
    m_frameSamples.clear(); m_frameCursor = 0;
    constexpr int prefix = sizeof(ProtocolFrame) + sizeof(TrackResult);
    if (frame.size() < prefix) return;
    const auto* bytes = reinterpret_cast<const uchar*>(frame.constData());
    if (qFromLittleEndian<quint16>(bytes + sizeof(ProtocolFrame)) != TRACK_INFO_MSG_ID) return;
    const int count = qFromLittleEndian<quint16>(bytes + prefix - 2);
    const int length = prefix + count * int(sizeof(trackInfo));
    if (count > 4096 || (frame.size() != length && frame.size() != length + int(sizeof(ProtocolEnd)))) return;
    const auto readFloat = [](const uchar* data) {
        const quint32 bits = qFromLittleEndian<quint32>(data);
        float value; std::memcpy(&value, &bits, sizeof(value)); return value;
    };
    m_frameSamples.reserve(count);
    for (int i = 0; i < count; ++i) {
        const auto* p = bytes + prefix + i * sizeof(trackInfo);
        FrameSample sample;
        sample.batch = qFromLittleEndian<quint16>(p + offsetof(trackInfo, batch));
        sample.statMethod = p[offsetof(trackInfo, statMethod)];
        sample.range = readFloat(p + offsetof(trackInfo, dis));
        sample.azimuth = readFloat(p + offsetof(trackInfo, azi));
        sample.elevation = readFloat(p + offsetof(trackInfo, ele));
        sample.spaceSpeed = readFloat(p + offsetof(trackInfo, spaceVel));
        const quint32 seconds = qFromLittleEndian<quint32>(p + offsetof(trackInfo, UTCtime));
        const quint32 ns = qFromLittleEndian<quint32>(p + offsetof(trackInfo, nsecond));
        if (seconds > 0 && ns < 1000000000u) sample.sourceNs = quint64(seconds) * 1000000000ULL + ns;
        m_frameSamples.append(sample);
    }
}
void Module::processTrack(const PointInfo& point)
{
    if (point.type != PointType::Track || point.batch > 65535) return;
    Motion motion;
    quint64 sourceNs = 0;
    if (m_frameCursor < m_frameSamples.size()) {
        const auto& sample = m_frameSamples[m_frameCursor];
        const auto same = [](float a, float b) { return a == b || (std::isnan(a) && std::isnan(b)); };
        if (sample.batch == int(point.batch) && sample.statMethod == point.statMethod
            && same(sample.range, point.range) && same(sample.azimuth, point.azimuth) && same(sample.elevation, point.elevation)) {
            motion.speedMps = sample.spaceSpeed; sourceNs = sample.sourceNs; ++m_frameCursor;
        } else { m_frameSamples.clear(); m_frameCursor = 0; } // 无精确对应则不用旧帧元数据。
    }
    if (point.statMethod == 250) return;
    const int batch = int(point.batch);
    if (point.statMethod == 2) {
        if (m_tracks.remove(batch)) {
            m_manual.remove(batch); if (m_inFlightBatch == batch) invalidate(); emit reportingStateChanged();
        }
        return;
    }
    double east, north, up;
    if (!Wgs84Coordinate::polarToEnu(point.range, point.azimuth, point.elevation, east, north, up)) return;
    const bool wasReporting = isReporting(batch);
    auto it = m_tracks.find(batch);
    if (it == m_tracks.end()) {
        if (m_tracks.size() >= m_settings.maxTracks) { logLimited(QStringLiteral("最新航迹缓存达到上限，忽略新增批号")); return; }
        Track track; track.id = m_settings.idPrefix + QString::number(batch) + QLatin1Char('-')
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
        it = m_tracks.insert(batch, track);
    }
    // 只约束本HTTP缓存，不改变原有显示/接收链路。回退/冲突时标不产生反向航向。
    if (sourceNs > 0 && it->sourceNs > 0) {
        if (sourceNs < it->sourceNs) return;
        if (sourceNs == it->sourceNs) {
            if (point.range != it->point.range || point.azimuth != it->point.azimuth || point.elevation != it->point.elevation) return;
            if (it->directionAnchor) motion.headingDeg = it->motion.headingDeg;
        } else if (it->directionAnchor && monotonicMs() - it->at < m_settings.trackTimeoutMs
                   && sourceNs - it->sourceNs < quint64(m_settings.trackTimeoutMs) * 1000000ULL) {
            const double deltaEast = east - it->eastM, deltaNorth = north - it->northM;
            if (std::hypot(deltaEast, deltaNorth) > 0.001) {
                const double degrees = std::atan2(deltaEast, deltaNorth) / Wgs84Coordinate::kDegToRad;
                motion.headingDeg = degrees < 0 ? degrees + 360.0 : degrees;
            }
        }
    }
    it->motion = motion; it->eastM = east; it->northM = north;
    it->sourceNs = sourceNs; it->directionAnchor = sourceNs > 0;
    it->point = point; it->at = monotonicMs(); it->utcMs = QDateTime::currentMSecsSinceEpoch();
    if (wasReporting != isReporting(batch)) {
        if (wasReporting && m_inFlightBatch == batch) invalidate();
        emit reportingStateChanged();
    }
}
bool Module::isReporting(int batch) const
{
    const auto it = m_tracks.constFind(batch);
    return m_ready && m_originValid && it != m_tracks.cend() && monotonicMs() - it->at < m_settings.trackTimeoutMs
        && (m_manual.contains(batch) || (m_auto && it->point.targetRecResult == 1));
}
bool Module::startReport(int batch)
{
    if (!m_ready || !m_originValid || !m_tracks.contains(batch)
        || monotonicMs() - m_tracks.value(batch).at >= m_settings.trackTimeoutMs) return false;
    if (!m_manual.contains(batch)) {
        m_manual.insert(batch); emit reportingStateChanged();
        writeLog(QStringLiteral("[TRACK-API][SELECT] batch=%1 手动上报开启").arg(batch));
    }
    refreshWindow(); return true;
}
void Module::stopReport(int batch)
{
    if (m_manual.remove(batch)) {
        writeLog(QStringLiteral("[TRACK-API][STOP] batch=%1 手动选择移除，auto=%2").arg(batch).arg(m_auto));
        if (m_inFlightBatch == batch && !isReporting(batch)) invalidate();
        emit reportingStateChanged();
    }
    refreshWindow();
}
void Module::stopAll()
{
    writeLog(QStringLiteral("[TRACK-API][STOP] 停止全部平台上报"));
    m_manual.clear(); m_auto = false; invalidate(); emit reportingStateChanged(); refreshWindow();
}
void Module::setAutoReport(bool enabled)
{
    if (m_auto == enabled || (enabled && !m_ready)) return;
    m_auto = enabled;
    writeLog(QStringLiteral("[TRACK-API][AUTO] enabled=%1").arg(enabled));
    if (!enabled && !m_manual.contains(m_inFlightBatch)) invalidate();
    emit reportingStateChanged(); refreshWindow();
}
void Module::tick()
{
    const auto now = m_clock.elapsed(), absoluteNow = monotonicMs();
    bool removed = false;
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (absoluteNow - it->at >= m_settings.trackTimeoutMs) {
            if (m_inFlightBatch == it.key()) invalidate();
            m_manual.remove(it.key()); it = m_tracks.erase(it); removed = true;
        } else ++it;
    }
    if (removed) emit reportingStateChanged();
    if (m_lastUi < 0 || now - m_lastUi >= 500) { m_lastUi = now; refreshWindow(); }
    if (!m_ready || !m_originValid || m_busy || now < m_nextRequest || m_tracks.isEmpty()) return;
    // 单请求在途 + 轮转选批；每次直接编码最新点，不积压每个接收样本。
    auto it = m_tracks.upperBound(m_cursor);
    for (int scanned = 0; scanned < m_tracks.size(); ++scanned) {
        if (it == m_tracks.end()) it = m_tracks.begin();
        if (isReporting(it.key()) && (it->lastSent < 0 || now - it->lastSent >= m_settings.reportIntervalMs)) {
            Request request;
            request.body = payload(it->id, it->point, m_origin, it->utcMs, m_settings.model, it->motion);
            if (!request.body.isEmpty()) {
                request.generation = m_generation->load(std::memory_order_relaxed); request.batch = it.key();
                request.expiresAt = it->at + m_settings.trackTimeoutMs;
                it->lastSent = now; m_cursor = it.key(); m_inFlightBatch = it.key(); m_busy = true; ++m_sent;
                m_nextRequest = now + m_settings.requestIntervalMs;
                if (allowLog(QStringLiteral("TX"))) {
                    QString text = QStringLiteral("[TRACK-API][TX] batch=%1 bytes=%2 本机调度，尚非平台确认")
                        .arg(request.batch).arg(request.body.size());
                    if (m_settings.logPayloadEnabled) text += QStringLiteral(" json=") + QString::fromUtf8(request.body);
                    writeLog(text);
                }
                emit submit(request); return;
            }
        }
        ++it;
    }
}
QString Module::statusText() const
{
    if (!m_ready) return m_problem;
    if (!m_originValid) return QStringLiteral("等待有效雷达经纬高");
    return QStringLiteral("请求 %1 / 平台确认成功 %2 / 未确认 %3\n%4")
        .arg(m_sent).arg(m_success).arg(m_failure).arg(m_lastResult);
}
void Module::refreshWindow()
{
    if (!m_window || !m_window->isVisible()) return;
    m_status->setText(statusText());
    QList<int> batches = m_manual.values(); std::sort(batches.begin(), batches.end());
    QStringList names; for (int batch : batches) names.append(QString::number(batch));
    m_selection->setText(QStringLiteral("手动选批：%1\n自动模式：%2；最新航迹缓存 %3 / %4")
        .arg(names.isEmpty() ? QStringLiteral("无") : names.join(QStringLiteral(", ")))
        .arg(m_auto ? QStringLiteral("全部已识别无人机") : QStringLiteral("关闭"))
        .arg(m_tracks.size()).arg(m_settings.maxTracks));
    const QSignalBlocker blocker(m_autoBox); m_autoBox->setChecked(m_auto);
    UiTheme::setStyle(m_status, [this] { return UiFontScale::style(QStringLiteral("QLabel {color:%1;}")
        .arg(QLatin1String(!m_ready || (m_failure > 0 && !m_lastOk) ? UiTheme::Fault : m_lastOk ? UiTheme::Normal : UiTheme::Secondary))); });
}
void Module::showWindow(QWidget* parent)
{
    if (m_window) { m_window->show(); refreshWindow(); m_window->raise(); m_window->activateWindow(); return; }
    auto* window = new CusWindow(QStringLiteral("平台航迹上报"),
        UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), parent);
    window->setAttribute(Qt::WA_DeleteOnClose); m_window = window;
    auto* content = new QWidget(window); content->setObjectName(QStringLiteral("RadarTrackApiContent"));
    content->setAttribute(Qt::WA_StyledBackground);
    UiTheme::setStyle(content, [] { return UiFontScale::style(QStringLiteral(
        "QWidget#RadarTrackApiContent {background:%1;} QWidget#RadarTrackApiContent QLabel, "
        "QWidget#RadarTrackApiContent QCheckBox {color:%2;background:transparent;font-size:16px;}")
        .arg(QLatin1String(UiTheme::Background), QLatin1String(UiTheme::Text))); });
    auto* layout = new QVBoxLayout(content); layout->setSpacing(12);
    auto label = [content, layout](const QString& text) {
        auto* l = new QLabel(text, content); l->setTextFormat(Qt::PlainText); l->setWordWrap(true); layout->addWidget(l); return l;
    };
    label(QStringLiteral("独立 HTTP 通路；可与光电 TCP 目指同时使用"));
    label(QStringLiteral("接口：") + (validEndpoint(m_settings.endpoint) ? m_settings.endpoint.toString() : QStringLiteral("配置无效")));
    m_status = label(statusText()); m_status->setObjectName(QStringLiteral("ApiStatus"));
    m_selection = label(QString());
    m_autoBox = new QCheckBox(QStringLiteral("自动上报全部无人机航迹（与手动选批独立）"), content);
    m_autoBox->setObjectName(QStringLiteral("ApiAutoReport")); m_autoBox->setEnabled(m_ready);
    connect(m_autoBox, &QCheckBox::toggled, this, &Module::setAutoReport); layout->addWidget(m_autoBox);
    auto* row = new QHBoxLayout;
    auto* batch = new QSpinBox(content); batch->setRange(0, 65535); batch->setPrefix(QStringLiteral("批号 "));
    auto* start = new QPushButton(QStringLiteral("开始手动上报"), content);
    auto* stop = new QPushButton(QStringLiteral("停止该批手动上报"), content);
    auto* all = new QPushButton(QStringLiteral("停止全部平台上报"), content);
    row->addWidget(batch); row->addWidget(start); row->addWidget(stop); layout->addLayout(row); layout->addWidget(all);
    connect(start, &QPushButton::clicked, this, [this, batch] {
        if (!startReport(batch->value())) m_status->setText(QStringLiteral("无法选批：请检查Token、雷达经纬高和未过期的普通航迹"));
    });
    connect(stop, &QPushButton::clicked, this, [this, batch] { stopReport(batch->value()); });
    connect(all, &QPushButton::clicked, this, &Module::stopAll);
    label(QStringLiteral("速度取原报文空间速度；航向由连续位置推算，真北0°、顺时针递增，首点或方向未知时省略。\n"
        "高度为绝对高度，时间为显控收到该样本的UTC毫秒。\n"
        "消批/超时后停止发送；停止操作不能撤回已发请求。自动模式仍可能上报已停止手动选批的无人机。"));
    layout->addStretch();
    auto* scroll = new QScrollArea(window); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true); scroll->setWidget(content); window->setContentWidget(scroll);
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const auto available = screen->availableGeometry();
        const QSize size(qMin(840, available.width() - 24), qMin(560, available.height() - 24));
        window->setInitialGeometry(QRect(available.center() - QPoint(size.width()/2, size.height()/2), size));
    }
    window->show(); refreshWindow();
}
}
