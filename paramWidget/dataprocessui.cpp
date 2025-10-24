/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:35
 * @Description: 
 */
﻿#include "dataprocessui.h"
#include "ui_dataprocessui.h"
#include <QPushButton>

DataProcessUI::DataProcessUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataProcessUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("数据处理参数"));
        // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &DataProcessUI::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DataProcessUI::onCancel);
}


void DataProcessUI::onAccept()
{
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

    emit setParam(param);
    parentWidget()->close();
}

void DataProcessUI::onCancel()
{
    parentWidget()->close();
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

DataProcessUI::~DataProcessUI()
{
    delete ui;
}
