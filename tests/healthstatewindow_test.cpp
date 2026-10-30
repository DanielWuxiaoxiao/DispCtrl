#include <QTest>
#include "Basic/healthstatewindow.h"

class HealthStateWindowTest final : public QObject {
    Q_OBJECT
    static BITReport normal(int id = 0) {
        BITReport r;
        r.radarId = static_cast<unsigned char>(id);
        r.bitGroup = 0x0F; // TX/RX closed is not a fault.
        r.powerState = 0x03;
        for (int i = 0; i < 5; ++i) r.subArrayPower[i] = i == 4 ? 0x0F : 0xFF;
        return r;
    }
private slots:
    void repeatedFaultsAndFlappingAreRateLimited() {
        HealthFaultLogGate gate;
        QVERIFY(gate.sample(0, 1, 0));
        QVERIFY(!gate.sample(0, 1, 5000));
        QVERIFY(!gate.sample(0, 2, 10000));
        QVERIFY(!gate.sample(0, 2, 30000));
        QVERIFY(gate.sample(0, 2, 300000));
        QVERIFY(!gate.sample(0, 2, 600000));
        QVERIFY(!gate.sample(0, 2, 3600000)); // no recurring reminder
        QVERIFY(gate.sample(1, 2, 5000)); // panels are independent
        QVERIFY(!gate.sample(0, 0, 3605000));
        QVERIFY(!gate.sample(0, 2, 3610000)); // brief recovery does not rearm
        for (qint64 t = 3615000; t <= 3675000; t += 5000)
            QVERIFY(!gate.sample(0, 0, t));
        QVERIFY(gate.sample(0, 2, 3680000)); // one minute confirmed recovery
        QVERIFY(gate.sample(4, 1, 0));
        QVERIFY(!gate.sample(5, 1, 0));
    }
    void missingReportsDoNotRearmFaultLogs() {
        HealthFaultLogGate gate;
        QVERIFY(gate.sample(0, 1, 0));
        QVERIFY(!gate.sample(0, 0, 5000));
        QVERIFY(!gate.sample(0, 0, 120000));
        QVERIFY(!gate.sample(0, 1, 125000));
        // Even changing faults are limited to one entry per slot per five minutes.
        for (qint64 t = 130000; t < 300000; t += 5000)
            QVERIFY(!gate.sample(0, quint64(t), t));
        QVERIFY(gate.sample(0, 3, 300000));
    }
    void hardwareFaultsLatchUntilNextWindow() {
        HealthStateWindow window;
        auto r = normal();
        window.pushBit(r);
        r.bitGroup |= 0x40;
        r.bitGroup &= ~0x04;
        r.powerState &= ~0x02;
        window.pushBit(r);
        window.pushBit(normal());
        HealthStateWindow::BitSnapshot snapshot;
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(HealthStateWindow::bitFault(snapshot));
        QVERIFY(snapshot.report.bitGroup & 0x40);
        QVERIFY(!(snapshot.report.bitGroup & 0x04));
        QVERIFY(!(snapshot.report.powerState & 0x02));
        QVERIFY(!window.takeBit(0, snapshot));
        window.pushBit(normal());
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
    }
    void strictNinetyPercentAndWindowReset() {
        HealthStateWindow window;
        HealthStateWindow::BitSnapshot snapshot;
        auto zero = normal();
        for (auto& byte : zero.subArrayPower) byte = 0;
        for (int i = 0; i < 9; ++i) window.pushBit(zero);
        window.pushBit(normal()); // exactly 90% faults: green
        QVERIFY(window.takeBit(0, snapshot));
        QCOMPARE(snapshot.powerSampleCount, quint32(10));
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
        for (int i = 0; i < 91; ++i) window.pushBit(zero);
        for (int i = 0; i < 9; ++i) window.pushBit(normal());
        QVERIFY(window.takeBit(0, snapshot));
        QCOMPARE(snapshot.powerSampleCount, quint32(100));
        QVERIFY(HealthStateWindow::bitFault(snapshot));
        window.pushBit(normal());
        QVERIFY(window.takeBit(0, snapshot));
        QCOMPARE(snapshot.powerSampleCount, quint32(1));
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
        QVERIFY(!window.takeBit(0, snapshot)); // no samples: no synthetic recovery
    }
    void unknownPowerAndIndependentPanels() {
        HealthStateWindow window;
        auto r = normal();
        for (auto& byte : r.subArrayPower) byte = 0;
        window.pushBit(r);
        window.pushBit(normal(1));
        HealthStateWindow::BitSnapshot a, b;
        QVERIFY(window.takeBit(0, a));
        QVERIFY(a.powerConfirmed);
        QVERIFY(HealthStateWindow::bitFault(a)); // one all-zero frame = 100%, no run-length debounce
        QVERIFY(window.takeBit(1, b));
        QVERIFY(b.powerConfirmed);
        QVERIFY(!HealthStateWindow::bitFault(b));
        for (int i = 0; i < 1200; ++i) window.pushBit(r);
        QVERIFY(window.takeBit(0, a));
        QCOMPARE(a.powerSampleCount, quint32(1200));
    }
    void channelsHaveIndependentRatiosAndIgnoreReservedBits() {
        HealthStateWindow window;
        for (int frame = 0; frame < 100; ++frame) {
            auto bit = normal(2);
            // channel1:91%, channel2:90%, channel36:100%; all others normal.
            if (frame < 91) bit.subArrayPower[0] &= ~1;
            if (frame < 90) bit.subArrayPower[0] &= ~2;
            bit.subArrayPower[4] = 0xF7; // high four reserved bits must not affect channel status
            window.pushBit(bit);
        }
        HealthStateWindow::BitSnapshot s;
        QVERIFY(window.takeBit(2, s));
        QVERIFY(!(s.report.subArrayPower[0] & 1));
        QVERIFY(s.report.subArrayPower[0] & 2);
        QCOMPARE(s.report.subArrayPower[4], quint8(0x07));
        QVERIFY(!window.takeBit(0, s));
    }
    void partialPowerFaultAndSoftwareFaults() {
        HealthStateWindow window;
        auto r = normal();
        r.subArrayPower[0] &= ~1;
        window.pushBit(r);
        window.pushBit(normal());
        HealthStateWindow::BitSnapshot snapshot;
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(snapshot.report.subArrayPower[0] & 1); // 50% faults -> green
        MonitorParam status;
        status.sigProSta = 3;
        window.pushSoftware(status);
        status.sigProSta = 0;
        window.pushSoftware(status);
        QVERIFY(window.takeSoftware(status));
        QCOMPARE(status.sigProSta, static_cast<unsigned char>(3));
        status.sigProSta = 0;
        status.dataProSta = 4; // Successful closure is not an alarm.
        window.pushSoftware(status);
        QVERIFY(window.takeSoftware(status));
        QVERIFY(!HealthStateWindow::softwareFault(status.sigProSta));
        QVERIFY(!HealthStateWindow::softwareFault(status.dataProSta));
        static_assert(sizeof(HealthStateWindow) < 1024, "Health aggregation must remain fixed and bounded");
    }
};
QTEST_GUILESS_MAIN(HealthStateWindowTest)
#include "healthstatewindow_test.moc"
