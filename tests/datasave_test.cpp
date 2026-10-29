#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QtEndian>
#include <cstdlib>
#include "paramWidget/datasaveui.h"

class DataSaveTest final : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    QString previous;
private slots:
    void init() {
        QVERIFY(directory.isValid()); previous = QDir::currentPath();
        QVERIFY(QDir::setCurrent(directory.path())); QFile::remove("data.txt");
    }
    void cleanup() { QDir::setCurrent(previous); }
    void wireLayoutAndEnvelope() {
        DataSave data;
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(&data), sizeof(data)).toHex(),
                 QByteArray("01cc0000000100010000000000000000"));
        data.saveSwitch = 1; data.dataID = 255;
        data.confirmBeam = 1; data.trackBeam = 0; data.searchBeam = 1; data.detection = 0;
        char* frame = packData(reinterpret_cast<char*>(&data), sizeof(data), 1, 2, 3);
        QVERIFY(frame);
        const QByteArray bytes(frame, sizeof(ProtocolFrame) + sizeof(data) + sizeof(ProtocolEnd));
        std::free(frame);
        QCOMPARE(bytes.size(), 35);
        QCOMPARE(bytes.mid(sizeof(ProtocolFrame), 16).toHex(), QByteArray("01cc01ff010001000000000000000000"));
        QCOMPARE(quint8(bytes[30]), calculateXOR(bytes.constData(), 30));
    }
    void checkboxDefaultsAndStartStopSignals() {
        DataSaveUI dialog;
        auto* confirm = dialog.findChild<QCheckBox*>("confirmBeamCheck");
        auto* track = dialog.findChild<QCheckBox*>("trackBeamCheck");
        auto* search = dialog.findChild<QCheckBox*>("searchBeamCheck");
        auto* detection = dialog.findChild<QCheckBox*>("detectionCheck");
        QVERIFY(confirm && track && search && detection);
        QVERIFY(!confirm->isChecked()); QVERIFY(track->isChecked());
        QVERIFY(!search->isChecked()); QVERIFY(detection->isChecked());
        auto* id = dialog.findChild<QLineEdit*>("saveID"); QVERIFY(id); id->setText("255");
        DataSet sent; int count = 0;
        connect(&dialog, &DataSaveUI::setParam, &dialog, [&](DataSet value) { sent = value; ++count; });
        dialog.stopSave(); QCOMPARE(count, 0); // no row selected must not send an accidental ID0 stop
        dialog.startSave(); QCOMPARE(count, 1);
        QVERIFY(sent.ifsave); QCOMPARE(sent.save.dataID, quint8(255)); QCOMPARE(sent.save.saveSwitch, quint8(1));
        QCOMPARE(sent.save.confirmBeam, quint8(0)); QCOMPARE(sent.save.trackBeam, quint8(1));
        QCOMPARE(sent.save.searchBeam, quint8(0)); QCOMPARE(sent.save.detection, quint8(1));
        confirm->setChecked(true); track->setChecked(false); search->setChecked(true); detection->setChecked(false);
        auto* table = dialog.findChild<QTableWidget*>(); QVERIFY(table); table->setCurrentCell(0, 0);
        dialog.stopSave(); QCOMPARE(count, 2); QCOMPARE(sent.save.saveSwitch, quint8(0));
        QCOMPARE(sent.save.dataID, quint8(255));
        QCOMPARE(sent.save.confirmBeam, quint8(1)); QCOMPARE(sent.save.trackBeam, quint8(0));
        QCOMPARE(sent.save.searchBeam, quint8(1)); QCOMPARE(sent.save.detection, quint8(0));
        for (auto byte : sent.save.reserve) QCOMPARE(byte, quint8(0));
    }
};
QTEST_MAIN(DataSaveTest)
#include "datasave_test.moc"
