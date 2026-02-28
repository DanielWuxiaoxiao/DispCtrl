/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:32
 * @Description: 
 */
/**
 * @file customlinechart.h
 * @brief 自定义线性图表组件头文件
 * @details 提供基于直角坐标系的二维图表基类，支持：
 *          - 可配置的X/Y轴范围和刻度
 *          - 网格显示（主网格线/次网格线）
 *          - 点绘制（检测点、航迹点）
 *          - 暗色主题样式（匹配 darkstyle.qss）
 *          - 高性能渲染（QGraphicsView/Scene架构）
 *
 * 设计特点：
 * - 分层渲染：背景层（网格）、数据层（点）、前景层（坐标轴）
 * - 灵活配置：轴范围、刻度间隔、颜色可定制
 * - 样式统一：与现有PPI显示保持一致的暗色风格
 *
 * @author DispCtrl Development Team
 * @date 2026-01-28
 * @version 1.0
 */

#ifndef CUSTOMLINECHART_H
#define CUSTOMLINECHART_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QVector>
#include <QPen>
#include <QBrush>

/**
 * @struct ChartAxisConfig
 * @brief 图表坐标轴配置结构
 */
struct ChartAxisConfig {
    double minValue = 0.0;          ///< 最小值
    double maxValue = 100.0;        ///< 最大值
    double majorTickInterval = 10.0; ///< 主刻度间隔
    double minorTickInterval = 2.0;  ///< 次刻度间隔
    QString label = "";              ///< 轴标签
    QString unit = "";               ///< 单位
};

/**
 * @class CustomLineChart
 * @brief 自定义线性图表基类
 * @details 提供直角坐标系图表的基础实现，支持：
 *          - X/Y双轴配置
 *          - 网格绘制（主次网格线）
 *          - 坐标轴刻度和标签
 *          - 数据点绘制接口
 *
 * 使用示例：
 * @code
 * CustomLineChart* chart = new CustomLineChart(parent);
 *
 * // 配置X轴（方位角）
 * ChartAxisConfig xAxis;
 * xAxis.minValue = 0;
 * xAxis.maxValue = 360;
 * xAxis.majorTickInterval = 45;
 * xAxis.minorTickInterval = 15;
 * xAxis.label = "方位角";
 * xAxis.unit = "°";
 * chart->setXAxisConfig(xAxis);
 *
 * // 配置Y轴（距离）
 * ChartAxisConfig yAxis;
 * yAxis.minValue = 0;
 * yAxis.maxValue = 5000;
 * yAxis.majorTickInterval = 1000;
 * yAxis.label = "距离";
 * yAxis.unit = "m";
 * chart->setYAxisConfig(yAxis);
 *
 * // 添加数据点
 * chart->addPoint(180, 2500, QColor(0, 255, 0));
 * @endcode
 */
class CustomLineChart : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit CustomLineChart(QWidget* parent = nullptr);

    /**
     * @brief 虚析构函数
     */
    virtual ~CustomLineChart();

    /**
     * @brief 设置X轴配置
     * @param config 轴配置参数
     */
    void setXAxisConfig(const ChartAxisConfig& config);

    /**
     * @brief 设置Y轴配置
     * @param config 轴配置参数
     */
    void setYAxisConfig(const ChartAxisConfig& config);

    /**
     * @brief 获取X轴配置
     * @return 当前X轴配置
     */
    ChartAxisConfig xAxisConfig() const { return m_xAxisConfig; }

    /**
     * @brief 获取Y轴配置
     * @return 当前Y轴配置
     */
    ChartAxisConfig yAxisConfig() const { return m_yAxisConfig; }

    /**
     * @brief 添加数据点
     * @param x X坐标值（数据坐标系）
     * @param y Y坐标值（数据坐标系）
     * @param color 点的颜色
     * @param size 点的大小（像素）
     * @return 创建的点图形项指针
     */
    QGraphicsEllipseItem* addPoint(double x, double y, const QColor& color, double size = 3.0);

    /**
     * @brief 清除所有数据点
     */
    void clearPoints();

    /**
     * @brief 设置网格可见性
     * @param visible true显示网格，false隐藏网格
     */
    void setGridVisible(bool visible);

    /**
     * @brief 设置坐标轴可见性
     * @param visible true显示坐标轴，false隐藏坐标轴
     */
    void setAxisVisible(bool visible);

    /** @brief 设置背景色（同时更新 scene backgroundBrush） */
    void setChartBgColor(const QColor& color);
    /** @brief 设置主网格线颜色 */
    void setChartGridMajorColor(const QColor& color);
    /** @brief 设置次网格线颜色 */
    void setChartGridMinorColor(const QColor& color);
    /** @brief 设置坐标轴线颜色 */
    void setChartAxisColor(const QColor& color);
    /** @brief 设置刻度/标签文字颜色 */
    void setChartTextColor(const QColor& color);

protected:
    /**
     * @brief 初始化场景
     * @details 创建QGraphicsScene并设置基本属性
     */
    void initScene();

    /**
     * @brief 绘制背景网格
     * @details 根据轴配置绘制主次网格线
     */
    void drawGrid();

    /**
     * @brief 绘制坐标轴
     * @details 绘制X/Y轴线、刻度和标签
     */
    void drawAxes();

    /**
     * @brief 将数据坐标转换为场景坐标
     * @param dataX 数据X坐标
     * @param dataY 数据Y坐标
     * @return 场景坐标点
     */
    QPointF dataToScene(double dataX, double dataY) const;

    /**
     * @brief 将场景坐标转换为数据坐标
     * @param sceneX 场景X坐标
     * @param sceneY 场景Y坐标
     * @return 数据坐标点
     */
    QPointF sceneToData(double sceneX, double sceneY) const;

    /**
     * @brief 窗口大小变化事件
     * @param event 事件对象
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene;                    ///< 图形场景
    ChartAxisConfig m_xAxisConfig;              ///< X轴配置
    ChartAxisConfig m_yAxisConfig;              ///< Y轴配置

    QVector<QGraphicsLineItem*> m_gridLines;    ///< 网格线列表
    QVector<QGraphicsTextItem*> m_axisLabels;   ///< 轴标签列表
    QVector<QGraphicsEllipseItem*> m_dataPoints; ///< 数据点列表

    bool m_gridVisible = true;                   ///< 网格可见性
    bool m_axisVisible = true;                   ///< 坐标轴可见性

    // 样式配置（匹配darkstyle.qss）
    QColor m_bgColor = QColor(30, 30, 30);           ///< 背景色
    QColor m_gridMajorColor = QColor(60, 60, 60);    ///< 主网格线颜色
    QColor m_gridMinorColor = QColor(45, 45, 45);    ///< 次网格线颜色
    QColor m_axisColor = QColor(200, 200, 200);      ///< 坐标轴颜色
    QColor m_textColor = QColor(220, 220, 220);      ///< 文字颜色

    // 布局参数
    double m_leftMargin = 60.0;    ///< 左边距（Y轴标签空间）
    double m_rightMargin = 20.0;   ///< 右边距
    double m_topMargin = 20.0;     ///< 上边距
    double m_bottomMargin = 60.0;  ///< 下边距（X轴标签空间）

    /**
     * @brief 清除所有网格线
     */
    void clearGrid();

    /**
     * @brief 清除所有轴标签
     */
    void clearAxisLabels();

    /**
     * @brief 重建图表
     * @details 重新绘制网格、坐标轴和数据点
     */
    void rebuild();
};

#endif // CUSTOMLINECHART_H
