/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-10-24 21:06:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:58
 * @Description: 
 */
#ifndef DATASAVEUI_H
#define DATASAVEUI_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class DataSaveUI;
}

constexpr char DATA_FILE[] = "/data.txt";



class DataSaveUI : public QDialog
{
    Q_OBJECT
public:
    static bool ifCurrentoffline;
    static unsigned char offlineDataID;
public:
    explicit DataSaveUI(QWidget *parent = nullptr);
    ~DataSaveUI();
    void dataSaveOK(DataSaveOK datasaveok);
    void dataDelOK(DataDelOK datadelok);

public slots:
    void showContextMenu(const QPoint &pos);
    void deleteData();
    void stopSave();
    void startSave();
    void offlineDeal();
    void onlineDeal();
    void updateLocalFile();

signals:
    void setParam(DataSet param);

private:
    void populateTable();
    Ui::DataSaveUI *ui;
};



#endif // DATASAVEUI_H
