#include "Basic/ConfigManager.h"
#include "Controller/subsystemnetworkmonitor.h"
#include <QTemporaryFile>
#include <QTest>

class SubsystemFeatureFlagsTest : public QObject {
    Q_OBJECT
private slots:
    void zhoushanHasIndependentExclusiveEndpoints() {
        QTemporaryFile config; QVERIFY(config.open());
        config.write("[zhoushan_optical]\nenabled=true\nlocal_ip=\"invalid-address\"\npeer_ip=\"\"\n"
                     "[photoelectric_v2]\nenabled=true\n[laser]\nenabled=true\n"
                     "[system]\nenable_photoelectric_tx=true\n[health_network]\nenabled=false\n");
        QVERIFY(config.flush()); QVERIFY(CF_INS.load(config.fileName()));
        QVERIFY(CF_INS.zhoushanOpticalEnabled()); QVERIFY(CF_INS.photoelectricV2ConfiguredEnabled());
        QVERIFY(!CF_INS.photoelectricV2Enabled()); QVERIFY(!CF_INS.laserReportEnabled()); QVERIFY(!CF_INS.photoelectricTxEnabled());
        SubsystemNetworkMonitor monitor; QList<int> endpoints;
        connect(&monitor, &SubsystemNetworkMonitor::statusChanged, this,
                [&](int index, bool, const QString&) { endpoints.append(index); });
        monitor.start();
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::ZhoushanLocal));
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::ZhoushanPeer));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::OpticalLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::OpticalPeer));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserPeer));
    }
    void opticalAndLaserAreExclusive() {
        QTemporaryFile config; QVERIFY(config.open());
        config.write("[zhoushan_optical]\nenabled=false\n[photoelectric_v2]\nenabled=true\nlocal_ip=\"invalid-address\"\npeer_ip=\"\"\n"
                     "[laser]\nenabled=true\n[system]\nenable_photoelectric_tx=true\n"
                     "[health_network]\nenabled=false\n");
        QVERIFY(config.flush()); QVERIFY(CF_INS.load(config.fileName()));
        QVERIFY(CF_INS.photoelectricV2Enabled());
        QVERIFY(CF_INS.laserConfiguredEnabled());
        QVERIFY(!CF_INS.laserReportEnabled());
        QVERIFY(!CF_INS.photoelectricTxEnabled());
        SubsystemNetworkMonitor monitor; QList<int> endpoints;
        connect(&monitor, &SubsystemNetworkMonitor::statusChanged, this,
                [&](int index, bool, const QString&) { endpoints.append(index); });
        monitor.start();
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::OpticalLocal));
        QVERIFY(endpoints.contains(SubsystemNetworkMonitor::OpticalPeer));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::LaserPeer));
    }
    void disabledLaserHasNoHealthEndpoints_data() {
        QTest::addColumn<bool>("healthEnabled");
        QTest::newRow("network-check-off") << false;
        QTest::newRow("network-check-on") << true;
    }
    void disabledLaserHasNoHealthEndpoints() {
        QFETCH(bool, healthEnabled);
        QTemporaryFile config;
        QVERIFY(config.open());
        const QByteArray settings = QByteArray("[zhoushan_optical]\nenabled=false\n[photoelectric_v2]\nenabled=false\n[laser]\nenabled=false\n[health_network]\nenabled=")
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
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::OpticalLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::OpticalPeer));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::ZhoushanLocal));
        QVERIFY(!endpoints.contains(SubsystemNetworkMonitor::ZhoushanPeer));
    }
};
QTEST_GUILESS_MAIN(SubsystemFeatureFlagsTest)
#include "subsystemfeatureflags_test.moc"
