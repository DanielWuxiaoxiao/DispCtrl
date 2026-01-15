/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:16
 * @Description: 
 */
#ifndef WAVEANDSAMPLE_H
#define WAVEANDSAMPLE_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class waveAndSample;
}

static unsigned char timeWidths[] = {2,2,4,10,20,35,1,1,1,10,25,35};
static unsigned char PRTs[] = {40,25,20,50,100,175,15,30,40,50,125,175};

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

private:
    Ui::waveAndSample *ui;
};

#endif // WAVEANDSAMPLE_H
