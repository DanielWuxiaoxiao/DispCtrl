#include "RadarTrackApi/radartrackapi.h"
#include "Basic/uithemeruntime.h"
#include "Basic/ConfigManager.h"
#include <QCheckBox>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QtEndian>
#include <cstring>
#include <limits>

namespace RadarTrackApi {
// 只监听本机回环，认证为虚构测试值，不访问实际平台。
class MockServer final : public QTcpServer {
public:
    QList<QByteArray> requests, bodies;
    QByteArray body = "{\"code\":200,\"message\":\"ok\"}";
    int status = 200;
    bool silent = false;
    explicit MockServer() {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto* socket = nextPendingConnection();
                struct Input { QByteArray bytes; bool done = false; };
                const auto input = std::make_shared<Input>();
                connect(socket, &QTcpSocket::readyRead, this, [this, socket, input] {
                    input->bytes += socket->readAll();
                    const auto split = input->bytes.indexOf("\r\n\r\n");
                    if (input->done || split < 0) return;
                    int size = 0;
                    for (const auto& line : input->bytes.left(split).split('\n'))
                        if (line.toLower().startsWith("content-length:")) size = line.mid(15).trimmed().toInt();
                    if (input->bytes.size() < split + 4 + size) return;
                    input->done = true; requests.append(input->bytes.left(split));
                    bodies.append(input->bytes.mid(split + 4, size));
                    if (silent) return;
                    socket->write("HTTP/1.1 " + QByteArray::number(status) + " Result\r\nContent-Type: application/json\r\n"
                        "Connection: close\r\nLocation: http://127.0.0.1:1/never-follow\r\nContent-Length: "
                        + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }
    Settings settings() const {
        Settings s; s.endpoint = QUrl(QStringLiteral("http://127.0.0.1:%1/test").arg(serverPort()));
        s.token = "unit-test-token-only"; s.timeoutMs = 150; s.reportIntervalMs = 200;
        s.requestIntervalMs = 50; return s;
    }
};

class ApiTest final : public QObject {
    Q_OBJECT
    static PointInfo point(unsigned batch = 42, unsigned classification = 1) {
        PointInfo p{}; p.type = PointType::Track; p.batch = batch; p.range = 1000;
        p.azimuth = 90; p.elevation = 0; p.targetRecResult = classification; return p;
    }
    static QByteArray frame(const PointInfo& point, float spaceSpeed, quint32 seconds, quint32 ns) {
        // 独立黄金字节偏移：14B帧头+4B航迹头+61B记录，不复用被测解码函数。
        QByteArray bytes(18 + 61, 0);
        auto* p = reinterpret_cast<uchar*>(bytes.data());
        qToLittleEndian<quint16>(0xEE01, p + 14); qToLittleEndian<quint16>(1, p + 16);
        qToLittleEndian<quint16>(quint16(point.batch), p + 18);
        qToLittleEndian<quint32>(seconds, p + 22); qToLittleEndian<quint32>(ns, p + 26);
        p[30] = point.statMethod;
        const auto put = [p](int at, float value) {
            quint32 bits; std::memcpy(&bits, &value, sizeof(bits)); qToLittleEndian<quint32>(bits, p + at);
        };
        put(39, point.range); put(43, point.azimuth); put(47, point.elevation);
        put(55, point.speed); put(59, spaceSpeed); return bytes;
    }
    static PointInfo at(double east, double north, double up = 0, unsigned batch = 42) {
        auto p = point(batch); const auto horizontal = std::hypot(east, north);
        p.range = float(std::hypot(horizontal, up));
        p.azimuth = float(std::atan2(east, north) / Wgs84Coordinate::kDegToRad);
        p.elevation = float(std::atan2(up, horizontal) / Wgs84Coordinate::kDegToRad);
        p.speed = -3; return p;
    }
    static void ingest(Module& module, const PointInfo& point, float spaceSpeed, quint32 sequence) {
        module.processTrackFrame(frame(point, spaceSpeed, 1700000000u + sequence, 123));
        module.processTrack(point);
    }
private slots:
    void initTestCase() { qRegisterMetaType<RadarTrackApi::Request>(); }
    void fieldsUnitsAndOmittedMotion() {
        auto p = point(); p.elevation = 30; p.altitute = -12345; p.speed = 100;
        const auto j = QJsonDocument::fromJson(payload("test-42", p, {120, 30, 100}, 1700000000123LL, "RADAR")).object();
        QCOMPARE(j.size(), 6); QCOMPARE(j["trackId"].toString(), QString("test-42"));
        QVERIFY(j["longitude"].isDouble()); QVERIFY(j["latitude"].isDouble());
        QVERIFY(std::abs(j["altitude"].toDouble() - 600.0) < 1e-6);
        QVERIFY(j["longitude"].toDouble() > 120); QVERIFY(std::abs(j["latitude"].toDouble() - 30) < 1e-9);
        QCOMPARE(qint64(j["timestamp"].toDouble()), 1700000000123LL);
        QVERIFY(!j.contains("speed")); QVERIFY(!j.contains("heading")); QVERIFY(!j.contains("aircraftState"));
        p.range = std::numeric_limits<float>::quiet_NaN(); QVERIFY(payload("x", p, {120,30,100}, 1, "RADAR").isEmpty());
        p = point(); QVERIFY(payload("", p, {120,30,100}, 1, "RADAR").isEmpty());
        QVERIFY(payload("x", p, {181,30,100}, 1, "RADAR").isEmpty());
        p.statMethod = 250; QVERIFY(payload("x", p, {120,30,100}, 1, "RADAR").isEmpty());
    }
    void independentSelectionLatestOnlyAndLifecycle() {
        Settings s; s.maxTracks = 2;
        Module m(s); m.m_ready = true; m.updateRadarPosition(30,120,100);
        m.processTrack(point(42, 0)); m.processTrack(point(43)); m.processTrack(point(44));
        QCOMPARE(m.m_tracks.size(), 2); QVERIFY(!m.isReporting(42)); QVERIFY(!m.isReporting(43));
        m.setAutoReport(true); QVERIFY(!m.isReporting(42)); QVERIFY(m.isReporting(43));
        QVERIFY(m.startReport(42)); QVERIFY(m.isReporting(42));
        m.setAutoReport(false); QVERIFY(m.isReporting(42)); QVERIFY(!m.isReporting(43));
        QVERIFY(m.startReport(43)); m.stopReport(42); QVERIFY(m.isReporting(43));
        const auto generation = m.m_generation->load();
        m.setAutoReport(true); m.processTrack(point(42));
        m.stopReport(43); QVERIFY(m.isReporting(43)); // 自动仍选中该无人机
        QCOMPARE(m.m_generation->load(), generation); // 新增选择不取消其他批次
        auto reclassified = point(42,0); m.processTrack(reclassified); QVERIFY(!m.isReporting(42));
        m.setAutoReport(false); QVERIFY(m.startReport(43));
        auto latest = point(43); for (int i = 0; i < 1000; ++i) { latest.range = 1000 + i; m.processTrack(latest); }
        QSignalSpy sends(&m, &Module::submit); m.tick(); QCOMPARE(sends.size(), 1);
        const auto req = qvariant_cast<Request>(sends.last()[0]);
        QCOMPARE(req.batch, 43);
        const auto expected = payload(m.m_tracks[43].id, latest, {120,30,100}, m.m_tracks[43].utcMs, "RADAR");
        QCOMPARE(req.body, expected); m.tick(); QCOMPARE(sends.size(), 1); // 单请求在途
        const auto id = m.m_tracks[43].id;
        latest.statMethod = 2; m.processTrack(latest); QVERIFY(!m.isManualReport(43)); QVERIFY(!m.isReporting(43));
        m.processTrack(point(43)); QVERIFY(m.m_tracks[43].id != id); QVERIFY(!m.isReporting(43));
        QVERIFY(m.startReport(43)); m.m_tracks[43].at -= s.trackTimeoutMs + 1;
        m.tick(); QVERIFY(!m.m_tracks.contains(43)); QVERIFY(!m.isManualReport(43));
        latest = point(99); latest.type = PointType::Detection; m.processTrack(latest); QVERIFY(!m.m_tracks.contains(99));
    }
    void spatialSpeedAndTrueNorthCourse() {
        Module m(Settings{}); m.updateRadarPosition(30,120,100);
        ingest(m, at(100,100), 25.5f, 1);
        auto sample = m.m_tracks[42];
        auto json = QJsonDocument::fromJson(payload(sample.id, sample.point, {120,30,100}, sample.utcMs, "RADAR", sample.motion)).object();
        QCOMPARE(json["speed"].toDouble(), 25.5); QVERIFY(!json.contains("heading"));
        const struct { double east, north, heading; } moves[] = {
            {100,110,0}, {110,110,90}, {110,100,180}, {100,100,270}
        };
        quint32 sequence = 2;
        for (const auto& move : moves) {
            ingest(m, at(move.east, move.north), 30.0f, sequence++);
            sample = m.m_tracks[42];
            QVERIFY(std::abs(std::remainder(sample.motion.headingDeg - move.heading, 360.0)) < 0.001);
        }
        QVERIFY(std::abs(sample.motion.headingDeg - sample.point.azimuth) > 100);
        const auto last = sample;
        ingest(m, at(500,500), 99, 1); // 乱序样本不得造成反向航向或覆盖当前速度。
        QCOMPARE(m.m_tracks[42].point.range, last.point.range);
        QCOMPARE(m.m_tracks[42].motion.speedMps, 30.0);
        ingest(m, at(100,100,10), 30, sequence++); // 仅垂直运动没有水平航向。
        QVERIFY(!std::isfinite(m.m_tracks[42].motion.headingDeg));
        m.updateRadarPosition(30.001,120,100);
        ingest(m, at(100,110), 30, sequence++); QVERIFY(!std::isfinite(m.m_tracks[42].motion.headingDeg));
        ingest(m, at(100,120), 30, sequence++); QVERIFY(std::isfinite(m.m_tracks[42].motion.headingDeg));
        ingest(m, at(100,120), -1, sequence++);
        sample = m.m_tracks[42];
        json = QJsonDocument::fromJson(payload(sample.id, sample.point, {120,30,100}, sample.utcMs, "RADAR", sample.motion)).object();
        QVERIFY(!json.contains("speed")); QVERIFY(!json.contains("heading"));
    }
    void frameAssociationRepeatedBatchAndMissingMetadata() {
        Module m(Settings{}); m.updateRadarPosition(30,120,100);
        const auto first = at(100,100), next = at(110,100);
        auto bytes = frame(first, 10, 1700000001, 0);
        const auto second = frame(next, 20, 1700000002, 0);
        bytes += second.mid(18); qToLittleEndian<quint16>(2, reinterpret_cast<uchar*>(bytes.data()) + 16);
        m.processTrackFrame(bytes); m.processTrack(first); QCOMPARE(m.m_tracks[42].motion.speedMps, 10.0);
        m.processTrack(next); QCOMPARE(m.m_tracks[42].motion.speedMps, 20.0);
        QVERIFY(std::abs(m.m_tracks[42].motion.headingDeg - 90) < 0.001);
        m.processTrack(at(120,100)); // 整帧用完后不能借用上一点spaceVel。
        QVERIFY(!std::isfinite(m.m_tracks[42].motion.speedMps));
        QVERIFY(!std::isfinite(m.m_tracks[42].motion.headingDeg));
        m.processTrackFrame(frame(next, 20, 1700000002, 0).left(35)); m.processTrack(next);
        QVERIFY(!std::isfinite(m.m_tracks[42].motion.speedMps));
        m.processTrackFrame(frame(next, 20, 1700000002, 1000000000u)); m.processTrack(next);
        QCOMPARE(m.m_tracks[42].motion.speedMps, 20.0); QVERIFY(!m.m_tracks[42].directionAnchor);
        auto remove = point(); remove.statMethod = 2; m.processTrack(remove);
        ingest(m, first, 0.0f, 3); QCOMPARE(m.m_tracks[42].motion.speedMps, 0.0);
        QVERIFY(!std::isfinite(m.m_tracks[42].motion.headingDeg));
    }
    void queuedCancellationAndExpiry() {
        auto generation = std::make_shared<std::atomic<quint64>>(2);
        Transport transport(Settings{}, generation); transport.start();
        QSignalSpy result(&transport, &Transport::completed);
        Request request; request.generation = 1; request.batch = 42; request.expiresAt = std::numeric_limits<qint64>::max();
        transport.send(request); QCOMPARE(result.size(), 1); QVERIFY(!result.last()[2].toBool());
        request.generation = 2; request.expiresAt = 0; transport.send(request);
        QCOMPARE(result.size(), 2); QVERIFY(!result.last()[2].toBool());
    }
    void httpPostAndStop() {
        MockServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        Module m(server.settings()); m.init(); m.updateRadarPosition(30,120,100);
        m.processTrack(point(42,0)); m.processTrack(point(43));
        QVERIFY(m.startReport(42)); m.setAutoReport(true);
        QTRY_VERIFY_WITH_TIMEOUT(m.m_success >= 2, 3000);
        QVERIFY(server.requests.first().startsWith("POST /test HTTP/1.1"));
        QVERIFY(server.requests.first().contains("X-Api-Token: unit-test-token-only"));
        QSet<QString> ids; for (const auto& b : server.bodies) {
            const auto json = QJsonDocument::fromJson(b).object(); ids.insert(json["trackId"].toString());
            QVERIFY(!json.contains("speed")); QVERIFY(!json.contains("heading"));
        }
        QCOMPARE(ids.size(), 2); m.stopAll();
        QTRY_VERIFY_WITH_TIMEOUT(!m.m_busy, 1000);
        const auto count = server.bodies.size(); QTest::qWait(350); QCOMPARE(server.bodies.size(), count);
    }
    void httpMotionFields() {
        MockServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        Module m(server.settings()); m.init(); m.updateRadarPosition(30,120,100);
        ingest(m, at(100,100), 25.0f, 1); ingest(m, at(110,100), 26.0f, 2);
        QVERIFY(m.startReport(42)); QTRY_VERIFY_WITH_TIMEOUT(m.m_success > 0, 2000);
        const auto json = QJsonDocument::fromJson(server.bodies.first()).object();
        QCOMPARE(json["speed"].toDouble(), 26.0); QVERIFY(json["heading"].isDouble());
        QVERIFY(std::abs(json["heading"].toDouble() - 90) < 0.001); m.stopAll();
    }
    void independentLoggingSwitchAndPayload() {
        for (bool enabled : {false, true}) for (bool payloadEnabled : {false, true}) {
            MockServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0));
            auto settings = server.settings(); settings.logEnabled = enabled; settings.logPayloadEnabled = payloadEnabled;
            Module m(settings); QSignalSpy logs(&m, &Module::logMessage);
            m.init(); m.updateRadarPosition(30,120,100); m.processTrack(point()); QVERIFY(m.startReport(42));
            QTRY_VERIFY_WITH_TIMEOUT(m.m_success > 0, 2000); m.stopAll();
            bool sawPayload = false;
            for (const auto& event : logs) {
                const auto text = event[0].toString();
                QVERIFY(!text.contains("unit-test-token-only")); QVERIFY(!text.contains("X-Api-Token"));
                sawPayload |= text.contains(" json=");
            }
            QCOMPARE(sawPayload, enabled && payloadEnabled);
            if (!enabled) QCOMPARE(logs.size(), 0);
            else {
                QVERIFY(m.allowLog("RATE_TEST")); QVERIFY(!m.allowLog("RATE_TEST"));
                QVERIFY(m.allowLog("OTHER_CATEGORY"));
            }
        }
    }
    void failures_data() {
        QTest::addColumn<int>("http"); QTest::addColumn<QByteArray>("body"); QTest::addColumn<bool>("silent");
        QTest::newRow("application-error") << 200 << QByteArray("{\"code\":500}") << false;
        QTest::newRow("http-error") << 500 << QByteArray("{\"code\":200}") << false;
        QTest::newRow("string-code") << 200 << QByteArray("{\"code\":\"200\"}") << false;
        QTest::newRow("invalid-json") << 200 << QByteArray("bad") << false;
        QTest::newRow("redirect") << 302 << QByteArray("{\"code\":200}") << false;
        QTest::newRow("too-large") << 200 << QByteArray(65537, 'x') << false;
        QTest::newRow("timeout") << 200 << QByteArray() << true;
    }
    void shutdownWithRequestInFlight() {
        MockServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0)); server.silent = true;
        auto* module = new Module(server.settings()); module->init();
        module->updateRadarPosition(30,120,100); module->processTrack(point()); QVERIFY(module->startReport(42));
        QTRY_VERIFY_WITH_TIMEOUT(server.requests.size() == 1, 2000);
        QElapsedTimer elapsed; elapsed.start(); delete module;
        QVERIFY(elapsed.elapsed() < 1000);
        QTest::qWait(250); QCOMPARE(server.requests.size(), 1);
    }
    void failures() {
        QFETCH(int, http); QFETCH(QByteArray, body); QFETCH(bool, silent);
        MockServer server; QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        server.status = http; server.body = body; server.silent = silent;
        Module m(server.settings()); QSignalSpy logs(&m, &Module::logMessage);
        m.init(); m.updateRadarPosition(30,120,100); m.processTrack(point()); QVERIFY(m.startReport(42));
        QTRY_VERIFY_WITH_TIMEOUT(m.m_failure > 0, 3000); QCOMPARE(m.m_success, quint64(0));
        m.stopAll(); QTest::qWait(250); QCOMPARE(server.requests.size(), 1);
        for (const auto& event : logs) QVERIFY(!event[0].toString().contains("unit-test-token-only"));
    }
    void missingTokenAndBothThemes() {
        Settings s; s.token.clear();
        Module m(s); m.init(); QVERIFY(!m.m_thread.isRunning());
        m.updateRadarPosition(30,120,100); m.processTrack(point()); QVERIFY(!m.startReport(42));
        QWidget parent;
        for (const auto& theme : {QStringLiteral("graphite"), QStringLiteral("classic")}) {
            UiTheme::Runtime::instance().apply(theme); m.showWindow(&parent);
            QVERIFY(m.m_window); QVERIFY(m.m_status->text().contains("Token"));
            QVERIFY(!m.m_autoBox->isEnabled()); QVERIFY(!m.m_window->styleSheet().contains("unit-test"));
            auto* content = m.m_window->findChild<QWidget*>(QStringLiteral("RadarTrackApiContent"));
            QVERIFY(content && content->styleSheet().contains(QLatin1String(UiTheme::Background)));
        }
        m.m_window->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!m.m_window); m.showWindow(&parent); QVERIFY(m.m_window);
    }
};
}
QTEST_MAIN(RadarTrackApi::ApiTest)
#include "radartrackapi_test.moc"
