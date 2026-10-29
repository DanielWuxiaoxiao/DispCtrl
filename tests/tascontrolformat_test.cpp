#include "Basic/tascontrolformat.h"
#include <QByteArray>
#include <QTest>

class TasControlFormatTest final : public QObject {
    Q_OBJECT
private slots:
    void fixedAndRandomFrequencyEncoding() {
        QCOMPARE(sizeof(BeamControl), std::size_t(54));
        QCOMPARE(offsetof(BeamControl, aziStart), std::size_t(4));
        QCOMPARE(offsetof(BeamControl, aziEnd), std::size_t(8));
        QCOMPARE(offsetof(BeamControl, beam1Flag), std::size_t(18));
        QCOMPARE(offsetof(BeamControl, beam2Flag), std::size_t(30));
        QCOMPARE(offsetof(BeamControl, beam3Flag), std::size_t(42));
        for (int index = 0; index <= 8; ++index) {
            QCOMPARE(TasControl::frequencyCode(index), index * 10);
            QCOMPARE(TasControl::frequencyIndex(index * 10), index);
        }
        QCOMPARE(TasControl::frequencyCode(9), 90);
        QCOMPARE(TasControl::frequencyIndex(90), 9);
        QCOMPARE(TasControl::frequencyCode(-1), -1);
        QCOMPARE(TasControl::frequencyIndex(9), -1);
        BeamControl param;
        const QByteArray baseline(reinterpret_cast<const char*>(&param), sizeof(param));
        param.freqID = static_cast<unsigned char>(TasControl::frequencyCode(9));
        const QByteArray wire(reinterpret_cast<const char*>(&param), sizeof(param));
        QCOMPARE(quint8(wire[2]), quint8(0x5A));
        QCOMPARE(wire.left(2), baseline.left(2));
        QCOMPARE(wire.mid(3), baseline.mid(3));
        QVERIFY(TasControl::frequencyText(90).contains(QStringLiteral("随机跳频")));
        QVERIFY(TasControl::frequencyText(40).contains("9.4GHz"));
        QVERIFY(TasControl::frequencyText(80).contains("9.8GHz"));
    }
    void logsUseTransmittedFields() {
        BeamControl beam;
        beam.freqID = 90; beam.type = 1;
        beam.aziStart = 9000; beam.aziEnd = 18000; beam.aziStep = 400;
        beam.scene = 0; beam.flagNum = 2; beam.pulseNum = 128;
        beam.sampleStart1 = 15; beam.sampleEnd1 = 250;
        beam.elestart1 = -100; beam.eleend1 = 3000; beam.elestep1 = 400;
        const QString details = TasControl::beamDetails(beam);
        QVERIFY(details.contains("freqID=90/0x5A"));
        QVERIFY(details.contains(QStringLiteral("方位=90.00~180.00°")));
        QVERIFY(details.contains("sceneRaw=0"));
        QVERIFY(details.contains(QStringLiteral("积累脉冲=128")));
        QVERIFY(details.contains(QStringLiteral("采样=1.5~25.0us")));
        QVERIFY(details.contains(QStringLiteral("俯仰=-1.00~30.00°")));
        QVERIFY(details.contains(QStringLiteral("波形3[")));
        ServoControlParam servo;
        servo.cmd = 2; servo.speed = 3; servo.az = 13500;
        QVERIFY(TasControl::servoDetails(servo).contains("135.00"));
        QVERIFY(TasControl::servoDetails(servo).contains("azRaw=13500"));
        ScanRange scan;
        scan.workMode = 1;
        QVERIFY(TasControl::scanDetails(scan).contains("TAS(raw=1)"));
    }
};
QTEST_GUILESS_MAIN(TasControlFormatTest)
#include "tascontrolformat_test.moc"
