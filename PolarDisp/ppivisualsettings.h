/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-23 09:44:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:52
 * @Description: 
 */
/**
 * @file ppivisualsettings.h
 * @brief PPI视觉设置组件头文件
 * @details 提供PPI视图的可视化参数配置界面，包括距离范围和地图类型设置
 *
 * 主要功能：
 * 1. 最大距离配置：用户可输入最大显示距离，支持回车键应用
 * 2. 地图类型选择：提供多种地图显示模式的切换功能
 * 3. 实时配置：参数变化即时生效，提升用户体验
 * 4. 参照设计：采用与MousePositionInfo相同的设计模式和布局风格
 *
 * 设计特点：
 * - 紧凑布局：适合作为叠加层显示在视图角落
 * - 直观操作：清晰的标签和控件，降低学习成本
 * - 即时反馈：配置变化立即通过信号通知相关组件
 * - 统一风格：与项目整体UI风格保持一致
 *
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

#ifndef PPIVISUALSETTINGS_H
#define PPIVISUALSETTINGS_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>

QT_BEGIN_NAMESPACE
namespace Ui { class PPIVisualSettings; }
QT_END_NAMESPACE

/**
 * @brief PPI视觉设置组件类
 * @details 提供PPI显示参数的可视化配置界面，支持距离范围和地图类型的实时调整
 *
 * 核心功能：
 * 1. 距离配置管理：
 *    - 最大距离输入：支持用户自定义显示范围上限
 *    - 实时验证：输入合法性检查，确保数值有效性
 *    - 快捷应用：支持回车键快速应用设置
 *
 * 2. 地图类型管理：
 *    - 多模式支持：无地图、路网图、标准图、卫星图、3D地图
 *    - 即时切换：选择变化立即生效，无需额外确认
 *    - 状态同步：与地图显示组件保持状态一致
 *
 * 3. 信号通信机制：
 *    - maxDistanceChanged: 最大距离变化通知
 *    - mapTypeChanged: 地图类型变化通知
 *    - 解耦设计：通过信号槽实现与其他组件的松耦合
 *
 * 使用模式：
 * - 叠加显示：作为PPIView的叠加层组件
 * - 固定位置：通常位于视图右下角，便于访问
 * - 响应式布局：根据视图大小调整自身位置
 *
 * 样式集成：
 * - QSS支持：可通过样式表定制外观
 * - 主题适配：支持深色/浅色主题切换
 * - 一致性：与项目整体视觉风格保持统一
 */
