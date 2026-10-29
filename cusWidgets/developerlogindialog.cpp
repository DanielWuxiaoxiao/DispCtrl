#include "developerlogindialog.h"
#include "Basic/authmanager.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "Basic/uifontscale.h"

DeveloperLoginDialog::DeveloperLoginDialog(QWidget* parent) : QDialog(parent)
{
    setObjectName(QStringLiteral("DeveloperLoginDialog"));
    setWindowTitle(QStringLiteral("开发者模式验证"));
    setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    setWindowModality(Qt::ApplicationModal);
    setMinimumWidth(420);
    setStyleSheet(UiFontScale::style(QStringLiteral(
        "QDialog#DeveloperLoginDialog { background:#101818; }"
        "QDialog#DeveloperLoginDialog QLabel { color:#D9FFFF; font-size:18px; }"
        "QDialog#DeveloperLoginDialog QLabel#DeveloperLoginError { color:#FF9B9B; }"
        "QDialog#DeveloperLoginDialog QLineEdit { color:#FFFFFF; background:#142D28; font-size:18px; "
        "border:1px solid #66CCAA; border-radius:4px; padding:6px; }"
        "QDialog#DeveloperLoginDialog QPushButton { color:#66FFCC; background:#123630; font-size:18px; "
        "border:1px solid #66FFCC; border-radius:5px; padding:6px 16px; min-height:28px; }"
        "QDialog#DeveloperLoginDialog QPushButton:hover { background:#195046; }")));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    auto* form = new QFormLayout;
    form->setSpacing(12);
    auto* username = new QLineEdit(this);
    username->setObjectName(QStringLiteral("DeveloperUsername"));
    username->setText(QStringLiteral("admin"));
    username->setMaxLength(64);
    auto* password = new QLineEdit(this);
    password->setObjectName(QStringLiteral("DeveloperPassword"));
    password->setEchoMode(QLineEdit::Password);
    password->setMaxLength(128);
    form->addRow(QStringLiteral("用户名"), username);
    form->addRow(QStringLiteral("密码"), password);
    layout->addLayout(form);
    auto* error = new QLabel(this);
    error->setObjectName(QStringLiteral("DeveloperLoginError"));
    error->setWordWrap(true);
    error->hide();
    layout->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* login = buttons->button(QDialogButtonBox::Ok);
    login->setText(QStringLiteral("进入开发者模式"));
    login->setDefault(true);
    login->setObjectName(QStringLiteral("DeveloperLoginAccept"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [this, username, password, error] {
        if (AuthManager::instance().enterDeveloperMode(username->text(), password->text())) {
            password->clear();
            accept();
        } else {
            password->clear();
            error->setText(QStringLiteral("用户名或密码错误，请重试"));
            error->show();
            password->setFocus();
        }
    });
    password->setFocus();
}
