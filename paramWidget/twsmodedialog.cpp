/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-21 11:05:23
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:48
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-21
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-21
 * @Description: TWS模式设置对话框实现
 */
#include "twsmodedialog.h"
#include "ui_twsmodedialog.h"
#include "Basic/ConfigManager.h"
#include "Controller/controller.h"
#include "cusWidgets/custommessagebox.h"
#include <QPushButton>
#include <algorithm>

// 波形编码定义（根据协议 2.2.1.5）
// 波形码0~11: 时宽-带宽-重复周期
// 0：2-6-40,  1：2-6-25,  2：4-6-20,   3：10-6-50,  4：20-6-100, 5：35-6-175
// 6：1-15-15, 7：1-15-30, 8：1-15-40,  9：10-15-50, 10：25-15-125, 11：35-15-175
unsigned char TWSModeDialog::timeWidths[] = {2, 2, 4, 10, 20, 35, 1, 1, 1, 10, 25, 35};
unsigned char TWSModeDialog::PRTs[] = {40, 25, 20, 50, 100, 175, 15, 30, 40, 50, 125, 175};

TWSModeDialog::TWSModeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TWSModeDialog)
{
    ui->setupUi(this);
    setFixedSize(1200, 800);
    setWindowTitle(tr("TWS模式设置"));
    setObjectName("TWSModeDialog");  // 设置对象名以应用darkstyle.qss中的样式

    // 断开UI文件中的默认连接
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &TWSModeDialog::onAccept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &TWSModeDialog::onCancel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 添加伺服控制指令选项
    ui->cmdCombo->addItem(tr("方位停转"), 0);
    ui->cmdCombo->addItem(tr("方位转动"), 1);
    ui->cmdCombo->addItem(tr("方位寻位"), 2);
    ui->cmdCombo->addItem(tr("方位归北"), 3);

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &TWSModeDialog::onSaveToConfig);
    }

    setupConnections();
}

TWSModeDialog::~TWSModeDialog()
{
    delete ui;
}

void TWSModeDialog::setupConnections()
{
    // 波形1改变时更新采样参数
    connect(ui->wave1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        updateSampleParams(index, 1);
    });

    // 波形2改变时更新采样参数
    connect(ui->wave2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        updateSampleParams(index, 2);
    });

    // 波形3改变时更新采样参数
    connect(ui->wave3, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        updateSampleParams(index, 3);
    });
}

void TWSModeDialog::updateSampleParams(int waveIndex, int beamNum)
{
    if (waveIndex < 0 || waveIndex >= 12) return;

    auto timeWidth = timeWidths[waveIndex];
    auto PRT = PRTs[waveIndex];
    // 采样起始 = 1.0us(固定发射起始) + 时宽 + 1.0us
    float sampleStart = 1.0f + timeWidth + 1.0f;
    float sampleEnd = float(PRT - 1.0f);  // 采样终止 = 重复周期-1.0f

    switch (beamNum) {
    case 1:
        ui->samplestart1->setText(QString::number(sampleStart));
        ui->samplelen1->setText(QString::number(sampleEnd));
        break;
    case 2:
        ui->samplestart2->setText(QString::number(sampleStart));
        ui->samplelen2->setText(QString::number(sampleEnd));
        break;
    case 3:
        ui->samplestart3->setText(QString::number(sampleStart));
        ui->samplelen3->setText(QString::number(sampleEnd));
        break;
    }
}

