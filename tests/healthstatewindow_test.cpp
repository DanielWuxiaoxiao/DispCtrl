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
        QVERIFY(gate.sample(0, 2, 30000));
        QVERIFY(!gate.sample(0, 2, 325000));
        QVERIFY(gate.sample(0, 2, 330000));
        QVERIFY(gate.sample(1, 2, 5000)); // panels are independent
        QVERIFY(!gate.sample(0, 0, 335000)); // normal snapshot resets fault identity, not cooldown
        QVERIFY(!gate.sample(0, 2, 340000));
        QVERIFY(gate.sample(0, 2, 360000));
        QVERIFY(gate.sample(4, 1, 0));
        QVERIFY(!gate.sample(5, 1, 0));
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
    void thousandZeroFramesAcrossWindows() {
        HealthStateWindow window;
        HealthStateWindow::BitSnapshot snapshot;
        window.pushBit(normal());
        QVERIFY(window.takeBit(0, snapshot));
        auto zero = normal();
        for (auto& byte : zero.subArrayPower) byte = 0;
        for (int i = 0; i < 500; ++i) window.pushBit(zero);
        QVERIFY(window.takeBit(0, snapshot));
        QCOMPARE(snapshot.consecutiveZeroFrames, 500);
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
        for (int i = 0; i < 499; ++i) window.pushBit(zero);
        QVERIFY(window.takeBit(0, snapshot));
        QCOMPARE(snapshot.consecutiveZeroFrames, 999);
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
        window.pushBit(zero);
        window.pushBit(normal()); // Recovery does not erase this window's confirmed fault.
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(HealthStateWindow::bitFault(snapshot));
        QCOMPARE(snapshot.consecutiveZeroFrames, 0);
        window.pushBit(normal());
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(!HealthStateWindow::bitFault(snapshot));
    }
    void unknownPowerAndIndependentPanels() {
        HealthStateWindow window;
        auto r = normal();
        for (auto& byte : r.subArrayPower) byte = 0;
        window.pushBit(r);
        window.pushBit(normal(1));
        HealthStateWindow::BitSnapshot a, b;
        QVERIFY(window.takeBit(0, a));
        QVERIFY(!a.powerConfirmed);
        QVERIFY(!HealthStateWindow::bitFault(a));
        QVERIFY(window.takeBit(1, b));
        QVERIFY(b.powerConfirmed);
        QVERIFY(!HealthStateWindow::bitFault(b));
        for (int i = 0; i < 1200; ++i) window.pushBit(r);
        QVERIFY(window.takeBit(0, a));
        QCOMPARE(a.consecutiveZeroFrames, 1000);
    }
    void partialPowerFaultAndSoftwareFaults() {
        HealthStateWindow window;
        auto r = normal();
        r.subArrayPower[0] &= ~1;
        window.pushBit(r);
        window.pushBit(normal());
        HealthStateWindow::BitSnapshot snapshot;
        QVERIFY(window.takeBit(0, snapshot));
        QVERIFY(!(snapshot.report.subArrayPower[0] & 1));
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
