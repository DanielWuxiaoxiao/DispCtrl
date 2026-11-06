#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/offlinerae.h"
#include "Basic/uifontscale.h"
#include "mainPanel/healthstatuswidgets.h"

#include <QFile>
#include <QCryptographicHash>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

class UiThemeTest : public QObject
{
    Q_OBJECT
private slots:
    void cleanup() { UiTheme::configure(QStringLiteral("graphite")); }

    void graphitePreservesCurrentStyles()
    {
        const QString style = "QLabel {color:#e8edf1; background:rgba(18,23,28,178); padding:8px;}";
        QCOMPARE(UiTheme::styleColors(style), style);
        QCOMPARE(DET_COLOR, QColor(120, 212, 165));
        QCOMPARE(TRA_COLOR, QColor(234, 140, 147));
        QCOMPARE(CO_TRACK_COLOR, QColor(120, 203, 216));
        QVERIFY(!CF_INS.displayFlag("detection_drone_red_enabled", false));
    }

    void classicUsesHistoricalRadarColors()
    {
        const QColor* address = &DET_COLOR;
        UiTheme::configure(QStringLiteral("classic"));
        QCOMPARE(&DET_COLOR, address);
        QCOMPARE(DET_COLOR, QColor(Qt::green));
        QCOMPARE(TRA_COLOR, QColor(Qt::red));
        QCOMPARE(DRONE_COLOR, QColor(Qt::yellow));
        QCOMPARE(TBD_COLOR, QColor(160, 64, 255));
        QCOMPARE(CO_TRACK_COLOR, QColor(0, 170, 255));
        QCOMPARE(PPI_ACCENT_COLOR, QColor(0, 255, 136));
        QCOMPARE(OfflineRae::kCyanDisplayColor, QColor(Qt::cyan));
        QCOMPARE(OfflineRae::kRedDisplayColor, QColor(Qt::red));
        QCOMPARE(OfflineRae::kBlueDisplayColor, QColor(Qt::blue));
    }

    void colorsAndFontScaleAreIndependent()
    {
        UiTheme::configure(QStringLiteral("classic"));
        const QString base = "QLabel {color:#E8EDF1; background:rgba(18, 23, 28, 178); "
            "border:1px solid #303b44; padding:8px; font-size:12px; min-width:70px;}";
        const QString style = UiFontScale::style(base, 1.5);
        QVERIFY(style.contains("color:#E8EDF1"));
        QVERIFY(style.contains("rgba(18, 23, 28, 178)"));
        QVERIFY(style.contains("border:1px solid #303b44"));
        QVERIFY(style.contains("padding:8px"));
        QVERIFY(style.contains("min-width:70px"));
        QVERIFY(style.contains("font-size:18px"));
        QCOMPARE(UiTheme::styleColors(style), style);
        QCOMPARE(UiTheme::color(QColor(120, 203, 216, 30)), QColor(120, 203, 216, 30));
    }

    void healthCardsUseSelectedSurfaces()
    {
        for (const QString& theme : {QStringLiteral("graphite")}) {
            UiTheme::configure(theme);
            for (auto state : {UiTheme::Status::Normal, UiTheme::Status::Fault, UiTheme::Status::Unknown}) {
                HealthUi::Field card(QStringLiteral("信号处理"), HealthUi::Field::Layout::Card);
                card.setStatus(state, QStringLiteral("状态"));
                card.resize(360, 140);
                card.ensurePolished();
                QImage image(card.size(), QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::black);
                card.render(&image);
                QCOMPARE(image.pixelColor(10, 10), QColor(state == UiTheme::Status::Fault
                    ? UiTheme::FaultSurface : UiTheme::Surface));
            }
        }
    }

    void classicResourcesMatchX576Baseline()
    {
        // 固定 x576/f338142 的原始资产摘要，防止再次以近似调色替代。
        const struct { const char* path; const char* sha256; } assets[] = {
            {"../resources/style/classic.qss", "f2bcd188747aa307cf4051ef5c56bcf9159b2eedb0d43f7bf82547958fddc8b5"},
            {"../htmls/index_classic.html", "58ba84026b6ae2b021bf75e40009bae620ca2f2b933d7a1bf373a4ce639da982"},
            {"../htmls/indexNoL_classic.html", "5cf030fc30327bf707f536ae2c7ee95bd61d643905caddabcea5d457f9167c62"},
            {"../htmls/indexS_classic.html", "d95d491100c2e87220a546f9a7f752735bf09eb57e0ef8cd413d4eccb69cde5e"},
            {"../htmls/index3d_classic.html", "6cdbcf2120ffd57de0faf87d44e49faa71813d4b2a84bcb451b691ba379c57c9"},
            {"../resources/web/track3d/track3d_classic.css", "34860f3b62d9a987481de47ee3e39ae665f0d8cf69f6f7408e8e066519bb042e"}
        };
        for (const auto& asset : assets) {
            QFile file(QTest::qFindTestData(asset.path, __FILE__, __LINE__));
            QVERIFY2(file.open(QIODevice::ReadOnly), asset.path);
            QCOMPARE(QCryptographicHash::hash(file.readAll().replace("\r\n", "\n"), QCryptographicHash::Sha256).toHex(),
                     QByteArray(asset.sha256));
        }
    }

    void classicRestoresOriginalFontScale()
    {
        const double previous = ScaleHelper::uiScale();
        for (double scale : {0.70, 0.85, 1.0, 1.25, 1.60}) {
            ScaleHelper::init(QStringLiteral("fixed"), scale);
            UiTheme::configure(QStringLiteral("classic"));
            QCOMPARE(UiFontScale::pixels(12), ScaleHelper::uiScaled(12));
            QCOMPARE(UiFontScale::factor(), scale);
            UiTheme::configure(QStringLiteral("graphite"));
            QCOMPARE(UiFontScale::pixels(12), qRound(12 * scale * 1.08));
        }
        ScaleHelper::init(QStringLiteral("fixed"), previous);
    }

    void configurationNormalizesAndFallsBack()
    {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        for (const QString& value : {QString(), QStringLiteral(" CLASSIC "), QStringLiteral("invalid")}) {
            QFile file(fixture.filePath("config.toml"));
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QByteArray text = "[ui]\n";
            if (!value.isEmpty()) text += "theme = \"" + value.toUtf8() + "\"\n";
            QCOMPARE(file.write(text), qint64(text.size()));
            file.close();
            QVERIFY(CF_INS.load(file.fileName()));
            UiTheme::configure(CF_INS.uiTheme());
            QCOMPARE(UiTheme::name(), value == QLatin1String(" CLASSIC ")
                ? QStringLiteral("classic") : QStringLiteral("graphite"));
        }
    }
};

QTEST_MAIN(UiThemeTest)
#include "uitheme_test.moc"
