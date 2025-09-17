#include "pviewtopleft.h"
#include "ui_pviewtopleft.h"

pviewTopLeft::pviewTopLeft(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::pviewTopLeft)
{
    ui->setupUi(this);
}

pviewTopLeft::~pviewTopLeft()
{
    delete ui;
}
