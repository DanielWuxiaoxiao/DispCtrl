/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#ifndef SIGPARAMUI_H
#define SIGPARAMUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class sigParamUI;
}

class sigParamUI : public QDialog
{
    Q_OBJECT

public:
    explicit sigParamUI(QWidget *parent = nullptr);
    ~sigParamUI();
    void restoreParam(const SigProParam& param);


signals:
    void setParam(const SigProParam param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::sigParamUI *ui;
};

#endif // SIGPARAMUI_H
