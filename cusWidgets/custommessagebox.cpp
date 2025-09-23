/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:16
 * @Description: 
 */
#include "custommessagebox.h"
#include <QGraphicsDropShadowEffect>
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
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(20, 20, 20, 20);
    containerLayout->setSpacing(10);

    m_titleLabel = new QLabel(container);
    m_titleLabel->setObjectName("MessageBoxTitle");
    containerLayout->addWidget(m_titleLabel);

    m_textLabel = new QLabel(container);
    m_textLabel->setWordWrap(true);
    m_textLabel->setObjectName("MessageBoxText");
    containerLayout->addWidget(m_textLabel);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(15);
    m_buttonLayout->addStretch();
    containerLayout->addLayout(m_buttonLayout);

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
    box.showDialog(title, text, Warning, OkCancel);
}
