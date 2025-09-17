#ifndef PVIEWTOPLEFT_H
#define PVIEWTOPLEFT_H

#include <QWidget>

namespace Ui {
class mainviewTopLeft;
}

class mainviewTopLeft : public QWidget
{
    Q_OBJECT

public:
    explicit mainviewTopLeft(QWidget *parent = nullptr);
    ~mainviewTopLeft();

protected:
    void paintEvent(QPaintEvent* event) override; // 声明 paintEvent

private:
    Ui::mainviewTopLeft *ui;
};

#endif // PVIEWTOPLEFT_H
