/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:35
 * @Description: 
 */
#include "batterycontrol.h"
#include "ui_batterycontrol.h"
#include <QPushButton>

BatteryControl::BatteryControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatteryControl)
{
    ui->setupUi(this);
    setWindowTitle(tr("象限电源控制"));

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &BatteryControl::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &BatteryControl::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
}

void BatteryControl::onAccept()
{
    BatteryControlM param;
    if(ui->c1->checkState() == Qt::Unchecked)
        param.quadrant1 = 0;
    else
        param.quadrant1 = 1;

    if(ui->c2->checkState() == Qt::Unchecked)
        param.quadrant2 = 0;
    else
        param.quadrant2 = 1;

    if(ui->c3->checkState() == Qt::Unchecked)
        param.quadrant3 = 0;
    else
        param.quadrant3 = 1;

    if(ui->c4->checkState() == Qt::Unchecked)
        param.quadrant4 = 0;
    else
        param.quadrant4 = 1;

    emit setParam(param);

    // 不再关闭窗口，允许用户继续修改参数
}

void BatteryControl::onCancel()
{
    // 关闭父窗口（CusWindow）
    if (QWidget* parentWindow = window()) {
        parentWindow->close();
    }
}

void BatteryControl::restoreParam(const BatteryControlM &param)
{
    if(param.quadrant1 != 0)
        ui->c1->setChecked(true);
    else
        ui->c1->setChecked(false);

    if(param.quadrant2 != 0)
        ui->c2->setChecked(true);
    else
        ui->c2->setChecked(false);

    if(param.quadrant3 != 0)
        ui->c3->setChecked(true);
    else
        ui->c3->setChecked(false);

    if(param.quadrant4 != 0)
        ui->c4->setChecked(true);
    else
        ui->c4->setChecked(false);
}

BatteryControl::~BatteryControl()
{
    delete ui;
}
