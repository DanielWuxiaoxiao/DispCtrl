#include <QTest>
#include "Basic/uitheme.h"
#include "cusWidgets/customcombobox.h"
#include "paramWidget/tasfrequencycombo.h"

class TasFrequencyComboTest final : public QObject {
    Q_OBJECT
private slots:
    void optionsRestoreAndWireValues() {
        for (const auto& theme : {QStringLiteral("graphite"), QStringLiteral("classic")}) {
            UiTheme::configure(theme);
            CustomComboBox combo;
            TasControl::setupFrequencyCombo(&combo);
            QCOMPARE(combo.count(), 10);
            QCOMPARE(combo.currentData().toInt(), 0);
            for (int code = 0; code <= 80; ++code) {
                TasControl::restoreFrequencyCombo(&combo, code);
                if (code % 10 != 0) {
                    QCOMPARE(combo.findData(code), -1);
                    QCOMPARE(combo.currentIndex(), -1);
                    QVERIFY(!combo.currentData().isValid());
                    continue;
                }
                QCOMPARE(combo.currentText(), QString::number(9.0 + code * 0.01, 'f', 1));
                BeamControl beam;
                beam.freqID = static_cast<uchar>(combo.currentData().toInt());
                const QByteArray bytes(reinterpret_cast<const char*>(&beam), sizeof(beam));
                QCOMPARE(quint8(bytes[2]), quint8(code));
            }
            TasControl::restoreFrequencyCombo(&combo, 90);
            QCOMPARE(combo.currentText(), QStringLiteral("跳频"));
            QCOMPARE(combo.currentData().toInt(), 90);
            TasControl::restoreFrequencyCombo(&combo, 255);
            QCOMPARE(combo.currentData().toInt(), 0);
        }
    }
};
QTEST_MAIN(TasFrequencyComboTest)
#include "tasfrequencycombo_test.moc"
