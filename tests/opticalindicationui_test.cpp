#include "ZhoushanOptical/zhoushanmodule.h"
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

namespace ZhoushanOptical {
class ModuleUiTest : public QObject {
    Q_OBJECT
    QString m_savedStyle;
    static QWidget* contentOf(Module& module) {
        return module.m_window->findChild<QWidget*>(QStringLiteral("OpticalIndicationContent"));
    }
    static QVariantMap responseRecord(Module& module, quint64 order) {
        QByteArray packet = module.m_request;
        packet[2] = 4; packet[3] = 1;
        packet[4] = char(3101 & 255); packet[5] = char(3101 >> 8);
        packet[6] = char(1100 & 255); packet[7] = char(1100 >> 8); packet[23] = 1;
        const auto sum = checksum(packet.mid(2, 42)); packet[44] = char(sum & 255); packet[45] = char(sum >> 8);
        return {{"packet", packet}, {"session", QVariant::fromValue<qulonglong>(module.m_session)},
            {"receivedMs", module.m_sentAt}, {"order", QVariant::fromValue<qulonglong>(order)},
            {"receivedTime", "2026-10-20 12:30:00.001"}};
    }
private slots:
    void init() {
        m_savedStyle = qApp->styleSheet();
        qApp->setStyleSheet(QStringLiteral("QLabel { color:#000000; font-size:12px; } QTableWidget { color:#000000; font-size:12px; }"));
    }
    void cleanup() { qApp->setStyleSheet(m_savedStyle); }
    void neutralNameLargeFontsAndOperatorLayout() {
        // No init(): these tests start no TCP server, radar or UDP resources.
        Module module; module.showWindow(nullptr); QCoreApplication::processEvents();
        QWidget* content = contentOf(module); QVERIFY(content); content->ensurePolished();
        QCOMPARE(content->palette().color(QPalette::Window), QColor("#101818"));
        for (const QString& name : {QStringLiteral("OpticalTcpStatus"), QStringLiteral("OpticalReportStatus"),
                                   QStringLiteral("OpticalLocalEndpoint"), QStringLiteral("OpticalPeerEndpoint")}) {
            auto* label = content->findChild<QLabel*>(name); QVERIFY(label); label->ensurePolished();
            QVERIFY2(label->palette().color(QPalette::WindowText).lightness() > 160, qPrintable(name));
            QVERIFY2(label->font().pixelSize() >= 22, qPrintable(name));
            QVERIFY(label->property("card").toBool()); QVERIFY(!label->text().contains(QStringLiteral("舟山")));
        }
        auto* title = content->window()->findChild<QLabel*>(QStringLiteral("TitleLabel"));
        QVERIFY(title); QCOMPARE(title->text(), QStringLiteral("光电目标指示"));
        QVERIFY(content->findChild<QLabel*>(QStringLiteral("OpticalLocalEndpoint"))->text().contains("192.168.101.14:9010"));
        QVERIFY(content->findChild<QLabel*>(QStringLiteral("OpticalPeerEndpoint"))->text().contains("192.168.101.10"));
        auto* table = content->findChild<QTableWidget*>(QStringLiteral("OpticalExchangeTable")); QVERIFY(table);
        table->ensurePolished(); QCOMPARE(table->rowCount(), 10); QCOMPARE(table->columnCount(), 4);
        QVERIFY(table->font().pixelSize() >= 22); QVERIFY(table->horizontalHeader()->font().pixelSize() >= 22);
        QCOMPARE(table->horizontalHeaderItem(1)->text(), QStringLiteral("最近下发"));
        QCOMPARE(table->horizontalHeaderItem(2)->text(), QStringLiteral("当前批次回传"));
        QCOMPARE(table->horizontalHeaderItem(3)->text(), QStringLiteral("上一批次回传"));
        QCOMPARE(table->editTriggers(), QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers));
        for (const auto* label : content->findChildren<QLabel*>()) {
            QVERIFY(!label->text().contains(QStringLiteral("仅第"))); QVERIFY(!label->text().contains(QStringLiteral("TCP的报文")));
        }
        auto* stop = content->findChild<QPushButton*>(QStringLiteral("OpticalStopReport")); QVERIFY(stop);
        QCOMPARE(stop->text(), QStringLiteral("停止下发")); QVERIFY(!stop->isEnabled());
    }
    void responseColumnsAndWindowReopen() {
        Module module; module.m_connected = true; module.m_session = 7; module.updateRadarPosition(30, 120, 100);
        PointInfo point{}; point.type = PointType::Track; point.batch = 42; point.range = 1000; point.azimuth = 90;
        module.processTrack(point); QVERIFY(module.startReport(42)); module.consume({responseRecord(module, 1)});
        module.showWindow(nullptr); auto* content = contentOf(module);
        auto* table = content->findChild<QTableWidget*>(QStringLiteral("OpticalExchangeTable")); QVERIFY(table);
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("42"));
        QCOMPARE(table->item(2, 2)->text(), QStringLiteral("1 · 已接收并处理"));
        QCOMPARE(table->item(7, 2)->text(), QStringLiteral("2026-10-20 12:30:00.001"));
        QCOMPARE(table->item(9, 2)->text(), QStringLiteral("已匹配"));
        QCOMPARE(table->item(2, 1)->text(), QStringLiteral("尚未下发")); // No worker acknowledged a write.
        module.recordSentPacket(module.m_sentAt); module.publish();
        QCOMPARE(table->item(0, 1)->text(), QStringLiteral("42"));
        QCOMPARE(table->item(2, 1)->text(), QStringLiteral("已提交发送"));
        QCOMPARE(table->item(3, 1)->text(), QString::number(readDouble(module.m_writePacket, 24), 'f', 7));
        QCOMPARE(table->item(4, 1)->text(), QString::number(readDouble(module.m_writePacket, 32), 'f', 7));
        QCOMPARE(table->item(5, 1)->text(), QString::number(readFloat(module.m_writePacket, 40), 'f', 1));
        auto* firstCell = table->item(0, 2);
        point.batch = 43; module.processTrack(point); QVERIFY(module.startReport(43));
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("43"));
        QCOMPARE(table->item(2, 2)->text(), QStringLiteral("等待回传"));
        QCOMPARE(table->item(0, 3)->text(), QStringLiteral("42"));
        QCOMPARE(table->item(2, 3)->text(), QStringLiteral("1 · 已接收并处理"));
        module.m_busy = false; module.tick(); module.consume({responseRecord(module, 2)});
        QCOMPARE(table->item(2, 2)->text(), QStringLiteral("1 · 已接收并处理"));
        QCOMPARE(table->item(0, 2), firstCell); // Fixed cells, no repeated item recreation.
        auto* stop = content->findChild<QPushButton*>(QStringLiteral("OpticalStopReport")); QVERIFY(stop->isEnabled());
        stop->click(); QCOMPARE(module.m_selected, -1); QVERIFY(!stop->isEnabled());
        module.m_window->hide(); module.showWindow(nullptr);
        QCOMPARE(table->item(0, 3)->text(), QStringLiteral("42"));
        module.m_window->close(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!module.m_window); module.showWindow(nullptr);
        table = contentOf(module)->findChild<QTableWidget*>(QStringLiteral("OpticalExchangeTable"));
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("43")); QCOMPARE(table->item(0, 3)->text(), QStringLiteral("42"));
    }
};
}
QTEST_MAIN(ZhoushanOptical::ModuleUiTest)
#include "opticalindicationui_test.moc"
