/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:51
 * @Description: 
 */
#ifndef POINTINFOW_H
#define POINTINFOW_H

#include <QWidget>
#include "Basic/Protocol.h"

namespace Ui {
class PointInfoW;
}

class PointInfoW : public QWidget
{
    Q_OBJECT

public:
    explicit PointInfoW(QWidget *parent = nullptr);
    ~PointInfoW();

    /**
     * @brief 设置选中的批次ID
     * @param batchID 批次ID，-1表示取消选择
     * @details 选中后会持续显示该批次的最新信息
     */
    void setSelectedBatch(int batchID);

    /**
     * @brief 获取当前选中的批次ID
     * @return 当前选中的批次ID，-1表示未选择
     */
    int selectedBatch() const { return m_selectedBatchID; }

    /**
     * @brief 更新显示的点信息
     * @param info 点信息
     * @details 更新界面上显示的所有字段
     */
    void updatePointInfo(const PointInfo& info);

    /**
     * @brief 清除显示的点信息
     * @details 将所有字段清空
     */
    void clearPointInfo();

public slots:
    /**
     * @brief 处理新航迹数据到来
     * @param info 航迹点信息
     * @details 如果批次ID匹配选中的批次，则更新显示
     */
    void onTrackDataReceived(const PointInfo& info);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Ui::PointInfoW *ui;
    int m_selectedBatchID = -1;  ///< 当前选中的批次ID，-1表示未选择
};

#endif // POINTINFOW_H
