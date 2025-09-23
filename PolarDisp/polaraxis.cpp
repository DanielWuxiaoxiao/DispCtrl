/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:08
 * @Description: 
 */
/**
 * @file polaraxis.cpp
 * @brief 极坐标轴系统实现文件
 * @details 实现极坐标轴系统的核心功能，包括坐标转换、距离计算、
 *          方位角计算等雷达显示系统必需的数学运算功能。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#include "polaraxis.h"
#include <QtMath>

/**
 * @brief PolarAxis构造函数
 * @details 初始化极坐标轴对象，使用默认的距离范围和像素密度设置
 * @param parent 父对象指针，用于Qt对象树管理
 * 
 * 默认设置：
 * - 最小距离：0米
 * - 最大距离：5000米（5公里）
 * - 像素密度：1像素/米
 */
PolarAxis::PolarAxis(QObject* parent)
    : QObject(parent) {

}

/**
 * @brief 设置距离显示范围
 * @details 更新PPI显示的最小和最大距离范围，并发射范围变化信号通知其他组件
 * @param minR 最小显示距离（米），通常为0
 * @param maxR 最大显示距离（米），如5000米表示5公里探测范围
 * 
 * 功能说明：
 * - 更新内部距离范围变量
 * - 触发rangeChanged信号，通知界面组件重新绘制
 * - 影响坐标转换的有效范围
 * 
 * 使用场景：
 * - 雷达探测范围变化时
 * - 用户调整显示比例时
 * - 不同工作模式的距离设置
 */
void PolarAxis::setRange(double minR, double maxR) {
    m_minRange = minR;
    m_maxRange = maxR;
    emit rangeChanged(m_minRange, m_maxRange);
}

/**
 * @brief 设置像素密度
 * @details 配置每米对应的像素数，控制距离与屏幕像素的转换比例
 * @param pixelsPerMeter 像素密度（像素/米）
 * 
 * 参数说明：
 * - 值越大，显示越精细，同样距离占用更多像素
 * - 值越小，显示越粗糙，适合大范围显示
 * - 典型值：0.1-10.0，根据显示需求调整
 * 
 * 影响范围：
 * - rangeToPixel()和pixelToRange()的转换结果
 * - polarToScene()和sceneToPolar()的精度
 * - 整体显示的缩放效果
 */
void PolarAxis::setPixelsPerMeter(double pixelsPerMeter)
{
    m_pixelsPerMeter = pixelsPerMeter;
}
/**
 * @brief 物理距离转换为像素距离
 * @details 将实际的物理距离（米）转换为屏幕显示的像素距离
 * @param distance 物理距离（米）
 * @return double 对应的像素距离
 * 
 * 转换公式：
 * 像素距离 = 物理距离 × 像素密度
 * 
 * 应用场景：
 * - 绘制雷达目标时确定屏幕位置
 * - 计算距离圈的半径
 * - 将探测数据映射到显示坐标
 * 
 * @note 返回值可能为小数，用于精确的图形绘制
 */
double PolarAxis::rangeToPixel(double distance) const {
    return distance * m_pixelsPerMeter;
}

/**
 * @brief 像素距离转换为物理距离
 * @details 将屏幕上的像素距离转换为实际的物理距离（米）
 * @param pixel 像素距离
 * @return double 对应的物理距离（米）
 * 
 * 转换公式：
 * 物理距离 = 像素距离 ÷ 像素密度
 * 
 * 应用场景：
 * - 鼠标点击位置转换为实际距离
 * - 测距功能的距离计算
 * - 用户交互位置的物理坐标获取
 * 
 * @note 与rangeToPixel()互为反函数
 */
double PolarAxis::pixelToRange(double pixel) const {
    return pixel / m_pixelsPerMeter;
}

/**
 * @brief 极坐标转换为场景坐标
 * @details 将雷达极坐标（距离+方位角）转换为Qt场景坐标系的点坐标
 * @param distance 物理距离（米）
 * @param azimuthDeg 方位角（度，0°为正北，顺时针为正方向）
 * @return QPointF 场景坐标点(x, y)
 * 
 * 坐标系转换说明：
 * 1. 雷达坐标系：以雷达为中心，0°为正北方向，顺时针递增
 * 2. 场景坐标系：Qt标准坐标系，X轴向右，Y轴向下
 * 3. 转换关系：
 *    - X坐标 = r × sin(θ)  （向右为正）
 *    - Y坐标 = -r × cos(θ) （向下为正，负号用于将0°对应到负Y轴）
 * 
 * 数学原理：
 * - 首先将距离转换为像素距离
 * - 将角度转换为弧度制
 * - 应用三角函数进行坐标变换
 * 
 * @note 0°方位角对应屏幕正上方（负Y轴方向）
 */
QPointF PolarAxis::polarToScene(double distance, double azimuthDeg) const {
    double r = rangeToPixel(distance);
    double rad = qDegreesToRadians(azimuthDeg);
    return QPointF(r * qSin(rad), -r * qCos(rad));
}

/**
 * @brief 场景坐标转换为极坐标
 * @details 将Qt场景坐标系的点坐标转换为雷达极坐标（距离+方位角）
 * @param scenePos 场景坐标点(x, y)
 * @return PolarCoord 极坐标结构体{距离(米), 方位角(度)}
 * 
 * 转换算法：
 * 1. 距离计算：r = √(x² + y²) - 计算点到原点的直线距离
 * 2. 方位角计算：θ = atan2(x, -y) - 使用反正切函数计算角度
 * 3. 角度标准化：将计算结果转换为[0, 360°)范围
 * 
 * 坐标系说明：
 * - 输入：Qt场景坐标（X右，Y下）
 * - 输出：雷达极坐标（0°北，顺时针）
 * - 转换：atan2(x, -y)确保0°对应正北方向
 * 
 * 特殊处理：
 * - 负角度自动转换为正角度（+360°）
 * - 像素距离转换为物理距离
 * - 弧度制转换为角度制
 * 
 * 应用场景：
 * - 鼠标点击位置的极坐标获取
 * - 目标选择时的距离方位计算
 * - 用户交互的坐标反算
 * 
 * @note 与polarToScene()互为反函数
 */
PolarAxis::PolarCoord PolarAxis::sceneToPolar(const QPointF& scenePos) const {
    double r = qSqrt(scenePos.x() * scenePos.x() + scenePos.y() * scenePos.y());
    double distance = pixelToRange(r);
    double azimuthDeg = qRadiansToDegrees(qAtan2(scenePos.x(), -scenePos.y()));
    if (azimuthDeg < 0) azimuthDeg += 360;
    return { distance, azimuthDeg };
}

