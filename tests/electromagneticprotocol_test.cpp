#include "ElectromagneticDevice/deviceprotocol.h"
#include <QTest>
#include <QtEndian>
#include <limits>

class ElectromagneticProtocolTest : public QObject {
    Q_OBJECT
private slots:
    void frameBytesAndCorruption()
    {
        using namespace ElectromagneticProtocol;
        const QByteArray packet = encode(0xBB08, 0xBB04, 1, 1, QByteArray(1, char(1)));
        QCOMPARE(packet.size(), 22);
        QCOMPARE(packet.left(17).toHex(), QByteArray("55fa55fa08bb04bb010000001100010001"));
        Frame frame; QString error;
        QVERIFY(decode(packet, frame, error));
        QCOMPARE(frame.source, quint16(0xBB08));
        QCOMPARE(frame.destination, quint16(0xBB04));
        QCOMPARE(frame.sequence, quint32(1));
        QVERIFY(validCommand(frame.message, frame.payload));
        QByteArray corrupted = packet; corrupted[16] = char(0);
        QVERIFY(!decode(corrupted, frame, error));
        QVERIFY(!decode(packet.left(20), frame, error));
        QVERIFY(encode(1, 2, 0, 5, QByteArray(1452, char(0))).isEmpty());
    }
    void commandRanges()
    {
        using namespace ElectromagneticProtocol;
        QVERIFY(validCommand(2, QByteArray::fromHex("0001")));
        QVERIFY(!validCommand(2, QByteArray::fromHex("01")));
        QVERIFY(!validCommand(1, QByteArray::fromHex("02")));
        QVERIFY(validCommand(4, QByteArray::fromHex("05")));
        QVERIFY(!validCommand(4, QByteArray::fromHex("00")));
        QVERIFY(!validCommand(6, QByteArray::fromHex("01")));
    }
    void recordsAndCoordinates()
    {
        using namespace ElectromagneticProtocol;
        const Wgs84Coordinate::Lla origin{120.0, 30.0, 100.0};
        detInfo d{}; d.dis = 1000; d.azi = 90; d.ele = 30;
        const QByteArray detectionRecord = detection(d, origin);
        QCOMPARE(detectionRecord.size(), DetectionBytes);
        const auto* bytes = reinterpret_cast<const uchar*>(detectionRecord.constData());
        QCOMPARE(qFromLittleEndian<qint32>(bytes + 16), qint32(600));
        QCOMPARE(qFromLittleEndian<qint32>(bytes + 60), qint32(300000000));
        QVERIFY(qFromLittleEndian<qint32>(bytes + 56) > 1200000000);
        trackInfo t{}; t.batch = 42; t.CPIID = 19; t.UTCtime = 123456; t.nsecond = 789;
        t.dis = d.dis; t.azi = d.azi; t.ele = d.ele;
        t.altitute = -9999; // WGS84 height must come from RAE, not display height.
        const QByteArray trackRecord = track(t, origin);
        QCOMPARE(trackRecord.size(), TrackBytes);
        bytes = reinterpret_cast<const uchar*>(trackRecord.constData());
        QCOMPARE(qFromLittleEndian<quint16>(bytes), quint16(42));
        QCOMPARE(qFromLittleEndian<quint16>(bytes + 2), quint16(19));
        QCOMPARE(qFromLittleEndian<quint32>(bytes + 4), quint32(123456));
        QCOMPARE(qFromLittleEndian<qint32>(bytes + 33), qint32(600));
        t.dis = -1; QVERIFY(track(t, origin).isEmpty());
        d.dis = std::numeric_limits<float>::quiet_NaN(); QVERIFY(detection(d, origin).isEmpty());
        QCOMPARE(panel(BITReport{}).size(), 15);
        QCOMPARE(software(MonitorParam{}).size(), 4);
    }
    void mtuBoundary()
    {
        using namespace ElectromagneticProtocol;
        QCOMPARE(encode(1, 2, 0, 5, QByteArray(2 + 22 * DetectionBytes, char(0))).size(), 1431);
        QVERIFY(encode(1, 2, 0, 5, QByteArray(2 + 23 * DetectionBytes, char(0))).isEmpty());
        QCOMPARE(encode(1, 2, 0, 6, QByteArray(2 + 23 * TrackBytes, char(0))).size(), 1426);
        QVERIFY(encode(1, 2, 0, 6, QByteArray(2 + 24 * TrackBytes, char(0))).isEmpty());
    }
    void uiShutdownStatesMapToAbnormal()
    {
        MonitorParam status; status.dataProSta = 4; status.beamConSta = 5;
        status.sigProSta = 2; status.targetRecSta = 3;
        QCOMPARE(ElectromagneticProtocol::software(status), QByteArray::fromHex("01010203"));
        status.targetRecSta = 6;
        QVERIFY(ElectromagneticProtocol::software(status).isEmpty());
    }
    void confirmedSubArrayPowerMatchesUi()
    {
        BITReport raw; raw.bitGroup = 0x9f; raw.fpgaTemp = 450; raw.yaw = 9000;
        std::array<bool, 36> normal{};
        normal.fill(true);
        // A transient all-zero raw frame must not overwrite the UI's confirmed normal bitmap.
        auto visible = ElectromagneticProtocol::withConfirmedSubArrayPower(raw, normal, true);
        QCOMPARE(ElectromagneticProtocol::panel(visible).mid(8, 5), QByteArray::fromHex("ffffffff0f"));
        QCOMPARE(visible.bitGroup, raw.bitGroup); QCOMPARE(visible.fpgaTemp, raw.fpgaTemp);
        QCOMPARE(visible.yaw, raw.yaw); QCOMPARE(raw.subArrayPower[0], static_cast<unsigned char>(0));
        normal.fill(false); // UI confirms fault at its existing 100-frame threshold.
        visible = ElectromagneticProtocol::withConfirmedSubArrayPower(raw, normal, true);
        QCOMPARE(ElectromagneticProtocol::panel(visible).mid(8, 5), QByteArray(5, char(0)));
        normal[0] = normal[7] = normal[8] = normal[35] = true;
        raw.subArrayPower[4] = 0xf0;
        visible = ElectromagneticProtocol::withConfirmedSubArrayPower(raw, normal, true);
        QCOMPARE(ElectromagneticProtocol::panel(visible).mid(8, 5), QByteArray::fromHex("81010000f8"));
        visible = ElectromagneticProtocol::withConfirmedSubArrayPower(raw, normal, false);
        QCOMPARE(ElectromagneticProtocol::panel(visible).mid(8, 5), QByteArray::fromHex("00000000f0"));
    }
};
QTEST_APPLESS_MAIN(ElectromagneticProtocolTest)
#include "electromagneticprotocol_test.moc"
