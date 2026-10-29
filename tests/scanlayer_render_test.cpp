#include <QImage>
#include <QPainter>
#include <QTest>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <cmath>
#include "PolarDisp/scanmarkeroverlay.h"
#include "PolarDisp/scanlayer.h"
#include "PolarDisp/polaraxis.h"

class ScanLayerRenderTest final : public QObject {
    Q_OBJECT
    static QImage render(ScanLayer& layer) {
        QImage result(440, 440, QImage::Format_ARGB32_Premultiplied);
        result.fill(Qt::black);
        QPainter painter(&result);
        painter.translate(220, 220);
        layer.paint(&painter, nullptr, nullptr);
        return result;
    }
    static bool red(const QColor& color) {
        return color.red() > 200 && color.green() < 40 && color.blue() < 40;
    }
private slots:
    void fixedMultiPanelCoverageAndIndependentTasBoundaries() {
        PolarAxis axis; axis.setRange(0, 180); axis.setPixelsPerMeter(1);
        ScanLayer layer(&axis); layer.setMultiPanelMode(true);
        layer.setReferenceYaw(false, 30); // automatic/reference yaw also defines ID0's centre
        BatteryControlM enabled;
        enabled.quadrant1 = enabled.quadrant3 = 1;
        enabled.quadrant2 = enabled.quadrant4 = 0;
        layer.setPanelEnableState(enabled);
        const auto point = [](double angle, double radius = 90) {
            const double rad = angle * 3.14159265358979323846 / 180.0;
            return QPointF(radius * std::sin(rad), -radius * std::cos(rad));
        };
        const auto coverage = layer.coveragePath();
        for (double angle : {0., 30., 60., 180., 210., 240.}) QVERIFY(coverage.contains(point(angle)));
        for (double angle : {90., 120., 270., 330.}) QVERIFY(!coverage.contains(point(angle)));
        QVERIFY(layer.tasBoundaryLines()[0].isNull()); // no submitted TAS yet
        layer.setSweepRange(-45, 45);
        QCOMPARE(layer.coveragePath(), coverage);
        const auto boundaries = layer.tasBoundaryLines();
        QVERIFY(QLineF(boundaries[0].p2(), point(-45, 180)).length() < 1e-6);
        QVERIFY(QLineF(boundaries[1].p2(), point(45, 180)).length() < 1e-6);
        layer.setSweepRange(350, 10); // cross-north TAS does not rotate/shrink coverage
        QCOMPARE(layer.coveragePath(), coverage);
        QVERIFY(QLineF(layer.tasBoundaryLines()[0].p2(), point(350, 180)).length() < 1e-6);
        layer.setReferenceYaw(true, 350);
        QVERIFY(layer.coveragePath().contains(point(350)));
        QVERIFY(!layer.coveragePath().contains(point(80)));
        layer.setMultiPanelMode(false);
        QVERIFY(layer.tasBoundaryLines()[0].isNull()); // preserve legacy single-panel presentation
    }
    void outerTickOnlyAndUniformPanels() {
        PolarAxis axis;
        axis.setRange(0, 180);
        axis.setPixelsPerMeter(1);
        ScanLayer layer(&axis);
        layer.setSweepRange(-30, 30);
        BatteryControlM enabled;
        enabled.quadrant1 = enabled.quadrant2 = enabled.quadrant3 = enabled.quadrant4 = 1;
        layer.setPanelEnableState(enabled);
        for (int id = 0; id < 4; ++id) {
            BITReport report;
            report.radarId = static_cast<unsigned char>(id);
            report.yaw = 4500; // Must not rotate the absolute BIT scan angle.
            report.scanAngle = static_cast<unsigned short>(id * 9000);
            layer.onBITReport(report);
        }
        const QImage image = render(layer);
        QVERIFY(red(image.pixelColor(220, 42)));
        QVERIFY(red(image.pixelColor(398, 220)));
        QVERIFY(red(image.pixelColor(220, 398)));
        QVERIFY(red(image.pixelColor(42, 220)));
        QCOMPARE(image.pixelColor(220, 130), image.pixelColor(310, 220));
        QCOMPARE(image.pixelColor(220, 130), image.pixelColor(220, 310));
        QCOMPARE(image.pixelColor(220, 130), image.pixelColor(130, 220));
        for (int y = 65; y < 375; ++y) {
            for (int x = 65; x < 375; ++x) {
                if (std::hypot(x - 220.0, y - 220.0) < 138)
                    QVERIFY(!red(image.pixelColor(x, y)));
            }
        }
        enabled.quadrant2 = enabled.quadrant3 = enabled.quadrant4 = 0;
        layer.setPanelEnableState(enabled);
        const QImage single = render(layer);
        QVERIFY(red(single.pixelColor(220, 42)));
        QVERIFY(single.pixelColor(220, 42).red() > single.pixelColor(225, 42).red()); // ends fade
        QVERIFY(!red(single.pixelColor(230, 42))); // Outside +/-2.5 degrees.
        QVERIFY(!red(single.pixelColor(220, 49))); // No old 36px radial block.
        QVERIFY(!red(single.pixelColor(220, 80))); // Inside the inner radius, not filled.
        QVERIFY(!red(single.pixelColor(398, 220)));
        QCOMPARE(single.pixelColor(310, 220), QColor(Qt::black));
        QCOMPARE(single.pixelColor(220, 130), image.pixelColor(220, 130));
    }
    void markersAboveSiblingWidgets() {
        PolarAxis axis;
        axis.setRange(0, 180);
        axis.setPixelsPerMeter(1);
        QGraphicsScene scene;
        scene.setSceneRect(-220, -220, 440, 440);
        auto* layer = new ScanLayer(&axis);
        scene.addItem(layer);
        BatteryControlM enabled;
        enabled.quadrant1 = 1;
        enabled.quadrant2 = enabled.quadrant3 = enabled.quadrant4 = 0;
        layer->setPanelEnableState(enabled);
        BITReport report;
        report.radarId = 0; report.scanAngle = 0;
        layer->onBITReport(report);
        QCOMPARE(layer->markerLines()[0].length(), ScanLayer::MarkerLength);
        QGraphicsView view(&scene);
        view.resize(480, 480);
        QWidget coveringControl(&view);
        coveringControl.setGeometry(view.rect());
        coveringControl.setStyleSheet("background:blue");
        coveringControl.show();
        ScanMarkerOverlay overlay(&view);
        overlay.setSource(layer);
        QVERIFY(overlay.testAttribute(Qt::WA_TransparentForMouseEvents));
        view.show();
        QTest::qWait(40);
        const QPoint marker = view.mapFromScene(QPointF(0, -177.5)) + view.viewport()->pos();
        const QImage image = view.grab().toImage();
        QVERIFY(red(image.pixelColor(marker)));
        report.scanAngle = 9000;
        layer->onBITReport(report);
        QTest::qWait(55);
        const QPoint next = view.mapFromScene(QPointF(177.5, 0)) + view.viewport()->pos();
        const QImage crossfade = view.grab().toImage();
        QVERIFY(crossfade.pixelColor(marker).red() > 20);
        QVERIFY(crossfade.pixelColor(next).red() > 20);
        QTest::qWait(150);
        const QImage finished = view.grab().toImage();
        QVERIFY(finished.pixelColor(marker).red() < 20);
        QVERIFY(red(finished.pixelColor(next)));
    }
};
QTEST_MAIN(ScanLayerRenderTest)
#include "scanlayer_render_test.moc"
