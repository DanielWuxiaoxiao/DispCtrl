/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:36
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

    emit setParam(param);
    parentWidget()->close();
}

void TranRecvUI::onCancel()
{
    parentWidget()->close();
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
}

TranRecvUI::~TranRecvUI()
{
    delete ui;
}
