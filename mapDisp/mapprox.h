/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:19
 * @Description: 
 */
/**
 * @file mapprox.h
 * @brief 地图代理组件头文件
 * @details Web地图集成显示系统：
 *          - 集成QWebEngineView显示在线地图
 *          - 提供Qt与JavaScript的双向通信桥梁
 *          - 支持地图中心定位和缩放控制
 *          - 提供地图样式和显示模式切换
 *          - 实现雷达数据与地理信息的叠加显示
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef MAPPROXYWIDGET_H
#define MAPPROXYWIDGET_H

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QtWebChannel>
#include <QWebEngineView>

/**
 * @class MapProxyWidget
 * @brief 地图代理组件
 * @details Web地图的Qt集成代理，提供：
 *          - QWebEngineView的封装和管理
 *          - Qt信号槽与JavaScript的通信机制
 *          - 地图显示参数的控制接口
 *          - 多种地图源和显示模式支持
 * 
 * 通信机制：
 * Qt Application ←→ MapProxyWidget ←→ QWebChannel ←→ JavaScript ←→ Web Map
 * 
 * 功能特点：
 * - 地理坐标定位和缩放
 * - 灰度显示模式切换
 * - 多地图源选择
 * - 实时参数同步
 * 
 * @example 基本使用：
 * @code
 * MapProxyWidget* mapProxy = new MapProxyWidget();
 * QWebEngineView* view = mapProxy->getView();
 * layout->addWidget(view);
 * mapProxy->setCenterOn(120.0, 30.0, 10.0);  // 定位到指定坐标
 * @endcode
 */
class MapProxyWidget : public QObject
{
    Q_OBJECT  ///< Qt元对象系统支持，启用信号槽机制
    
public:
    /**
     * @brief 构造函数
     * @details 初始化地图代理组件：
     *          - 创建QWebEngineView实例
     *          - 设置WebChannel通信桥梁
     *          - 配置JavaScript接口
     *          - 加载地图HTML页面
     */
    MapProxyWidget();
    
    /**
     * @brief 获取Web视图对象
     * @return QWebEngineView指针
     * @details 返回内部的Web视图对象，用于：
     *          - 嵌入到Qt布局中
     *          - 设置显示属性
     *          - 访问Web页面内容
     */
    QWebEngineView* getView() {
        return mView;
    }

private:
    QWebEngineView* mView;   ///< Web引擎视图对象，承载地图显示

public slots:
    /**
     * @defgroup MapControlSlots 地图控制槽函数
     * @brief 接收来自HTML/JavaScript的地图控制请求
     * @{
     */
    
    /**
     * @brief 设置地图中心位置和缩放级别
     * @param lng 经度值(度)
     * @param lat 纬度值(度)  
     * @param range 显示范围(公里)
     * @details 响应HTML页面的定位请求：
     *          - 将雷达坐标转换为地理坐标
     *          - 计算合适的地图缩放级别
     *          - 通过信号通知HTML更新地图视图
     */
    void setCenterOn(float lng, float lat, float range);
    
    /**
     * @brief 同步雷达中心位置和范围到地图
     * @param longitude 雷达中心经度
     * @param latitude 雷达中心纬度
     * @param range 雷达范围（公里）
     * @details 响应PPIView的雷达位置/范围变化，同步更新地图显示范围
     */
    void syncRadarToMap(double longitude, double latitude, double range);
    
    /**
     * @brief 设置地图灰度显示模式
     * @param value 灰度值，0-100的整数值
     * @details 控制地图的灰度显示：
     *          - 0: 完全彩色显示
     *          - 100: 完全灰度显示
     *          - 中间值: 部分灰度效果
     *          - 用于在雷达数据叠加时减少背景干扰
     */
    void setGray(int value);
    
    /**
     * @brief 选择地图类型
     * @param index 地图类型索引
     * @details 切换不同的地图数据源：
     *          - 0: 卫星地图
     *          - 1: 街道地图  
     *          - 2: 地形地图
     *          - 3: 海图模式
     *          - 根据雷达应用场景选择最适合的地图类型
     */
    void chooseMap(int index);
    
    /** @} */ // end of MapControlSlots group

signals:
    /**
     * @defgroup MapControlSignals 地图控制信号
     * @brief 发送给HTML/JavaScript的地图控制信号
     * @{
     */
    
    /**
     * @brief 地图中心定位信号
     * @param lng 经度值(度)
     * @param lat 纬度值(度)
     * @param range 显示范围(公里)
     * @details 通知HTML页面更新地图中心位置：
     *          - 由Qt应用程序发起的地图定位
     *          - JavaScript监听此信号并调用地图API
     *          - 实现雷达数据与地图的同步显示
     */
    void centerOn(float lng, float lat, float range);
    
    /**
     * @brief 灰度模式变化信号
     * @param value 新的灰度值
     * @details 通知HTML页面更新地图灰度显示：
     *          - 传递灰度参数到JavaScript
     *          - JavaScript应用CSS滤镜效果
     *          - 实时调整地图视觉效果
     */
    void changeGrayScale(int value);
    
    /** @} */ // end of MapControlSignals group
    
    /**
     * @note 预留信号接口
     * @details 这里可以添加更多与主窗口通信的信号：
     *          - 地图点击事件
     *          - 地图缩放变化事件
     *          - 地图加载状态事件
     */
    // 预留：更多与mainwindow的通信信号
};

#endif // MAPPROXYWIDGET_H

