/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:52
 * @Description: 
 */
#ifndef PVIEWTOPLEFT_H
#define PVIEWTOPLEFT_H

#include <QWidget>
#include <QPointer>
#include "Basic/Protocol.h"

namespace Ui {
class mainviewTopLeft;
}
class QPushButton;
class QLineEdit;
class ElectromagneticDeviceModule;
class RadarPanelPositionDialog;
class CusWindow;

class mainviewTopLeft : public QWidget
{
    Q_OBJECT

public:
    explicit mainviewTopLeft(QWidget *parent = nullptr);
    ~mainviewTopLeft();

    // 雷达平台位置与姿态读取（供坐标转换使用）
    double getLatitude()  const;
    double getLongitude() const;
    double getAltitude()  const;  // 高度，单位：米
    double getYaw()       const;  // 阵面偏航角（真北参考），单位：度
    void setElectromagneticDeviceModule(ElectromagneticDeviceModule* module);
signals:
    void overlaySizeChanged();

protected:
    void paintEvent(QPaintEvent* event) override; // 声明 paintEvent

private:
    void refreshReference();
    void fitReferenceLayout();
    void showReferenceDialog();
    void showPanelPositionDialog();
    QPushButton* m_originButton = nullptr;
    QPushButton* m_sendPositionButton = nullptr;
    QLineEdit* m_rollEdit = nullptr;
    QPointer<ElectromagneticDeviceModule> m_electromagneticModule;
    QPointer<RadarPanelPositionDialog> m_panelPositionDialog;
    QPointer<CusWindow> m_panelPositionWindow;
    Ui::mainviewTopLeft *ui;
};

#endif // PVIEWTOPLEFT_H
