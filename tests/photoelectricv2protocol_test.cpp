#include "PhotoelectricV2/protocol.h"
#include <QTest>
#include <limits>
using namespace PhotoelectricV2;

class PhotoelectricV2ProtocolTest : public QObject {
    Q_OBJECT
private:
    static QByteArray incoming(quint16 function) {
        QByteArray b = header(function);
        if (function == Heartbeat) {
            append(b, 7, 4); append(b, 1700000000123ULL, 8);
        } else {
            append(b, 0, 1); append(b, 1, 4); append(b, 7, 4); append(b, 1700000000123ULL, 8);
            if (function == Target) {
                append(b, 42, 4); append(b, 0xffffffffffffffffULL, 8);
                append(b, 9000, 2); append(b, 9500, 2); append(b, 1000, 4);
                append(b, 8, 1); append(b, 600, 2); append(b, 20, 2);
                append(b, 12012345, 4); append(b, 3012345, 4);
            } else {
                append(b, 12012345, 4); append(b, 3012345, 4); append(b, 600, 2);
                append(b, 9000, 2); append(b, 9500, 2); append(b, 2, 1); append(b, 5, 1);
            }
        }
        return finish(b);
    }
private slots:
    void guidanceWireLayout() {
        const auto b = guidance(1, 7, 1700000000123ULL, 0x100000002ULL, 120.12345, 30.12345, 100, 15);
        QCOMPARE(b.size(), 47);
        QCOMPARE(b.toHex(), QByteArray("014c5d6d7d2f00003401000000070000007b68e5cf8b0100000200000001000000394bb700f9f62d0058020f0000e5"));
        QCOMPARE(b.left(9).toHex(), QByteArray("014c5d6d7d2f000034"));
        QCOMPARE(read(b, 9, 4), quint64(1));
        QCOMPARE(read(b, 13, 4), quint64(7));
        QCOMPARE(read(b, 17, 8), quint64(1700000000123ULL));
        QCOMPARE(read(b, 25, 8), quint64(0x100000002ULL));
        QCOMPARE(read(b, 33, 4), quint64(12012345));
        QCOMPARE(read(b, 37, 4), quint64(3012345));
        QCOMPARE(read(b, 41, 2), quint64(600));
        QCOMPARE(read(b, 43, 2), quint64(15));
        QCOMPARE(quint8(b[45]), quint8(0));
        QCOMPARE(quint8(b.back()), checksum(b.left(46)));
    }
    void controlsAndHeartbeat() {
        const auto b = control(1, 7, 100, QByteArray::fromHex("0101010104010706"));
        QCOMPARE(b.toHex(), QByteArray("014c5d6d7d220000350100000007000000640000000000000001010101040107066d"));
        QCOMPARE(b.size(), 34); QCOMPARE(read(b, 5, 2), quint64(34));
        QCOMPARE(read(b, 7, 2), quint64(Control));
        QCOMPARE(b.mid(25, 8).toHex(), QByteArray("0101010104010706"));
        QVERIFY(control(1, 0, 0, QByteArray(7, 0)).isEmpty());
        QVERIFY(control(1, 0, 0, QByteArray::fromHex("0200000000000000")).isEmpty());
        const auto heartbeatFrame = heartbeat(7, 100);
        QCOMPARE(heartbeatFrame.size(), 22);
        QVariantMap f; QString error;
        QVERIFY2(parse(heartbeatFrame, f, error), qPrintable(error));
        QCOMPARE(f["terminal"].toUInt(), 7U);
        QCOMPARE(f["timestamp"].toULongLong(), 100ULL);
    }
    void targetAndState() {
        QVariantMap f; QString error;
        auto b = incoming(Target); QCOMPARE(b.size(), 60);
        QVERIFY2(parse(b, f, error), qPrintable(error));
        QCOMPARE(f["guideTarget"].toULongLong(), 0xffffffffffffffffULL);
        QCOMPARE(f["batch"].toUInt(), 42U);
        QCOMPARE(f["azimuth"].toDouble(), 90.0);
        QCOMPARE(f["elevation"].toDouble(), -5.0);
        QCOMPARE(f["longitude"].toDouble(), 120.12345);
        QCOMPARE(f["latitude"].toDouble(), 30.12345);
        QCOMPARE(f["altitude"].toInt(), 100);
        QCOMPARE(f["relativeHeight"].toUInt(), 20U);
        b = incoming(Status); QCOMPARE(b.size(), 43);
        QVERIFY2(parse(b, f, error), qPrintable(error));
        QCOMPARE(f["fault"].toUInt(), 5U); QCOMPARE(f["mode"].toUInt(), 2U);
        b = incoming(Heartbeat); QVERIFY(parse(b, f, error));
    }
    void invalidFrames() {
        QVariantMap f; QString error;
        const auto b = incoming(Target);
        for (int n = 0; n < b.size(); ++n) QVERIFY(!parse(b.left(n), f, error));
        auto changed = b; changed[11] = char(quint8(changed[11]) ^ 1);
        QVERIFY(!parse(changed, f, error));
        changed = b; changed[0] = 2; changed = finish(changed.left(changed.size() - 1));
        QVERIFY(!parse(changed, f, error));
        changed = b; changed[9] = 2; changed = finish(changed.left(changed.size() - 1));
        QVERIFY(!parse(changed, f, error));
        QVERIFY(!parse(b + b, f, error)); // preserve UDP frame boundary
        changed = incoming(Status); changed[40] = 7; changed = finish(changed.left(changed.size() - 1));
        QVERIFY(!parse(changed, f, error));
        const double nan = std::numeric_limits<double>::quiet_NaN();
        QVERIFY(guidance(1, 0, 0, 1, nan, 30, 100, 1).isEmpty());
        QVERIFY(guidance(1, 0, 0, 1, -120, 30, 100, 1).isEmpty());
        QVERIFY(guidance(1, 0, 0, 1, 120, 30, -501, 1).isEmpty());
        QVERIFY(guidance(1, 0, 0, 1, 120, 30, 100, -1).isEmpty());
        QVERIFY(guidance(1, 0, 0, 1, 120, 30, 100, 65536).isEmpty());
    }
};
QTEST_GUILESS_MAIN(PhotoelectricV2ProtocolTest)
#include "photoelectricv2protocol_test.moc"
