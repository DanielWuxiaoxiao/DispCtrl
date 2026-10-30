#include <QTest>
#include "Basic/Protocol.h"

class TrackDataRateTest final : public QObject {
    Q_OBJECT
private slots:
    void layoutAndGoldenFrame() {
        TrackDataRateControl command;
        QCOMPARE(command.mesID, quint16(0xAA09));
        QCOMPARE(command.rateHz, quint8(2));
        QVERIFY(TrackDataRateControl::build(0x1234, 100, command));
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(&command), sizeof(command)).toHex(),
                 QByteArray("09aa341264"));
        char* packed = packData(reinterpret_cast<char*>(&command), sizeof(command), 1, 2, 3);
        QVERIFY(packed);
        const QByteArray bytes(packed, sizeof(ProtocolFrame) + sizeof(command) + sizeof(ProtocolEnd));
        std::free(packed);
        QCOMPARE(bytes.size(), 24);
        QCOMPARE(bytes.mid(sizeof(ProtocolFrame), 5).toHex(), QByteArray("09aa341264"));
        QCOMPARE(quint8(bytes[19]), calculateXOR(bytes.constData(), 19));
    }
    void validatesWithoutTruncation() {
        TrackDataRateControl command;
        QVERIFY(TrackDataRateControl::build(65535, 1, command));
        for (int batch : {-1, 65536}) QVERIFY(!TrackDataRateControl::build(batch, 2, command));
        for (int hz : {-1, 0, 101, 256}) QVERIFY(!TrackDataRateControl::build(1, hz, command));
        QCOMPARE(command.batch, quint16(65535)); QCOMPARE(command.rateHz, quint8(1));
        QVERIFY(TrackDataRateControl::build(0, 2, command));
        QCOMPARE(command.batch, quint16(0));
    }
};
QTEST_APPLESS_MAIN(TrackDataRateTest)
#include "trackdatarate_test.moc"
