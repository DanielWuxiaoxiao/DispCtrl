#include "Basic/radarreference.h"
#include <QTest>
#include <limits>

class RadarReferenceTest final : public QObject {
    Q_OBJECT
private slots:
    void independentPrioritiesAndZero() {
        RadarReference reference({108, 34, 400, 15, 5});
        QCOMPARE(reference.value(RadarReference::Altitude), 400.0);
        QCOMPARE(int(reference.source(RadarReference::Latitude)), int(RadarReference::Source::Boot));
        reference.updateReport(RadarReference::Longitude, 110);
        reference.updateReport(RadarReference::Latitude, 35);
        RadarReference::Values manual{};
        manual[RadarReference::Longitude] = 0;
        manual[RadarReference::Yaw] = 0;
        QVERIFY(reference.setManual(manual));
        QVERIFY(reference.updateReport(RadarReference::Longitude, 120));
        reference.updateReport(RadarReference::Latitude, 36);
        QVERIFY(!reference.updateReport(RadarReference::Yaw, 70));
        QCOMPARE(reference.value(RadarReference::Longitude), 120.0);
        QCOMPARE(reference.value(RadarReference::Latitude), 36.0);
        QCOMPARE(reference.value(RadarReference::Altitude), 400.0);
        QCOMPARE(reference.value(RadarReference::Yaw), 0.0);
        QVERIFY(reference.setManual({}));
        QCOMPARE(reference.value(RadarReference::Longitude), 120.0);
        QCOMPARE(reference.value(RadarReference::Yaw), 70.0);
        QCOMPARE(reference.value(RadarReference::Pitch), 5.0);
    }

    void latestPositionAndStickyAttitude() {
        RadarReference reference({108, 34, 400, 15, 5, 2});
        RadarReference::Values manual{};
        manual[RadarReference::Longitude] = 109;
        manual[RadarReference::Yaw] = 350;
        manual[RadarReference::Pitch] = -15;
        manual[RadarReference::Roll] = -2.5;
        QVERIFY(reference.setManual(manual));
        QVERIFY(reference.updateReport(RadarReference::Longitude, 110));
        QVERIFY(!reference.updateReport(RadarReference::Yaw, 10));
        QVERIFY(!reference.updateReport(RadarReference::Roll, 20));
        QCOMPARE(reference.value(RadarReference::Yaw), 350.0);
        QCOMPARE(reference.value(RadarReference::Roll), -2.5);
        QCOMPARE(reference.value(RadarReference::Longitude), 110.0);
        manual[RadarReference::Longitude] = 111;
        QVERIFY(reference.setManual(manual));
        QCOMPARE(reference.value(RadarReference::Longitude), 111.0);
        QVERIFY(!reference.updateReport(RadarReference::Longitude, 181));
        QCOMPARE(reference.value(RadarReference::Longitude), 111.0);
        QVERIFY(reference.updateReport(RadarReference::Longitude, 112));
        QCOMPARE(reference.value(RadarReference::Longitude), 112.0);
        QCOMPARE(reference.value(RadarReference::Pitch), -15.0);
        QVERIFY(!RadarReference::validUi(RadarReference::Roll, 90.01));
    }

    void invalidReportAndAtomicManualValidation() {
        RadarReference reference({108, 34, 400, 15, 5});
        QVERIFY(!reference.updateReport(RadarReference::Latitude, 91));
        QVERIFY(!reference.updateReport(RadarReference::Longitude, std::numeric_limits<double>::quiet_NaN()));
        QCOMPARE(reference.value(RadarReference::Latitude), 34.0);
        RadarReference::Values manual{};
        manual[RadarReference::Latitude] = 30;
        manual[RadarReference::Pitch] = -10;
        QVERIFY(reference.setManual(manual));
        manual[RadarReference::Longitude] = 181;
        QVERIFY(!reference.setManual(manual));
        QCOMPARE(reference.value(RadarReference::Longitude), 108.0);
        QCOMPARE(reference.value(RadarReference::Pitch), -10.0);
        manual = reference.manual();
        manual[RadarReference::Latitude].reset();
        QVERIFY(reference.setManual(manual));
        QCOMPARE(reference.value(RadarReference::Latitude), 34.0);
        QCOMPARE(reference.value(RadarReference::Pitch), -10.0);
    }

    void fixedPanelOrderAndWrapping() {
        QCOMPARE(RadarReference::panelHeading(350, 0), 350.0);
        QCOMPARE(RadarReference::panelHeading(350, 1), 80.0);
        QCOMPARE(RadarReference::panelHeading(350, 2), 170.0);
        QCOMPARE(RadarReference::panelHeading(350, 3), 260.0);
        QCOMPARE(RadarReference::panelHeading(360, 0), 0.0);
    }

    void uiHeightLimits() {
        QVERIFY(RadarReference::validUi(RadarReference::Altitude, -1000));
        QVERIFY(RadarReference::validUi(RadarReference::Altitude, 5000));
        QVERIFY(!RadarReference::validUi(RadarReference::Altitude, -1000.01));
        QVERIFY(!RadarReference::validUi(RadarReference::Altitude, 5000.01));
        QVERIFY(!RadarReference::validUi(RadarReference::Altitude, std::numeric_limits<double>::infinity()));
        // The narrower UI limit must not change the existing device-report acceptance range.
        QVERIFY(RadarReference::valid(RadarReference::Altitude, 6000));
    }
};

QTEST_GUILESS_MAIN(RadarReferenceTest)
#include "radarreference_test.moc"
