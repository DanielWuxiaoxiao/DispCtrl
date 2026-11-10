#include "Basic/uithemeruntime.h"
#include "Basic/uifontscale.h"
#include "Basic/ConfigManager.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTest>

class UiThemeRuntimeTest final : public QObject {
    Q_OBJECT
private slots:
    void repeatedChangesPreserveWidgetStateAndScaleFromDesignSizes() {
        UiTheme::configure(QStringLiteral("graphite"));
        UiTheme::Runtime::instance().setApplicationRefresh([] {
            QFont font = QApplication::font();
            font.setPixelSize(UiFontScale::pixels(12));
            QApplication::setFont(font);
        });
        QWidget panel;
        QLineEdit value(QStringLiteral("123.456"), &panel);
        value.setSelection(1, 3);
        QCheckBox enabled(&panel);
        enabled.setChecked(true);
        QSplitter splitter;
        new QWidget(&splitter);
        new QWidget(&splitter);
        splitter.resize(900, 400);
        splitter.setSizes({250, 650});
        splitter.show();
        QCoreApplication::processEvents();
        const auto sizes = splitter.sizes();
        const auto factory = [] {
            return UiFontScale::style(UiTheme::classicEnabled
                ? QStringLiteral("QLineEdit {color:#66ffcc;font-size:16px;}")
                : QStringLiteral("QLineEdit {color:#e8edf1;font-size:16px;}"));
        };
        UiTheme::setStyle(&value, factory);
        const QString graphite = value.styleSheet();
        for (int i = 0; i < 30; ++i) {
            QVERIFY(UiTheme::Runtime::instance().apply(QStringLiteral("classic")));
            QCOMPARE(value.styleSheet(), factory());
            QVERIFY(UiTheme::Runtime::instance().apply(QStringLiteral("graphite")));
            QCOMPARE(value.styleSheet(), graphite);
            QCOMPARE(value.text(), QStringLiteral("123.456"));
            QCOMPARE(value.selectedText(), QStringLiteral("23."));
            QVERIFY(enabled.isChecked());
            QCoreApplication::processEvents();
            QCOMPARE(splitter.sizes(), sizes);
        }
    }

    void destroyedBindingsAndReentrantChangesAreSafe() {
        UiTheme::configure(QStringLiteral("graphite"));
        int calls = 0;
        auto* stale = new QWidget;
        UiTheme::setStyle(stale, [&calls] { ++calls; return QString(); });
        UiTheme::onChanged(stale, [] { QFAIL("A destroyed owner must not receive theme callbacks"); });
        delete stale;
        QObject owner;
        UiTheme::onChanged(&owner, [] {
            QVERIFY(!UiTheme::Runtime::instance().apply(QStringLiteral("graphite")));
        });
        QVERIFY(UiTheme::Runtime::instance().apply(QStringLiteral("classic")));
        QCOMPARE(calls, 1);
        QCOMPARE(UiTheme::name(), QStringLiteral("classic"));
        QVERIFY(!UiTheme::Runtime::instance().apply(QStringLiteral("invalid")));
    }

    void styleUpdatesReplaceEarlierStateAndIconsKeepEmptyButtonsEmpty() {
        UiTheme::configure(QStringLiteral("graphite"));
        QWidget panel;
        UiTheme::setStyle(&panel, [] { return QStringLiteral("color:red"); });
        UiTheme::setStyle(&panel, [] { return QStringLiteral("color:green"); });
        QVERIFY(UiTheme::Runtime::instance().apply(QStringLiteral("classic")));
        QCOMPARE(panel.styleSheet(), QStringLiteral("color:green"));
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QPixmap oldIcon(16, 16), newIcon(16, 16);
        oldIcon.fill(Qt::green); newIcon.fill(Qt::cyan);
        const QString classic = dir.filePath(QStringLiteral("classic.png"));
        const QString graphite = dir.filePath(QStringLiteral("graphite.png"));
        QVERIFY(oldIcon.save(classic)); QVERIFY(newIcon.save(graphite));
        const QIcon icon = UiTheme::themedIcon(classic, graphite);
        QVERIFY(UiTheme::Runtime::instance().apply(QStringLiteral("graphite")));
        QCOMPARE(UiTheme::refreshedIcon(icon).pixmap(16, 16).toImage().pixelColor(0, 0), QColor(Qt::cyan));
        QVERIFY(UiTheme::refreshedIcon(QIcon()).isNull());
    }

    void savesOnlyThemeAndPreservesConfigCommentsAndLineEndings() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("config.toml"));
        const QByteArray original = "[radar]\r\nlongitude = 109.0\r\n[ui]\r\n"
            "theme = \"graphite\" # selected skin\r\nui_scale = 1.2\r\n[other]\r\nvalue = 7\r\n";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(original), qint64(original.size()));
        file.close();
        auto& config = ConfigManager::instance();
        QVERIFY(config.load(path));
        QVERIFY(config.saveUiTheme(QStringLiteral("classic")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray saved = file.readAll();
        file.close();
        QVERIFY(saved.contains("theme = \"classic\"  # selected skin\r\n"));
        QVERIFY(saved.contains("longitude = 109.0\r\n"));
        QVERIFY(saved.contains("ui_scale = 1.2\r\n"));
        QCOMPARE(config.uiTheme(), QStringLiteral("classic"));
        QVERIFY(!config.saveUiTheme(QStringLiteral("invalid")));

        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray withBom = QByteArray("\xEF\xBB\xBF") + "[ui]\r\ntheme = \"classic\"\r\n";
        QCOMPARE(file.write(withBom), qint64(withBom.size()));
        file.close();
        QVERIFY(config.load(path));
        QVERIFY(config.saveUiTheme(QStringLiteral("graphite")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray updated = file.readAll();
        QVERIFY(updated.startsWith("\xEF\xBB\xBF"));
        QCOMPARE(updated.count("[ui]"), 1);
        QVERIFY(updated.contains("theme = \"graphite\"\r\n"));
    }
};
QTEST_MAIN(UiThemeRuntimeTest)
#include "uithemeruntime_test.moc"
