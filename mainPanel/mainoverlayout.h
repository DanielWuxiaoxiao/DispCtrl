/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-26 15:06:48
 * @Description: 
 */
/**
 * @file mainoverlayout.h
 * @brief 主界面覆盖层布局管理器
 * @details 管理雷达显示系统的主要界面组件布局和交互
 *
 * 功能特性：
 * - 集成PPI雷达显示界面管理
 * - 主要视图组件协调
 * - 界面布局自适应调整
 *
 * 组件架构：
 * - PPIView: 主要雷达显示视图
 * - PPIScene: 雷达场景管理器
 *
 * 使用场景：
 * - 雷达显示主界面布局
 * - 用户界面集成控制
 *
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#ifndef MAINOVERLAYOUT_H
#define MAINOVERLAYOUT_H

#include <QWidget>
#include "ui_mainoverlayout.h"
#include "Basic/MarineProtocol.h"

// 前向声明 - 雷达显示相关组件
class PPIView;        ///< PPI雷达显示视图
class PPIScene;       ///< PPI雷达场景管理器
class CusWindow;       ///< 自定义窗口
class QPushButton;     ///< Qt按钮
class QLabel;          ///< Qt标签
class QComboBox;       ///< Qt组合框
class QSlider;         ///< Qt滑动条
class ColorBarWidget;  ///< 色阶图例
class EchoLineChart;        ///< 回波A显折线图


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

public slots:
    void clearTrackTables();

signals:

private slots:
    void applyScaledSizes();
    void setupPPIOverlay();

private:
    Ui::MainOverLayOut *ui;           ///< UI界面对象指针
    PPIView* mView;                   ///< PPI雷达显示视图
    PPIScene* mScene;                 ///< PPI雷达场景管理器

    /**
     * @brief 记录命令日志
     * @param commandName 命令名称
     * @param parameters 命令参数描述
     * @details 记录到统一日志系统，船用界面不再保留旧 X576 日志文本框入口
     */
    void logCommand(const QString &commandName, const QString &parameters);

    // PPI浮动覆盖层成员
    QWidget* m_ppiOverlay = nullptr;           ///< PPI左上角浮动覆盖层容器
    QLabel* m_lblOverlayRange = nullptr;       ///< 量程显示标签
    QLabel* m_lblRangeRing = nullptr;          ///< 距标圈指示标签
    QLabel* m_lblTransmitIndicator = nullptr;  ///< 发射状态指示标签

    // PPI左下角SIMRAD导航信息覆盖层
    QWidget* m_ppiNavOverlay = nullptr;        ///< PPI左下角浮动导航信息容器
    QLabel* m_lblNavPos = nullptr;             ///< 经纬度显示标签
    QLabel* m_lblNavCursor = nullptr;          ///< 光标距离(NM)+方位(°T)标签

    // 船用雷达控制相关
    ColorBarWidget* m_colorBar = nullptr;       ///< PPI色阶图例
    QComboBox*   m_rangeCombo = nullptr;        ///< 量程选择下拉框
    QComboBox*   m_gainCombo = nullptr;         ///< 波束锐化选择 (关/低/中/高)
    QComboBox*   m_interferenceCombo = nullptr;  ///< 同频干扰抑制 (关/低/中/高)
    QSlider*     m_levelSlider = nullptr;        ///< 截位选择滑块
    QSlider*     m_seaSlider = nullptr;         ///< 海浪抑制滑块
    QSlider*     m_rainSlider = nullptr;        ///< 雨雪抑制滑块
    QPushButton* m_btnTxToggle = nullptr;       ///< 发射开/关切换按钮
    QComboBox*   m_servoCombo = nullptr;        ///< 天线转速选择 (0/8)
    QPushButton* m_btnSendMarineControl = nullptr;
    QLabel*      m_levelValLabel = nullptr;
    QLabel*      m_seaValLabel = nullptr;
    QLabel*      m_rainValLabel = nullptr;

    void setupMarineControls();                 ///< 替换雷达控制页为船用控件
    void setupColorBar();                       ///< 创建PPI色阶图例
    void syncMarineRange(int rangeIndex);       ///< 同步量程到各组件
    MarineControlFrame buildMarineControlFrameFromUi() const;
    void sendMarineControlFromUi();
    void sendMarineControlCommandFromUi(uint8_t command);
    void setupSimradNavPanel();                 ///< 创建SIMRAD风格导航数据面板
    void setupSimradTheme();                    ///< 应用SIMRAD橙色主题

    // SIMRAD 导航数据面板成员
    QWidget* m_navPanel = nullptr;              ///< 导航数据面板容器
    QLabel*  m_lblSOGVal = nullptr;             ///< SOG 数值标签
    QLabel*  m_lblHDGVal = nullptr;             ///< HDG 数值标签
    QLabel*  m_lblCOGVal = nullptr;             ///< COG 数值标签
    QLabel*  m_lblTURNVal = nullptr;            ///< TURN 数值标签
    QLabel*  m_lblPOSLat = nullptr;             ///< 纬度标签
    QLabel*  m_lblPOSLon = nullptr;             ///< 经度标签
    QLabel*  m_lblDepthVal = nullptr;           ///< 水深数值标签
    QLabel*  m_lblNavTime = nullptr;            ///< 时间标签
    QLabel*  m_lblNavDate = nullptr;            ///< 日期标签
    QWidget* m_marineCtrlPanel = nullptr;       ///< 折叠式雷达控制面板

    // 伺服上行状态显示标签
    QLabel*  m_lblStRangeVal = nullptr;          ///< 当前量程
    QLabel*  m_lblStTxState  = nullptr;          ///< 发射状态
    QLabel*  m_lblStGain     = nullptr;          ///< 波束锐化
    QLabel*  m_lblStLevel    = nullptr;          ///< 截位选择
    QLabel*  m_lblStSea      = nullptr;          ///< 海浪抑制
    QLabel*  m_lblStRain     = nullptr;          ///< 雨雪抑制
    QLabel*  m_lblStGanRao   = nullptr;          ///< 同频干扰
    QLabel*  m_lblStFreq     = nullptr;          ///< 频综状态
    void setupServoStatusPanel();                ///< 创建伺服状态显示面板
    void onMarineStatusUpdated(const MarineRadarStatus& status); ///< 状态更新槽

    // PPI / A显 切换
    EchoLineChart* m_echoLineChart = nullptr;    ///< 回波A显折线图
    QPushButton*   m_btnToggleAScope = nullptr;  ///< PPI↔A显 切换按钮
    bool           m_showAScope = false;         ///< 当前是否显示A显
    void setupAScopeToggle();                    ///< 创建切换按钮
};

#endif // MAINOVERLAYOUT_H
