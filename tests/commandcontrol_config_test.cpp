#include <QTest>
#include <QTextCodec>
#include <QTemporaryDir>
#include "Basic/ConfigManager.h"
#include "CommandControl/commandcontrolconfig.h"

class CommandControlConfigTest final : public QObject {
    Q_OBJECT
    struct LocaleGuard {
        QTextCodec* previous = QTextCodec::codecForLocale();
        LocaleGuard() { QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK")); }
        ~LocaleGuard() { QTextCodec::setCodecForLocale(previous); }
    };
private slots:
    void utf8SwitchesDoNotDependOnSystemCodec() {
        LocaleGuard guard;
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.filePath("config.toml");
        const auto write = [&](bool enabled) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
            const QByteArray bytes = QStringLiteral(
                "# 显控配置：总控通信开关，修改后重启生效。\n"
                "[command_control]\nenabled = %1\ntime_sync_enabled = false\n"
                "local_port = 23456\n[displayConfig]\ndefault_panel_count = 4\n").arg(enabled ? "true" : "false").toUtf8();
            return file.write(bytes) == bytes.size();
        };
        for (bool enabled : {false, true}) {
            QVERIFY(write(enabled));
            QVERIFY(CF_INS.load(path));
            QCOMPARE(CF_INS.defaultPanelCount(), 4);
            const auto settings = CommandControlConfig::load();
            QCOMPARE(settings.enabled, enabled);
            QVERIFY(!settings.timeSyncEnabled);
            QCOMPARE(settings.localPort, quint16(23456));
        }
    }
    void releaseConfigWithChineseWindowsLocale() {
        const QString path = qEnvironmentVariable("DISPCTRL_CONFIG_FIXTURE");
        if (path.isEmpty()) QSKIP("Set DISPCTRL_CONFIG_FIXTURE for the local release config diagnostic");
        LocaleGuard guard;
        QVERIFY(CF_INS.load(path));
        const auto settings = CommandControlConfig::load();
        QVERIFY(!settings.enabled);
        QVERIFY(!settings.timeSyncEnabled);
    }
};
QTEST_GUILESS_MAIN(CommandControlConfigTest)
#include "commandcontrol_config_test.moc"
