/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-24 11:22:47
 * @Description: 
 */
/**
 * @file mainoverlayout.h
 * @brief 主界面覆盖层布局管理器
 * @details 管理雷达显示系统的主要界面组件布局和交互
 *
 * 功能特性：
 * - 集成PPI雷达显示界面管理
 * - 缩放视图控制器集成
 * - 扇区显示控件管理
 * - 主要视图组件协调
 * - 界面布局自适应调整
 *
 * 组件架构：
 * - PPIView: 主要雷达显示视图
 * - PPIScene: 雷达场景管理器
 * - ZoomViewWidget: 缩放控制面板
 * - SectorWidget: 扇区控制面板
 *
 * 使用场景：
 * - 雷达显示主界面布局
 * - 多视图协调管理
 * - 用户界面集成控制
 *
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef MAINOVERLAYOUT_H
#define MAINOVERLAYOUT_H

#include <QWidget>
#include <QMap>
#include <QDateTime>
#include <QTableWidget>
#include "ui_mainoverlayout.h"
#include "Basic/Protocol.h"

// 前向声明 - 雷达显示相关组件
class PPIView;        ///< PPI雷达显示视图
class PPIScene;       ///< PPI雷达场景管理器
class ZoomViewWidget; ///< 缩放视图控制器
class SectorWidget;   ///< 扇区控制面板
class AzElRangeWidget; ///< 方位角和俯仰角范围控制部件
class mainviewTopLeft; ///< PPI视图左上角控制面板
class CustomComboBox;  ///< 自定义组合框


namespace Ui {
class MainOverLayOut;
}

/**
 * @class MainOverLayOut
 * @brief 主界面覆盖层布局管理器
 * @details 雷达显示系统的主要界面组件布局管理器
 *
 * 该类负责整合和管理雷达显示系统的各个核心UI组件：
 * - 统一管理PPI雷达显示界面
 * - 协调各个控制面板的布局和交互
 * - 提供界面组件的初始化和配置接口
 * - 管理视图间的数据同步和状态更新
 *
 * 布局架构：
 * - 主视图区域：PPI雷达显示
 * - 右上角控制区：缩放和扇区控制
 * - 覆盖层管理：透明控制面板
 * - 响应式布局：适应窗口尺寸变化
 *
 * @example
 * ```cpp
 * MainOverLayOut* mainLayout = new MainOverLayOut(parent);
 * mainLayout->mainPView();    // 初始化主视图
 * mainLayout->topRightSet();  // 配置右上角控制区
 * mainLayout->show();
 * ```
 */
class MainOverLayOut : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @details 初始化主界面布局管理器，创建UI界面并设置基本布局
     */
    explicit MainOverLayOut(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     * @details 清理资源，释放UI组件和相关对象
     */
    ~MainOverLayOut();

    /**
     * @brief 初始化主PPI视图
     * @details 创建和配置主要的PPI雷达显示视图
     *
     * 执行操作：
     * - 初始化PPIView和PPIScene
     * - 建立视图与场景的关联
     * - 配置显示参数和交互模式
     * - 设置视图在布局中的位置
     */
    void mainPView();

    /**
     * @brief 配置右上角控制区域
     * @details 设置和配置右上角的控制面板区域
     *
     * 执行操作：
     * - 初始化缩放控制面板
     * - 配置扇区控制面板
     * - 设置控制面板的布局位置
     * - 建立控制面板与主视图的交互连接
     */
    void topRightSet();

    /**
     * @brief 初始化范围设置组件
     * @details 创建和配置方位角和俯仰角范围控制组件
     *
     * 执行操作：
     * - 初始化AzElRangeWidget组件
     * - 将组件添加到范围设置tab中
     * - 设置默认角度范围
     * - 建立信号连接进行参数同步
     */
    void setupRangeSettings();

    /**
     * @brief 初始化工作方式设置组件
     * @details 创建和配置雷达工作方式设置组件
     *
     * 执行操作：
     * - 初始化工作方式相关控件
     * - 设置控件默认值和验证器
     * - 建立信号连接
     * - 与PpiView topleft联动
     */
    void setupWorkModeSettings();

    /**
     * @brief 发送扫描范围参数
     * @details 打包当前工作方式参数并发送ScanRange命令
     */
    void sendScanRangeParams();

    /**
     * @brief 获取PPI视图指针
     * @return PPIView指针，用于外部访问PPI视图
     * @details 提供对内部PPI视图组件的访问接口，支持信号连接等操作
     */
    PPIView* getPPIView() const { return mView; }

private slots:
    /**
     * @brief 更新航迹显示列表
     * @param info 航迹点信息
     * @details 处理来自TrackManager的航迹数据，更新总航迹列表
     */
    void updateTrackList(const PointInfo& info);

    /**
     * @brief 更新无人机航迹列表
     * @param info 航迹点信息
     * @details 处理无人机类型的航迹数据，更新无人机专用航迹列表
     */
    void updateDroneTrackList(const PointInfo& info);

    /**
     * @brief 目标分类结果更新
     * @param batchID 批次号
     * @param targetType 目标类型
     * @details 根据目标分类结果更新航迹显示类型
     */
    void updateTargetClassification(unsigned short batchID, int targetType);

    /**
     * @brief 清空所有航迹列表
     * @details 响应数据清理信号，清空航迹表格内容
     */
    void clearAllTracks();

private:
    Ui::MainOverLayOut *ui;           ///< UI界面对象指针
    PPIView* mView;                   ///< PPI雷达显示视图
    PPIScene* mScene;                 ///< PPI雷达场景管理器
    ZoomViewWidget* m_zoomView;       ///< 缩放视图控制器
    SectorWidget* m_sectorWidget;     ///< 扇区显示控制器
    AzElRangeWidget* m_azElRangeWidget; ///< 方位角和俯仰角范围控制器
    mainviewTopLeft* m_topLeftWidget;   ///< PPI视图左上角控制面板，用于联动偏航和倾角

    // 航迹管理相关成员
    QMap<unsigned short, int> m_targetTypes;  ///< 批次号到目标类型编号的映射
    QMap<unsigned short, QDateTime> m_trackStartTimes; ///< 批次号到航迹开始时间的映射

    /**
     * @brief 初始化航迹管理功能
     * @details 设置航迹表格列头、连接信号槽、配置表格属性
     */
    void setupTrackManagement();

    /**
     * @brief 添加或更新航迹表格中的行
     * @param tableWidget 目标表格
     * @param info 航迹信息
     * @param targetType 目标类型文本
     * @return 更新的行号
     */
    int addOrUpdateTrackRow(QTableWidget* tableWidget, const PointInfo& info, const QString& targetType);

    /**
     * @brief 对航迹表格进行排序
     * @param tableWidget 要排序的表格
     * @details 无人机目标置顶，相同类别按时间排序
     */
    void sortTrackTable(QTableWidget* tableWidget);

    /**
     * @brief 获取目标类型文本
     * @param targetType 目标类型编号
     * @return 目标类型文本描述
     */
    QString getTargetTypeText(int targetType) const;
};

#endif // MAINOVERLAYOUT_H
