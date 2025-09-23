/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:08
 * @Description: 
 */
#ifndef POINTINFOW_H
#define POINTINFOW_H

#include <QWidget>

namespace Ui {
class PointInfoW;
}

class PointInfoW : public QWidget
{
    Q_OBJECT

public:
    explicit PointInfoW(QWidget *parent = nullptr);
    ~PointInfoW();

protected:
    void paintEvent(QPaintEvent* event) override; // 声明 paintEvent

private:
    Ui::PointInfoW *ui;
};

#endif // POINTINFOW_H
