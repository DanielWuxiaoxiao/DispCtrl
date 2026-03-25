/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 17:09:15
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
