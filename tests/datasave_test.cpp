#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QtEndian>
#include <cstdlib>
#include <QTimer>
#include <QDialogButtonBox>
#include <QPushButton>
#include "Basic/ConfigManager.h"
#include "paramWidget/datasaveui.h"

class DataSaveTest final : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    QString previous;
    bool loadMode(int panels) {
        QFile file(directory.filePath("config.toml"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        file.write("[displayConfig]\ndefault_panel_count = " + QByteArray::number(panels) + "\n");
        file.close();
        return CF_INS.load(file.fileName());
    }
private slots:
    void init() {
        QVERIFY(directory.isValid()); previous = QDir::currentPath();
        QVERIFY(QDir::setCurrent(directory.path())); QFile::remove("data.txt");
        QVERIFY(loadMode(1));
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
    void offlineWireLayoutAndModeLengths() {
        DataSet param;
        QCOMPARE(sizeof(param.off), std::size_t(8));
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(&param.off), 8).toHex(), QByteArray("03cc000000000000"));
        QCOMPARE(param.offlinePayloadSize(), unsigned(4));
        param.extendedOffline = true;
        param.off.onSwitch = 1; param.off.dataID = 7;
        param.off.replayPanel[0] = 1; param.off.replayPanel[2] = 1;
        QCOMPARE(param.offlinePayloadSize(), unsigned(8));
        char* frame = packData(reinterpret_cast<char*>(&param.off), param.offlinePayloadSize(), 1, 2, 3);
        QVERIFY(frame);
        const QByteArray bytes(frame, sizeof(ProtocolFrame) + 8 + sizeof(ProtocolEnd));
        std::free(frame);
        QCOMPARE(bytes.size(), 27);
        QCOMPARE(bytes.mid(sizeof(ProtocolFrame), 8).toHex(), QByteArray("03cc010701000100"));
        QCOMPARE(quint8(bytes[22]), calculateXOR(bytes.constData(), 22));
    }
    void offlineSelectionAndCancel() {
        QVERIFY(loadMode(4));
        DataSaveUI dialog;
        auto* table = dialog.findChild<QTableWidget*>(); QVERIFY(table);
        DataSet sent; int count = 0;
        connect(&dialog, &DataSaveUI::setParam, &dialog, [&](DataSet value) { sent = value; ++count; });
        dialog.offlineDeal(); dialog.onlineDeal(); QCOMPARE(count, 0);
        table->setRowCount(1); table->setItem(0, 0, new QTableWidgetItem("7")); table->setCurrentCell(0, 0);
        QTimer::singleShot(0, &dialog, [&] {
            auto* selection = dialog.findChild<QDialog*>("offlinePanelSelection"); QVERIFY(selection);
            auto* buttons = selection->findChild<QDialogButtonBox*>(); QVERIFY(buttons);
            for (int id = 0; id < 4; ++id) {
                auto* box = selection->findChild<QCheckBox*>(QString("replayPanel%1").arg(id));
                QVERIFY(box); QVERIFY(!box->isChecked());
            }
            buttons->button(QDialogButtonBox::Ok)->click(); // empty selection must not send
            QVERIFY(selection->isVisible()); QCOMPARE(count, 0);
            selection->findChild<QCheckBox*>("replayPanel0")->setChecked(true);
            selection->findChild<QCheckBox*>("replayPanel2")->setChecked(true);
            buttons->button(QDialogButtonBox::Ok)->click();
        });
        dialog.offlineDeal(); QCOMPARE(count, 1);
        QVERIFY(sent.ifoffline && sent.extendedOffline); QCOMPARE(sent.off.onSwitch, quint8(1));
        QCOMPARE(sent.off.dataID, quint8(7));
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(sent.off.replayPanel), 4).toHex(), QByteArray("01000100"));
        QTimer::singleShot(0, &dialog, [&] {
            auto* selection = dialog.findChild<QDialog*>("offlinePanelSelection"); QVERIFY(selection); selection->reject();
        });
        dialog.offlineDeal(); QCOMPARE(count, 1);
        dialog.onlineDeal(); QCOMPARE(count, 2); QVERIFY(sent.extendedOffline); QCOMPARE(sent.off.onSwitch, quint8(0));
        for (auto flag : sent.off.replayPanel) QCOMPARE(flag, quint8(0));
        QVERIFY(loadMode(1));
        dialog.offlineDeal(); QCOMPARE(count, 3); QVERIFY(!sent.extendedOffline);
        QCOMPARE(sent.offlinePayloadSize(), unsigned(4));
    }
};
QTEST_MAIN(DataSaveTest)
#include "datasave_test.moc"
