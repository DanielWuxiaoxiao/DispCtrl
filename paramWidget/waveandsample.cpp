/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:16
 * @Description: 
 */
#include "waveandsample.h"
#include "ui_waveandsample.h"
#include <QPushButton>

waveAndSample::waveAndSample(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::waveAndSample)
{
    ui->setupUi(this);
    setFixedSize(1200,600);

    setWindowTitle(tr("波形及采样控制"));

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &waveAndSample::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &waveAndSample::onCancel);

    connect(ui->wave1,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](int index){
        auto timeWidth = timeWidths[index];
        auto PRT = PRTs[index];
        // 采样起始 = 1.0us(固定发射起始) + 时宽 + 1.0us
        ui->samplestart1->setText(QString::number(1.0f + timeWidth + 1.0f));
        ui->samplelen1->setText(QString::number(float(PRT-1.0f)));  //采样终止 = 重复周期-1.0f
    });

    connect(ui->wave2,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](int index){
        auto timeWidth = timeWidths[index];
        auto PRT = PRTs[index];
        ui->samplestart2->setText(QString::number(1.0f + timeWidth + 1.0f));
        ui->samplelen2->setText(QString::number(float(PRT-1.0f)));
    });

    connect(ui->wave3,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](int index){
        auto timeWidth = timeWidths[index];
        auto PRT = PRTs[index];
        ui->samplestart3->setText(QString::number(1.0f + timeWidth + 1.0f));
        ui->samplelen3->setText(QString::number(float(PRT-1.0f)));
    });

}

void waveAndSample::onAccept()
{
    BeamControl param;
    param.flagNum = 0;

    if(ui->enable1->checkState() == 0)
    {
        param.beam1Flag = 0;
    }
    else
    {
        param.beam1Flag = 1;
        param.flagNum++;
    }

    if(ui->enable2->checkState() == 0)
    {
        param.beam2Flag = 0;
    }
    else
    {
        param.beam2Flag = 1;
        param.flagNum++;
    }

    param.freqID = ui->freq->currentIndex() * 10;  // 0-8 映射到 0-80 的频点值
    param.type = ui->type->currentIndex() + 1;

    param.aziStart = ui->azistart->text().toFloat() / 0.01f;
    param.aziEnd = ui->aziend->text().toFloat() / 0.01f;
    param.aziStep = ui->azistep->text().toFloat() / 0.01f;

    if(ui->enable3->checkState() == 0)
    {
        param.beam3Flag = 0;
    }
    else
    {
        param.beam3Flag = 1;
        param.flagNum++;
    }

    param.beam1Code = ui->wave1->currentIndex();
    param.beam2Code = ui->wave2->currentIndex();
    param.beam3Code = ui->wave3->currentIndex();

    param.pulseNum1 = ui->pulseNum1->currentText().toUInt();
    param.pulseNum2 = ui->pulseNum2->currentText().toUInt();
    param.pulseNum3 = ui->pulseNum3->currentText().toUInt();

    param.sampleStart1 = ui->samplestart1->text().toFloat()/0.1f;
    param.sampleStart2 = ui->samplestart2->text().toFloat()/0.1f;
    param.sampleStart3 = ui->samplestart3->text().toFloat()/0.1f;

    param.sampleEnd1 = ui->samplelen1->text().toFloat()/0.1f;  // UI中的samplelen实际是采样终止
    param.sampleEnd2 = ui->samplelen2->text().toFloat()/0.1f;
    param.sampleEnd3 = ui->samplelen3->text().toFloat()/0.1f;

    param.elestart1 = ui->elestart1->text().toFloat()/0.01f;
    param.elestart2 = ui->elestart2->text().toFloat()/0.01f;
    param.elestart3 = ui->elestart3->text().toFloat()/0.01f;

    param.eleend1 = ui->eleend1->text().toFloat()/0.01f;
    param.eleend2 = ui->eleend2->text().toFloat()/0.01f;
    param.eleend3 = ui->eleend3->text().toFloat()/0.01f;

    param.elestep1 = ui->elestep1->text().toFloat()/0.01f;
    param.elestep2 = ui->elestep2->text().toFloat()/0.01f;
    param.elestep3 = ui->elestep3->text().toFloat()/0.01f;
    emit setParam(param);
    // 保持窗口与布局，不关闭父窗口
}

