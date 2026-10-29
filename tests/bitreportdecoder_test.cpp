#include <QTest>
#include "Basic/bitreportdecoder.h"

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
        memset(bytes.data() + 69, 0x5A, 34);
        qToLittleEndian<qint64>(-12500000000LL, bytes.data() + 39);
        qToLittleEndian<qint64>(-45123456789LL, bytes.data() + 47);
        qToLittleEndian<qint32>(-12345, bytes.data() + 55);
        qToLittleEndian<quint32>(359999, bytes.data() + 59);
        qToLittleEndian<qint32>(-12345, bytes.data() + 63);
        qToLittleEndian<qint16>(-1234, bytes.data() + 67);
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
        for (auto value : status.reserve) QCOMPARE(value, static_cast<unsigned char>(0xA5));
        for (auto value : extended.legacyReserve) QCOMPARE(value, static_cast<unsigned char>(0xA5));
        for (auto value : extended.reserve) QCOMPARE(value, static_cast<unsigned char>(0x5A));
        QVERIFY(!decodeBitReportPayload(bytes.constData(), bytes.size(), false, status, extended, present));
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
                if (size == (multi ? 103 : 39)) continue;
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
