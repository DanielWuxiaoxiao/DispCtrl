/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:49
 * @Description: 
 */
#ifndef WAVEANDSAMPLE_H
#define WAVEANDSAMPLE_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class waveAndSample;
}

// 波形编码定义（根据协议 2.2.1.5）
// 波形码0~11: 时宽-带宽-重复周期
// 0：2-6-40,  1：2-6-25,  2：4-6-20,   3：10-6-50,  4：20-6-100, 5：35-6-175
// 6：1-15-15, 7：1-15-30, 8：1-15-40,  9：10-15-50, 10：25-15-125, 11：35-15-175
static unsigned char timeWidths[] = {2, 2, 4, 10, 20, 35, 1, 1, 1, 10, 25, 35};
static unsigned char PRTs[] = {40, 25, 20, 50, 100, 175, 15, 30, 40, 50, 125, 175};

class waveAndSample : public QDialog
{
    Q_OBJECT

public:
    explicit waveAndSample(QWidget *parent = nullptr);
    ~waveAndSample();

    void restoreParam(const BeamControl& param);


signals:
    void setParam(const BeamControl param);

private slots:
    void onAccept();
    void onCancel();
    void onSaveToConfig();

private:
    Ui::waveAndSample *ui;
};

#endif // WAVEANDSAMPLE_H
