#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/targetrecognition.h"
#include "PointManager/detmanager.h"
#include "PointManager/trackmanager.h"
#include "PointManager/sectordetmanager.h"
#include "PolarDisp/rangeazimuthchart.h"
#include "PolarDisp/rangeheightchart.h"

#include <QFile>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>

class DetectionColorRenderTest : public QObject
{
    Q_OBJECT

    static int colorCount(QGraphicsScene& scene, const QColor& color)
    {
        QImage image(800, 600, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        QPainter painter(&image);
        scene.render(&painter);
        painter.end();

        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) == color) ++count;
            }
        }
        return count;
    }

private slots:
    void recognitionLabelsAndWireSizes() {
        const QStringList names{"未知", "无人机", "行人", "车辆", "鸟", "其它"};
        for (unsigned i = 0; i < 6; ++i) {
            PointInfo point{}; point.targetRecResult = i;
            QCOMPARE(TargetRecognition::text(i), names[int(i)]);
            QCOMPARE(TargetRecognition::text(point), names[int(i)]);
        }
        QCOMPARE(TargetRecognition::text(65537u), QStringLiteral("未知"));
        PointInfo offline{}; offline.statMethod = OfflineRae::kStatMethod;
        offline.targetRecResult = OfflineRae::kColorRed;
        QCOMPARE(TargetRecognition::text(offline), QStringLiteral("其它"));
        QCOMPARE(sizeof(detInfo), size_t(56)); QCOMPARE(sizeof(trackInfo), size_t(61));
    }
    void detectionColor_data()
    {
        QTest::addColumn<int>("configValue");
        QTest::addColumn<unsigned>("recognition");
        QTest::addColumn<QString>("theme");
        for (const QString& theme : {QStringLiteral("graphite"), QStringLiteral("classic")}) {
            for (int flag : {0, 1}) {
                for (unsigned result : {0u, 1u, 2u, 3u, 4u, 5u, 65537u}) {
                    const QByteArray name = theme.toUtf8() + '-' + QByteArray::number(flag)
                        + '-' + QByteArray::number(result);
                    QTest::newRow(name.constData()) << flag << result << theme;
                }
            }
        }
    }

    void detectionColor()
    {
        QFETCH(int, configValue);
        QFETCH(unsigned, recognition);
        QFETCH(QString, theme);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile config(fixture.filePath("config.toml"));
        QVERIFY(config.open(QIODevice::WriteOnly));
        QByteArray content = "[ui]\ntheme = \"" + theme.toUtf8() + "\"\n[displayConfig]\n";
        content += QByteArray("detection_drone_red_enabled = ")
            + (configValue == 1 ? "true\n" : "false\n");
        QCOMPARE(config.write(content), qint64(content.size()));
        config.close();
        QVERIFY(CF_INS.load(config.fileName()));
        UiTheme::configure(CF_INS.uiTheme());

        PointInfo info{};
        info.type = PointType::Detection;
        info.range = 2000;
        info.azimuth = 0;
        info.altitute = 200;
        info.targetRecResult = recognition;
        const bool highlighted = configValue == 1 && recognition == 1;
        const QColor expected = highlighted ? TRA_COLOR : DET_COLOR;
        const QColor unexpected = highlighted ? DET_COLOR : TRA_COLOR;

        QGraphicsScene ppiScene;
        ppiScene.setSceneRect(-2500, -2500, 5000, 5000);
        PolarAxis ppiAxis;
        ppiAxis.setRange(0, 5000);
        DetManager ppi(&ppiScene, &ppiAxis);
        ppi.setPointSizeRatio(10);
        ppi.addDetPoint(info);
        QVERIFY(colorCount(ppiScene, expected) > 0);
        QCOMPARE(colorCount(ppiScene, unexpected), 0);
        ppi.setAllVisible(false);
        QCOMPARE(colorCount(ppiScene, expected), 0);

        QGraphicsScene trackScene;
        trackScene.setSceneRect(-2500,-2500,5000,5000);
        TrackManager tracks(&trackScene, &ppiAxis);
        tracks.setPointSizeRatio(10);
        auto trackInfo = info; trackInfo.type = PointType::Track; trackInfo.batch = 42;
        tracks.addTrackPoint(trackInfo);
        const QColor trackExpected = recognition == 1 ? TRA_COLOR : DRONE_COLOR;
        const QColor trackUnexpected = recognition == 1 ? DRONE_COLOR : TRA_COLOR;
        QVERIFY(colorCount(trackScene, trackExpected) > 0);
        QCOMPARE(colorCount(trackScene, trackUnexpected), 0);

        QGraphicsScene sectorScene;
        sectorScene.setSceneRect(-2500, -5000, 5000, 5000);
        PolarAxis sectorAxis;
        sectorAxis.setRange(0, 5000);
        SectorDetManager sector(&sectorScene, &sectorAxis);
        sector.setPointSizeRatio(50);
        sector.addDetPoint(info);
        QVERIFY(colorCount(sectorScene, expected) > 0);
        QCOMPARE(colorCount(sectorScene, unexpected), 0);

        RangeAzimuthChart azimuth;
        azimuth.resize(800, 600);
        azimuth.setDetectionSizeRatio(3);
        info.azimuth = 180;
        azimuth.addPointInfo(info);
        QVERIFY(colorCount(*azimuth.scene(), expected) > 0);
        QCOMPARE(colorCount(*azimuth.scene(), unexpected), 0);
        azimuth.setDetectionVisible(false);
        QCOMPARE(colorCount(*azimuth.scene(), expected), 0);

        RangeHeightChart height;
        height.resize(800, 600);
        height.setDetectionSizeRatio(3);
        height.addPointInfo(info);
        QVERIFY(colorCount(*height.scene(), expected) > 0);
        QCOMPARE(colorCount(*height.scene(), unexpected), 0);
        height.clearRadarData();
        QCOMPARE(colorCount(*height.scene(), expected), 0);
    }
};

QTEST_MAIN(DetectionColorRenderTest)
#include "detectioncolor_render_test.moc"
