#include "ZhoushanOptical/protocol.h"
#include <QTest>
using namespace ZhoushanOptical;

class ZhoushanOpticalProtocolTest : public QObject {
    Q_OBJECT
private:
    static QByteArray reply(quint8 kind = 1, quint8 state = 1) {
        QByteArray b = indication(3101, 1100, kind, 42, {120.125, 30.25, 100},
            QDateTime(QDate(2026, 10, 20), QTime(12, 34, 56, 789), Qt::UTC));
        b[2] = 4; b[3] = 1; b[23] = char(state);
        b[44] = char(checksum(b.mid(2, 42)) & 255); b[45] = char(checksum(b.mid(2, 42)) >> 8);
        return b;
    }
private slots:
    void requestLayout() {
        const auto b = indication(1100, 3101, 1, 42, {120.125, 30.25, 100},
            QDateTime(QDate(2026, 10, 20), QTime(12, 34, 56, 789), Qt::UTC));
        QCOMPARE(b.size(), 48); QCOMPARE(b.left(8).toHex(), QByteArray("eb9004004c041d0c"));
        QCOMPARE(b.toHex(), QByteArray("eb9004004c041d0c1a0a140c223815031a00012a000000010000000000085e400000000000403e400000c84251984c5a"));
        QCOMPARE(b.mid(8, 8).toHex(), QByteArray("1a0a140c22381503"));
        QCOMPARE(b.mid(16, 8).toHex(), QByteArray("1a00012a00000001"));
        QCOMPARE(readDouble(b, 24), 120.125); QCOMPARE(readDouble(b, 32), 30.25);
        QCOMPARE(readFloat(b, 40), 100.0f); QCOMPARE(b.right(2).toHex(), QByteArray("4c5a"));
        QCOMPARE(checksum(QByteArray::fromHex("010203")), quint16(0x0204));
        QCOMPARE(checksum(QByteArray::fromHex("ffffffff01")), quint16(0xffff));
    }
    void responseStatesAndInvalidFields() {
        Response result; QString error;
        for (quint8 kind = 1; kind <= 2; ++kind) {
            for (quint8 state = 1; state <= (kind == 1 ? 2 : 3); ++state) {
                QVERIFY2(parseResponse(reply(kind, state), 3101, 1100, result, error), qPrintable(error));
                QCOMPARE(result.kind, kind); QCOMPARE(result.state, state); QCOMPARE(result.batch, 42U);
            }
        }
        QVERIFY(!parseResponse(reply(1, 3), 3101, 1100, result, error));
        QVERIFY(!parseResponse(reply(2, 0), 3101, 1100, result, error));
        QVERIFY(!parseResponse(reply(), 3102, 1100, result, error));
        QVERIFY(!parseResponse(reply(), 3101, 1101, result, error));
        auto altitude = reply();
        altitude[40] = char(0xff); altitude[41] = char(0xff); altitude[42] = char(0xff); altitude[43] = char(0x0f);
        auto sum = checksum(altitude.mid(2, 42)); altitude[44] = char(sum & 255); altitude[45] = char(sum >> 8);
        QVERIFY(parseResponse(altitude, 3101, 1100, result, error)); QVERIFY(!result.altitudeValid);
        auto b = reply(); b[24] = char(quint8(b[24]) ^ 1); QVERIFY(!parseResponse(b, 3101, 1100, result, error));
        for (int n = 0; n < FrameSize; ++n) QVERIFY(!parseResponse(reply().left(n), 3101, 1100, result, error));
        QVERIFY(indication(1100, 3101, 0, 1, {120, 30, 100}, QDateTime::currentDateTime()).isEmpty());
        QVERIFY(indication(1100, 3101, 1, 1, {181, 30, 100}, QDateTime::currentDateTime()).isEmpty());
        QVERIFY(indication(1100, 3101, 1, 1, {120, 30, 100}, QDateTime(QDate(2100, 1, 1), QTime(0, 0))).isEmpty());
    }
    void splitCoalescedAndNoise() {
        const auto b = reply();
        for (int split = 0; split <= b.size(); ++split) {
            StreamDecoder decoder; quint64 discarded = 0;
            QVERIFY(decoder.appendData(b.left(split)));
            auto first = decoder.take(discarded);
            if (split == b.size()) QCOMPARE(first, b); else QVERIFY(first.isEmpty());
            QVERIFY(decoder.appendData(b.mid(split)));
            if (split < b.size()) QCOMPARE(decoder.take(discarded), b);
            QCOMPARE(discarded, 0ULL);
        }
        StreamDecoder decoder; quint64 discarded = 0;
        QVERIFY(decoder.appendData(QByteArray::fromHex("00ffeb") + b + b));
        QCOMPARE(decoder.take(discarded), b); QCOMPARE(decoder.take(discarded), b);
        QCOMPARE(discarded, 3ULL); QVERIFY(decoder.take(discarded).isEmpty());
        auto invalid = b; invalid[44] = 0; invalid[45] = 0;
        QVERIFY(decoder.appendData(invalid + b)); QCOMPARE(decoder.take(discarded), b);
        QVERIFY(!decoder.appendData(QByteArray(8193, 0)));
    }
};
QTEST_GUILESS_MAIN(ZhoushanOpticalProtocolTest)
#include "zhoushanopticalprotocol_test.moc"
