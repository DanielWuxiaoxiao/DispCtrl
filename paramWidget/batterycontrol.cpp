/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:58
 * @Description: 
 */
#include "batterycontrol.h"
#include "ui_batterycontrol.h"
#include "Basic/DispBasci.h"
#include <QPushButton>

BatteryControl::BatteryControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatteryControl)
{
    ui->setupUi(this);
    setWindowTitle(tr("阵面开启控制"));
    // 窗口居中显示
    centerWidgetOnScreen(this);

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 自定义按钮信号，与 ServoControl 保持一致
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &BatteryControl::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &BatteryControl::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
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
    // 向上查找 CusWindow 父窗口并关闭
    QWidget* w = this;
    while (w) {
        // 通过 objectName 或类名查找 CusWindow
        if (w->objectName() == "CusWindow") {
            w->close();
            return;
        }
        w = w->parentWidget();
    }
    // 如果没找到，关闭自身
    close();
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
