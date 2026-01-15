/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-09 15:09:57
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#include "servocontrol.h"
#include "ui_servocontrol.h"
#include <QPushButton>
#include <algorithm>

ServoControl::ServoControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServoControl)
{
    ui->setupUi(this);
    setWindowTitle(tr("伺服控制"));

    // 自定义按钮行为
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ServoControl::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ServoControl::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    ui->cmdCombo->addItem(tr("方位停转"), 0);
    ui->cmdCombo->addItem(tr("方位转动"), 1);
    ui->cmdCombo->addItem(tr("方位寻位"), 2);
    ui->cmdCombo->addItem(tr("方位归北"), 3);

    ui->speedSpin->setRange(0, 255);
    ui->speedSpin->setValue(10);
    ui->angleSpin->setRange(0.0, 360.0);
    ui->angleSpin->setDecimals(2);
    ui->angleSpin->setSingleStep(0.1);
}

ServoControl::~ServoControl()
{
    delete ui;
}

void ServoControl::restoreParam(const ServoControlParam &param)
{
    ui->cmdCombo->setCurrentIndex(std::clamp<int>(param.cmd, 0, ui->cmdCombo->count() - 1));
    ui->speedSpin->setValue(param.speed);
    ui->angleSpin->setValue(static_cast<double>(param.az) / 100.0);
}

void ServoControl::onAccept()
{
    ServoControlParam param;
    param.cmd = static_cast<unsigned char>(ui->cmdCombo->currentData().toInt());
    param.speed = static_cast<unsigned char>(ui->speedSpin->value());
    double angleDeg = ui->angleSpin->value();
    int angle = static_cast<int>(angleDeg * 100.0 + 0.5);
    angle = std::clamp(angle, 0, 36000);
    param.az = static_cast<unsigned short>(angle);

    emit setParam(param);
    // 不关闭窗口，便于多次下发
}

void ServoControl::onCancel()
{
    // 向上查找 CusWindow 父窗口并关闭
    QWidget* w = this;
    while (w) {
        if (w->objectName() == "CusWindow") {
            w->close();
            return;
        }
        w = w->parentWidget();
    }
    close();
}
