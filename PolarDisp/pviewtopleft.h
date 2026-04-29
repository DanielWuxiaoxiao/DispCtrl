/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-29 10:48:05
 * @Description: 
 */
#ifndef PVIEWTOPLEFT_H
#define PVIEWTOPLEFT_H

#include <QWidget>
#include "Basic/Protocol.h"

namespace Ui {
class mainviewTopLeft;
}

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

public slots:
    // 接收BIT上报信息，更新阵面偏航角
    void onBITReport(BITReport report);

    // 接收经纬高上报，实时更新经纬高显示
    void onGeoLocationUpdated(double latitude, double longitude, double altitude);

protected:
    void paintEvent(QPaintEvent* event) override; // 声明 paintEvent

private:
    Ui::mainviewTopLeft *ui;
};

#endif // PVIEWTOPLEFT_H
