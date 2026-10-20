#include "PhotoelectricV2/module.h"
#include "PhotoelectricV2/protocol.h"
#include <QTest>

namespace PhotoelectricV2 {
class ModuleTest : public QObject {
    Q_OBJECT
private:
    static PointInfo point(unsigned batch = 42) {
        PointInfo p{}; p.type = PointType::Track; p.batch = batch;
        p.range = 1000; p.azimuth = 90; p.elevation = 0; p.speed = -15;
        return p;
    }
    static void prepare(Module& m) {
        m.m_ready = true;
        m.updateRadarPosition(30, 120, 100);
    }
private slots:
    void lifecycleAndBackpressure() {
        Module m; prepare(m);
        int batches = 0; QList<QByteArray> frames;
        connect(&m, &Module::sendFrames, this, [&](const QList<QByteArray>& data) { ++batches; frames = data; });
        auto p = point(); m.processTrack(p);
        QVERIFY(m.startReport(42)); QCOMPARE(batches, 1); QCOMPARE(frames.size(), 1);
        QCOMPARE(read(frames[0], 7, 2), quint64(Guide));
        QCOMPARE(read(frames[0], 43, 2), quint64(15));
        for (int n = 0; n < 10000; ++n) { p.range = 1000 + n; m.processTrack(p); m.flush(); }
        QCOMPARE(m.m_tracks.size(), 1); QCOMPARE(batches, 1); // only one queued worker batch
        m.m_busy = false; m.m_lastTx = -1; m.flush(); QCOMPARE(batches, 2);
        p.statMethod = 2; m.processTrack(p);
        QVERIFY(!m.hasTrack(42)); QVERIFY(!m.isReporting(42)); QCOMPARE(m.m_selected, -1);
        QCOMPARE(batches, 2); // protocol has no cancel packet
    }
    void sourceIsolationAndLimits() {
        Module m; prepare(m); m.m_settings.maxTracks = 2;
        auto p = point(); p.type = PointType::TBDPointType; m.processTrack(p);
        p.type = PointType::CooperativeTrackPointType; m.processTrack(p);
        p.type = PointType::Detection; m.processTrack(p);
        p.type = PointType::Track; p.statMethod = 250; m.processTrack(p);
        QVERIFY(m.m_tracks.isEmpty());
        p.statMethod = 0;
        for (unsigned n = 0; n < 100; ++n) { p.batch = n; m.processTrack(p); }
        QCOMPARE(m.m_tracks.size(), 2);
        auto it = m.m_tracks.begin(); const int key = it.key(); it->updated = -20000;
        m.flush(); QVERIFY(!m.hasTrack(key));
    }
    void originAndTimeout() {
        Module m; auto p = point(); m.processTrack(p); m.m_ready = true;
        QVERIFY(!m.startReport(42)); // no invented origin at zero
        prepare(m); const auto frame = m.encode(p); QVERIFY(!frame.isEmpty());
        QCOMPARE(read(frame, 41, 2), quint64(600)); // height=origin+RAE up, not PointInfo.altitute
        m.updateRadarPosition(30, 120, 200);
        QCOMPARE(read(m.encode(p), 41, 2), quint64(700));
        QVariantMap status; status["function"] = Status;
        m.consume({status}); QVERIFY(m.statusText().contains(QStringLiteral("设备状态:已收到")));
        m.m_settings.timeoutMs = 1;
        QTest::qWait(5);
        QVERIFY(m.statusText().contains(QStringLiteral("设备状态:超时")));
    }
};
}
QTEST_GUILESS_MAIN(PhotoelectricV2::ModuleTest)
#include "photoelectricv2module_test.moc"
