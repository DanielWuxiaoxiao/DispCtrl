/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:24
 * @Description: 
 */
#include "sigparamui.h"
#include "ui_sigparamui.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "cusWidgets/custommessagebox.h"
#include <QPushButton>
#include <QDebug>

sigParamUI::sigParamUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sigParamUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("信号处理参数"));
    // 窗口居中显示
    centerWidgetOnScreen(this);

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &sigParamUI::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &sigParamUI::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &sigParamUI::onSaveToConfig);
    }
}

void sigParamUI::onAccept()
{
    LOG_DEBUG("[sigParamUI] onAccept() called");
    SigProParam param;
    param.algorithmSwitch = 0;
    param.noise = ui->noise->text().toFloat()/0.01f;
    param.thresh1 = ui->firstthresh->text().toFloat()/0.01f;
    param.thresh2 = ui->secondthre->text().toFloat()/0.01f;
    param.clutterThresh = ui->clutterthresh->text().toFloat()/0.01f;
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

    LOG_DEBUG("[sigParamUI] emitting setParam signal");
    emit setParam(param);
    // 保持窗口与布局，不关闭父窗口
}

void sigParamUI::onCancel()
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


void sigParamUI::restoreParam(const SigProParam &param)
{
    ui->noise->setText(QString::number(param.noise*0.01f));
    ui->firstthresh->setText(QString::number(param.thresh1*0.01f));
    ui->secondthre->setText(QString::number(param.thresh2*0.01f));
    ui->clutterthresh->setText(QString::number(param.clutterThresh*0.01f));
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

void sigParamUI::onSaveToConfig()
{
    // 弹出确认对话框
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前信号处理参数保存到配置文件？\n下次启动将自动加载这些参数。"))) {
        return;
    }

    // 提取UI值（与onAccept相同的逻辑）
    unsigned char algorithmSwitch = 0;
    if(ui->cluttersense->checkState() == Qt::Checked)
        algorithmSwitch |= 1;
    if(ui->sidelobehide->checkState() == Qt::Checked)
        algorithmSwitch |= (1 << 1);
    if(ui->jinqubumang->checkState() == Qt::Checked)
        algorithmSwitch |= (1 << 2);

    // 保存信号处理参数到配置（修正：参数顺序必须与 ConfigManager::saveSigProParam 一致）
    CF_INS.saveSigProParam(
        ui->noise->text().toFloat() / 0.01f,           // 第1个参数：noise
        ui->firstthresh->text().toFloat() / 0.01f,     // 第2个参数：thresh1
        ui->secondthre->text().toFloat() / 0.01f,      // 第3个参数：thresh2
        ui->clutterthresh->text().toFloat() / 0.01f,   // 第4个参数：clutterThresh
        ui->clutterfalse->currentIndex(),              // 第5个参数：clutterMapFlaseRate
        ui->cfartype->currentIndex(),                  // 第6个参数：CFARType
        ui->dispro->text().toShort(),                  // 第7个参数：disProWin
        ui->disref->text().toShort(),                  // 第8个参数：disRefWin
        ui->doppro->text().toShort(),                  // 第9个参数：dopProWin
        ui->dopref->text().toShort(),                  // 第10个参数：dopRefWin
        ui->mtdtype->currentIndex(),                   // 第11个参数：MTDWinType
        ui->cluttertype->currentIndex(),               // 第12个参数：clutterMode
        ui->clutterwid->text().toShort(),              // 第13个参数：clutterChannelWidth
        ui->clutterrefresh->text().toShort(),          // 第14个参数：clutterUnitWin
        ui->clutteriter->text().toShort(),             // 第15个参数：clutterIter
        algorithmSwitch                                // 第16个参数：algorithmSwitch（最后一个！）
    );

    // 保存到文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("信号处理参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}

sigParamUI::~sigParamUI()
{
    delete ui;
}
