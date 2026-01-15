/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:14
 * @Description: 
 */
#ifndef DATAPROCESSUI_H
#define DATAPROCESSUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class DataProcessUI;
}

class DataProcessUI : public QDialog
{
    Q_OBJECT

public:
    explicit DataProcessUI(QWidget *parent = nullptr);
    ~DataProcessUI();

    void restoreParam(const DataProParam& param);


signals:
    void setParam(const DataProParam param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::DataProcessUI *ui;
};

#endif // DATAPROCESSUI_H
