/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:58
 * @Description: 
 */
#ifndef FREQCONTROLUI_H
#define FREQCONTROLUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class FreqControlUI;
}

class FreqControlUI : public QDialog
{
    Q_OBJECT

public:
    explicit FreqControlUI(QWidget *parent = nullptr);
    ~FreqControlUI();

    void restoreParam(const DirGramScan& param);


signals:
    void setParam(const DirGramScan param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::FreqControlUI *ui;
};

#endif // FREQCONTROLUI_H
