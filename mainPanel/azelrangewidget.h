/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-24 11:22:47
 * @Description: 
 */
/**
 * @file azelrangewidget.h
 * @brief 方位角和俯仰角范围控制部件
 * @details 提供图形化的雷达扫描角度范围设置界面，包含方位角(Azimuth)和俯仰角(Elevation)的范围控制
 *
 * 功能特性：
 * - 方位角：0-360度，北向为0度，顺时针递增
 * - 俯仰角：-45到+45度，水平为0度，向上为正
 * - 可视化刻度盘显示当前角度范围
 * - 交互式输入框实时更新角度值
 * - 自动角度归一化和范围限制
 *
 * 使用场景：
 * - 雷达扫描参数配置
 * - 搜索区域范围设定
 * - 天线指向控制界面
 *
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef AZELRANGEWIDGET_H
#define AZELRANGEWIDGET_H

#pragma once
#include <QWidget>
#include <QLineEdit>

/**
 * @class AzElRangeWidget
 * @brief 方位角和俯仰角范围控制部件
 * @details 专用于雷达系统的角度范围设置控件，提供直观的图形化界面
 *
 * 该控件集成了方位角和俯仰角的可视化显示与输入控制：
 * - 方位角使用圆形刻度盘显示，支持0-360度范围
 * - 俯仰角使用垂直条形图显示，支持-45到+45度范围
 * - 提供输入框进行精确数值设置
 * - 实时同步图形显示与数值输入
 *
 * 坐标系统：
 * - 方位角：北向为0度，顺时针方向为正
 * - 俯仰角：水平为0度，向上为正，向下为负
 *
 * @example
 * ```cpp
 * AzElRangeWidget* rangeWidget = new AzElRangeWidget(parent);
 * rangeWidget->setAzRange(30, 120);  // 设置方位角范围
 * rangeWidget->setElRange(-10, 45);  // 设置俯仰角范围
 *
 * connect(rangeWidget, &AzElRangeWidget::azRangeChanged,
 *         this, [](int min, int max) {
 *             qDebug() << "方位角范围变更:" << min << "-" << max;
 *         });
 * ```
 */
class AzElRangeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 初始化角度范围控制部件，设置默认角度范围并创建UI组件
     */
    explicit AzElRangeWidget(QWidget *parent = nullptr);

    /**
     * @brief 设置方位角范围
     * @param minDeg 最小方位角 (0-360度)
     * @param maxDeg 最大方位角 (0-360度)
     * @details 设置雷达扫描的方位角范围，北向为0度，顺时针递增
     * 自动处理角度归一化和跨越0度的情况
     */
    void setAzRange(int minDeg, int maxDeg);

    /**
     * @brief 设置俯仰角范围
     * @param minDeg 最小俯仰角 (-45到+45度)
     * @param maxDeg 最大俯仰角 (-45到+45度)
     * @details 设置雷达扫描的俯仰角范围，水平为0度
     * 正值表示向上仰角，负值表示向下俯角
     */
    void setElRange(int minDeg, int maxDeg);

signals:
    /**
     * @brief 方位角范围变更信号
     * @param minDeg 新的最小方位角
     * @param maxDeg 新的最大方位角
     * @details 当用户通过界面修改方位角范围时发射此信号
     */
    void azRangeChanged(int minDeg, int maxDeg);

    /**
     * @brief 俯仰角范围变更信号
     * @param minDeg 新的最小俯仰角
     * @param maxDeg 新的最大俯仰角
     * @details 当用户通过界面修改俯仰角范围时发射此信号
     */
    void elRangeChanged(int minDeg, int maxDeg);

protected:
    /**
     * @brief 绘制事件处理
     * @param event 绘制事件对象
     * @details 重写绘制函数，渲染方位角刻度盘和俯仰角条形图
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 最小尺寸提示
     * @return 推荐的最小控件尺寸
     */
    QSize minimumSizeHint() const override { return {360, 260}; }

    /**
     * @brief 尺寸提示
     * @return 推荐的控件尺寸
     */
    QSize sizeHint() const override { return {420, 300}; }

private:
    // === 角度值存储 ===
    int mAzMin = 15;          ///< 最小方位角 [0,360)
    int mAzMax = 75;          ///< 最大方位角 [0,360)
    int mElMin = -10;         ///< 最小俯仰角 [-45,45]
    int mElMax =  30;         ///< 最大俯仰角 [-45,45]

    // === UI组件 ===
    QLineEdit *edAzMin = nullptr;  ///< 最小方位角输入框
    QLineEdit *edAzMax = nullptr;  ///< 最大方位角输入框
    QLineEdit *edElMin = nullptr;  ///< 最小俯仰角输入框
    QLineEdit *edElMax = nullptr;  ///< 最大俯仰角输入框

    // === 工具方法 ===

    /**
     * @brief 角度归一化到[0,360)范围
     * @param d 输入角度值
     * @return 归一化后的角度值
     * @details 将任意角度值转换到0-360度标准范围内
     */
    static int norm360(int d);

    /**
     * @brief 数值限制到指定范围
     * @param v 输入值
     * @param lo 下限
     * @param hi 上限
     * @return 限制后的值
     */
    static int clamp(int v, int lo, int hi);

    /**
     * @brief 计算顺时针角度跨度
     * @param a1 起始角度
     * @param a2 结束角度
     * @return 顺时针方向的角度跨度 [0,360)
     * @details 计算从a1到a2的顺时针角度距离，处理跨越0度的情况
     */
    static int cwSpan(int a1, int a2);

    /**
     * @brief 方位角转换为数学角度
     * @param azDeg 方位角度数（北向为0度，顺时针递增）
     * @return 数学角度（弧度制）
     * @details 将雷达方位角坐标系转换为Qt绘图坐标系
     *
     * 坐标系转换说明：
     * - 输入：北向为0度，顺时针递增的方位角
     * - 输出：数学坐标系角度（弧度），3点钟方向为0，逆时针为正
     * - 转换公式：数学角 = 90° - 方位角
     */
    static double azToThetaRad(int azDeg);

    /**
     * @brief 同步编辑框显示值
     * @details 将内部角度值同步显示到各个输入框中
     */
    void syncEditors();

    /**
     * @brief 连接编辑框信号
     * @details 建立输入框与内部逻辑的信号槽连接
     */
    void connectEditors();

    /**
     * @brief 绘制方位角刻度盘
     * @param p 绘制器对象
     * @param rcDial 刻度盘绘制区域
     * @details 绘制圆形方位角刻度盘，显示当前角度范围
     */
    void drawAzDial(QPainter &p, const QRect &rcDial);

    /**
     * @brief 绘制俯仰角条形图
     * @param p 绘制器对象
     * @param rcBar 条形图绘制区域
     * @details 绘制垂直俯仰角条形图，显示当前角度范围
     */
    void drawElBar(QPainter &p, const QRect &rcBar);
    int toDisplayAngle(int internalDeg);
};

#endif // AZELRANGEWIDGET_H
