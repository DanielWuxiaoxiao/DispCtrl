/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-21 11:10:12
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:48
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-21
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-21
 * @Description: TAS模式设置对话框 - 整合工作模式、波形采样和伺服控制
 */
#ifndef TASMODEDIALOG_H
#define TASMODEDIALOG_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class TASModeDialog;
}

// TAS模式参数结构体（整合三个子参数）
typedef struct _TASModeParam
{
    ScanRange scanRange;           // 工作模式
    BeamControl beamControl;       // 波形及采样控制
    ServoControlParam servoControl; // 伺服控制

    _TASModeParam()
    {
        // 使用各自结构体的默认值
        scanRange.workMode = 1;  // TAS模式
    }
} TASModeParam;

class TASModeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TASModeDialog(QWidget *parent = nullptr);
    ~TASModeDialog();

    void restoreParam(const TASModeParam& param);

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
    Ui::TASModeDialog *ui;

    // 波形参数数据
    static unsigned char timeWidths[];
    static unsigned char PRTs[];

    void setupConnections();
    void updateSampleParams(int waveIndex, int beamNum);
    void updateServoCenterAngle();  // TAS模式：自动计算并更新伺服中心角度
};

#endif // TASMODEDIALOG_H
