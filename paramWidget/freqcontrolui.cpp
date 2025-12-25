/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 16:19:35
 * @Description: 
 */
#include "freqcontrolui.h"
#include "ui_freqcontrolui.h"
#include <QPushButton>

static unsigned char timeWidths[] = {2,2,4,10,20,35,2,2,2,10,25,35};
static unsigned char PRTs[] = {40,25,20,50,100,175,15,30,40,50,125,175};

FreqControlUI::FreqControlUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FreqControlUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("频综控制"));
        // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    connect(ui->wave,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[this](int index){
        auto timeWidth = timeWidths[index];
        auto PRT = PRTs[index];
        ui->samplestart->setText(QString::number(ui->transtart->text().toFloat()+timeWidth+1.0f));
        ui->sampleend->setText(QString::number(float(PRT-1)));
    });
    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &FreqControlUI::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &FreqControlUI::onCancel);
}


void FreqControlUI::onAccept()
{
    DirGramScan param;
    param.gramControl = ui->dircontrol->currentIndex();
    param.waveID = ui->wave->currentIndex();
    param.scanStart = ui->scanstart->text().toFloat()/0.01f;
    param.scanEnd = ui->scanend->text().toFloat()/0.01f;
    param.scanStep = ui->scanstep->text().toFloat()/0.01f;
    param.tranStart = ui->transtart->text().toFloat()/0.1f;
    param.sampleLen = ui->sampleend->text().toFloat()/0.1f - ui->samplestart->text().toFloat()/0.1f;
    param.sampleStart =  ui->samplestart->text().toFloat()/0.1f;
    emit setParam(param);
    parentWidget()->close();
}

void FreqControlUI::onCancel()
{
    parentWidget()->close();
}


void FreqControlUI::restoreParam(const DirGramScan &param)
{
    ui->dircontrol->setCurrentIndex(param.gramControl);
    ui->wave->setCurrentIndex(param.waveID);
    ui->scanstart->setText(QString::number(param.scanStart*0.01f));
    ui->scanend->setText(QString::number(param.scanEnd*0.01f));
    ui->scanstep->setText(QString::number(param.scanStep*0.01f));
    ui->transtart->setText(QString::number(param.tranStart*0.1f));

    ui->samplestart->setText(QString::number(param.sampleStart*0.1f));
    ui->sampleend->setText(QString::number((param.sampleStart + param.sampleLen)*0.1f));
}

FreqControlUI::~FreqControlUI()
{
    delete ui;
}
