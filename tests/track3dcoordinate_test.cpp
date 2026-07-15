#include "PolarDisp/track3dcoordinate.h"

#include <QTest>

#include <cmath>
#include <limits>

class Track3DCoordinateTest : public QObject
{
    Q_OBJECT

private slots:
    void northAtZeroAzimuth();
    void eastAtNinetyDegrees();
    void normalizesNegativeAzimuth();
    void projectsHighElevationToHorizontalPlane();
    void rejectsInvalidCoordinates();
};

void Track3DCoordinateTest::northAtZeroAzimuth()
{
    PointInfo info{};
    info.range = 100.0f;
    info.azimuth = 0.0f;
    info.elevation = 0.0f;
    info.altitute = 25.0f;

    Track3DCoordinate coordinate;
    QVERIFY(calculateTrack3DCoordinate(info, coordinate));
    QVERIFY(std::abs(coordinate.eastM) < 1e-9);
    QVERIFY(std::abs(coordinate.northM - 100.0) < 1e-9);
    QCOMPARE(coordinate.heightM, 25.0);
}

void Track3DCoordinateTest::eastAtNinetyDegrees()
{
    PointInfo info{};
    info.range = 200.0f;
    info.azimuth = 90.0f;
    info.elevation = 0.0f;

    Track3DCoordinate coordinate;
    QVERIFY(calculateTrack3DCoordinate(info, coordinate));
    QVERIFY(std::abs(coordinate.eastM - 200.0) < 1e-9);
    QVERIFY(std::abs(coordinate.northM) < 1e-9);
}

void Track3DCoordinateTest::normalizesNegativeAzimuth()
{
    PointInfo info{};
    info.range = 100.0f;
    info.azimuth = -90.0f;
    info.elevation = 0.0f;

    Track3DCoordinate coordinate;
    QVERIFY(calculateTrack3DCoordinate(info, coordinate));
    QVERIFY(std::abs(coordinate.eastM + 100.0) < 1e-9);
    QVERIFY(std::abs(coordinate.northM) < 1e-9);
    QCOMPARE(coordinate.azimuthDeg, 270.0);
}

void Track3DCoordinateTest::projectsHighElevationToHorizontalPlane()
{
    PointInfo info{};
    info.range = 100.0f;
    info.azimuth = 0.0f;
    info.elevation = 60.0f;

    Track3DCoordinate coordinate;
    QVERIFY(calculateTrack3DCoordinate(info, coordinate));
    QVERIFY(std::abs(coordinate.northM - 50.0) < 1e-6);
}

void Track3DCoordinateTest::rejectsInvalidCoordinates()
{
    PointInfo info{};
    Track3DCoordinate coordinate;

    info.range = -1.0f;
    QVERIFY(!calculateTrack3DCoordinate(info, coordinate));

    info.range = 100.0f;
    info.elevation = 91.0f;
    QVERIFY(!calculateTrack3DCoordinate(info, coordinate));

    info.elevation = 0.0f;
    info.altitute = std::numeric_limits<float>::quiet_NaN();
    QVERIFY(!calculateTrack3DCoordinate(info, coordinate));
}

QTEST_APPLESS_MAIN(Track3DCoordinateTest)

#include "track3dcoordinate_test.moc"
