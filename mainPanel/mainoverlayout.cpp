#include "mainoverlayout.h"
#include "ui_mainoverlayout.h"

MainOverLayOut::MainOverLayOut(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainOverLayOut)
{
    ui->setupUi(this);
}

MainOverLayOut::~MainOverLayOut()
{
    delete ui;
}
