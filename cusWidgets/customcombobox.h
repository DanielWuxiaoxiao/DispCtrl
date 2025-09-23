/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-19 11:00:49
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:16
 * @Description: 
 */
/**
 * @file customcombobox.h
 * @brief 自定义ComboBox组件头文件
 * @details 实现带有自定义箭头绘制的QComboBox，支持绿色主题的下拉箭头
 * 
 * 功能特性：
 * 1. 自定义箭头绘制：绿色三角形箭头，符合应用主题
 * 2. Hover效果：鼠标悬浮时箭头颜色变化
 * 3. 完全兼容：保持QComboBox的所有原有功能
 * 4. 简洁设计：专注于箭头绘制，不影响其他功能
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QStyleOptionComboBox>
#include <QEvent>

/**
 * @brief 自定义ComboBox类
 * @details 继承自QComboBox，重写paintEvent来绘制自定义的绿色箭头
 * 
 * 设计要点：
 * 1. 保持原有功能：完全兼容QComboBox的所有API和行为
 * 2. 自定义箭头：绘制符合应用主题的绿色三角形箭头
 * 3. 状态响应：支持normal和hover状态的视觉反馈
 * 4. 简洁实现：最小化代码量，专注核心功能
 */
class CustomComboBox : public QComboBox
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口组件
     * @details 初始化自定义ComboBox，设置基本属性
     */
    explicit CustomComboBox(QWidget *parent = nullptr);

protected:
    /**
     * @brief 重写绘制事件
     * @param event 绘制事件对象
     * @details 绘制ComboBox背景和自定义箭头
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief 绘制自定义下拉箭头
     * @param painter 绘制器对象
     * @param rect ComboBox区域
     * @details 在指定区域绘制绿色三角形箭头，支持hover效果
     */
    void drawCustomArrow(QPainter *painter, const QRect &rect);
};

#endif // CUSTOMCOMBOBOX_H