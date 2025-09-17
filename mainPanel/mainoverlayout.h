#ifndef MAINOVERLAYOUT_H
#define MAINOVERLAYOUT_H

#include <QWidget>

namespace Ui {
class MainOverLayOut;
}

class MainOverLayOut : public QWidget
{
    Q_OBJECT

public:
    explicit MainOverLayOut(QWidget *parent = nullptr);
    ~MainOverLayOut();

private:
    Ui::MainOverLayOut *ui;
};

#endif // MAINOVERLAYOUT_H
