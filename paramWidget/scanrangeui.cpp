/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:48
 * @Description: 
 */
#include "scanrangeui.h"
#include "ui_scanrangeui.h"
#include "Basic/ConfigManager.h"
#include "cusWidgets/custommessagebox.h"
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

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &ScanRangeUI::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ScanRangeUI::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 从配置文件加载默认值
    ui->workMode->setCurrentIndex(CF_INS.scanRangeWorkMode(0));

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &ScanRangeUI::onSaveToConfig);
    }
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

void ScanRangeUI::onSaveToConfig()
{
    // 弹出确认对话框
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前参数保存到配置文件？\n下次启动将自动加载这些参数。"))) {
        return;
    }

    // 获取当前UI参数
    unsigned char workMode = static_cast<unsigned char>(ui->workMode->currentIndex());

    // 保存到ConfigManager
    CF_INS.saveScanRangeParam(workMode);

    // 保存到文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("扫描范围参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}

ScanRangeUI::~ScanRangeUI()
{
    delete ui;
}
