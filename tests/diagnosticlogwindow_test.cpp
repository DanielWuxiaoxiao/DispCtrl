#include "Basic/diagnosticlogwindow.h"
#include <QTest>
#include <array>

class DiagnosticLogWindowTest : public QObject {
    Q_OBJECT
private slots:
    void firstAndWindowBoundary()
    {
        DiagnosticLogWindow window;
        QVERIFY(window.sample(0, 5000).write);
        for (int time = 1; time < 5000; ++time) {
            QVERIFY(!window.sample(time, 5000).write);
        }
        const auto next = window.sample(5000, 5000);
        QVERIFY(next.write);
        QCOMPARE(qulonglong(next.suppressed), qulonglong(4999));
        QVERIFY(!window.sample(5000, 5000).write);
    }

    void independentPanelsAndDiagnostics()
    {
        std::array<DiagnosticLogWindow, 5> angles;
        DiagnosticLogWindow badId;
        QVERIFY(angles[0].sample(100, 5000).write);
        QVERIFY(angles[1].sample(100, 5000).write);
        QVERIFY(!angles[0].sample(101, 5000).write);
        QVERIFY(badId.sample(101, 5000).write);
        QVERIFY(angles[4].sample(101, 5000).write);
    }

    void changingPayloadDoesNotCreateNewKeys()
    {
        DiagnosticLogWindow angle;
        int writes = 0;
        qulonglong suppressed = 0;
        // 2 kHz for 20 minutes, even when every raw angle differs.
        for (int time = 0; time <= 1200000; ++time) {
            for (int sample = 0; sample < 2; ++sample) {
                const auto decision = angle.sample(time, 5000);
                writes += decision.write ? 1 : 0;
                suppressed += decision.suppressed;
            }
        }
        QCOMPARE(writes, 241);
        QCOMPARE(suppressed, qulonglong(2399760));
    }

    void monotonicClockReset()
    {
        DiagnosticLogWindow window;
        QVERIFY(window.sample(10000, 5000).write);
        QVERIFY(!window.sample(10001, 5000).write);
        const auto reset = window.sample(0, 5000);
        QVERIFY(reset.write);
        QCOMPARE(qulonglong(reset.suppressed), qulonglong(1));
    }
};

QTEST_GUILESS_MAIN(DiagnosticLogWindowTest)
#include "diagnosticlogwindow_test.moc"
