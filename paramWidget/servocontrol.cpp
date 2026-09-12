/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-15 14:23:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:59
 * @Description: 
 */
#include "servocontrol.h"
#include "ui_servocontrol.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "cusWidgets/custommessagebox.h"
#include <QPushButton>
#include <algorithm>

ServoControl::ServoControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServoControl)
{
    ui->setupUi(this);
    setWindowTitle(tr("伺服控制"));
    // 窗口居中显示
    centerWidgetOnScreen(this);

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
    ui->angleSpin->setRange(0.0, 360.0);
    ui->angleSpin->setDecimals(2);
    ui->angleSpin->setSingleStep(0.1);

    // 从配置文件加载默认值
    LOG_INFO("ServoControl: Loading from ConfigManager...");
    LOG_INFO(QString("  Config file path: %1").arg(CF_INS.getConfigFilePath()));
    LOG_INFO(QString("  servoCmd: %1").arg(CF_INS.servoCmd(0)));
    LOG_INFO(QString("  servoSpeed: %1").arg(CF_INS.servoSpeed(10)));
    LOG_INFO(QString("  servoAz: %1").arg(CF_INS.servoAz(0)));

    ui->cmdCombo->setCurrentIndex(CF_INS.servoCmd(0));
    ui->speedSpin->setValue(CF_INS.servoSpeed(10));
    ui->angleSpin->setValue(CF_INS.servoAz(0) / 100.0);

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &ServoControl::onSaveToConfig);
    }
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

void ServoControl::onSaveToConfig()
{
    // 弹出确认对话框
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前参数保存到配置文件？\n下次启动将自动加载这些参数。"))) {
        return;
    }

    // 获取当前UI参数
    unsigned char cmd = static_cast<unsigned char>(ui->cmdCombo->currentData().toInt());
    unsigned char speed = static_cast<unsigned char>(ui->speedSpin->value());
    double angleDeg = ui->angleSpin->value();
    int angle = static_cast<int>(angleDeg * 100.0 + 0.5);
    angle = std::clamp(angle, 0, 36000);
    unsigned short az = static_cast<unsigned short>(angle);

    // 保存到ConfigManager
    CF_INS.saveServoParam(cmd, speed, az);

    // 保存到文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("伺服控制参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}