void TWSModeDialog::onAccept()
{
    // 1. 工作模式参数
    ScanRange scanRangeParam;
    scanRangeParam.workMode = ui->workMode->currentIndex();

    // 2. 波形及采样控制参数
    BeamControl beamParam;
    beamParam.flagNum = 0;

    // 波形1使能
    if (ui->enable1->checkState() == Qt::Unchecked) {
        beamParam.beam1Flag = 0;
    } else {
        beamParam.beam1Flag = 1;
        beamParam.flagNum++;
    }

    // 波形2使能
    if (ui->enable2->checkState() == Qt::Unchecked) {
        beamParam.beam2Flag = 0;
    } else {
        beamParam.beam2Flag = 1;
        beamParam.flagNum++;
    }

    // 波形3使能
    if (ui->enable3->checkState() == Qt::Unchecked) {
        beamParam.beam3Flag = 0;
    } else {
        beamParam.beam3Flag = 1;
        beamParam.flagNum++;
    }

    // 频率和类型
    beamParam.freqID = ui->freq->currentIndex() * 10;  // 0-7 直接使用索引值
    beamParam.type = ui->type->currentIndex() + 1;

    // 发射扇区（方位参数）
    beamParam.aziStart = ui->azistart->text().toFloat() / 0.01f;
    beamParam.aziEnd = ui->aziend->text().toFloat() / 0.01f;
    beamParam.aziStep = ui->azistep->text().toFloat() / 0.01f;

    // 波形码
    beamParam.beam1Code = ui->wave1->currentIndex();
    beamParam.beam2Code = ui->wave2->currentIndex();
    beamParam.beam3Code = ui->wave3->currentIndex();

    // 统一的积累脉冲数
    beamParam.pulseNum = ui->pulseNum1->currentText().toUInt();

    // 采样参数
    beamParam.sampleStart1 = ui->samplestart1->text().toFloat() / 0.1f;
    beamParam.sampleStart2 = ui->samplestart2->text().toFloat() / 0.1f;
    beamParam.sampleStart3 = ui->samplestart3->text().toFloat() / 0.1f;

    beamParam.sampleEnd1 = ui->samplelen1->text().toFloat() / 0.1f;
    beamParam.sampleEnd2 = ui->samplelen2->text().toFloat() / 0.1f;
    beamParam.sampleEnd3 = ui->samplelen3->text().toFloat() / 0.1f;

    // 俯仰参数
    beamParam.elestart1 = ui->elestart1->text().toFloat() / 0.01f;
    beamParam.elestart2 = ui->elestart2->text().toFloat() / 0.01f;
    beamParam.elestart3 = ui->elestart3->text().toFloat() / 0.01f;

    beamParam.eleend1 = ui->eleend1->text().toFloat() / 0.01f;
    beamParam.eleend2 = ui->eleend2->text().toFloat() / 0.01f;
    beamParam.eleend3 = ui->eleend3->text().toFloat() / 0.01f;

    beamParam.elestep1 = ui->elestep1->text().toFloat() / 0.01f;
    beamParam.elestep2 = ui->elestep2->text().toFloat() / 0.01f;
    beamParam.elestep3 = ui->elestep3->text().toFloat() / 0.01f;

    // 3. 伺服控制参数
    ServoControlParam servoParam;
    servoParam.cmd = static_cast<unsigned char>(ui->cmdCombo->currentData().toInt());
    servoParam.speed = static_cast<unsigned char>(ui->speedEdit->text().toInt());
    double angleDeg = ui->angleEdit->text().toDouble();
    int angle = static_cast<int>(angleDeg * 100.0 + 0.5);
    angle = std::clamp(angle, 0, 36000);
    servoParam.az = static_cast<unsigned short>(angle);

    // 按顺序依次发送三个配置
    emit setScanRange(scanRangeParam);
    emit setBeamControl(beamParam);
    emit setServoControl(servoParam);

    // 更新扫描范围到显控扫描线
    // 将方位参数转换为度并发送给Controller
    double aziStartDeg = beamParam.aziStart * 0.01;
    double aziEndDeg = beamParam.aziEnd * 0.01;
    emit CON_INS->scanRangeChanged(aziStartDeg, aziEndDeg);

    // 不关闭窗口，便于多次下发
}

void TWSModeDialog::onCancel()
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

void TWSModeDialog::restoreParam(const TWSModeParam &param)
{
    // 1. 恢复工作模式
    ui->workMode->setCurrentIndex(0);
    ui->workMode->setEnabled(false);

    // 2. 恢复波形及采样控制参数
    const BeamControl& beamParam = param.beamControl;

    // 使能状态
    ui->enable1->setCheckState(beamParam.beam1Flag ? Qt::Checked : Qt::Unchecked);
    ui->enable2->setCheckState(beamParam.beam2Flag ? Qt::Checked : Qt::Unchecked);
    ui->enable3->setCheckState(beamParam.beam3Flag ? Qt::Checked : Qt::Unchecked);

    // 波形码
    ui->wave1->setCurrentIndex(beamParam.beam1Code);
    ui->wave2->setCurrentIndex(beamParam.beam2Code);
    ui->wave3->setCurrentIndex(beamParam.beam3Code);

    // 频率和类型
    ui->freq->setCurrentIndex(beamParam.freqID / 10);
    ui->type->setCurrentIndex(beamParam.type - 1);

    // 发射扇区
    ui->azistart->setText(QString::number(beamParam.aziStart * 0.01f));
    ui->aziend->setText(QString::number(beamParam.aziEnd * 0.01f));
    ui->azistep->setText(QString::number(beamParam.aziStep * 0.01f));

    // 积累脉冲数
    ui->pulseNum1->setCurrentText(QString::number(beamParam.pulseNum));

    // 采样参数
    ui->samplestart1->setText(QString::number(beamParam.sampleStart1 * 0.1f));
    ui->samplestart2->setText(QString::number(beamParam.sampleStart2 * 0.1f));
    ui->samplestart3->setText(QString::number(beamParam.sampleStart3 * 0.1f));

    ui->samplelen1->setText(QString::number(beamParam.sampleEnd1 * 0.1f));
    ui->samplelen2->setText(QString::number(beamParam.sampleEnd2 * 0.1f));
    ui->samplelen3->setText(QString::number(beamParam.sampleEnd3 * 0.1f));

    // 俯仰参数
    ui->elestart1->setText(QString::number(beamParam.elestart1 * 0.01f));
    ui->elestart2->setText(QString::number(beamParam.elestart2 * 0.01f));
    ui->elestart3->setText(QString::number(beamParam.elestart3 * 0.01f));

    ui->eleend1->setText(QString::number(beamParam.eleend1 * 0.01f));
    ui->eleend2->setText(QString::number(beamParam.eleend2 * 0.01f));
    ui->eleend3->setText(QString::number(beamParam.eleend3 * 0.01f));

    ui->elestep1->setText(QString::number(beamParam.elestep1 * 0.01f));
    ui->elestep2->setText(QString::number(beamParam.elestep2 * 0.01f));
    ui->elestep3->setText(QString::number(beamParam.elestep3 * 0.01f));

    // 3. 恢复伺服控制参数
    const ServoControlParam& servoParam = param.servoControl;
    ui->cmdCombo->setCurrentIndex(std::clamp<int>(servoParam.cmd, 0, ui->cmdCombo->count() - 1));
    ui->speedEdit->setText(QString::number(servoParam.speed));
    ui->angleEdit->setText(QString::number(static_cast<double>(servoParam.az) / 100.0, 'f', 2));
}

