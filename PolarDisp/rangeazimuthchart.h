/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28 11:21:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:46
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-28
 * @Description: 距离-方位图表显示组件 - 基于直角坐标系
 */
/**
 * @file rangeazimuthchart.h
 * @brief 距离-方位图表组件头文件
 * @details 提供基于直角坐标系的距离-方位雷达显示，特点：
 *          - 横轴：方位角 0-360°
 *          - 纵轴：距离（米/公里）
 *          - 与主PPI距离范围自动同步
 *          - 支持检测点和航迹显示
 *          - 点迹可见性和大小可控
 *
 * 架构：继承自 CustomLineChart，专注于雷达数据显示
 *
 * @author DispCtrl Development Team
 * @date 2026-01-28
 * @version 1.0
 */

#ifndef RANGEAZIMUTHCHART_H
#define RANGEAZIMUTHCHART_H

#include "../cusWidgets/customlinechart.h"
#include "../Basic/Protocol.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMap>

/**
 * @class RangeAzimuthChartToolBar
 * @brief 距离-方位图表工具栏
 * @details 提供图表控制按钮、方位角范围设置和状态显示
 */
class RangeAzimuthChartToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit RangeAzimuthChartToolBar(QWidget* parent = nullptr);

    /**
     * @brief 获取最小方位角
     */
    double getMinAzimuth() const;

    /**
     * @brief 获取最大方位角
     */
    double getMaxAzimuth() const;

    /**
     * @brief 更新状态标签
     * @param detCount 检测点数量
     * @param trackCount 航迹数量
     */
    void updateStatus(int detCount, int trackCount);

signals:
    /**
     * @brief 清除数据请求信号
     */
    void clearRequested();

    /**
     * @brief 重置视图请求信号
     */
    void resetRequested();

    /**
     * @brief 方位角范围更新请求
     * @param minAz 最小方位角
     * @param maxAz 最大方位角
     */
    void azimuthRangeChanged(double minAz, double maxAz);

private slots:
    void onAzimuthRangeChanged();

private:
    QLineEdit* m_minAzimuthEdit;   ///< 最小方位角输入框
    QLineEdit* m_maxAzimuthEdit;   ///< 最大方位角输入框
    QPushButton* m_clearButton;    ///< 清除按钮
    QPushButton* m_resetButton;    ///< 重置按钮
    QLabel* m_statusLabel;         ///< 状态标签
};/**
 * @class RangeAzimuthChart
 * @brief 距离-方位图表核心类
 * @details 继承自 CustomLineChart，专门用于雷达数据的直角坐标系显示
 *
 * 主要功能：
 * - 显示检测点（绿色）和航迹点（黄色）
 * - 与主PPI距离范围同步
 * - 支持点数限制（FIFO队列）
 * - 可见性和大小控制
 *
 * 使用示例：
 * @code
 * RangeAzimuthChart* chart = new RangeAzimuthChart(parent);
 * chart->setRangeFromMain(0, 5000);  // 设置距离范围
 * chart->addDetectionPoint(pointInfo);  // 添加检测点
 * @endcode
 */
