/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:58
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
