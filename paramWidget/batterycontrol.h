/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:35
 * @Description: 
 */
#ifndef BATTERYCONTROL_H
#define BATTERYCONTROL_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class BatteryControl;
}

class BatteryControl : public QDialog
{
    Q_OBJECT

public:
    explicit BatteryControl(QWidget *parent = nullptr);
    ~BatteryControl();
    void restoreParam(const BatteryControlM& param);

signals:
    void setParam(const BatteryControlM param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::BatteryControl *ui;
};

#endif // BATTERYCONTROL_H
