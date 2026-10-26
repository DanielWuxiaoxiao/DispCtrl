#include "Basic/authmanager.h"
#include "cusWidgets/developerlogindialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

class AuthModeTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QVERIFY(!AuthManager::instance().isAdminMode()); }
    void init() { AuthManager::instance().setAdminMode(false); }
    void cleanup() { AuthManager::instance().setAdminMode(false); }

    void authenticationAndNotifications() {
        auto& auth = AuthManager::instance();
        QSignalSpy changed(&auth, &AuthManager::modeChanged);
        QVERIFY(!auth.enterDeveloperMode("wrong", "wrong"));
        QVERIFY(!auth.isAdminMode());
        QCOMPARE(changed.count(), 0);
        QVERIFY(!auth.enterDeveloperMode("admin", ""));
        QVERIFY(!auth.isAdminMode());
        QVERIFY(auth.enterDeveloperMode(" admin ", "xidian"));
        QVERIFY(auth.isAdminMode());
        QCOMPARE(changed.count(), 1);
        QVERIFY(auth.enterDeveloperMode("admin", "xidian"));
        QCOMPARE(changed.count(), 1);
        auth.setAdminMode(false);
        QVERIFY(!auth.isAdminMode());
        QCOMPARE(changed.count(), 2);
        QCOMPARE(changed.last().first().toBool(), false);
    }

    void invalidInputAndCancelStayInUserMode() {
        DeveloperLoginDialog dialog;
        dialog.show();
        QCoreApplication::processEvents();
        auto* username = dialog.findChild<QLineEdit*>("DeveloperUsername");
        auto* password = dialog.findChild<QLineEdit*>("DeveloperPassword");
        auto* login = dialog.findChild<QPushButton*>("DeveloperLoginAccept");
        auto* error = dialog.findChild<QLabel*>("DeveloperLoginError");
        QVERIFY(username && password && login && error);
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        password->setText("wrong");
        login->click();
        QVERIFY(!AuthManager::instance().isAdminMode());
        QVERIFY(password->text().isEmpty());
        QVERIFY(!error->isHidden());
        dialog.reject();
        QVERIFY(!AuthManager::instance().isAdminMode());
    }

    void successfulDialogAuthentication() {
        DeveloperLoginDialog dialog;
        auto* password = dialog.findChild<QLineEdit*>("DeveloperPassword");
        auto* login = dialog.findChild<QPushButton*>("DeveloperLoginAccept");
        QVERIFY(password && login);
        password->setText("xidian");
        login->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QVERIFY(AuthManager::instance().isAdminMode());
        QVERIFY(password->text().isEmpty());
    }
};
QTEST_MAIN(AuthModeTest)
#include "authmode_test.moc"
