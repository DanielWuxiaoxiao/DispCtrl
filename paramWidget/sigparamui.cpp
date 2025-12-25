/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 11:44:24
 * @Description: 
 */
#include "sigparamui.h"
#include "ui_sigparamui.h"
#include <QPushButton>

sigParamUI::sigParamUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sigParamUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("信号处理参数"));
        // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定下发");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    // 连接按钮信号到自定义槽

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &sigParamUI::onAccept);
    // 连接按钮信号到自定义槽
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &sigParamUI::onCancel);
}

void sigParamUI::onAccept()
{
    SigProParam param;
    param.noise = ui->noise->text().toFloat()/0.1f;
    param.thresh1 = ui->firstthresh->text().toFloat()/0.1f;
    param.thresh2 = ui->secondthre->text().toFloat()/0.1f;
    param.clutterThresh = ui->clutterthresh->text().toFloat()/0.1f;
    param.clutterMapFlaseRate = ui->clutterfalse->currentIndex();
    param.CFARType = ui->cfartype->currentIndex();
    param.disProWin = ui->dispro->text().toShort();
    param.disRefWin = ui->disref->text().toShort();
    param.dopProWin = ui->doppro->text().toShort();
    param.dopRefWin = ui->dopref->text().toShort();
    param.MTDWinType = ui->mtdtype->currentIndex();
    param.clutterMode = ui->cluttertype->currentIndex();
    param.clutterChannelWidth = ui->clutterwid->text().toShort();
    param.clutterUnitWin = ui->clutterrefresh->text().toShort();
    param.clutterIter = ui->clutteriter->text().toShort();
    if(ui->cluttersense->checkState() == Qt::Checked)
        param.algorithmSwitch |= 1;
    else
        param.algorithmSwitch &= ~1;

    if(ui->sidelobehide->checkState() == Qt::Checked)
        param.algorithmSwitch |= (1 << 1);
    else
        param.algorithmSwitch &= ~(1 << 1);

    if(ui->jinqubumang->checkState() == Qt::Checked)
        param.algorithmSwitch |= (1 << 2);
    else
        param.algorithmSwitch &= ~(1 << 2);

    emit setParam(param);
    parentWidget()->close();
}

void sigParamUI::onCancel()
{
    parentWidget()->close();
}


void sigParamUI::restoreParam(const SigProParam &param)
{
    ui->noise->setText(QString::number(param.noise*0.1f));
    ui->firstthresh->setText(QString::number(param.thresh1*0.1f));
    ui->secondthre->setText(QString::number(param.thresh2*0.1f));
    ui->clutterthresh->setText(QString::number(param.clutterThresh*0.1f));
    ui->clutterfalse->setCurrentIndex(param.clutterMapFlaseRate);
    ui->cfartype->setCurrentIndex(param.CFARType);
    ui->dispro->setText(QString::number(param.disProWin));
    ui->disref->setText(QString::number(param.disRefWin));
    ui->doppro->setText(QString::number(param.dopProWin));
    ui->dopref->setText(QString::number(param.dopRefWin));
    ui->mtdtype->setCurrentIndex(param.MTDWinType);
    ui->cluttertype->setCurrentIndex(param.clutterMode);
    ui->clutterwid->setText(QString::number(param.clutterChannelWidth));
    ui->clutterrefresh->setText(QString::number(param.clutterUnitWin));
    ui->clutteriter->setText(QString::number(param.clutterIter));

    if(param.algorithmSwitch & 1)
    {
        ui->cluttersense->setCheckState(Qt::Checked);
    }
    else
    {
        ui->cluttersense->setCheckState(Qt::Unchecked);
    }

    if(param.algorithmSwitch & (1<<1))
    {
        ui->sidelobehide->setCheckState(Qt::Checked);
    }
    else
    {
        ui->sidelobehide->setCheckState(Qt::Unchecked);
    }

    if(param.algorithmSwitch & (1<<2))
    {
        ui->jinqubumang->setCheckState(Qt::Checked);
    }
    else
    {
        ui->jinqubumang->setCheckState(Qt::Unchecked);
    }
}

sigParamUI::~sigParamUI()
{
    delete ui;
}
