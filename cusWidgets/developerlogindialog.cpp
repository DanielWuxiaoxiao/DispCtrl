#include "Basic/uitheme.h"
#include "developerlogindialog.h"
#include "Basic/authmanager.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "Basic/uifontscale.h"

DeveloperLoginDialog::DeveloperLoginDialog(QWidget* parent) : QDialog(parent)
{
    if (UiTheme::classicEnabled) {
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
        return;
    }

    setObjectName(QStringLiteral("DeveloperLoginDialog"));
    setWindowTitle(QStringLiteral("开发者模式验证"));
    setWindowIcon((UiTheme::classicEnabled ? QIcon(":/resources/icon/radararray.png") : QIcon(":/resources/icon/ui/radar.svg")));
    setWindowModality(Qt::ApplicationModal);
    setMinimumWidth(ScaleHelper::uiScaled(480));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* header = new QFrame(this);
    header->setObjectName(QStringLiteral("DeveloperLoginHeader"));
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(28, 24, 28, 22);
    headerLayout->setSpacing(6);
    auto* title = new QLabel(QStringLiteral("开发者模式验证"), header);
    title->setObjectName(QStringLiteral("DeveloperLoginTitle"));
    auto* subtitle = new QLabel(QStringLiteral("验证后可使用开发者设置"), header);
    subtitle->setObjectName(QStringLiteral("DeveloperLoginSubtitle"));
    subtitle->setWordWrap(true);
    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    layout->addWidget(header);
    auto* bodyLayout = new QVBoxLayout;
    bodyLayout->setContentsMargins(28, 24, 28, 24);
    bodyLayout->setSpacing(18);
    layout->addLayout(bodyLayout);
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
    bodyLayout->addLayout(form);
    auto* error = new QLabel(this);
    error->setObjectName(QStringLiteral("DeveloperLoginError"));
    error->setWordWrap(true);
    error->hide();
    bodyLayout->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* login = buttons->button(QDialogButtonBox::Ok);
    login->setText(QStringLiteral("进入开发者模式"));
    login->setDefault(true);
    login->setObjectName(QStringLiteral("DeveloperLoginAccept"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    bodyLayout->addWidget(buttons);
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
    setStyleSheet(UiFontScale::style(QString(
        (UiTheme::classicEnabled ? "QDialog#DeveloperLoginDialog { background:#101818; }"
        "QDialog#DeveloperLoginDialog QLabel { color:#D9FFFF; font-size:18px; }"
        "QDialog#DeveloperLoginDialog QLabel#DeveloperLoginError { color:#FF9B9B; }"
        "QDialog#DeveloperLoginDialog QLineEdit { color:#FFFFFF; background:#142D28; font-size:18px; "
        "border:1px solid #66CCAA; border-radius:4px; padding:6px; }"
        "QDialog#DeveloperLoginDialog QPushButton { color:#66FFCC; background:#123630; font-size:18px; "
        "border:1px solid #66FFCC; border-radius:5px; padding:6px 16px; min-height:28px; }"
        "QDialog#DeveloperLoginDialog QPushButton:hover { background:#195046; }" : "QDialog#DeveloperLoginDialog { background:#12171c; }"
        "QDialog#DeveloperLoginDialog QFrame#DeveloperLoginHeader { background:#1b2228; border:none; border-bottom:1px solid #303b44; }"
        "QDialog#DeveloperLoginDialog QLabel { color:#a6b2bd; font-size:16px; background:transparent; }"
        "QDialog#DeveloperLoginDialog QLabel#DeveloperLoginTitle { color:#e8edf1; font-size:22px; font-weight:bold; }"
        "QDialog#DeveloperLoginDialog QLabel#DeveloperLoginSubtitle { color:#a6b2bd; font-size:14px; }"
        "QDialog#DeveloperLoginDialog QLabel#DeveloperLoginError { color:#ea8c93; background:#35252d; border-left:3px solid #ea8c93; padding:10px; }"
        "QDialog#DeveloperLoginDialog QLineEdit { color:#e8edf1; background:#1b2228; font-size:18px; "
        "border:1px solid #3b4a55; border-radius:4px; padding:8px; selection-background-color:#2e4b57; selection-color:#ffffff; }"
        "QDialog#DeveloperLoginDialog QLineEdit:focus { border-color:#78cbd8; }"
        "QDialog#DeveloperLoginDialog QPushButton { color:#e8edf1; background:#222c33; font-size:16px; "
        "border:1px solid #3b4a55; border-radius:4px; padding:8px 16px; min-height:28px; }"
        "QDialog#DeveloperLoginDialog QPushButton:hover { background:#293b44; border-color:#78cbd8; }"
        "QDialog#DeveloperLoginDialog QPushButton:focus { border-color:#78cbd8; }"
        "QDialog#DeveloperLoginDialog QPushButton:pressed { background:#23414a; }"
        "QDialog#DeveloperLoginDialog QPushButton#DeveloperLoginAccept { color:#c2e8ed; background:#23414a; border-color:#78cbd8; }"
        "QDialog#DeveloperLoginDialog QPushButton#DeveloperLoginAccept:hover { background:#2e4b57; }"
        "QDialog#DeveloperLoginDialog QPushButton:disabled { color:#8e9ba6; background:#1b232a; border-color:#303b44; }"))));
    password->setFocus();
}
