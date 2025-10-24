/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:36
 * @Description: 
 */
﻿#include "datasaveui.h"
#include "ui_datasaveui.h"
#include <QAction>
#include <QPushButton>
#include <QMenu>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

bool DataSaveUI::ifCurrentoffline = false;
unsigned char DataSaveUI::offlineDataID = 0;

DataSaveUI::DataSaveUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataSaveUI)
{
    ui->setupUi(this);
    setWindowTitle(tr("数据存储/删除"));

    // 设置表格 objectName 以应用 darkstyle.qss 中的样式
    ui->tab->setObjectName("tableWidget");

    // 设置表格列宽度模式
    ui->tab->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); //自动伸展resize
    ui->tab->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->tab->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    ui->tab->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);

    // 隐藏垂直表头
    ui->tab->verticalHeader()->setHidden(true);

    // 设置焦点策略和上下文菜单
    ui->tab->setFocusPolicy(Qt::NoFocus);  //取消选中后的虚线�?
    ui->tab->setContextMenuPolicy(Qt::CustomContextMenu); //右键菜单

    // 启用交替行颜色（已在 .ui 文件中设置，此处确保生效）
    ui->tab->setAlternatingRowColors(true);

    // 设置选择行为（整行选择）
    ui->tab->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置选择模式（单行选择）
    ui->tab->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->tab,&QTableWidget::customContextMenuRequested, this, &DataSaveUI::showContextMenu);
    connect(ui->startSave,&QPushButton::clicked, this, &DataSaveUI::startSave);
    connect(ui->save,&QPushButton::clicked,this,&DataSaveUI::updateLocalFile);

    populateTable();
    ui->tab->scrollToBottom();
}

void DataSaveUI::populateTable()
{
    ui->tab->setRowCount(0);

    QFile file(QDir::currentPath() + DATA_FILE);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QTextStream in(&file);
    int row = 0;
    while(!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(",");
        if(fields.size()>=4)
        {
            ui->tab->insertRow(row);
            ui->tab->setItem(row,0, new QTableWidgetItem(fields.at(0)));
            ui->tab->setItem(row,1, new QTableWidgetItem(fields.at(1)));
            ui->tab->setItem(row,2, new QTableWidgetItem(fields.at(2)));
            ui->tab->setItem(row,3, new QTableWidgetItem(fields.at(3)));

            ui->tab->item(row,0)->setFlags(ui->tab->item(row,0)->flags() & ~(Qt::ItemIsEditable));
            ui->tab->item(row,1)->setFlags(ui->tab->item(row,1)->flags() & ~(Qt::ItemIsEditable));
            ui->tab->item(row,2)->setFlags(ui->tab->item(row,2)->flags() & ~(Qt::ItemIsEditable));
            //ui->tab->item(row,3)->setFlags(ui->tab->item(row,3)->flags() & (Qt::ItemIsEditable));

            row++;
        }
    }
    file.close();
}

void DataSaveUI::startSave()
{
    int rowCount = ui->tab->rowCount();

    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item0 = ui->tab->item(row,0);
        if(item0)
        {
            if(item0->text().toInt() == ui->saveID->text().toInt())
            {
                QMessageBox::information(this, tr("错误"), tr("重复下发数据存储，无效操作"));
                return;
                return;
            }
        }
    }
    DataSet param;
    param.ifsave = true;

    param.save.dataID = ui->saveID->text().toInt();
    param.save.saveSwitch = 1;

    emit setParam(param);

    ui->tab->insertRow(rowCount);
    ui->tab->setItem(rowCount, 0 , new QTableWidgetItem(ui->saveID->text()));

    auto time = QDateTime::currentDateTime();
    ui->tab->setItem(rowCount, 1 , new QTableWidgetItem(time.toString("yyyy-MM-dd hh:mm:ss dddd")));

    ui->tab->setItem(rowCount, 2 , new QTableWidgetItem(""));
    ui->tab->setItem(rowCount, 3 , new QTableWidgetItem(""));
    updateLocalFile();
}