class RangeAzimuthChart : public CustomLineChart
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit RangeAzimuthChart(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~RangeAzimuthChart();

    /**
     * @brief 添加检测点
     * @param info 检测点信息
     */
    void addDetectionPoint(const PointInfo& info);

    /**
     * @brief 添加航迹点（从trackInfo结构）
     * @param info 航迹信息
     */
    void addTrackPoint(const trackInfo& info);

    /**
     * @brief 添加点信息（统一接口，根据type自动分发）
     * @param info 点信息（可以是检测点或航迹）
     * @note 根据 info.type 自动判断：1=检测点，2=航迹，3=TBD
     */
    void addPointInfo(const PointInfo& info);

    /**
     * @brief 清除所有雷达数据点
     */
    void clearRadarData();

    /**
     * @brief 设置距离范围（从主视图同步）
     * @param minRange 最小距离（米）
     * @param maxRange 最大距离（米）
     */
    void setRangeFromMain(double minRange, double maxRange);

    /**
     * @brief 设置检测点可见性
     * @param visible true显示，false隐藏
     */
    void setDetectionVisible(bool visible);

    /**
     * @brief 设置航迹可见性
     * @param visible true显示，false隐藏
     */
    void setTrackVisible(bool visible);

    /**
     * @brief 设置检测点大小比例
     * @param ratio 大小比例（0.5-3.0）
     */
    void setDetectionSizeRatio(double ratio);

    /**
     * @brief 设置航迹大小比例
     * @param ratio 大小比例（0.5-3.0）
     */
    void setTrackSizeRatio(double ratio);

    /**
     * @brief 设置最大检测点数量
     * @param maxPoints 最大点数
     */
    void setMaxDetectionPoints(int maxPoints);

    /**
     * @brief 设置最大航迹点数量
     * @param maxTracks 最大航迹数
     */
    void setMaxTrackPoints(int maxTracks);

    /**
     * @brief 设置方位角范围过滤
     * @param minAz 最小方位角（度）
     * @param maxAz 最大方位角（度）
     */
    void setAzimuthRange(double minAz, double maxAz);

signals:
    /**
     * @brief 点数统计更新信号
     * @param detCount 检测点数量
     * @param trackCount 航迹数量
     */
    void pointCountChanged(int detCount, int trackCount);

private:
    /**
     * @brief 检测点图形项结构
     */
    struct DetectionItem {
        QGraphicsEllipseItem* graphicsItem;  ///< 图形项
        PointInfo info;                      ///< 点信息
        qint64 timestamp;                    ///< 添加时间戳
    };

    /**
     * @brief 航迹图形项结构
     */
    struct TrackItem {
        QGraphicsEllipseItem* graphicsItem;  ///< 图形项
        trackInfo trackData;                 ///< 航迹信息
        qint64 timestamp;                    ///< 添加时间戳
    };

    QMap<int, DetectionItem> m_detections;    ///< 检测点映射表（批次号->DetectionItem）
    QVector<TrackItem> m_tracks;              ///< 航迹列表

    bool m_detectionVisible = true;           ///< 检测点可见性
    bool m_trackVisible = true;               ///< 航迹可见性

    double m_detectionSizeRatio = 1.0;        ///< 检测点大小比例
    double m_trackSizeRatio = 1.0;            ///< 航迹大小比例

    int m_maxDetectionPoints = 10000;         ///< 最大检测点数
    int m_maxTrackPoints = 1000;              ///< 最大航迹数

    // 方位角过滤范围
    double m_minAzimuth = 0.0;                ///< 最小方位角
    double m_maxAzimuth = 360.0;              ///< 最大方位角

    // 样式配置
    QColor m_detectionColor = QColor(0, 255, 0);    ///< 检测点颜色（绿色）
    QColor m_trackColor = QColor(255, 255, 0);      ///< 航迹颜色（黄色）

    double m_baseDetectionSize = 3.0;         ///< 基础检测点大小
    double m_baseTrackSize = 5.0;             ///< 基础航迹大小

    /**
     * @brief 限制检测点数量（FIFO）
     */
    void limitDetectionPoints();

    /**
     * @brief 限制航迹数量（FIFO）
     */
    void limitTrackPoints();

    /**
     * @brief 更新点数统计
     */
    void updatePointCount();

    /**
     * @brief 判断方位角是否在过滤范围内
     * @param azimuth 方位角（度）
     * @return true在范围内，false不在范围内
     */
    bool isAzimuthInRange(double azimuth) const;
};

/**
 * @class RangeAzimuthChartWidget
 * @brief 距离-方位图表完整控件
 * @details 包含工具栏和图表的完整控件，对外提供统一接口
 */
class RangeAzimuthChartWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit RangeAzimuthChartWidget(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~RangeAzimuthChartWidget();

    /**
     * @brief 获取图表实例
     * @return 图表指针
     */
    RangeAzimuthChart* chart() const { return m_chart; }

public slots:
    /**
     * @brief 处理清除请求
     */
    void onClearRequested();

    /**
     * @brief 处理重置请求
     */
    void onResetRequested();

private:
    RangeAzimuthChartToolBar* m_toolbar;  ///< 工具栏
    RangeAzimuthChart* m_chart;           ///< 图表
};

#endif // RANGEAZIMUTHCHART_H
