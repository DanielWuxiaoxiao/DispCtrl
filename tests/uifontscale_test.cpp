#include <QApplication>
#include <QLabel>
#include <QTableWidget>
#include <QTest>
#include "Basic/uifontscale.h"

class UiFontScaleTest final : public QObject {
    Q_OBJECT
private slots:
    void compactLaptopSizes() {
        QVERIFY(ScaleHelper::compactSize(1920, 1080));
        QVERIFY(ScaleHelper::compactSize(1920, 1200));
        QVERIFY(ScaleHelper::compactSize(1536, 960));
        QVERIFY(!ScaleHelper::compactSize(2560, 1440));
        QVERIFY(!ScaleHelper::compactSize(3840, 2160));
    }
    void declarationsOnly() {
        const QString base = "QLabel {font-size:18px; border:1px solid red; padding:8px;} "
                             "QPushButton {font:bold 11px 'Sans'; min-width:70px;} "
                             "QLineEdit {font-size:10pt;}";
        const QString small = UiFontScale::style(base, 0.85);
        QVERIFY(small.contains("font-size:15px"));
        QVERIFY(small.contains("font:bold 9px"));
        QVERIFY(small.contains("font-size:8.5pt"));
        QVERIFY(small.contains("border:1px"));
        QVERIFY(small.contains("padding:8px"));
        QVERIFY(small.contains("min-width:70px"));
        QCOMPARE(UiFontScale::style(base, 1.0), base);
        QCOMPARE(UiFontScale::style("background:#101818; width:42px;", 0.85),
                 QString("background:#101818; width:42px;"));
    }
    void widgetAndPainterFonts_data() {
        QTest::addColumn<double>("factor");
        QTest::newRow("minimum") << 0.70;
        QTest::newRow("small") << 0.85;
        QTest::newRow("normal") << 1.0;
        QTest::newRow("large") << 1.25;
        QTest::newRow("maximum") << 1.60;
    }
    void widgetAndPainterFonts() {
        QFETCH(double, factor);
        ScaleHelper::init(QStringLiteral("fixed"), factor);
        QWidget parent;
        QLabel label("109.123922221", &parent);
        QLabel copied("copy", &parent);
        QTableWidget table(1, 1, &parent);
        const QString base = "font-size:18px; color:#66ffcc;";
        for (int i = 0; i < 10; ++i) label.setStyleSheet(UiFontScale::style(base));
        copied.setStyleSheet(label.styleSheet());
        table.setStyleSheet(UiFontScale::style("QTableWidget {font-size:22px;}"));
        parent.ensurePolished(); label.ensurePolished(); copied.ensurePolished(); table.ensurePolished();
        QCOMPARE(label.font().pixelSize(), qRound(18 * factor));
        QCOMPARE(copied.font().pixelSize(), qRound(18 * factor));
        QCOMPARE(table.font().pixelSize(), qRound(22 * factor));
        QCOMPARE(UiFontScale::pointFont("Sans", 10).pixelSize(), qRound(10 * 96.0 / 72.0 * factor));
        QCOMPARE(ScaleHelper::uiScaled(12), qRound(12 * factor));
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    UiFontScaleTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "uifontscale_test.moc"
