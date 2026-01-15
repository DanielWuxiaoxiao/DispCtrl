/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:15
 * @Description: 
 */
#ifndef SCANRANGEUI_H
#define SCANRANGEUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class ScanRangeUI;
}

class ScanRangeUI : public QDialog
{
    Q_OBJECT

public:
    explicit ScanRangeUI(QWidget *parent = nullptr);
    ~ScanRangeUI();
    void restoreParam(const ScanRange& param);


signals:
    void setParam(const ScanRange param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::ScanRangeUI *ui;
};

#endif // SCANRANGEUI_H
