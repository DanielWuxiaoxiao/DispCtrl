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
