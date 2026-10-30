/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:58
 * @Description: 
 */
#include "datasaveui.h"
#pragma pack(push, 8)
#include "ui_datasaveui.h"
#include <QIntValidator>
#pragma pack(pop)
#include "Basic/log.h"
#include "Basic/DispBasci.h"
#include "cusWidgets/custommessagebox.h"
#include <QAction>
#include <QPushButton>
#include <QMenu>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <array>
#include "Basic/ConfigManager.h"
#include "Basic/uifontscale.h"

bool DataSaveUI::ifCurrentoffline = false;
unsigned char DataSaveUI::offlineDataID = 0;

DataSaveUI::DataSaveUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataSaveUI)
{
    ui->setupUi(this);
    ui->saveID->setValidator(new QIntValidator(0, 255, ui->saveID));
    setWindowTitle(tr("数据存储/删除"));
    setMinimumSize(800, 400);
    // 窗口居中显示
    centerWidgetOnScreen(this);

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
    bool validID = false;
    const int dataID = ui->saveID->text().toInt(&validID);
    if (!validID || dataID < 0 || dataID > 255) {
        QMessageBox::information(this, tr("输入错误"), tr("数据编号必须为0～255的整数。"));
        return;
    }
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

    param.save.dataID = static_cast<unsigned char>(dataID);
    param.save.saveSwitch = 1;
    fillStorageOptions(param.save);

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

    LOG_DEBUG(QString("DataSaveUI offline flag: %1").arg(ifCurrentoffline));
    LOG_DEBUG(QString("DataSaveUI offline data ID: %1").arg(offlineDataID));
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
    int currentRow = ui->tab->currentRow();
    if(currentRow < 0)
    {
        return;
    }

    QTableWidgetItem* currentItem = ui->tab->item(currentRow,0);
    if(!currentItem)
    {
        return;
    }

    const int dataID = currentItem->text().toInt();
    const QString confirmText = QString::fromUtf8("是否删除数据 ID: ") + QString::number(dataID) + QString::fromUtf8("？");
    if(!CustomMessageBox::showConfirm(this, tr("确认删除"), confirmText))
    {
        return;
    }

    DataSet param;
    param.ifdel = true;

    param.del.dataID = dataID;
    emit setParam(param);
}

void DataSaveUI::stopSave()
{
    DataSet param;
    param.ifsave = true;

    int currentRow = ui->tab->currentRow();
    if(currentRow >= 0 && ui->tab->item(currentRow, 0))
    {
        param.save.dataID = ui->tab->item(currentRow,0)->text().toInt();
        param.save.saveSwitch = 0;
    } else return;
    fillStorageOptions(param.save);
    emit setParam(param);
}

void DataSaveUI::fillStorageOptions(DataSave& save) const
{
    save.confirmBeam = ui->confirmBeamCheck->isChecked() ? 1 : 0;
    save.trackBeam = ui->trackBeamCheck->isChecked() ? 1 : 0;
    save.searchBeam = ui->searchBeamCheck->isChecked() ? 1 : 0;
    save.detection = ui->detectionCheck->isChecked() ? 1 : 0;
}

void DataSaveUI::offlineDeal()
{
    const int currentRow = ui->tab->currentRow();
    if (currentRow < 0 || !ui->tab->item(currentRow, 0)) return;
    DataSet param;
    param.ifoffline = true;
    param.off.dataID = ui->tab->item(currentRow,0)->text().toInt();
    param.off.onSwitch = 1;
    param.extendedOffline = CF_INS.defaultPanelCount() > 1;
    if (param.extendedOffline) {
        QDialog selection(this);
        selection.setObjectName(QStringLiteral("offlinePanelSelection"));
        selection.setWindowTitle(tr("选择回放阵面"));
        selection.setStyleSheet(UiFontScale::style(QStringLiteral(
            "QDialog { background:#0d1918; color:#d9ffff; }"
            "QLabel,QCheckBox { color:#d9ffff; font-size:16px; }"
            "QPushButton { color:#55ffdd; background:#153b35; border:1px solid #63bba8;"
            "border-radius:5px; padding:7px 18px; font-size:16px; }")));
        auto* layout = new QVBoxLayout(&selection);
        layout->addWidget(new QLabel(tr("数据编号 %1：勾选需要回放的阵面").arg(param.off.dataID), &selection));
        std::array<QCheckBox*, 4> checks{};
        for (int id = 0; id < 4; ++id) {
            checks[id] = new QCheckBox(tr("阵面%1 / ID%2").arg(id + 1).arg(id), &selection);
            checks[id]->setObjectName(QStringLiteral("replayPanel%1").arg(id));
            layout->addWidget(checks[id]);
        }
        auto* error = new QLabel(tr("请至少选择一个阵面。"), &selection);
        error->setStyleSheet(UiFontScale::style("color:#ffaaaa; font-size:14px;"));
        error->hide(); layout->addWidget(error);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &selection);
        buttons->button(QDialogButtonBox::Ok)->setText(tr("确定回放"));
        buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &selection, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &selection, [&] {
            bool any = false;
            for (int id = 0; id < 4; ++id) {
                param.off.replayPanel[id] = checks[id]->isChecked() ? 1 : 0;
                any = any || param.off.replayPanel[id];
            }
            if (!any) { error->show(); return; }
            selection.accept();
        });
        if (selection.exec() != QDialog::Accepted) return;
    }
    emit setParam(param);
}

void DataSaveUI::onlineDeal()
{
    const int currentRow = ui->tab->currentRow();
    if (currentRow < 0 || !ui->tab->item(currentRow, 0)) return;
    DataSet param;
    param.ifoffline = true;  //UDP发送时判断是什么消息类�?

    param.extendedOffline = CF_INS.defaultPanelCount() > 1;
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
