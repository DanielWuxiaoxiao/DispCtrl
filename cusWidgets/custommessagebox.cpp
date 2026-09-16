/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:30
 * @Description: 
 */
#include "custommessagebox.h"
#include <QGraphicsDropShadowEffect>
#include <QIntValidator>
#include <QLineEdit>
#include <QTimer>
// 自定义消息框构造函数
CustomMessageBox::CustomMessageBox(QWidget *parent)  
    : QDialog(parent)
{
    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);

    setupUI();
    applyStyle();
}

void CustomMessageBox::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    QWidget *container = new QWidget(this);
    container->setObjectName("MessageBoxContainer");
    m_containerLayout = new QVBoxLayout(container);
    m_containerLayout->setContentsMargins(20, 20, 20, 20);
    m_containerLayout->setSpacing(10);

    m_titleLabel = new QLabel(container);
    m_titleLabel->setObjectName("MessageBoxTitle");
    m_containerLayout->addWidget(m_titleLabel);

    m_textLabel = new QLabel(container);
    m_textLabel->setWordWrap(true);
    m_textLabel->setObjectName("MessageBoxText");
    m_containerLayout->addWidget(m_textLabel);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(15);
    m_buttonLayout->addStretch();
    m_containerLayout->addLayout(m_buttonLayout);

    mainLayout->addWidget(container);

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 180));
    container->setGraphicsEffect(shadow);
}

void CustomMessageBox::applyStyle()
{
    setStyleSheet(R"(
        #MessageBoxContainer {
            background-color: rgba(20, 30, 30, 0.95);
            border: 2px solid #00ff88;
            border-radius: 12px;
        }
        #MessageBoxTitle {
            color: #00ff88;
            font-size: 16px;
            font-weight: bold;
        }
        #MessageBoxText {
            color: #ffffff;
            font-size: 14px;
        }
        #MessageBoxInput {
            min-height: 26px;
            padding: 4px 8px;
            color: #d9ffff;
            background-color: #071a1a;
            border: 1px solid #00bfa5;
            border-radius: 4px;
            selection-background-color: #087a75;
        }
        #MessageBoxInput:focus {
            border: 1px solid #00ffcc;
        }
        QPushButton {
            min-width: 80px;
            padding: 6px 12px;
            border-radius: 6px;
            border: 2px solid rgba(0, 255, 136, 0.4);
            background-color: transparent;
            color: #00ff88;
        }
        QPushButton:hover {
            border: 2px solid #00ff88;
            background-color: rgba(0, 255, 136, 0.2);
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: rgba(0, 255, 136, 0.4);
        }
    )");
}

int CustomMessageBox::showDialog(const QString &title, const QString &text,
                                  MessageType type, ButtonType buttons)
{
    m_titleLabel->setText(title);
    m_textLabel->setText(text);

    // 清空旧按钮
    QLayoutItem *item;
    while ((item = m_buttonLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_buttonLayout->addStretch();

    int result = QDialog::Rejected;

    auto addButton = [&](const QString &text, int dialogResult) {
        QPushButton *btn = new QPushButton(text, this);
        m_buttonLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, dialogResult, &result]() {
            result = dialogResult;
            accept();
        });
    };

    if (buttons == Ok) {
        addButton("确定", QDialog::Accepted);
    } else if (buttons == OkCancel) {
        addButton("确定", QDialog::Accepted);
        addButton("取消", QDialog::Rejected);
    } else if (buttons == YesNo) {
        addButton("是", QDialog::Accepted);
        addButton("否", QDialog::Rejected);
    }

    exec();
    return result;
}

bool CustomMessageBox::showConfirm(QWidget *parent, const QString &title, const QString &text)
{
    CustomMessageBox box(parent);
    return box.showDialog(title, text, Confirm, YesNo) == QDialog::Accepted;
}

void CustomMessageBox::showInfo(QWidget *parent, const QString &title, const QString &text)
{
    CustomMessageBox box(parent);
    box.showDialog(title, text, Info, Ok);
}

void CustomMessageBox::showWarning(QWidget *parent, const QString &title, const QString &text)
{
    CustomMessageBox box(parent);
    box.showDialog(title, text, Warning, Ok);
}

bool CustomMessageBox::getInteger(QWidget *parent, const QString &title, const QString &prompt,
                                  int minimum, int maximum, int &value)
{
    if (minimum > maximum) {
        return false;
    }

    CustomMessageBox box(parent);
    box.m_titleLabel->setText(title);
    box.m_textLabel->setText(prompt);

    auto *input = new QLineEdit(&box);
    input->setObjectName(QStringLiteral("MessageBoxInput"));
    input->setValidator(new QIntValidator(minimum, maximum, input));
    input->setText(QString::number(qBound(minimum, value, maximum)));
    input->selectAll();
    box.m_containerLayout->insertWidget(box.m_containerLayout->count() - 1, input);

    QLayoutItem *item = nullptr;
    while ((item = box.m_buttonLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    box.m_buttonLayout->addStretch();

    int result = QDialog::Rejected;
    auto *confirm = new QPushButton(QStringLiteral("确定"), &box);
    auto *cancel = new QPushButton(QStringLiteral("取消"), &box);
    box.m_buttonLayout->addWidget(confirm);
    box.m_buttonLayout->addWidget(cancel);
    const auto acceptInput = [&box, input, &result, &value]() {
        if (!input->hasAcceptableInput()) {
            input->setFocus();
            return;
        }
        value = input->text().toInt();
        result = QDialog::Accepted;
        box.accept();
    };
    connect(confirm, &QPushButton::clicked, &box, acceptInput);
    connect(cancel, &QPushButton::clicked, &box, [&box]() { box.reject(); });
    connect(input, &QLineEdit::returnPressed, &box, acceptInput);
    QTimer::singleShot(0, input, [input]() { input->setFocus(); });

    box.exec();
    return result == QDialog::Accepted;
}