void DataSaveUI::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(tr("菜单"),this);
    QAction* deleteAction = new QAction(tr("删除数据"),this);
    connect(deleteAction,&QAction::triggered, this, &DataSaveUI::deleteData);

    contextMenu.addAction(deleteAction);

    QAction* stopAction = new QAction(tr("停止数据存储"),this);
    connect(stopAction,&QAction::triggered, this, &DataSaveUI::stopSave);

    contextMenu.addAction(stopAction);

    int currentRow = ui->tab->currentRow();
    unsigned char currentDataID = 255;
    if(currentRow >= 0)
        currentDataID = ui->tab->item(currentRow,0)->text().toInt();

    qDebug() << ifCurrentoffline;

    qDebug() << offlineDataID;
    if(ifCurrentoffline == false)
    {
        QAction* offlineAction = new QAction(tr("离线处理"),this);
        connect(offlineAction,&QAction::triggered, this, &DataSaveUI::offlineDeal);
        contextMenu.addAction(offlineAction);
    }
    else
    {
        if(currentDataID == offlineDataID)
        {
            QAction* onlineAction = new QAction(tr("正常处理"),this);
            connect(onlineAction,&QAction::triggered, this, &DataSaveUI::onlineDeal);
            contextMenu.addAction(onlineAction);
        }
    }

    contextMenu.exec(ui->tab->mapToGlobal(pos));
}

void DataSaveUI::deleteData()
{
    DataSet param;
    param.ifdel = true;

    int currentRow = ui->tab->currentRow();
    if(currentRow >= 0)
    {
        param.del.dataID = ui->tab->item(currentRow,0)->text().toInt();
    }
    emit setParam(param);
}

void DataSaveUI::stopSave()
{
    DataSet param;
    param.ifsave = true;

    int currentRow = ui->tab->currentRow();
    if(currentRow >= 0)
    {
        param.save.dataID = ui->tab->item(currentRow,0)->text().toInt();
        param.save.saveSwitch = 0;
    }
    emit setParam(param);
}

void DataSaveUI::offlineDeal()
{
    DataSet param;
    param.ifoffline = true;

    int currentRow = ui->tab->currentRow();
    if(currentRow >= 0)
    {
        param.off.dataID = ui->tab->item(currentRow,0)->text().toInt();
        param.off.onSwitch = 1;
    }
    emit setParam(param);
}

void DataSaveUI::onlineDeal()
{
    DataSet param;
    param.ifoffline = true;  //UDP发送时判断是什么消息类�?

    int currentRow = ui->tab->currentRow();
    if(currentRow >= 0)
    {
        param.off.dataID = ui->tab->item(currentRow,0)->text().toInt();
        param.off.onSwitch = 0;
    }
    emit setParam(param);
}


void DataSaveUI::dataSaveOK(DataSaveOK datasaveok)
{
    int rowCount = ui->tab->rowCount();
    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item0 = ui->tab->item(row,0);
        if(item0)
        {
            if(item0->text().toInt() == datasaveok.dataID)
            {
                ui->tab->setItem(row,2, new QTableWidgetItem(QString::number(datasaveok.dataSize)));
                break;
            }
        }
    }
    updateLocalFile();
}

void DataSaveUI::dataDelOK(DataDelOK datadelok)
{
    int rowCount = ui->tab->rowCount();
    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item0 = ui->tab->item(row,0);
        if(item0)
        {
            if(item0->text().toInt() == datadelok.dataID)
            {
                ui->tab->removeRow(row);
                updateLocalFile();
                return;
            }
        }
    }
}

void DataSaveUI::updateLocalFile()
{
    QFile file(QDir::currentPath() + DATA_FILE);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.setFileName(QDir::currentPath() + DATA_FILE);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return;
        }
    }

    QTextStream out(&file);
    int rowCount = ui->tab->rowCount();
    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item0 = ui->tab->item(row,0);
        QTableWidgetItem* item1 = ui->tab->item(row,1);
        QTableWidgetItem* item2 = ui->tab->item(row,2);
        QTableWidgetItem* item3 = ui->tab->item(row,3);
        if(item0 && item1 && item2 && item3)
        {
            out << item0->text() << "," << item1->text() << "," << item2->text() << "," << item3->text() << "\n";
        }
    }
    file.close();
}

DataSaveUI::~DataSaveUI()
{
    delete ui;
}
