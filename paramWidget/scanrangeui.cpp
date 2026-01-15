/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
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

    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ScanRangeUI::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ScanRangeUI::onCancel);
}

void ScanRangeUI::onAccept()
{
    ScanRange param;

    param.workMode = ui->workMode->currentIndex();


    emit setParam(param);
    // 保持窗口与布局，不关闭父窗口
}

void ScanRangeUI::onCancel()
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


void ScanRangeUI::restoreParam(const ScanRange &param)
{

    ui->workMode->setCurrentIndex(param.workMode);

}

ScanRangeUI::~ScanRangeUI()
{
    delete ui;
}
