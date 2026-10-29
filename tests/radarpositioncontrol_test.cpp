#include <QTest>
#include <QtEndian>
#include <limits>
#include "Basic/gpspositionfilter.h"
#include "Basic/radarpositioncontrol.h"

class RadarPositionControlTest final : public QObject {
    Q_OBJECT
private slots:
    void wireLayoutAndQuantization() {
        RadarReference reference({120, 30, 100, 350, -15, -2.5});
        RadarPositionControl command;
        QVERIFY(buildRadarPositionControl(reference, command));
        QCOMPARE(sizeof(command), std::size_t(142));
        const auto* bytes = reinterpret_cast<const uchar*>(&command);
        QCOMPARE(qFromLittleEndian<quint16>(bytes), quint16(0xAA08));
        const quint32 headings[] = {350000, 80000, 170000, 260000};
        for (int i = 0; i < 4; ++i) {
            const auto* record = bytes + 2 + 31 * i;
            QCOMPARE(record[0], uchar(i));
            QCOMPARE(qFromLittleEndian<qint64>(record + 1), qint64(30000000000LL));
            QCOMPARE(qFromLittleEndian<qint64>(record + 9), qint64(120000000000LL));
            QCOMPARE(qFromLittleEndian<qint32>(record + 17), qint32(10000));
            QCOMPARE(qFromLittleEndian<quint32>(record + 21), headings[i]);
            QCOMPARE(qFromLittleEndian<qint32>(record + 25), qint32(-15000));
            QCOMPARE(qFromLittleEndian<qint16>(record + 29), qint16(-250));
        }
        for (int i = 126; i < 142; ++i) QCOMPARE(bytes[i], uchar(0));
    }

    void signedBoundaryAndHeadingWrap() {
        RadarReference reference({-180, -90, -1000, 360, -90, 90});
        RadarPositionControl command;
        QVERIFY(buildRadarPositionControl(reference, command));
        QCOMPARE(command.panels[0].latitudeE9, qint64(-90000000000LL));
        QCOMPARE(command.panels[0].longitudeE9, qint64(-180000000000LL));
        QCOMPARE(command.panels[0].altitudeCm, qint32(-100000));
        QCOMPARE(command.panels[0].headingMilliDeg, quint32(0));
        QCOMPARE(command.panels[3].headingMilliDeg, quint32(270000));
        QCOMPARE(command.panels[0].pitchMilliDeg, qint32(-90000));
        QCOMPARE(command.panels[0].rollCentiDeg, qint16(9000));
    }

    void gpsRejectionAndFirstFix() {
        GpsPositionFilter filter;
        GpsPositionFilter::Fix mean{1, 2, 3};
        using R = GpsPositionFilter::Result;
        QCOMPARE(int(filter.push({0, 0, 0}, mean)), int(R::AllZero));
        QCOMPARE(int(filter.push({91, 0, 1}, mean)), int(R::Invalid));
        QCOMPARE(int(filter.push({30, std::numeric_limits<double>::quiet_NaN(), 100}, mean)), int(R::Invalid));
        QCOMPARE(mean.latitude, 1.0);
        QCOMPARE(filter.count(), std::size_t(0));
        QCOMPARE(int(filter.push({30, 120, 0}, mean)), int(R::Accepted));
        QCOMPARE(int(filter.push({30.002, 120, 0}, mean)), int(R::Deviation));
        QCOMPARE(int(filter.push({30, 120, 51}, mean)), int(R::Deviation));
        QCOMPARE(filter.count(), std::size_t(1));
        QCOMPARE(mean.altitude, 0.0);
        filter.setLimits(500, 100);
        QCOMPARE(int(filter.push({30.002, 120, 51}, mean)), int(R::Accepted));
        QCOMPARE(mean.altitude, 25.5);
    }

    void gpsNewestHundredMeanAndDateline() {
        GpsPositionFilter filter;
        GpsPositionFilter::Fix mean;
        using R = GpsPositionFilter::Result;
        for (int i = 0; i < 101; ++i)
            QCOMPARE(int(filter.push({30, 120, 100 + i * 0.1}, mean)), int(R::Accepted));
        QCOMPARE(filter.count(), std::size_t(100));
        QVERIFY(std::abs(mean.altitude - 105.05) < 1e-10);
        GpsPositionFilter dateline;
        QCOMPARE(int(dateline.push({0, 179.9998, 100}, mean)), int(R::Accepted));
        QCOMPARE(int(dateline.push({0, -179.9998, 100}, mean)), int(R::Accepted));
        QVERIFY(std::abs(std::abs(mean.longitude) - 180) < 1e-8);
    }
};

QTEST_GUILESS_MAIN(RadarPositionControlTest)
#include "radarpositioncontrol_test.moc"
