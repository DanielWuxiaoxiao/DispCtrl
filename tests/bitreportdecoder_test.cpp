#include <QTest>
#include "Basic/bitreportdecoder.h"
#include "Basic/mechanicalgps.h"

class BitReportDecoderTest : public QObject {
    Q_OBJECT
private slots:
    void signedFieldsAndCommonStatus() {
        QByteArray bytes(103, '\0');
        qToLittleEndian<quint16>(0xDE02, bytes.data());
        bytes[2] = char(0x9F); bytes[3] = 3; bytes[17] = 2;
        qToLittleEndian<quint16>(615, bytes.data() + 4);
        qToLittleEndian<quint16>(35999, bytes.data() + 8);
        bytes[14] = 0x0F;
        qToLittleEndian<quint16>(12345, bytes.data() + 15);
        memset(bytes.data() + 18, 0xA5, 21);
        memset(bytes.data() + 73, 0x5A, 30);
        qToLittleEndian<qint64>(-12500000000LL, bytes.data() + 39);
        qToLittleEndian<qint64>(-45123456789LL, bytes.data() + 47);
        qToLittleEndian<qint32>(-12345, bytes.data() + 55);
        qToLittleEndian<quint32>(359999, bytes.data() + 59);
        qToLittleEndian<qint32>(-12345, bytes.data() + 63);
        qToLittleEndian<qint16>(-1234, bytes.data() + 67);
        qToLittleEndian<quint32>(123456, bytes.data() + 69);
        BITReport status; BITReportV2 extended; bool present = false;
        QVERIFY(decodeBitReportPayload(bytes.constData(), bytes.size(), true, status, extended, present));
        QVERIFY(present);
        QCOMPARE(status.radarId, static_cast<unsigned char>(2));
        QCOMPARE(status.fpgaTemp, static_cast<unsigned short>(615));
        QCOMPARE(status.scanAngle, static_cast<unsigned short>(12345));
        QCOMPARE(status.subArrayPower[4], static_cast<unsigned char>(0x0F));
        QCOMPARE(extended.latitudeE9, qint64(-12500000000LL));
        QCOMPARE(extended.longitudeE9, qint64(-45123456789LL));
        QCOMPARE(extended.altitudeCm, qint32(-12345));
        QCOMPARE(extended.headingMilliDeg, quint32(359999));
        QCOMPARE(extended.pitchMilliDeg, qint32(-12345));
        QCOMPARE(extended.rollCentiDeg, qint16(-1234));
        QCOMPARE(extended.servoEncoderMilliDeg, quint32(123456));
        for (auto value : status.reserve) QCOMPARE(value, static_cast<unsigned char>(0xA5));
        for (auto value : extended.legacyReserve) QCOMPARE(value, static_cast<unsigned char>(0xA5));
        for (auto value : extended.reserve) QCOMPARE(value, static_cast<unsigned char>(0x5A));
        bytes[17] = 0; // Mechanical system uses radar ID 0 with the same extended layout.
        QVERIFY(decodeBitReportPayload(bytes.constData(), bytes.size(), false, status, extended, present));
        QVERIFY(present);
        QCOMPARE(extended.radarId, static_cast<unsigned char>(0));
        QCOMPARE(extended.headingMilliDeg, quint32(359999));
        QCOMPARE(extended.servoEncoderMilliDeg, quint32(123456));
    }
    void mechanicalDeviationKeepsSignedRawSamplesAndGaps() {
        MechanicalGps gps;
        BITReportV2 report;
        report.bitGroup = 0x03;
        report.headingMilliDeg = 359999;
        report.servoEncoderMilliDeg = 0;
        gps.ingest(report);
        QCOMPARE(gps.latest().heading.value(), 359.999);
        QCOMPARE(gps.latest().encoder.value(), 0.0);
        QCOMPARE(gps.latest().deviation.value(), -359.999);

        report.headingMilliDeg = 1234;
        report.servoEncoderMilliDeg = 201234;
        gps.ingest(report);
        QCOMPARE(gps.latest().deviation.value(), 200.0);
        report.bitGroup = 0x01;
        gps.ingest(report);
        QVERIFY(!gps.latest().encoder);
        QVERIFY(!gps.latest().deviation);
        QCOMPARE(gps.sample(0).value(), -359.999);
        QCOMPARE(gps.sample(1).value(), 200.0);
        QVERIFY(!gps.sample(2));

        report.bitGroup = 0x03;
        report.servoEncoderMilliDeg = 360001;
        gps.ingest(report);
        QVERIFY(!gps.latest().encoder);
        report.servoEncoderMilliDeg = 0;
        report.headingMilliDeg = 360001;
        gps.ingest(report);
        QVERIFY(!gps.latest().heading);
        QVERIFY(!gps.latest().deviation);
    }
    void mechanicalHistoryDropsOnlyOldestSamples() {
        MechanicalGps gps;
        BITReportV2 report;
        report.bitGroup = 0x03;
        report.headingMilliDeg = 90000;
        for (quint32 i = 0; i <= 200; ++i) {
            report.servoEncoderMilliDeg = 90000 + i;
            gps.ingest(report);
        }
        QCOMPARE(gps.count(), std::size_t(200));
        QCOMPARE(gps.sample(0).value(), 0.001);
        QCOMPARE(gps.sample(199).value(), 0.200);
    }
    void northDeviationIsSignedAndUsesTheCurrentReport() {
        MechanicalGps gps;
        ServoControlParam command;
        command.cmd = 3;
        command.speed = 3;
        QVERIFY(!gps.fillNorthDeviation(command, false));

        BITReportV2 report;
        report.bitGroup = 3;
        report.headingMilliDeg = 20000;
        report.servoEncoderMilliDeg = 109000;
        gps.ingest(report);
        QVERIFY(gps.fillNorthDeviation(command, false));
        QCOMPARE(command.northDeviationMilliDeg, qint32(89000));

        report.servoEncoderMilliDeg = 19000;
        gps.ingest(report);
        QVERIFY(gps.fillNorthDeviation(command, false));
        QCOMPARE(command.northDeviationMilliDeg, qint32(-1000));
        const QByteArray bytes(reinterpret_cast<const char*>(&command), sizeof(command));
        QCOMPARE(bytes.toHex(), QByteArray("03aa0303000018fcffff000000"));

        report.servoEncoderMilliDeg = 20123;
        gps.ingest(report);
        QVERIFY(gps.fillNorthDeviation(command, false));
        QCOMPARE(command.northDeviationMilliDeg, qint32(123));

        report.bitGroup = 1;
        gps.ingest(report);
        QVERIFY(!gps.fillNorthDeviation(command, false));
        QVERIFY(gps.fillNorthDeviation(command, true));
        QCOMPARE(command.northDeviationMilliDeg, qint32(0));
    }
    void legacyHasNoReferenceAndTruncationIsRejected() {
        QByteArray bytes(120, '\0');
        qToLittleEndian<quint16>(0xDE02, bytes.data());
        bytes[18] = char(0xA5); // Legacy reserve is NOT interpreted as latitude.
        BITReport status; BITReportV2 extended; bool present = true;
        QVERIFY(decodeBitReportPayload(bytes.constData(), 39, false, status, extended, present));
        QVERIFY(!present);
        QCOMPARE(status.reserve[0], static_cast<unsigned char>(0xA5));
        for (bool multi : {false, true}) {
            for (int size = 0; size < bytes.size(); ++size) {
                if (size == 103 || (!multi && size == 39)) continue;
                present = true;
                QVERIFY(!decodeBitReportPayload(bytes.constData(), size, multi, status, extended, present));
                QVERIFY(!present);
            }
        }
        bytes[0] = 0;
        QVERIFY(!decodeBitReportPayload(bytes.constData(), 103, true, status, extended, present));
        QVERIFY(!decodeBitReportPayload(nullptr, 103, true, status, extended, present));
    }
};
QTEST_APPLESS_MAIN(BitReportDecoderTest)
#include "bitreportdecoder_test.moc"