void TWSModeDialog::onSaveToConfig()
{
    // 弹出确认对话框
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前TWS参数保存到配置文件？\n下次启动将自动加载这些参数。"))) {
        return;
    }

    // 获取所有UI参数并保存（复用onAccept的逻辑，但只保存不发送）

    // 1. 工作模式
    unsigned char workMode = static_cast<unsigned char>(ui->workMode->currentIndex());
    CF_INS.saveScanRangeParam(workMode);

    // 2. 波形及采样控制参数
    unsigned char freqID = ui->freq->currentIndex() * 10;
    unsigned char type = ui->type->currentIndex() + 1;
    short aziStart = ui->azistart->text().toFloat() / 0.01f;
    short aziEnd = ui->aziend->text().toFloat() / 0.01f;
    short aziStep = ui->azistep->text().toFloat() / 0.01f;
    unsigned short pulseNum = ui->pulseNum1->currentText().toUInt();
    unsigned char flagNum = 0;

    unsigned char beam1Flag = (ui->enable1->checkState() == Qt::Checked) ? 1 : 0;
    unsigned char beam2Flag = (ui->enable2->checkState() == Qt::Checked) ? 1 : 0;
    unsigned char beam3Flag = (ui->enable3->checkState() == Qt::Checked) ? 1 : 0;
    if (beam1Flag) flagNum++;
    if (beam2Flag) flagNum++;
    if (beam3Flag) flagNum++;

    unsigned char beam1Code = ui->wave1->currentIndex();
    unsigned char beam2Code = ui->wave2->currentIndex();
    unsigned char beam3Code = ui->wave3->currentIndex();

    unsigned short sampleStart1 = ui->samplestart1->text().toFloat() / 0.1f;
    unsigned short sampleStart2 = ui->samplestart2->text().toFloat() / 0.1f;
    unsigned short sampleStart3 = ui->samplestart3->text().toFloat() / 0.1f;

    unsigned short sampleEnd1 = ui->samplelen1->text().toFloat() / 0.1f;
    unsigned short sampleEnd2 = ui->samplelen2->text().toFloat() / 0.1f;
    unsigned short sampleEnd3 = ui->samplelen3->text().toFloat() / 0.1f;

    short elestart1 = ui->elestart1->text().toFloat() / 0.01f;
    short elestart2 = ui->elestart2->text().toFloat() / 0.01f;
    short elestart3 = ui->elestart3->text().toFloat() / 0.01f;

    short eleend1 = ui->eleend1->text().toFloat() / 0.01f;
    short eleend2 = ui->eleend2->text().toFloat() / 0.01f;
    short eleend3 = ui->eleend3->text().toFloat() / 0.01f;

    short elestep1 = ui->elestep1->text().toFloat() / 0.01f;
    short elestep2 = ui->elestep2->text().toFloat() / 0.01f;
    short elestep3 = ui->elestep3->text().toFloat() / 0.01f;

    CF_INS.saveBeamControlParam(
        freqID, type, aziStart, aziEnd, aziStep, flagNum, pulseNum,
        beam1Flag, beam1Code, sampleStart1, sampleEnd1, elestart1, eleend1, elestep1,
        beam2Flag, beam2Code, sampleStart2, sampleEnd2, elestart2, eleend2, elestep2,
        beam3Flag, beam3Code, sampleStart3, sampleEnd3, elestart3, eleend3, elestep3);

    // 3. 伺服控制参数（与 onAccept() 保持完全一致）
    unsigned char cmd = static_cast<unsigned char>(ui->cmdCombo->currentData().toInt());
    unsigned char speed = static_cast<unsigned char>(ui->speedEdit->text().toInt());
    double angleDeg = ui->angleEdit->text().toDouble();
    int angle = static_cast<int>(angleDeg * 100.0 + 0.5);
    angle = std::clamp(angle, 0, 36000);
    unsigned short az = static_cast<unsigned short>(angle);

    CF_INS.saveServoParam(cmd, speed, az);

    // 保存到文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("TWS参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}
