/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-18 16:12:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:08
 * @Description: 
 */
#ifndef MOUSEPOSITIONINFO_H
#define MOUSEPOSITIONINFO_H

#include <QWidget>

namespace Ui {
class MousePositionInfo;
}

class MousePositionInfo : public QWidget
{
    Q_OBJECT

public:
    explicit MousePositionInfo(QWidget *parent = nullptr);
    ~MousePositionInfo();

    // 更新鼠标位置信息
    void updatePosition(double distance, double azimuth);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Ui::MousePositionInfo *ui;
};

#endif // MOUSEPOSITIONINFO_H