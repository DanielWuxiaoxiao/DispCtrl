/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:33
 * @Description: 
 */
#include "photoelectricparam.h"
#include "ui_photoelectricparam.h"
#include "Basic/DispBasci.h"
#include <QPushButton>

PhotoElectricParam::PhotoElectricParam(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PhotoElectricParam)
{
    ui->setupUi(this);
    setWindowTitle(tr("光电系统控制"));
    // 窗口居中显示
    centerWidgetOnScreen(this);

    // 断开UI文件中的默认连接
        disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &PhotoElectricParam::onAccept);
        connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &PhotoElectricParam::onCancel);

        ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定下发"));
        ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    connect(ui->comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index)
    {
        emit changePhotoParam(index);
        ui->photoTab->setCurrentIndex(index);
    });
    // 按钮信号已在上方统一连接


//    ui->buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
//    ui->buttonBox->button(QDialogButtonBox::Cancel)->setVisible(false);
//    ui->photoTab->setVisible(false);
//    // 遍历 gridLayout 中的所有控件并隐藏
//    for (int row = 0; row < ui->totalSet->rowCount(); ++row) {
//        for (int col = 0; col < ui->totalSet->columnCount(); ++col) {
//            // 获取布局�?(row, col) 位置的控�?
//            QLayoutItem *item = ui->totalSet->itemAtPosition(row, col);
//            if (item && item->widget()) { // 确保是控件（非布局或空�?
//                item->widget()->setVisible(false); // 隐藏该控�?
//            }
//        }
//    }
    // 如需重新显示所有控件，�?setVisible(false) 改为 setVisible(true)
}

void PhotoElectricParam::onAccept()
{
    PhotoElectricParamSet param;

    param.targetNum = ui->ID->text().toULong();

    param.targetType = ui->type->currentIndex();
    param.alt = ui->alt->text().toFloat() + 500;

    param.lat = ui->lat->text().toFloat()*100000;
    param.lon = ui->lon->text().toFloat()*100000;
    param.speed = ui->speed->text().toFloat();

    PhotoElectricParamSet2 param2;

    param2.targetNum = ui->ID->text().toULong();

    param2.targetType = ui->type->currentIndex();
    param2.r = ui->R->text().toFloat();

    param2.a = ui->A->text().toFloat();
    param2.e = ui->E->text().toFloat();
    param2.speed = ui->speed->text().toFloat();

    if(ui->photoTab->currentIndex() == 0)
    {
        emit setParam(param);
    }
    else if(ui->photoTab->currentIndex() == 1)
    {
        emit setParam2(param2);
    };

        // 保持窗口与布局，不关闭窗口
}

void PhotoElectricParam::onCancel()
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

void PhotoElectricParam::restoreParam(const PhotoElectricParamSet &param)
{
    ui->ID->setText(QString::number(param.targetNum));

    ui->type->setCurrentIndex(param.targetType);
    ui->alt->setText(QString::number(param.alt-500));

    ui->lat->setText(QString::number(float(param.lat)/100000));
    ui->lon->setText(QString::number(float(param.lon)/100000));

    ui->speed->setText(QString::number(float(param.speed)));
}

void PhotoElectricParam::restoreParam2(const PhotoElectricParamSet2& param)
{
    ui->ID->setText(QString::number(param.targetNum));

    ui->type->setCurrentIndex(param.targetType);
    ui->R->setText(QString::number(float(param.r)));
    ui->A->setText(QString::number(float(param.a)));
    ui->E->setText(QString::number(float(param.e)));
    ui->speed->setText(QString::number(float(param.speed)));
}

PhotoElectricParam::~PhotoElectricParam()
{
    delete ui;
}
