/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:36
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
