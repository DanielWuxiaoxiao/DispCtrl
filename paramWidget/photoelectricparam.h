/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:04:46
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:36
 * @Description: 
 */
#ifndef PHOTOELECTRICPARAM_H
#define PHOTOELECTRICPARAM_H

#include <QDialog>
#include "Basic/Protocol.h"

namespace Ui {
class PhotoElectricParam;
}

class PhotoElectricParam : public QDialog
{
    Q_OBJECT

public:
    explicit PhotoElectricParam(QWidget *parent = nullptr);
    ~PhotoElectricParam();
    void restoreParam(const PhotoElectricParamSet& param);
    void restoreParam2(const PhotoElectricParamSet2& param);


signals:
    void setParam(const PhotoElectricParamSet param);
    void setParam2(const PhotoElectricParamSet2 param);
    void changePhotoParam(int param);

private slots:
    void onAccept();
    void onCancel();

private:
    Ui::PhotoElectricParam *ui;
};

#endif // PHOTOELECTRICPARAM_H
