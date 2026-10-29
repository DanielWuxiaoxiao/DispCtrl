#include <QApplication>
#include <QScreen>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QPointer>
#include <QLabel>
#include "cusWidgets/cuswindow.h"
#include "Basic/radarpanelstate.h"
#include "Basic/uiscale.h"
#include "paramWidget/radarpanelpositiondialog.h"

class RadarPanelPositionDialogTest final : public QObject {
    Q_OBJECT
private slots:
    void customHostEmbeddingAndCloseLifetime() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        QPointer<CusWindow> host = new CusWindow(QStringLiteral("测试窗口"), QIcon());
        QPointer<RadarPanelPositionDialog> content = new RadarPanelPositionDialog(
            state.dialogValues(), {true, true, true, true}, {}, host);
        content->attachToWindow(host);
        host->show(); QApplication::processEvents();
        QVERIFY(host->windowFlags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(!content->isWindow()); QVERIFY(!content->isModal());
        QVERIFY(content->findChild<QLabel*>("panelPositionTitle")->isHidden());
        QVERIFY(!content->testAttribute(Qt::WA_DeleteOnClose));
        content->reject();
        QApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(host.isNull()); QVERIFY(content.isNull());
    }
    void selectingAnotherPanelRefreshesAutomaticLocks() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50, true);
        RadarPanelPositionDialog::Actions actions;
        actions.live = [&] { return state.snapshot(); };
        actions.unlock = [&](int id) -> std::optional<RadarReference::Pose> {
            if (!state.unlock(id)) return std::nullopt;
            return state.reference(id).pose();
        };
        RadarPanelPositionDialog dialog(state.dialogValues(), {true, true, true, true}, actions);
        dialog.findChild<QPushButton*>("unlockPanel0")->click();
        QVERIFY(!state.reference(0).locked());
        dialog.findChild<QPushButton*>("unlockPanel1")->click();
        QTest::qWait(250);
        QVERIFY(state.reference(0).locked()); QVERIFY(!state.reference(1).locked());
        QVERIFY(!dialog.findChild<QPushButton*>("lockPanel0")->isEnabled());
        QVERIFY(dialog.findChild<QPushButton*>("unlockPanel0")->isEnabled());
        QVERIFY(dialog.findChild<QPushButton*>("lockPanel1")->isEnabled());
    }
    void liveRefreshDoesNotOverwriteLockedDraft() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        QVERIFY(state.lock(0, state.reference(0).pose()));
        RadarPanelPositionDialog::Actions actions;
        actions.live = [&] { return state.snapshot(); };
        RadarPanelPositionDialog dialog(state.dialogValues(), {true, false, false, false}, actions);
        auto* locked = dialog.findChild<QLineEdit*>("panel0Field0");
        locked->setText("121");
        BITReportV2 bit; bit.radarId = 1; bit.latitudeE9 = 30000000000LL;
        bit.longitudeE9 = 122000000000LL; bit.altitudeCm = 10000;
        state.ingest(bit, 1000);
        QTest::qWait(250);
        QCOMPARE(locked->text(), QString("121"));
        QCOMPARE(dialog.findChild<QLineEdit*>("panel1Field0")->text(), QString("122.000000000"));
    }
    void positionEditsPropagateExceptLockedRows() {
        RadarPanelState state;
        state.initialize({120, 30, 100, 0, 0, 0}, 100, 50);
        RadarPanelPositionDialog dialog(state.dialogValues(), {true, true, false, true}, {});
        for (int field = 0; field < 3; ++field) {
            auto* source = dialog.findChild<QLineEdit*>(QString("panel1Field%1").arg(field));
            const QString edited = QString::number(40 + field);
            source->setText(edited);
            QVERIFY(QMetaObject::invokeMethod(source, "textEdited", Q_ARG(QString, edited)));
            QCOMPARE(dialog.findChild<QLineEdit*>(QString("panel0Field%1").arg(field))->text(), edited);
            QCOMPARE(dialog.findChild<QLineEdit*>(QString("panel3Field%1").arg(field))->text(), edited);
            QVERIFY(dialog.findChild<QLineEdit*>(QString("panel2Field%1").arg(field))->text() != edited);
        }
        auto* yaw = dialog.findChild<QLineEdit*>("panel1Field3");
        yaw->setText("123");
        QVERIFY(QMetaObject::invokeMethod(yaw, "textEdited", Q_ARG(QString, QString("123"))));
        QCOMPARE(dialog.findChild<QLineEdit*>("panel0Field3")->text(), QString("0.000"));
    }
    void editLockUnlockRememberAndCancel() {
        ScaleHelper::init(QStringLiteral("fixed"), 0.85);
        RadarPanelState state;
        state.initialize({120, 30, 100, 350, 0, 0}, 100, 50);
        int submits = 0, locks = 0;
        RadarPanelPositionDialog::Actions actions;
        actions.submit = [&](const RadarPanelValues& values) {
            RadarPositionControl command;
            if (!state.buildCommand(values, command)) return false;
            ++submits; state.rememberSubmitted(command); return true;
        };
        actions.lock = [&](int id, const RadarReference::Pose& values) { ++locks; return state.lock(id, values); };
        actions.unlock = [&](int id) -> std::optional<RadarReference::Pose> {
            if (!state.unlock(id)) return std::nullopt;
            return state.reference(id).pose();
        };
        RadarPanelPositionDialog dialog(state.dialogValues(), {}, actions);
        dialog.show(); QApplication::processEvents();
        QVERIFY(!dialog.isModal());
        QCOMPARE(dialog.windowModality(), Qt::NonModal);
        QVERIFY(QApplication::activeModalWidget() == nullptr);
        auto* content = dialog.findChild<QWidget*>("panelPositionContent");
        auto* viewport = dialog.findChild<QWidget*>("panelPositionViewport");
        QVERIFY(content && viewport);
        QCOMPARE(content->palette().color(QPalette::Window), QColor("#111e1b"));
        QCOMPARE(viewport->palette().color(QPalette::Window), QColor("#101818"));
        QVERIFY(dialog.width() <= QGuiApplication::primaryScreen()->availableGeometry().width());
        QVERIFY(dialog.height() <= QGuiApplication::primaryScreen()->availableGeometry().height());
        QCOMPARE(dialog.findChildren<QLineEdit*>().size(), 24);
        auto* lon = dialog.findChild<QLineEdit*>("panel1Field0");
        auto* yaw = dialog.findChild<QLineEdit*>("panel1Field3");
        auto* lock = dialog.findChild<QPushButton*>("lockPanel1");
        auto* unlock = dialog.findChild<QPushButton*>("unlockPanel1");
        auto* submit = dialog.findChild<QPushButton*>("submitPanels");
        QVERIFY(lon && yaw && lock && unlock && submit);
        QCOMPARE(yaw->text(), QString("80.000"));
        lon->setText("181"); lock->click(); QCOMPARE(locks, 0);
        lon->setText("120");
        lock->click(); QCOMPARE(locks, 1); QCOMPARE(submits, 0);
        QVERIFY(!lon->isReadOnly()); QVERIFY(!yaw->isReadOnly()); QVERIFY(unlock->isEnabled());
        QVERIFY(dialog.findChild<QLineEdit*>("panel0Field0")->isReadOnly());
        lon->setText("121.123456789"); yaw->setText("111.123");
        submit->click(); QCOMPARE(submits, 1); QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QVERIFY(state.reference(1).locked());
        RadarPanelPositionDialog reopened(state.dialogValues(), {false, true, false, false}, actions);
        QCOMPARE(reopened.findChild<QLineEdit*>("panel1Field0")->text(), QString("121.123456789"));
        QCOMPARE(reopened.findChild<QLineEdit*>("panel1Field3")->text(), QString("111.123"));
        reopened.findChild<QPushButton*>("unlockPanel1")->click();
        QVERIFY(!state.reference(1).locked());
        QVERIFY(reopened.findChild<QLineEdit*>("panel1Field0")->isReadOnly());
        reopened.findChild<QPushButton*>("lockPanel0")->click();
        reopened.findChild<QLineEdit*>("panel0Field2")->setText("5000.01");
        reopened.findChild<QPushButton*>("submitPanels")->click(); QCOMPARE(submits, 1);
        reopened.reject(); QCOMPARE(submits, 1);
        RadarPanelPositionDialog failed(state.dialogValues(), {}, {[](const RadarPanelValues&) { return false; }, {}, {}});
        failed.findChild<QPushButton*>("submitPanels")->click();
        QCOMPARE(failed.result(), int(QDialog::Rejected));
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    RadarPanelPositionDialogTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "radarpanelpositiondialog_test.moc"
