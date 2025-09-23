/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:13
 * @Description: 
 */
/**
 * @file sectorpolargrid.h
 * @brief 扇形极坐标网格定义
 * @details 专门为扇形显示设计的网格类，提供角度受限的极坐标网格渲染
 * 
 * 功能特性：
 * 1. 扇形范围控制：支持任意角度范围的网格显示
 * 2. 智能网格间隔：根据显示范围自动调整网格密度
 * 3. 弧线绘制：在扇形范围内绘制距离圆弧而非完整圆
 * 4. 边界标识：清晰标识扇形的角度边界
 * 5. 标签系统：智能放置距离和角度标签
 * 
 * 设计特点：
 * - 继承QGraphicsItem：集成到Qt图形场景系统
 * - 组合QObject：支持信号槽机制和对象管理
 * - 坐标转换：与PolarAxis紧密配合进行坐标映射
 * - 性能优化：只绘制可见范围内的网格元素
 * 
 * 应用场景：
 * - 雷达扇形扫描显示的背景网格
 * - 局部监控区域的坐标参考
 * - 角度受限场景的极坐标可视化
 * - 分屏显示中的专用网格系统
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

// sectorpolargrid.h - 专门为扇形显示设计的网格类
#ifndef SECTORPOLARGRID_H
#define SECTORPOLARGRID_H

#include <QObject>
#include <QGraphicsItem>
#include <QPen>
#include <QPainterPath>

class PolarAxis;

/**
 * @class SectorPolarGrid
 * @brief 扇形极坐标网格类
 * @details 专门为扇形显示优化的网格渲染组件，支持角度范围限制的极坐标网格
 * 
 * 核心功能：
 * 1. 扇形网格渲染：
 *    - 距离圆弧：在指定角度范围内绘制同心圆弧
 *    - 角度射线：从中心向外延伸的径向线条
 *    - 边界标识：清晰的扇形边界线条
 *    - 智能裁剪：只绘制扇形范围内的网格元素
 * 
 * 2. 自适应网格密度：
 *    - 距离间隔：根据显示范围自动计算合适的距离步长
 *    - 角度间隔：根据扇形跨度自动调整角度步长
 *    - 标签密度：避免标签重叠的智能标签放置
 * 
 * 3. 坐标系统集成：
 *    - PolarAxis依赖：利用极坐标轴进行精确的坐标转换
 *    - 像素映射：距离值到像素坐标的精确映射
 *    - 角度转换：多种角度系统间的准确转换
 * 
 * 4. 视觉定制：
 *    - 画笔配置：距离线、角度线、文本的独立样式
 *    - 颜色主题：适配暗色背景的网格颜色
 *    - 线条粗细：不同类型线条的层次化显示
 * 
 * 渲染优化：
 * - 边界检查：只计算和绘制可见范围内的元素
 * - 反锯齿：平滑的线条和文本渲染
 * - 层次管理：网格置于背景层，不遮挡数据
 * 
 * 坐标转换：
 * - Qt坐标系：0度在3点钟方向，逆时针为正
 * - 雷达坐标系：0度在12点钟方向，顺时针为正
 * - 自动转换：内部处理坐标系统差异
 */
class SectorPolarGrid : public QObject, public QGraphicsItem {
    Q_OBJECT
public:
    explicit SectorPolarGrid(PolarAxis* axis, QGraphicsItem* parent = nullptr);

    /**
     * @brief 设置扇形显示范围
     * @param minAngle 最小角度（度）
     * @param maxAngle 最大角度（度）
     * @details 设置网格显示的角度范围，只在此范围内绘制网格元素
     * 
     * 角度定义：
     * - 0度：正北方向（12点钟方向）
     * - 正值：顺时针方向
     * - 负值：逆时针方向
     * - 范围：通常-180到+180度
     * 
     * 效果：
     * - 距离圆：只绘制角度范围内的圆弧段
     * - 角度线：只绘制范围内的径向射线
     * - 边界线：高亮显示扇形边界
     * - 标签：只在可见范围内放置标签
     */
    // 设置扇形显示范围
    void setSectorRange(float minAngle, float maxAngle);

    /**
     * @brief 获取边界矩形
     * @return 图形项的边界矩形
     * @details 计算包含所有网格元素和标签的最小边界矩形
     */
    // QGraphicsItem 接口
    QRectF boundingRect() const override;
    
    /**
     * @brief 绘制网格
     * @param painter 绘图对象
     * @param option 绘图选项
     * @param widget 目标窗口组件
     * @details 执行扇形网格的完整绘制，包括圆弧、射线和标签
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

public slots:
    /**
     * @brief 更新网格显示
     * @details 触发网格重绘，通常在坐标轴参数变化时调用
     */
    void updateGrid();

private:
    PolarAxis* m_axis;               ///< 极坐标轴系统指针
    float m_minAngle = -30.0f;       ///< 最小角度（默认-30度）
    float m_maxAngle = 30.0f;        ///< 最大角度（默认30度）

    QPen m_rangePen;                 ///< 距离圈绘制画笔
    QPen m_anglePen;                 ///< 角度线绘制画笔
    QPen m_textPen;                  ///< 文本标签画笔
    QPen m_borderPen;                ///< 扇形边界线画笔

    /**
     * @brief 绘制距离同心圆
     * @param painter 绘图对象
     * @details 在扇形范围内绘制等距离的同心圆弧
     */
    void drawRangeCircles(QPainter* painter);
    
    /**
     * @brief 绘制角度射线
     * @param painter 绘图对象
     * @details 绘制从中心向外的径向线条和扇形边界
     */
    void drawAngleLines(QPainter* painter);
    
    /**
     * @brief 绘制标签
     * @param painter 绘图对象
     * @details 在合适位置放置距离和角度标签
     */
    void drawLabels(QPainter* painter);
    
    /**
     * @brief 绘制扇形背景
     * @param painter 绘图对象
     * @details 绘制半透明的扇形背景填充
     */
    void drawSectorBackground(QPainter* painter);

    /**
     * @brief 检查角度是否在扇形范围内
     * @param angle 要检查的角度值
     * @return true表示在范围内，false表示超出范围
     * @details 判断指定角度是否在当前设置的扇形范围内
     */
    // 检查角度是否在扇形范围内
    bool isAngleInSector(float angle) const;
};

#endif // SECTORPOLARGRID_H
