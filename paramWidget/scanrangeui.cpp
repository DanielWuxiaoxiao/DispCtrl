/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 11:44:24
 * @Description: 
 */
#include "scanrangeui.h"
#include "ui_scanrangeui.h"
#include <QPushButton>

ScanRangeUI::ScanRangeUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScanRangeUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("搜索范围控制"));
        // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    ui->eleend->setVisible(false);
    ui->elestart->setVisible(false);
    ui->label_7->setVisible(false);
    ui->label_8->setVisible(false);
    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ScanRangeUI::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ScanRangeUI::onCancel);
}

void ScanRangeUI::onAccept()
{
    ScanRange param;
    param.place = ui->place->currentIndex();
    param.method = ui->scan->currentIndex();
    param.workMode = ui->workMode->currentIndex();
    param.azi = (ui->dir->text()).toFloat()/0.01f;
    param.ele = (ui->ele->text()).toFloat()/0.01f;

    emit setParam(param);
    parentWidget()->close();
}

void ScanRangeUI::onCancel()
{
    parentWidget()->close();
}


void ScanRangeUI::restoreParam(const ScanRange &param)
{
    ui->place->setCurrentIndex(param.place);
    ui->scan->setCurrentIndex(param.method);
    ui->workMode->setCurrentIndex(param.workMode);
    ui->dir->setText(QString::number(param.azi*0.01f));
    ui->ele->setText(QString::number(param.ele*0.01f));
}

ScanRangeUI::~ScanRangeUI()
{
    delete ui;
}
