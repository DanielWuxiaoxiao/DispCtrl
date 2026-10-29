#include <QTest>
#include <QTemporaryDir>
#include <limits>
#include "Basic/radarpanelstate.h"
#include "Basic/radarpanelstore.h"

class RadarPanelStateTest final : public QObject {
    Q_OBJECT
    static BITReportV2 report(int id) {
        BITReportV2 bit;
        bit.radarId = static_cast<unsigned char>(id);
        bit.longitudeE9 = 120000000000LL; bit.latitudeE9 = 30000000000LL;
        bit.altitudeCm = 20000; bit.headingMilliDeg = 123456;
        bit.pitchMilliDeg = -1234; bit.rollCentiDeg = -234;
        return bit;
    }
private slots:
    void debugSnapshotSeparatesWireFieldsAndRejectedHeadingSamples() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        QVERIFY(state.unlock(0));
        auto bit = report(0); bit.yaw = 10155; bit.headingMilliDeg = 97787;
        state.ingest(bit);
        auto debug = state.debugSnapshot(0);
        QCOMPARE(debug.gpsCount, std::size_t(1)); QCOMPARE(debug.attitudeCount, std::size_t(1));
        QCOMPARE(debug.curveCount, std::size_t(1));
        QVERIFY(std::abs(debug.attitudeMean[0] - 97.787) < 1e-9);
        QVERIFY(std::abs(debug.curveMean[0] - 101.55) < 1e-9);
        bit.latitudeE9 = 0; bit.longitudeE9 = 0; bit.altitudeCm = 0;
        bit.headingMilliDeg = 120000; bit.yaw = 10200;
        const auto update = state.ingest(bit);
        QVERIFY(!update.attitudeValid);
        debug = state.debugSnapshot(0);
        QCOMPARE(debug.attitudeCount, std::size_t(1)); QCOMPARE(debug.curveCount, std::size_t(2));
        QVERIFY(std::abs(debug.attitude[0][0] - 97.787) < 1e-9);
        QCOMPARE(debug.curve[0][0], 101.55); QCOMPARE(debug.curve[1][0], 102.0);
        QVERIFY(std::abs(debug.curveMean[0] - 101.775) < 1e-9);
    }
    void diagnosticMeanUsesExactlyNewestHundredSamples() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        QVERIFY(state.unlock(0));
        auto bit = report(0); bit.yaw = 10000; state.ingest(bit);
        bit.yaw = 2000;
        for (int i = 0; i < 99; ++i) state.ingest(bit);
        auto snap = state.snapshot();
        QCOMPARE(snap.historyCount, std::size_t(100));
        QVERIFY(snap.history[99].average > 20.0);
        state.ingest(bit);
        snap = state.snapshot();
        QCOMPARE(snap.historyCount, std::size_t(101));
        QVERIFY(std::abs(snap.history[100].average - 20.0) < 1e-9);
    }
    void persistentLockedReferencesRoundTripAndAtomicErrors() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.filePath("radar_panel_reference.toml");
        RadarPanelState state;
        state.initialize({109.123922221, 34.225249343, 781.48, 0, 15, 0}, 100, 50, true);
        const auto expected = state.dialogValues();
        QString error;
        QVERIFY(RadarPanelStore::save(path, expected, error));
        RadarPanelValues loaded{};
        QVERIFY(RadarPanelStore::load(path, loaded, error));
        QCOMPARE(loaded, expected);
        auto invalid = expected; invalid[2][0] = 181;
        QVERIFY(!RadarPanelStore::save(path, invalid, error));
        QVERIFY(RadarPanelStore::load(path, loaded, error)); QCOMPARE(loaded, expected);
        RadarPanelState restored;
        restored.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        restored.restoreLocked(loaded);
        QCOMPARE(restored.dialogValues(), expected);
        for (int id = 0; id < 4; ++id) QVERIFY(restored.reference(id).locked());
        QVERIFY(!RadarPanelStore::save(directory.filePath("missing/ref.toml"), expected, error));
        QFile corrupt(path); QVERIFY(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
        corrupt.write("format_version = 1\n[panel0]\nlongitude = 999\n"); corrupt.close();
        loaded = expected;
        QVERIFY(!RadarPanelStore::load(path, loaded, error)); QCOMPARE(loaded, expected);
    }
    void yawPlotRangeAndCircularDiagnosticMean() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        auto bit = report(0); bit.yaw = 35900; state.ingest(bit);
        bit.yaw = 100; state.ingest(bit);
        auto s = state.snapshot();
        QVERIFY(s.history[1].average < 1e-6 || s.history[1].average > 359.999999);
        auto plot = RadarPanelState::yawPlotData(s);
        QCOMPARE(plot.raw[0], 359.0); QCOMPARE(plot.raw[1], 1.0);
        QVERIFY(plot.average[1] < 1e-6);
        QCOMPARE(plot.lower, -5.0); QCOMPARE(plot.upper, 364.0);
        s.historyCount = 3;
        s.history[0] = {1, 300, 0, 300}; s.history[1] = {2, 350, 0, 325};
        s.history[2] = {3, 40, 0, 350};
        plot = RadarPanelState::yawPlotData(s);
        QCOMPARE(plot.raw[2], 40.0); // never invent 400 degrees from the north crossing
        RadarPanelState::Snapshot constant;
        constant.historyCount = 1; constant.history[0] = {1, 100, 0, 100};
        plot = RadarPanelState::yawPlotData(constant);
        QCOMPARE(plot.lower, 95.0); QCOMPARE(plot.upper, 105.0);
    }
    void gatedPanelsAndContinuousCurveAcrossSwitches() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        for (int id = 0; id < 4; ++id) QVERIFY(state.reference(id).locked());
        auto bit = report(0); bit.yaw = 1000; state.ingest(bit, 1);
        QCOMPARE(state.snapshot().historyCount, std::size_t(0));
        QCOMPARE(state.reference(0).value(RadarReference::Altitude), 100.0);
        QVERIFY(state.unlock(0));
        state.ingest(bit, 2);
        QCOMPARE(state.snapshot().historyCount, std::size_t(1));
        QVERIFY(state.unlock(1));
        QVERIFY(state.reference(0).locked()); QVERIFY(!state.reference(1).locked());
        bit = report(1); bit.yaw = 9000; state.ingest(bit, 2);
        auto s = state.snapshot();
        QCOMPARE(s.historyCount, std::size_t(2)); QCOMPARE(s.history[1].yaw, 90.0);
        QCOMPARE(s.history[1].id, 1); QVERIFY(std::abs(s.history[1].average - 50.0) < 1e-9);
        bit = report(0); bit.yaw = 3000; state.ingest(bit, 3);
        s = state.snapshot(); QCOMPARE(s.historyCount, std::size_t(2));
        QCOMPARE(s.history[1].id, 1); // locked ID0 does not enter the mean or curve.
        bit = report(1); bit.yaw = 0; state.ingest(bit, 4);
        s = state.snapshot(); QCOMPARE(s.historyCount, std::size_t(3));
        QCOMPARE(s.history[2].yaw, 0.0);
        QCOMPARE(s.receivedMs[0], qint64(3)); QCOMPARE(s.receivedMs[1], qint64(4));
        bit.yaw = 65535; state.ingest(bit, 5);
        s = state.snapshot(); QCOMPARE(s.historyCount, std::size_t(4));
        QCOMPARE(s.history[3].yaw, 655.35); // no range filter for diagnostic yaw.
    }
    void hundredAttitudesCircularMeanAndBoundedHistory() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        auto bit = report(1);
        for (int i = 0; i < 101; ++i) {
            bit.headingMilliDeg = i % 2 ? 1000 : 359000;
            bit.pitchMilliDeg = i * 100;
            bit.rollCentiDeg = i * 10;
            state.ingest(bit, 1000 + i);
        }
        const auto pose = state.reference(1).pose();
        QVERIFY(pose[3] < 1e-6 || pose[3] > 359.999999);
        QVERIFY(std::abs(pose[4] - 5.05) < 1e-9);
        QVERIFY(std::abs(pose[5] - 5.05) < 1e-9);
        for (int i = 0; i < 300; ++i) state.ingest(bit, 2000 + i);
        const auto snap = state.snapshot();
        QCOMPARE(snap.historyCount, std::size_t(200));
        QCOMPARE(snap.history[199].frame - snap.history[0].frame, quint64(199));
        QCOMPARE(snap.receivedMs[1], qint64(2299));
        bit.yaw = 65535;
        state.ingest(bit, 3000);
        QCOMPARE(state.snapshot().history[199].frame, snap.history[199].frame + 1);
    }
    void defaultsAndRememberedIndependentCommand() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 350, 5, -2}, 100, 50);
        auto rows = state.dialogValues();
        const double headings[] = {350, 80, 170, 260};
        for (int id = 0; id < 4; ++id) QCOMPARE(rows[id][3], headings[id]);
        rows[1] = {-45.123456789, -12.123456789, -123.45, 111.123, -3.456, -4.56};
        QVERIFY(state.lock(1, state.reference(1).pose()));
        RadarPositionControl command;
        QVERIFY(state.buildCommand(rows, command));
        QCOMPARE(sizeof(command), std::size_t(142));
        for (int id = 0; id < 4; ++id) QCOMPARE(command.panels[id].radarId, quint8(id));
        for (auto value : command.reserve) QCOMPARE(value, quint8(0));
        QCOMPARE(command.panels[1].longitudeE9, qint64(-45123456789LL));
        QCOMPARE(command.panels[1].latitudeE9, qint64(-12123456789LL));
        QCOMPARE(command.panels[1].headingMilliDeg, quint32(111123));
        QCOMPARE(command.panels[2].headingMilliDeg, quint32(170000));
        state.rememberSubmitted(command);
        state.ingest(report(0));
        const auto reopened = state.dialogValues();
        QVERIFY(std::abs(reopened[1][0] - rows[1][0]) < 1e-9);
        QVERIFY(std::abs(reopened[1][3] - rows[1][3]) < 1e-9);
        QCOMPARE(reopened[0][2], 200.0); // unlocked form follows effective live values.
        QCOMPARE(state.reference(0).value(RadarReference::Altitude), 200.0);
    }
    void locksAreIndependentAndCannotBeBypassed() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 10, 0, 0}, 100, 50);
        auto locked = state.reference(2).pose();
        locked[0] = 121; locked[3] = 222;
        QVERIFY(state.lock(2, locked));
        const auto update = state.ingest(report(2));
        QVERIFY(update.attitudeValid);
        QCOMPARE(int(update.position), int(GpsPositionFilter::Result::Accepted));
        QVERIFY(!update.changed);
        QCOMPARE(state.reference(2).pose(), locked);
        QCOMPARE(state.reference(0).value(RadarReference::Yaw), 10.0);
        QCOMPARE(int(state.reference(2).source(RadarReference::Latitude)), int(RadarReference::Source::Locked));
        QVERIFY(!state.reference(2).setManual({}));
        auto rows = state.dialogValues();
        rows[2][0] = std::numeric_limits<double>::quiet_NaN();
        RadarPositionControl command;
        QVERIFY(!state.buildCommand(rows, command)); // invalid edited locked value is rejected.
        QVERIFY(state.unlock(2));
        QCOMPARE(state.reference(2).value(RadarReference::Longitude), 120.0);
        QVERIFY(std::abs(state.reference(2).value(RadarReference::Yaw) - 123.456) < 1e-9);
        QVERIFY(!state.reference(2).locked());
    }
    void validReportsContinueFilteringWithoutChangingLockedPrimary() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        auto bit = report(0);
        state.ingest(bit);
        RadarReference::Values manual{}; manual[RadarReference::Yaw] = 80;
        QVERIFY(state.reference(0).setManual(manual));
        const auto frozen = state.reference(0).pose();
        QVERIFY(state.lock(0, frozen));
        bit.longitudeE9 += 10000; bit.altitudeCm += 200; bit.headingMilliDeg = 200000;
        QVERIFY(!state.ingest(bit).changed);
        QCOMPARE(state.reference(0).pose(), frozen);
        bit.latitudeE9 = 0; bit.longitudeE9 = 0; bit.altitudeCm = 0;
        QCOMPARE(int(state.ingest(bit).position), int(GpsPositionFilter::Result::AllZero));
        QVERIFY(state.unlock(0));
        QVERIFY(std::abs(state.reference(0).value(RadarReference::Longitude) - 120.000005) < 1e-9);
        QCOMPARE(state.reference(0).value(RadarReference::Altitude), 201.0);
        QVERIFY(std::abs(state.reference(0).value(RadarReference::Yaw) - 161.728) < 1e-6);
    }
    void invalidEditsAreAtomicAndReportsDoNotPoisonCache() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        auto rows = state.dialogValues();
        QVERIFY(state.lock(3, rows[3]));
        rows[3][2] = 5000.01;
        RadarPositionControl command;
        command.panels[0].altitudeCm = 123;
        QVERIFY(!state.buildCommand(rows, command));
        QCOMPARE(command.panels[0].altitudeCm, qint32(123));
        QVERIFY(!state.lock(3, rows[3])); QVERIFY(!state.lock(-1, rows[0]));
        QVERIFY(!state.unlock(4));
        auto bit = report(1);
        state.ingest(bit);
        const auto before = state.reference(1).pose();
        bit.latitudeE9 = 91000000000LL; bit.headingMilliDeg = 360001;
        const auto update = state.ingest(bit);
        QVERIFY(!update.changed); QVERIFY(!update.attitudeValid);
        QCOMPARE(state.reference(1).pose(), before);
    }
    void unlockWithoutReportRestoresDefaults() {
        RadarPanelState state;
        const RadarReference::Pose initial{120, 30, 100, 10, 0, 0};
        state.initialize(initial, 100, 50);
        auto edited = initial; edited[0] = 121; edited[3] = 200;
        QVERIFY(state.lock(0, edited));
        QVERIFY(!state.lock(0, initial)); // changing a locked panel requires explicit unlock.
        QVERIFY(state.unlock(0));
        QCOMPARE(state.reference(0).pose(), initial);
        QCOMPARE(int(state.reference(0).source(RadarReference::Longitude)), int(RadarReference::Source::Boot));
    }
    void attitudeOnlyChangeDoesNotReplayPosition() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 10, 0, 0}, 100, 50);
        auto bit = report(0);
        state.ingest(bit);
        bit.headingMilliDeg += 1000;
        const auto update = state.ingest(bit);
        QVERIFY(update.changed); QVERIFY(!update.positionChanged);
    }
};
QTEST_GUILESS_MAIN(RadarPanelStateTest)
#include "radarpanelstate_test.moc"
