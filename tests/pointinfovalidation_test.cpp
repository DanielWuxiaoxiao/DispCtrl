#include "Basic/pointinfovalidation.h"

#include <QTest>

#include <cmath>
#include <limits>

class PointInfoValidationTest : public QObject
{
    Q_OBJECT

private slots:
    void acceptsPhysicalBoundaryValues();
    void rejectsOutOfRangeGeometry();
    void rejectsNonFiniteFields();
    void preservesFiniteAncillaryValues();
    void acceptsValidPointsAfterAnInvalidPoint();
};

void PointInfoValidationTest::acceptsPhysicalBoundaryValues()
{
    PointInfo info{};
    info.type = PointType::Detection;
    QVERIFY(isValidDetectionPointInfo(info));

    info.range = 1000000.0f;
    info.azimuth = std::nextafter(360.0f, 0.0f);
    info.elevation = -90.0f;
    info.altitute = -500.0f;
    QVERIFY(isValidDetectionPointInfo(info));

    info.elevation = 90.0f;
    info.altitute = 2000000.0f;
    QVERIFY(isValidDetectionPointInfo(info));
}

void PointInfoValidationTest::rejectsOutOfRangeGeometry()
{
    using Field = float PointInfo::*;
    const Field fields[] = {
        &PointInfo::range, &PointInfo::range, &PointInfo::range,
        &PointInfo::azimuth, &PointInfo::azimuth,
        &PointInfo::elevation, &PointInfo::elevation
    };
    const float values[] = {
        -1.0f, -2.77759e+35f, std::nextafter(1000000.0f, 2000000.0f),
        -0.01f, 360.0f,
        std::nextafter(-90.0f, -100.0f), std::nextafter(90.0f, 100.0f)
    };

    for (int index = 0; index < static_cast<int>(sizeof(fields) / sizeof(fields[0])); ++index) {
        PointInfo info{};
        info.*fields[index] = values[index];
        QVERIFY(!isValidDetectionPointInfo(info));
    }
}

void PointInfoValidationTest::rejectsNonFiniteFields()
{
    using Field = float PointInfo::*;
    const Field fields[] = {
        &PointInfo::range, &PointInfo::azimuth, &PointInfo::elevation,
        &PointInfo::altitute, &PointInfo::SNR, &PointInfo::speed,
        &PointInfo::amp, &PointInfo::targetConfidence
    };
    const float invalidValues[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };

    for (Field field : fields) {
        for (float invalidValue : invalidValues) {
            PointInfo info{};
            info.*field = invalidValue;
            QVERIFY(!isValidDetectionPointInfo(info));
        }
    }
}

void PointInfoValidationTest::preservesFiniteAncillaryValues()
{
    PointInfo info{};
    info.range = 2500.0f;
    info.azimuth = 315.0f;
    info.elevation = -15.0f;
    info.altitute = -100.0f;
    info.SNR = -20.0f;
    info.speed = -300.0f;
    info.amp = -50.0f;
    info.targetConfidence = 2.0f;
    const PointInfo original = info;

    QVERIFY(isValidDetectionPointInfo(info));
    QCOMPARE(info.azimuth, original.azimuth);
    QCOMPARE(info.elevation, original.elevation);
    QCOMPARE(info.altitute, original.altitute);
    QCOMPARE(info.targetConfidence, original.targetConfidence);
}

void PointInfoValidationTest::acceptsValidPointsAfterAnInvalidPoint()
{
    PointInfo points[3]{};
    points[0].range = 100.0f;
    points[1].range = -2.77759e+35f;
    points[2].range = 300.0f;
    int accepted = 0;
    float lastAcceptedRange = 0.0f;
    for (const PointInfo& info : points) {
        if (!isValidDetectionPointInfo(info)) {
            continue;
        }
        ++accepted;
        lastAcceptedRange = info.range;
    }
    QCOMPARE(accepted, 2);
    QCOMPARE(lastAcceptedRange, 300.0f);
}

QTEST_APPLESS_MAIN(PointInfoValidationTest)

#include "pointinfovalidation_test.moc"
