/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 15:46:56
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:37
 * @Description: 
 */
/********************************************************************************
** Form generated from reading UI file 'batterycontrol.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TEST_H
#define TEST_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_BatteryControl
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QCheckBox *c1;
    QCheckBox *c2;
    QCheckBox *c4;
    QCheckBox *c3;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *BatteryControl)
    {
        if (BatteryControl->objectName().isEmpty())
            BatteryControl->setObjectName(QString::fromUtf8("BatteryControl"));
        BatteryControl->resize(400, 161);
        verticalLayout = new QVBoxLayout(BatteryControl);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        c1 = new QCheckBox(BatteryControl);
        c1->setObjectName(QString::fromUtf8("c1"));
        c1->setChecked(true);

        gridLayout->addWidget(c1, 0, 0, 1, 1);

        c2 = new QCheckBox(BatteryControl);
        c2->setObjectName(QString::fromUtf8("c2"));
        c2->setChecked(true);

        gridLayout->addWidget(c2, 0, 1, 1, 1);

        c4 = new QCheckBox(BatteryControl);
        c4->setObjectName(QString::fromUtf8("c4"));
        c4->setChecked(true);

        gridLayout->addWidget(c4, 1, 1, 1, 1);

        c3 = new QCheckBox(BatteryControl);
        c3->setObjectName(QString::fromUtf8("c3"));
        c3->setChecked(true);

        gridLayout->addWidget(c3, 1, 0, 1, 1);


        verticalLayout->addLayout(gridLayout);

        buttonBox = new QDialogButtonBox(BatteryControl);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(BatteryControl);
        QObject::connect(buttonBox, SIGNAL(accepted()), BatteryControl, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), BatteryControl, SLOT(reject()));

        QMetaObject::connectSlotsByName(BatteryControl);
    } // setupUi

    void retranslateUi(QDialog *BatteryControl)
    {
        BatteryControl->setWindowTitle(QCoreApplication::translate("BatteryControl", "Dialog", nullptr));
        c1->setText(QCoreApplication::translate("BatteryControl", "\350\261\241\351\231\2201", nullptr));
        c2->setText(QCoreApplication::translate("BatteryControl", "\350\261\241\351\231\2202", nullptr));
        c4->setText(QCoreApplication::translate("BatteryControl", "\350\261\241\351\231\2204", nullptr));
        c3->setText(QCoreApplication::translate("BatteryControl", "\350\261\241\351\231\2203", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BatteryControl: public Ui_BatteryControl {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TEST_H
