/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-09 15:09:31
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#ifndef SERVOCONTROL_H
#define SERVOCONTROL_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class ServoControl;
}

class ServoControl : public QDialog
{
    Q_OBJECT

public:
    explicit ServoControl(QWidget *parent = nullptr);
    ~ServoControl();

    void restoreParam(const ServoControlParam &param);

signals:
    void setParam(ServoControlParam param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::ServoControl *ui;
};

#endif // SERVOCONTROL_H