void waveAndSample::onCancel()
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

//根据默认结构体的值，来实现界面的默认值
void waveAndSample::restoreParam(const BeamControl &param)
{
    if(param.beam1Flag == 0)
        ui->enable1->setCheckState(Qt::Unchecked);
    else
        ui->enable1->setCheckState(Qt::Checked);

    if(param.beam2Flag == 0)
        ui->enable2->setCheckState(Qt::Unchecked);
    else
        ui->enable2->setCheckState(Qt::Checked);

    if(param.beam3Flag == 0)
        ui->enable3->setCheckState(Qt::Unchecked);
    else
        ui->enable3->setCheckState(Qt::Checked);

    //获取保存的�?
    ui->samplestart1->setText(QString::number(param.sampleStart1*0.1f));
    ui->samplestart2->setText(QString::number(param.sampleStart2*0.1f));
    ui->samplestart3->setText(QString::number(param.sampleStart3*0.1f));

    ui->wave1->setCurrentIndex(param.beam1Code);
    ui->wave2->setCurrentIndex(param.beam2Code);
    ui->wave3->setCurrentIndex(param.beam3Code);

    //此时不会触发combobox的函数，必须在默认值结构体里写出这个�?
//    auto timeWidth = timeWidths[param.beam1Code];
//    auto PRT = PRTs[param.beam1Code];
//    ui->samplestart1->setText(QString::number(ui->tran1->text().toFloat()+timeWidth+1.0f));
//    ui->samplelen1->setText(QString::number(float(PRT-1))); //ui的samplelen实际是终止�?

//    timeWidth = timeWidths[param.beam2Code];
//    PRT = PRTs[param.beam2Code];
//    ui->samplestart2->setText(QString::number(ui->tran2->text().toFloat()+timeWidth+1.0f));
//    ui->samplelen2->setText(QString::number(float(PRT-1)));

//    timeWidth = timeWidths[param.beam3Code];
//    PRT = PRTs[param.beam3Code];
//    ui->samplestart3->setText(QString::number(ui->tran3->text().toFloat()+timeWidth+1.0f));
//    ui->samplelen3->setText(QString::number(float(PRT-1)));

    ui->freq->setCurrentIndex(param.freqID / 10);  // 0-80 映射回 0-8 的索引
    ui->type->setCurrentIndex(param.type - 1);
    ui->azistart->setText(QString::number(param.aziStart*0.01f));
    ui->aziend->setText(QString::number(param.aziEnd*0.01f));
    ui->azistep->setText(QString::number(param.aziStep*0.01f));

    ui->pulseNum1->setCurrentText(QString::number(param.pulseNum1));
    ui->pulseNum2->setCurrentText(QString::number(param.pulseNum2));
    ui->pulseNum3->setCurrentText(QString::number(param.pulseNum3));

    //获取保存的值 - UI中的samplelen实际显示的是采样终止
    ui->samplelen1->setText(QString::number(param.sampleEnd1*0.1f));
    ui->samplelen2->setText(QString::number(param.sampleEnd2*0.1f));
    ui->samplelen3->setText(QString::number(param.sampleEnd3*0.1f));

    ui->elestart1->setText(QString::number(param.elestart1*0.01f));
    ui->elestart2->setText(QString::number(param.elestart2*0.01f));
    ui->elestart3->setText(QString::number(param.elestart3*0.01f));

    ui->eleend1->setText(QString::number(param.eleend1*0.01f));
    ui->eleend2->setText(QString::number(param.eleend2*0.01f));
    ui->eleend3->setText(QString::number(param.eleend3*0.01f));

    ui->elestep1->setText(QString::number(param.elestep1*0.01f));
    ui->elestep2->setText(QString::number(param.elestep2*0.01f));
    ui->elestep3->setText(QString::number(param.elestep3*0.01f));
}

waveAndSample::~waveAndSample()
{
    delete ui;
}
