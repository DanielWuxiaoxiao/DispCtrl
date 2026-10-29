#include "Basic/ConfigManager.h"
#include <QTemporaryDir>
#include <QTest>

class RadarReferenceConfigTest final : public QObject {
    Q_OBJECT
private slots:
    void wideBeamAzimuthConfigDoesNotTruncate() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("config.toml");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[params.beamcontrol]\naziStart = -36000\naziEnd = 36000\n");
        file.close();
        auto& config = ConfigManager::instance();
        QVERIFY(config.load(path));
        QCOMPARE(config.beamAziStart(), -36000);
        QCOMPARE(config.beamAziEnd(), 36000);
    }

    void savesFiveFieldsAndPreservesOtherContent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("config.toml");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray original = "# keep header\r\n[radar]\r\nlongitude = 109 # keep comment\r\n"
            "latitude = 34\r\naltitude = 400\r\nroll = 3\r\n[other]\r\nvalue = 7 # untouched\r\n";
        QCOMPARE(file.write(original), qint64(original.size()));
        file.close();
        auto& config = ConfigManager::instance();
        QVERIFY(config.load(path));
        QVERIFY(config.saveRadarReference(109.123456789, 34.987654321, -1000, 350, -15));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray saved = file.readAll();
        file.close();
        QVERIFY(saved.contains("# keep header\r\n"));
        QVERIFY(saved.contains("# keep comment\r\n"));
        QVERIFY(saved.contains("roll = 3\r\n"));
        QVERIFY(saved.contains("[other]\r\nvalue = 7 # untouched\r\n"));
        QVERIFY(config.load(path));
        QCOMPARE(config.longitude(), 109.123456789);
        QCOMPARE(config.latitude(), 34.987654321);
        QCOMPARE(config.altitude(), -1000.0);
        QCOMPARE(config.azimuth(), 350.0);
        QCOMPARE(config.pitch(), -15.0);
        QCOMPARE(config.roll(), 3.0);
        QVERIFY(config.saveRadarReference(109, 34, 400, 10, 15, -2.5));
        QVERIFY(config.load(path));
        QCOMPARE(config.roll(), -2.5);
    }

    void addsMissingSectionAndRejectsInvalidValues() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("config.toml");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[other]\nvalue = 7\n");
        file.close();
        auto& config = ConfigManager::instance();
        QVERIFY(config.load(path));
        QVERIFY(config.saveRadarReference(0, 0, 5000, 360, 90));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray valid = file.readAll();
        file.close();
        QVERIFY(!config.saveRadarReference(0, 0, 5000.01, 0, 0));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), valid);
        file.close();
        QCOMPARE(config.altitude(), 5000.0);
        QVERIFY(file.remove());
        QVERIFY(!config.saveRadarReference(10, 20, 30, 40, 50));
        QCOMPARE(config.altitude(), 5000.0);
    }
};

QTEST_GUILESS_MAIN(RadarReferenceConfigTest)
#include "radarreference_config_test.moc"
