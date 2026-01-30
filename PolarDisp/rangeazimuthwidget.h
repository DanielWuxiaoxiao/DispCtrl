/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28 09:35:27
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
 * @Description: 距离-方位显示组件
 */
/**
 * @file rangeazimuthwidget.h
 * @brief 距离-方位显示控件定义
 * @details 提供完整的距离-方位雷达显示控件，类似扇区显示但方位角范围更大
 *
 * 功能特性：
 * 1. 方位角范围：0~360°（可设置）
 * 2. 距离同步：与主PPI视图的距离范围自动同步
 * 3. 点迹同步：与主视图的点迹可见性和数量限制同步
 * 4. 点迹大小同步：支持全局点迹大小控制
 *
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2026
 */

#ifndef RANGEAZIMUTHWIDGET_H
#define RANGEAZIMUTHWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

class SectorScene;
class SectorDetManager;
class SectorTrackManager;
class PolarAxis;

/**
 * @class RangeAzimuthToolBar
 * @brief 距离-方位显示工具栏
 * @details 提供方位角范围控制的工具栏组件
 */
class RangeAzimuthToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit RangeAzimuthToolBar(QWidget* parent = nullptr);

    // 获取当前方位角范围
    double getMinAzimuth() const;
    double getMaxAzimuth() const;

signals:
    /**
     * @brief 方位角范围更新请求信号
     * @param minAzimuth 最小方位角(度)
     * @param maxAzimuth 最大方位角(度)
     */
    void azimuthRangeUpdateRequested(double minAzimuth, double maxAzimuth);

private slots:
    void onParameterChanged();

private:
    QLineEdit* m_minAzimuthLineEdit;
    QLineEdit* m_maxAzimuthLineEdit;
};

/**
 * @class RangeAzimuthView
 * @brief 距离-方位显示视图
 * @details 专门用于距离-方位显示的图形视图
 */
class RangeAzimuthView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit RangeAzimuthView(SectorScene* scene, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
};

/**
 * @class RangeAzimuthWidget
 * @brief 距离-方位显示主控件
 * @details 整合工具栏和视图的完整距离-方位显示组件
 */
class RangeAzimuthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RangeAzimuthWidget(QWidget* parent = nullptr);
    ~RangeAzimuthWidget();

    /**
     * @brief 设置距离范围（从主视图同步）
     * @param minRange 最小距离(公里)
     * @param maxRange 最大距离(公里)
     */
    void setRangeFromMain(double minRange, double maxRange);

    /**
     * @brief 设置检测点可见性（从主视图同步）
     * @param visible 是否可见
     */
    void setDetectionVisible(bool visible);

    /**
     * @brief 设置航迹可见性（从主视图同步）
     * @param visible 是否可见
     */
    void setTrackVisible(bool visible);

    /**
     * @brief 设置最大检测点数量（从主视图同步）
     * @param maxPoints 最大检测点数量
     */
    void setMaxDetectionPoints(int maxPoints);

    /**
     * @brief 设置最大航迹数量（从主视图同步）
     * @param maxTracks 最大航迹数量
     */
    void setMaxTrackPoints(int maxTracks);

    /**
     * @brief 设置检测点大小比例
     * @param ratio 缩放比例（1.0为默认大小）
     */
    void setDetectionSizeRatio(float ratio);

    /**
     * @brief 设置航迹点大小比例
     * @param ratio 缩放比例（1.0为默认大小）
     */
    void setTrackSizeRatio(float ratio);

    // 获取管理器指针（用于与主控制器连接）
    SectorDetManager* getDetManager() const { return m_detManager; }
    SectorTrackManager* getTrackManager() const { return m_trackManager; }

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onAzimuthRangeUpdateRequested(double minAzimuth, double maxAzimuth);

private:
    void setupUI();
    void connectSignals();

    RangeAzimuthToolBar* m_toolbar;
    RangeAzimuthView* m_view;
    SectorScene* m_scene;
    PolarAxis* m_axis;
    SectorDetManager* m_detManager;
    SectorTrackManager* m_trackManager;

    double m_currentMinRange;
    double m_currentMaxRange;
};

#endif // RANGEAZIMUTHWIDGET_H