class PPIVisualSettings : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口组件，用于Qt对象树管理
     * @details 初始化UI界面，连接信号槽，设置默认值
     */
    explicit PPIVisualSettings(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     * @details 清理UI资源，确保内存正确释放
     */
    ~PPIVisualSettings();

    /**
     * @brief 获取当前最大距离设置
     * @return 最大距离值（公里）
     * @details 返回用户设置的最大显示距离，用于其他组件读取当前配置
     */
    double getMaxDistance() const;

    /**
     * @brief 设置最大距离值
     * @param distance 最大距离（公里）
     * @details 程序化设置最大距离，更新UI显示但不触发信号
     */
    void setMaxDistance(double distance);

    /**
     * @brief 获取当前地图类型索引
     * @return 地图类型索引 (0:无地图, 1:路网图, 2:标准图, 3:卫星图, 4:3D地图)
     * @details 返回当前选择的地图类型，便于状态同步和保存
     */
    int getMapType() const;

    /**
     * @brief 设置地图类型
     * @param index 地图类型索引
     * @details 程序化设置地图类型,更新UI显示但不触发信号
     */
    void setMapType(int index);

    /**
     * @brief 获取当前最大检测点数量设置
     * @return 最大检测点数量
     * @details 返回用户设置的最大检测点数量，用于限制内存占用
     */
    int getMaxPoints() const;

    /**
     * @brief 获取当前单批航迹最大点数设置
     * @return 单批航迹最大点数
     */
    int getMaxTrackPoints() const;

    /**
     * @brief 设置最大检测点数量
     * @param maxPoints 最大检测点数量
     * @details 程序化设置最大检测点数量，更新UI显示但不触发信号
     */
    void setMaxPoints(int maxPoints);

    /**
     * @brief 设置单批航迹最大点数
     * @param maxPoints 单批航迹最大点数
     */
    void setMaxTrackPoints(int maxPoints);

    /**
     * @brief 更新离线处理状态显示
     * @param processFlag 处理标志 (0=正常处理, 1=离线处理)
     * @param dataId 数据编号 (0~255)
     * @details 接收0xDD04离线处理状态帧后更新UI标签
     */
    void updateOfflineStatus(unsigned char processFlag, unsigned char dataId);

signals:
    /**
     * @brief 最大距离变化信号
     * @param distance 新的最大距离值（公里）
     * @details 当用户修改最大距离设置时发出，通知相关组件更新显示范围
     */
    void maxDistanceChanged(double distance);

    /**
     * @brief 地图类型变化信号
     * @param index 新的地图类型索引
     * @details 当用户切换地图类型时发出，通知地图组件切换显示模式
     */
    void mapTypeChanged(int index);

    /**
     * @brief 测距模式变化信号
     * @param enabled 是否启用测距模式
     * @details 当用户切换测距模式时发出，通知PPI视图启用/禁用测距功能
     */
    void measureModeChanged(bool enabled);

    /**
     * @brief 最大检测点数量变化信号
     * @param maxPoints 新的最大检测点数量
     * @details 当用户修改最大检测点数量设置时发出，通知DetManager更新限制
     */
    void maxPointsChanged(int maxPoints);

    /**
     * @brief 单批航迹最大点数变化信号
     * @param maxPoints 新的单批航迹最大点数
     */
    void maxTrackPointsChanged(int maxPoints);

    /**
     * @brief 请求清除P显数据信号
     * @details 当用户点击"显清"按钮并确认后发出，通知PPIView清除检测点和航迹点
     */
    void clearDisplayRequested();

    /**
     * @brief 道路点可见性变化信号
     * @param visible true表示显示道路点，false表示隐藏
     */
    void roadVisibilityChanged(bool visible);


protected:
    /**
     * @brief 重写paintEvent以确保样式表正确渲染并绘制自定义箭头
     * @param event 绘制事件对象
     * @details 使用QStyleOption确保QSS样式能够正确应用，并手绘ComboBox箭头
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief 事件过滤器
     * @param obj 事件目标对象
     * @param event 事件对象
     * @return 是否处理了事件
     * @details 监听ComboBox的Enter/Leave事件，触发重绘以更新箭头颜色
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief 距离输入回车处理
     * @details 响应用户在距离输入框中按下回车键，验证并应用新的距离设置
     */
    void onDistanceEditReturnPressed();

    /**
     * @brief 地图类型选择变化处理
     * @param index 新选择的地图类型索引
     * @details 响应用户在地图类型下拉框中的选择变化，立即应用新设置
     */
    void onMapTypeChanged(int index);

    /**
     * @brief 测距按钮切换处理
     * @param checked 测距按钮是否被选中
     * @details 响应用户点击测距按钮，切换测距模式状态
     */
    void onMeasureToggled(bool checked);

    /**
     * @brief 最大检测点数量输入回车处理
     * @details 响应用户在检测点数量输入框中按下回车键，验证并应用新的数量限制
     */
    void onMaxPointsEditReturnPressed();

    /**
     * @brief 单批航迹最大点数输入回车处理
     */
    void onMaxTrackPointsEditReturnPressed();

    /**
     * @brief 显清按钮点击处理
     * @details 响应用户点击"显清"按钮，弹出确认对话框后清除P显数据
     */
    void onClearDisplayClicked();

private:
    Ui::PPIVisualSettings *ui;  ///< UI界面对象指针
    QLabel *m_processStatusLabel = nullptr;  ///< 数据处理状态标签
    QCheckBox *m_roadCheckBox = nullptr;     ///< 道路点显示开关
    QLabel *m_maxTrackPointsLabel = nullptr; ///< 单批航迹最大点数标签
    QLineEdit *m_maxTrackPointsEdit = nullptr; ///< 单批航迹最大点数输入框
    // Map engine selection removed per rollback decision

    /**
     * @brief 设置组件样式
     * @details 应用统一的样式表，确保与项目整体风格一致
     */
    void setupStyle();

    /**
     * @brief 连接信号槽
     * @details 建立UI控件与处理函数的信号槽连接
     */
    void connectSignals();

    /**
     * @brief 创建单批航迹最大点数配置行
     */
    void setupTrackPointLimitRow();

    /**
     * @brief 验证距离输入
     * @param distance 输入的距离值
     * @return 验证结果，true表示有效
     * @details 检查距离输入的合法性，确保在合理范围内
     */
    bool validateDistance(double distance) const;
};

#endif // PPIVISUALSETTINGS_H
