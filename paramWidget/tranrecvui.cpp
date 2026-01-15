/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#include "tranrecvui.h"
#include "ui_tranrecvui.h"
#include <QPushButton>

TranRecvUI::TranRecvUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranRecvUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("发射接收控制"));

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &TranRecvUI::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &TranRecvUI::onCancel);
}

void TranRecvUI::onAccept()
{
    TranRecControl param;
    if(ui->r->checkState() == Qt::Unchecked)
        param.recv = 0;
    else
        param.recv = 1;

    if(ui->t->checkState() == Qt::Unchecked)
        param.tran = 0;
    else
        param.tran = 1;

    // 角度以 0.01° 量化
    param.tranStart = static_cast<unsigned short>(ui->spinStart->value());
    param.tranEnd = static_cast<unsigned short>(ui->spinEnd->value());

    emit setParam(param);
    // 保持窗口与布局，不关闭父窗口
}

void TranRecvUI::onCancel()
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


void TranRecvUI::restoreParam(const TranRecControl &param)
{
    if(param.recv != 0)
        ui->r->setChecked(true);
    else
        ui->r->setChecked(false);

    if(param.tran != 0)
        ui->t->setChecked(true);
    else
        ui->t->setChecked(false);

    ui->spinStart->setValue(param.tranStart);
    ui->spinEnd->setValue(param.tranEnd);
}

TranRecvUI::~TranRecvUI()
{
    delete ui;
}
