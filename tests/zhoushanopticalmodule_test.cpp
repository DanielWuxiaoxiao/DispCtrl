#include "ZhoushanOptical/zhoushanmodule.h"
#include <QSignalSpy>
#include <QTest>
namespace ZhoushanOptical {
class ModuleTest : public QObject {
    Q_OBJECT
private:
    static PointInfo point(unsigned batch = 42) {
        PointInfo p{}; p.type = PointType::Track; p.batch = batch; p.range = 1000; p.azimuth = 90; p.elevation = 0; return p;
    }
    static void prepare(Module& m) { m.m_connected = true; m.m_session = 7; m.updateRadarPosition(30, 120, 100); }
    static QByteArray reply(const QByteArray& request, quint8 state = 1) {
        QByteArray b = request; b[2] = 4; b[3] = 1;
        b[4] = char(3101 & 255); b[5] = char(3101 >> 8); b[6] = char(1100 & 255); b[7] = char(1100 >> 8);
        b[23] = char(state); const auto sum = checksum(b.mid(2, 42)); b[44] = char(sum & 255); b[45] = char(sum >> 8); return b;
    }
    static QVariantMap responseRecord(Module& m, const QByteArray& request, quint64 order, quint8 state = 1) {
        return {{"packet", reply(request, state)}, {"session", QVariant::fromValue<qulonglong>(m.m_session)},
            {"receivedMs", m.m_sentAt}, {"order", QVariant::fromValue<qulonglong>(order)},
            {"receivedTime", "2026-10-20 12:30:00.001"}};
    }
private slots:
    void singleTargetLatestCoordinatesAndCancellation() {
        Module m; prepare(m); QCOMPARE(m.m_settings.kind, quint8(1));
        m.processTrack(point(42)); m.processTrack(point(43));
        QList<QByteArray> sent;
        connect(&m, &Module::submitPacket, this, [&](quint64, quint64, const QByteArray& b) { sent.append(b); });
        QVERIFY(m.startReport(42)); const auto oldToken = m.m_writeToken;
        QCOMPARE(read(sent.last(), 19, 4), quint64(42));
        const auto oldCoordinates = sent.last().mid(24, 20);
        auto latest = point(43); latest.range = 2000; latest.elevation = 10; m.processTrack(latest);
        const auto expected = m.encode(latest);
        QVERIFY(m.startReport(43)); QVERIFY(!m.isSelected(42)); QVERIFY(m.isSelected(43));
        QVERIFY(m.m_allowedToken->load() != oldToken); QCOMPARE(sent.size(), 1);
        // Emulate completion of the sole outstanding worker submit, then send only the new target.
        m.m_busy = false; m.tick(); QCOMPARE(sent.size(), 2);
        QCOMPARE(read(sent.last(), 19, 4), quint64(43));
        QCOMPARE(sent.last().mid(24, 20), expected.mid(24, 20));
        QVERIFY(sent.last().mid(24, 20) != oldCoordinates);
        m.consume({responseRecord(m, m.m_request, 1)}); m.m_busy = false;
        latest.range = 3000; m.processTrack(latest); m.m_nextSend = 0; m.tick();
        QCOMPARE(sent.size(), 3); QCOMPARE(sent.last().mid(24, 20), m.encode(latest).mid(24, 20));
        auto removed = point(42); removed.statMethod = 2; m.processTrack(removed);
        QVERIFY(m.isSelected(43));
        // Cancellation remains available while disconnected and prevents a later reconnect/resume.
        m.m_connected = false; QVERIFY(!m.isReporting(43)); QVERIFY(m.isSelected(43));
        m.stopReport(); QVERIFY(!m.isSelected(43));
        m.m_connected = true; m.m_busy = false; m.m_nextSend = 0; m.tick(); QCOMPARE(sent.size(), 3);
        QVERIFY(m.startReport(43)); QCOMPARE(sent.size(), 4);
        removed = point(43); removed.statMethod = 2; m.processTrack(removed);
        QCOMPARE(m.m_selected, -1); m.m_busy = false; m.tick(); QCOMPARE(sent.size(), 4);
    }
    void currentAndPreviousDifferentBatchResponses() {
        Module m; prepare(m); m.processTrack(point(42)); m.processTrack(point(43));
        QVERIFY(m.startReport(42)); const auto request42 = m.m_request;
        m.consume({responseRecord(m, request42, 1)});
        QVERIFY(m.m_currentResponse.text.contains(QStringLiteral("已接收并处理")));
        QCOMPARE(m.m_previousResponse.batch, -1);
        // Re-selecting the same batch does not rotate the two history slots.
        QVERIFY(m.startReport(42)); QCOMPARE(m.m_previousResponse.batch, -1);
        QVERIFY(m.startReport(43)); QCOMPARE(m.m_currentResponse.batch, 43);
        QCOMPARE(m.m_previousResponse.batch, 42); QVERIFY(m.m_currentResponse.text.isEmpty());
        m.m_busy = false; m.tick(); const auto request43 = m.m_request;
        m.consume({responseRecord(m, request42, 2, 2)}); QVERIFY(m.m_pending);
        QVERIFY(m.m_previousResponse.text.contains(QStringLiteral("已接收但忽略")));
        QVERIFY(m.m_currentResponse.text.isEmpty());
        auto staleSession = responseRecord(m, request43, 3); staleSession["session"] = 6;
        m.consume({staleSession}); QVERIFY(m.m_currentResponse.text.isEmpty()); QVERIFY(m.m_pending);
        // Same-millisecond responses may arrive in unordered hash iteration order.
        m.consume({responseRecord(m, request43, 5, 2), responseRecord(m, request43, 4, 1)});
        QCOMPARE(m.m_currentResponse.order, quint64(5));
        QVERIFY(m.m_currentResponse.text.contains(QStringLiteral("已接收但忽略")));
        QCOMPARE(m.m_previousResponse.batch, 42);
        const auto unknown = m.encode(point(99));
        m.consume({responseRecord(m, unknown, 6)}); QCOMPARE(m.m_currentResponse.batch, 43);
        QCOMPARE(m.m_previousResponse.batch, 42);
        m.stopReport(); QVERIFY(!m.m_currentResponse.text.isEmpty());
        QVERIFY(m.startReport(42)); QCOMPARE(m.m_currentResponse.batch, 42);
        QVERIFY(m.m_currentResponse.text.isEmpty()); QCOMPARE(m.m_previousResponse.batch, 43);
    }
    void cancelledWorkerJobIsRejectedBeforeSocketAccess() {
        auto token = std::make_shared<std::atomic<quint64>>(2);
        Transport transport(Settings{}, token); QSignalSpy submitted(&transport, &Transport::submitted);
        transport.submit(7, 1, QByteArray(FrameSize, 0)); QCOMPARE(submitted.size(), 1);
        QVERIFY(!submitted.first().at(1).toBool());
        QVERIFY(submitted.first().at(3).toString().contains(QStringLiteral("目标已切换或停止")));
    }
    void pendingAndSourceIsolation() {
        Module m; prepare(m); int sends = 0;
        connect(&m, &Module::submitPacket, this, [&](quint64, quint64, const QByteArray&) { ++sends; });
        auto p = point(); p.type = PointType::TBDPointType; m.processTrack(p);
        p.type = PointType::CooperativeTrackPointType; m.processTrack(p); QVERIFY(m.m_tracks.isEmpty());
        p.type = PointType::Track; p.statMethod = 250; m.processTrack(p); QVERIFY(m.m_tracks.isEmpty());
        p.statMethod = 0; m.processTrack(p); QVERIFY(m.startReport(42)); QCOMPARE(sends, 1);
        for (int n = 0; n < 10000; ++n) { p.range = 1000 + n; m.processTrack(p); m.tick(); }
        QCOMPARE(sends, 1); QCOMPARE(m.m_tracks.size(), 1);
        // A rapid selection cannot queue another job while the first submit is in flight.
        QVERIFY(m.startReport(42)); QCOMPARE(sends, 1);
        p.statMethod = 2; m.processTrack(p); QCOMPARE(m.m_selected, -1); QCOMPARE(sends, 1);
    }
    void responseMatching() {
        Module m; prepare(m); m.processTrack(point()); QVERIFY(m.startReport(42));
        QVariantMap record; record["session"] = QVariant::fromValue<qulonglong>(7); record["receivedMs"] = m.m_sentAt;
        const auto packet = reply(m.m_request);
        record["packet"] = packet; record["session"] = QVariant::fromValue<qulonglong>(6);
        m.consume({record}); QVERIFY(m.m_pending);
        record["session"] = QVariant::fromValue<qulonglong>(7);
        auto different = packet; different[24] = char(quint8(different[24]) ^ 1);
        auto sum = checksum(different.mid(2, 42)); different[44] = char(sum & 255); different[45] = char(sum >> 8);
        record["packet"] = different; m.consume({record}); QVERIFY(m.m_pending);
        record["packet"] = packet; m.consume({record}); QVERIFY(!m.m_pending); QVERIFY(!m.m_timedOut);
        QVERIFY(m.m_resultText.contains(QStringLiteral("已接收并处理")));
    }
    void confirmationIsNotAutomaticallyRetried() {
        Module m; prepare(m); m.m_settings.kind = 2; m.processTrack(point()); int sends = 0;
        connect(&m, &Module::submitPacket, this, [&](quint64, quint64, const QByteArray&) { ++sends; });
        QVERIFY(m.startReport(42)); QCOMPARE(sends, 1);
        m.m_busy = false; m.m_sentAt = -10000; m.tick(); QVERIFY(m.m_timedOut);
        m.m_nextSend = 0; m.tick(); QCOMPARE(sends, 1);
        QVERIFY(m.startReport(42)); QCOMPARE(sends, 2);
    }
};
}
QTEST_GUILESS_MAIN(ZhoushanOptical::ModuleTest)
#include "zhoushanopticalmodule_test.moc"
