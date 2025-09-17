#ifndef PVIEWTOPLEFT_H
#define PVIEWTOPLEFT_H

#include <QWidget>

namespace Ui {
class pviewTopLeft;
}

class pviewTopLeft : public QWidget
{
    Q_OBJECT

public:
    explicit pviewTopLeft(QWidget *parent = nullptr);
    ~pviewTopLeft();

private:
    Ui::pviewTopLeft *ui;
};

#endif // PVIEWTOPLEFT_H
