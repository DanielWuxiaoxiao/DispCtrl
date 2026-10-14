#include "ElectromagneticDevice/devicemodule.h"
#include "Controller/testtrackgenerator.h"
#include <QTest>
#include <QtEndian>
#include <cstring>

class ElectromagneticReportingTest : public QObject {
    Q_OBJECT
private:
    static PointInfo point(unsigned batch = 42) {
        PointInfo p{}; p.type = PointType::Track; p.batch = batch;
        p.range = 1000; p.azimuth = 90; p.elevation = 1; p.speed = 15; p.SNR = 24;
        return p;
    }
    static void prepare(ElectromagneticDeviceModule& module, const PointInfo& p) {
        module.m_enabled = true;
        module.updateRadarPosition(30, 120, 100);
        trackInfo t{}; t.batch = quint16(p.batch); t.CPIID = 19; t.UTCtime = 1234;
        t.dis = p.range; t.azi = p.azimuth; t.ele = p.elevation; t.spaceVel = 20;
        module.m_trackMetadata.insert(t.batch, t);
    }
private slots:
    void manualAutomaticAndRemoval() {
        ElectromagneticDeviceModule module;
        auto p = point(); prepare(module, p);
        module.processTrackPoint(p);
        QVERIFY(module.m_pendingTracks.isEmpty());
        QVERIFY(module.toggleManualReport(p.batch));
        QCOMPARE(module.m_pendingTracks.size(), 1);
        const auto bytes = module.m_pendingTracks.value(42);
        QCOMPARE(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(bytes.constData()) + 2), quint16(19));
        QCOMPARE(qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()) + 4), quint32(1234));
        QVERIFY(module.toggleManualReport(p.batch));
        QVERIFY(module.m_pendingTracks.isEmpty());
        module.setAutoTracks(true); QCOMPARE(module.m_pendingTracks.size(), 1);
        module.setAutoTracks(false); QVERIFY(module.m_pendingTracks.isEmpty());
        QVERIFY(module.toggleManualReport(p.batch));
        p.statMethod = 2; module.processTrackPoint(p);
        QCOMPARE(quint8(module.m_pendingTracks.value(42)[12]), quint8(2));
        QVERIFY(!module.isManualReportActive(42));
        QVERIFY(module.m_trackPoints.isEmpty());
    }
    void rawFramesNeverSendAndCommonEntryCoalesces() {
        ElectromagneticDeviceModule module;
        auto p = point(); prepare(module, p); module.m_autoTracks = true;
        trackInfo t = module.m_trackMetadata.value(42);
        const QByteArray frame = ElectromagneticProtocol::encode(1, 2, 0, 0xEE01,
            QByteArray::fromHex("0100") + QByteArray(reinterpret_cast<const char*>(&t), sizeof(t)));
        module.processTrackFrame(frame);
        QVERIFY(module.m_pendingTracks.isEmpty());
        module.processTrackPoint(p); module.processTrackPoint(p);
        QCOMPARE(module.m_pendingTracks.size(), 1);
        p.type = PointType::TBDPointType; module.processTrackPoint(p);
        QCOMPARE(module.m_trackPoints.size(), 1);
        QVERIFY(!module.toggleManualReport(70000));
    }
    void detectionSelectionAndBoundedQueue() {
        ElectromagneticDeviceModule module;
        auto p = point(); prepare(module, p); p.type = PointType::Detection; p.batch = 0;
        detInfo d{}; d.dis = p.range; d.azi = p.azimuth; d.ele = p.elevation; d.vel = p.speed;
        d.statSNR = 12; d.disChannel = 17;
        module.m_detectionMetadata.insert(module.detectionKey(p), d);
        module.processDetectionPoint(p); QVERIFY(module.m_detections.isEmpty());
        QVERIFY(module.sendDetection(p)); QCOMPARE(module.m_manualDetections.size(), 1);
        auto bytes = module.m_manualDetections.head();
        QCOMPARE(qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()) + 40), quint32(17));
        module.setAutoDetections(true); module.processDetectionPoint(p);
        QCOMPARE(module.m_detections.size(), 1);
        for (int i = 0; i < 3000; ++i) module.processDetectionPoint(p);
        QCOMPARE(module.m_detections.size() + module.m_manualDetections.size(), 2048);
        module.setAutoDetections(false); QVERIFY(module.m_detections.isEmpty());
        QCOMPARE(module.m_manualDetections.size(), 1); // Auto-off must not cancel a manual point.
        module.m_originValid = false;
        QVERIFY(!module.sendDetection(p)); // Test fallback never applies to real detection.
    }
    void explicitTestProvenanceAndDd05Priority() {
        ElectromagneticDeviceModule module; module.m_enabled = true; module.m_autoTracks = true;
        module.setTestRadarPosition(30, 120, 100);
        auto p = point(101);
        module.processTrackPoint(p); QVERIFY(module.m_pendingTracks.isEmpty());
        module.prepareTestPoint(p); module.processTrackPoint(p);
        QCOMPARE(module.m_pendingTracks.size(), 1);
        auto bytes = module.m_pendingTracks.value(101);
        const auto* data = reinterpret_cast<const uchar*>(bytes.constData());
        QCOMPARE(qFromLittleEndian<quint16>(data + 2), quint16(0));
        QVERIFY(qFromLittleEndian<quint32>(data + 4) > 0);
        const qint32 testLatitude = qFromLittleEndian<qint32>(data + 57);
        module.updateRadarPosition(31, 121, 200);
        module.processTrackPoint(p); bytes = module.m_pendingTracks.value(101);
        QVERIFY(qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(bytes.constData()) + 57) > testLatitude);
        module.m_originValid = false;
        p.type = PointType::Detection; p.batch = 0;
        QVERIFY(!module.sendDetection(p));
        module.prepareTestPoint(p); QVERIFY(module.sendDetection(p));
        // The same physical sample arriving from DD01 revokes the synthetic marker.
        PointInfo real = p; real.batch = 0;
        module.m_testDetectionKeys.remove(module.detectionKey(real));
        QVERIFY(!module.sendDetection(real));
    }
    void independentDetectionGenerator() {
        TestDetectionGenerator generator;
        int count = 0; bool valid = true;
        connect(&generator, &TestDetectionGenerator::detectionGenerated, this, [&](PointInfo p) {
            ++count; valid = valid && p.type == PointType::Detection && p.batch == 0
                && p.range > 0 && p.speed == 15;
        });
        QVERIFY(generator.start()); QCOMPARE(count, 10); QVERIFY(valid);
        QVERIFY(generator.isRunning()); QVERIFY(generator.stop()); QVERIFY(!generator.isRunning());
        QVERIFY(generator.start()); QCOMPARE(count, 20); QVERIFY(generator.stop());
    }
    void categorizedLogThrottleAndFailureCounters() {
        ElectromagneticDeviceModule module;
        module.warn("origin missing", ElectromagneticDeviceModule::Warning::Origin);
        module.warn("origin missing", ElectromagneticDeviceModule::Warning::Origin);
        module.warn("queue full", ElectromagneticDeviceModule::Warning::Queue);
        QCOMPARE(module.m_warningSuppressed[size_t(ElectromagneticDeviceModule::Warning::Origin)], quint64(1));
        QCOMPARE(module.m_warningSuppressed[size_t(ElectromagneticDeviceModule::Warning::Queue)], quint64(0));
        const auto p = point(); module.m_enabled = true;
        module.processTrackPoint(p);
        QCOMPARE(module.m_trackInputs, quint64(1));
        QCOMPARE(module.m_rejected, quint64(1));
        QCOMPARE(module.rejectionReason(p), QStringLiteral("NO_VALID_ORIGIN"));
        module.updateRadarPosition(30, 120, 100);
        QCOMPARE(module.rejectionReason(p), QStringLiteral("TRACK_METADATA_MISSING"));
        module.flush(); QCOMPARE(module.m_flushNotReady, quint64(1));
    }
    void periodicDefaultAndLastRealStatus() {
        ElectromagneticDeviceModule module;
        module.m_enabled = true; module.m_ready = true; module.m_lastStatus = -1000;
        QList<QByteArray> packets;
        connect(&module, &ElectromagneticDeviceModule::outbound, this,
                [&](const QList<QByteArray>& batch) { packets = batch; });
        module.flush(); QCOMPARE(packets.size(), 2);
        ElectromagneticProtocol::Frame frame; QString error;
        QVERIFY(ElectromagneticProtocol::decode(packets[0], frame, error));
        QCOMPARE(frame.message, quint16(7)); QCOMPARE(frame.payload, QByteArray(15, char(0)));
        QVERIFY(ElectromagneticProtocol::decode(packets[1], frame, error));
        QCOMPARE(frame.message, quint16(9)); QCOMPARE(frame.payload, QByteArray(4, char(1)));
        packets.clear(); module.m_busy = false;
        module.flush(); QVERIFY(packets.isEmpty()); // Not due before one second.
        BITReport bit; bit.bitGroup = 0x9f; bit.fpgaTemp = 450;
        MonitorParam software; software.dataProSta = 2;
        module.processBitReport(bit); module.processSoftwareReport(software);
        module.m_receiveTimeout = 0; module.m_lastStatus = -1000;
        module.flush(); QCOMPARE(packets.size(), 2); // Stale real values still sent.
        QVERIFY(ElectromagneticProtocol::decode(packets[0], frame, error));
        QCOMPARE(frame.payload, ElectromagneticProtocol::panel(bit));
        QVERIFY(ElectromagneticProtocol::decode(packets[1], frame, error));
        QCOMPARE(frame.payload, ElectromagneticProtocol::software(software));
        module.m_busy = false; module.m_reportStatus = false; module.m_lastStatus = -1000;
        packets.clear(); module.flush(); QVERIFY(packets.isEmpty());
    }
    void softwareSeedAndUpdatesUseUiValues() {
        ElectromagneticDeviceModule module;
        QCOMPARE(module.m_software, QByteArray::fromHex("01010101"));
        MonitorParam ui; ui.dataProSta = 1; ui.beamConSta = 2; ui.sigProSta = 0; ui.targetRecSta = 3;
        module.setInitialSoftwareStatus(ui);
        QCOMPARE(module.m_software, QByteArray::fromHex("01020003"));
        QCOMPARE(module.m_lastSoftware, qint64(-1)); // Initial UI snapshot is not a real report.
        ui.dataProSta = 4; ui.beamConSta = 5;
        module.processSoftwareReport(ui);
        QCOMPARE(module.m_software, QByteArray::fromHex("01010003"));
        QVERIFY(module.m_lastSoftware >= 0);
    }
};
QTEST_GUILESS_MAIN(ElectromagneticReportingTest)
#include "electromagneticreporting_test.moc"
