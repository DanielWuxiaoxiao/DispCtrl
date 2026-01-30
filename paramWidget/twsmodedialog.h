/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-21 11:04:27
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:49
 * @Description: 
 */
#ifndef TWSMODEDIALOG_H
#define TWSMODEDIALOG_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class TWSModeDialog;
}

class TWSModeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TWSModeDialog(QWidget *parent = nullptr);
    ~TWSModeDialog();

    void restoreParam(const TWSModeParam& param);

signals:
    // 按顺序依次发送三个配置
    void setScanRange(const ScanRange param);
    void setBeamControl(const BeamControl param);
    void setServoControl(const ServoControlParam param);

private slots:
    void onAccept();
    void onCancel();
    void onSaveToConfig();  // 保存参数到配置文件

private:
    Ui::TWSModeDialog *ui;

    // 波形参数数据
    static unsigned char timeWidths[];
    static unsigned char PRTs[];

    void setupConnections();
    void updateSampleParams(int waveIndex, int beamNum);
};

#endif // TWSMODEDIALOG_H
