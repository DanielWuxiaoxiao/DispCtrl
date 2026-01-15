/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#ifndef TRANRECVUI_H
#define TRANRECVUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class TranRecvUI;
}

class TranRecvUI : public QDialog
{
    Q_OBJECT

public:
    explicit TranRecvUI(QWidget *parent = nullptr);
    ~TranRecvUI();

    void restoreParam(const TranRecControl& param);


signals:
    void setParam(const TranRecControl param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::TranRecvUI *ui;
};

#endif // TRANRECVUI_H
