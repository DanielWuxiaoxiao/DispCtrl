#include "pointinfow.h"
#include "ui_pointinfow.h"

PointInfoW::PointInfoW(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointInfoW)
{
    ui->setupUi(this);
}

PointInfoW::~PointInfoW()
{
    delete ui;
}
