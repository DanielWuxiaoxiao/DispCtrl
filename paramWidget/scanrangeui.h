/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:59
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
    void onSaveToConfig();  // 保存参数到配置文件

private:
    Ui::ScanRangeUI *ui;
};

#endif // SCANRANGEUI_H
