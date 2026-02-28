/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:33
 * @Description: 
 */
#include "dataprocessui.h"
#include "ui_dataprocessui.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "cusWidgets/custommessagebox.h"
#include <QPushButton>
#include <QDebug>

DataProcessUI::DataProcessUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataProcessUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("数据处理参数"));
    // 窗口居中显示
    centerWidgetOnScreen(this);

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DataProcessUI::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DataProcessUI::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &DataProcessUI::onSaveToConfig);
    }
}


void DataProcessUI::onAccept()
{
    qDebug() << "[DataProcessUI] onAccept() called";
    DataProParam param;
    param.startWinLen = ui->batchwinlen->text().toFloat();
    param.startPoint = ui->batchnum->text().toFloat();
    param.endWinLen = ui->endlen->text().toFloat();
    param.endPoint = ui->endnum->text().toFloat();
    param.noiseVar = ui->noisevar->text().toFloat()*100;
    param.trackDisLower = ui->trackdown->text().toFloat()*10;
    param.trackDisUpper = ui->trackup->text().toFloat()*10;
    param.trackAziThresh = ui->trackazithr->text().toFloat()*10;
    param.trackEleThresh = ui->trackelethr->text().toFloat()*10;
    param.trackVelThresh = ui->trackvelthr->text().toFloat()*10;
    param.trackStatThresh = ui->trackgatedisthr->text().toFloat()*10;
    param.accuDisGate = ui->disgate->text().toFloat();
    param.accuAziGate = ui->azigate->text().toFloat()*10;
    param.accuEleGate = ui->elegate->text().toFloat()*10;
    param.accuVelGate = ui->dopgate->text().toFloat()*10;

    qDebug() << "[DataProcessUI] emitting setParam signal";
    emit setParam(param);
    // 保持窗口与布局，不关闭父窗口
}

void DataProcessUI::onCancel()
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

/**
 * @brief Restores and displays the given data processing parameters in the UI.
 *
 * This function takes a DataProParam structure containing various data processing
 * parameters and updates the corresponding UI fields with their values. Some parameters
 * are converted or scaled before being displayed to the user.
 *
 * @param param The DataProParam structure containing the parameters to restore.
 */
void DataProcessUI::restoreParam(const DataProParam &param)
{
    ui->batchwinlen->setText(QString::number(param.startWinLen));
    ui->batchnum->setText(QString::number(param.startPoint));
    ui->endlen->setText(QString::number(param.endWinLen));
    ui->endnum->setText(QString::number(param.endPoint));
    ui->noisevar->setText(QString::number(float(param.noiseVar)*0.01f));
    ui->trackdown->setText(QString::number(float(param.trackDisLower)*0.1f));
    ui->trackup->setText(QString::number(float(param.trackDisUpper)*0.1f));
    ui->trackazithr->setText(QString::number(float(param.trackAziThresh)*0.1f));
    ui->trackelethr->setText(QString::number(float(param.trackEleThresh)*0.1f));
    ui->trackvelthr->setText(QString::number(float(param.trackVelThresh)*0.1f));
    ui->trackgatedisthr->setText(QString::number(float(param.trackStatThresh)*0.1f));
    ui->disgate->setText(QString::number(param.accuDisGate));
    ui->azigate->setText(QString::number(float(param.accuAziGate)*0.1f));
    ui->elegate->setText(QString::number(float(param.accuEleGate)*0.1f));
    ui->dopgate->setText(QString::number(float(param.accuVelGate)*0.1f));
}

void DataProcessUI::onSaveToConfig()
{
    // 弹出确认对话框
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前数据处理参数保存到配置文件？\n下次启动将自动加载这些参数。"))) {
        return;
    }

    // 提取UI值（必须与onAccept()保持完全一致的转换逻辑）
    CF_INS.saveDataProParam(
        ui->batchwinlen->text().toFloat(),              // startWinLen (unsigned char)
        ui->batchnum->text().toFloat(),                 // startPoint (unsigned char)
        ui->endlen->text().toFloat(),                   // endWinLen (unsigned char)
        ui->endnum->text().toFloat(),                   // endPoint (unsigned char)
        ui->noisevar->text().toFloat() * 100,           // noiseVar (unsigned short)
        ui->trackdown->text().toFloat() * 10,           // trackDisLower (unsigned short)
        ui->trackup->text().toFloat() * 10,             // trackDisUpper (unsigned short)
        ui->trackazithr->text().toFloat() * 10,         // trackAziThresh (unsigned short)
        ui->trackelethr->text().toFloat() * 10,         // trackEleThresh (unsigned short)
        ui->trackvelthr->text().toFloat() * 10,         // trackVelThresh (unsigned short)
        ui->trackgatedisthr->text().toFloat() * 10,     // trackStatThresh (unsigned short)
        ui->disgate->text().toFloat(),                  // accuDisGate (unsigned char) - 不乘10
        ui->azigate->text().toFloat() * 10,             // accuAziGate (unsigned char) - 乘10
        ui->elegate->text().toFloat() * 10,             // accuEleGate (unsigned char) - 乘10
        ui->dopgate->text().toFloat() * 10              // accuVelGate (unsigned char) - 乘10
    );

    // 保存到文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("数据处理参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}

DataProcessUI::~DataProcessUI()
{
    delete ui;
}
