#include "Basic/ConfigManager.h"
#include "Controller/subsystemnetworkmonitor.h"
#include <QTemporaryFile>
#include <QTest>

class SubsystemFeatureFlagsTest : public QObject {
    Q_OBJECT
private slots:
    void disabledLaserHasNoHealthEndpoints_data() {
        QTest::addColumn<bool>("healthEnabled");
        QTest::newRow("network-check-off") << false;
        QTest::newRow("network-check-on") << true;
    }
    void disabledLaserHasNoHealthEndpoints() {
        QFETCH(bool, healthEnabled);
        QTemporaryFile config;
        QVERIFY(config.open());
        const QByteArray settings = QByteArray("[laser]\nenabled=false\n[health_network]\nenabled=")
            + (healthEnabled ? "true" : "false")
            + "\nradar_local_ip=\"invalid-address\"\nradar_peer_ip=\"\"\n";
        QCOMPARE(config.write(settings), qint64(settings.size()));
        QVERIFY(config.flush());
        QVERIFY(CF_INS.load(config.fileName()));
        QVERIFY(!CF_INS.laserReportEnabled(true));
        SubsystemNetworkMonitor monitor;
        QList<int> endpoints;
        connect(&monitor, &SubsystemNetworkMonitor::statusChanged, this,
                [&](int index, bool, const QString&) { endpoints.append(index); });
        monitor.start(); // Invalid radar IP prevents ping; test starts no network requests.
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::RadarLocal));
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::RadarPeer));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserPeer));
    }
};
QTEST_GUILESS_MAIN(SubsystemFeatureFlagsTest)
#include "subsystemfeatureflags_test.moc"
