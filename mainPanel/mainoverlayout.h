/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-28 13:44:06
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
#include <QTimer>
#include <functional>
#include <vector>
#include "ui_mainoverlayout.h"
#include "Basic/Protocol.h"

// 前向声明 - 雷达显示相关组件
class PPIView;        ///< PPI雷达显示视图
class PPIScene;       ///< PPI雷达场景管理器
class ZoomViewWidget; ///< 缩放视图控制器
class SectorWidget;   ///< 扇区控制面板
class RangeAzimuthChartWidget; ///< 距离-方位图表显示面板
class RangeHeightChartWidget; ///< 距离-高度图表显示面板
class AzElRangeWidget; ///< 方位角和俯仰角范围控制部件
class mainviewTopLeft; ///< PPI视图左上角控制面板
class CustomComboBox;  ///< 自定义组合框
class CusWindow;       ///< 自定义窗口
class QPushButton;     ///< Qt按钮
class QLabel;          ///< Qt标签
class DataSaveUI;      ///< 数据存储管理对话框
class FrozenColumnHelper; ///< 表格冻结列辅助类
class ScreenRecorderWidget; ///< 屏幕录制与回放组件
class QTabWidget;      ///< Qt标签页控件
class QSplitter;
class QToolButton;


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
    /**
     * @brief 清除航迹表格数据
     * @details 响应"显清"按钮，清除航迹列表和无人机航迹列表的所有数据
     */
    void clearTrackTables();

    /**
     * @brief 追加外部系统日志到当前界面日志区
     * @param text 已格式化日志文本
     */
    void appendExternalLog(const QString& text);

signals:
    // TWS/TAS模式设置信号
    void sig_SetScanRangeParam(const ScanRange param);
    void sig_SetBeamControlParam(const BeamControl param);
    void sig_SetServoControlParam(const ServoControlParam param);

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
    void updateTargetClassification(unsigned int batchID, int targetType);

    /**
     * @brief 航迹删除通知
     * @param batchID 批次号
     * @details 当收到 statMethod==2 时，从表格中删除对应批号的航迹
     */
    void onTrackRemoved(int batchID);

    /**
     * @brief 清空所有航迹列表
     * @details 响应数据清理信号，清空航迹表格内容
     */
    void clearAllTracks();

    void flushPendingTrackTableUpdates();

    void flushPendingLogLines();

    /**
     * @brief 航迹标签页切换处理
     * @param index 当前标签页索引
     * @details 切换到无人机页签时，仅显示普通无人机航迹
     */
    void onTrackTabChanged(int index);

    // 雷达控制槽函数
    /**
     * @brief 打开处理软件启动对话框
     * @details 发送系统启动命令
     */
    void onStartSoftwareClicked();

    /**
     * @brief 打开处理软件关闭对话框
     * @details 发送系统关闭命令
     */
    void onStopSoftwareClicked();

    /**
     * @brief 打开TWS模式设置对话框
     * @details 配置TWS（Track-While-Scan）模式参数
     */
    void onTWSModeClicked();

    /**
     * @brief 打开TAS模式设置对话框
     * @details 配置TAS（Target Alert System）模式参数
     */
    void onTASModeClicked();

    /**
     * @brief 打开伺服控制对话框
     * @details 配置伺服控制参数
     */
    void onServoControlClicked();

    /**
     * @brief 执行伺服归北序列
     * @details 先下发方位归北0°，4秒后下发方位寻位0°
     */
    void onServoNorthClicked();

    /**
     * @brief 打开数据存储管理对话框
     * @details 配置数据保存和删除参数
     */
    void onDataStorageClicked();

    // 参数设置槽函数
    /**
     * @brief 打开数据处理参数对话框
     * @details 配置数据处理相关参数
     */
    void onDataProcessClicked();

    /**
     * @brief 打开信号处理参数对话框
     * @details 配置信号处理相关参数
     */
    void onSignalProcessClicked();

    /**
     * @brief 打开波形及采样控制对话框
     * @details 配置频率控制和波形参数
     */
    void onFreqControlClicked();

    /**
     * @brief 打开电调控制对话框
     * @details 配置波束控制参数
     */
    void onBatteryControlClicked();

    /**
     * @brief 打开方向图扫描控制对话框
     * @details 配置扫描范围参数
     */
    void onScanRangeClicked();

    /**
     * @brief 打开光电系统控制对话框
     * @details 配置光电参数
     */
    void onPhotoelectricClicked();

    /**
     * @brief 监控参数刷新槽函数
     * @param res 监控参数结构
     * @details 接收来自Controller的监控参数，更新系统健康状态
     */
    void monitorParamRef(MonitorParam res);

    /**
     * @brief 处理伺服控制回送
     * @param res 伺服回送数据
     */
    void onServoCtrlRet(ServoCtrlRet res);

    /**
     * @brief 处理BIT上报
     * @param res BIT上报数据
     */
    void onBITReport(BITReport res);

    /**
     * @brief 打开雷达系统健康管理对话框
     * @details 显示信号处理、数据处理、波束调度三个子系统的运行状态
     */
    void onRadarSystemClicked();

    /**
     * @brief 打开录屏回放窗口
     * @details 显示屏幕录制与回放管理界面
     */
    void onRecordPlayClicked();

    /**
     * @brief 绘制 adsb.m 对应的离线RAE点迹数据
     */
    void onDrawOfflineAdsbClicked();

    /**
     * @brief 绘制 compare.m 对应的离线RAE点迹数据
     */
    void onDrawOfflineCompareClicked();

    /**
     * @brief 处理发射开关按钮点击
     * @details 弹出确认对话框，确认后下发发射开关命令（接收默认开启）
     */
    void onTransmitControlClicked();

    /**
     * @brief 处理雷达待机按钮点击
     * @details 设置雷达工作模式为待机模式（workMode=2）
     */
    void onRadarStandbyClicked();

    /**
     * @brief 根据屏幕分辨率动态设置面板宽度、按钮高度等尺寸
     * @details 在 setupUi() 后调用，覆盖 .ui 中的固定像素值
     */
    void applyScaledSizes();

private:
    void setupResizableMainLayout();
    void setupOfflineRaeButtons();
    void drawOfflineRaeDataset(const QString& filePath, const QString& datasetName);
    QString resolveRuntimePath(const QString& path) const;

    Ui::MainOverLayOut *ui;           ///< UI界面对象指针
    PPIView* mView;                   ///< PPI雷达显示视图
    PPIScene* mScene;                 ///< PPI雷达场景管理器
    ZoomViewWidget* m_zoomView;       ///< 缩放视图控制器
    SectorWidget* m_sectorWidget = nullptr;     ///< 扇区显示控制器
    RangeAzimuthChartWidget* m_rangeAzimuthWidget; ///< 距离-方位图表显示控制器
    RangeHeightChartWidget* m_rangeHeightWidget; ///< 距离-高度图表显示控制器
    AzElRangeWidget* m_azElRangeWidget; ///< 方位角和俯仰角范围控制器
    mainviewTopLeft* m_topLeftWidget;   ///< PPI视图左上角控制面板，用于联动偏航和倾角
    QSplitter* m_mainSplitter = nullptr;
    QWidget* m_leftSidebar = nullptr;
    QWidget* m_rightSidebar = nullptr;

    // 航迹管理相关成员
    QMap<unsigned int, int> m_targetTypes;  ///< 批次号到目标类型编号的映射
    QMap<unsigned int, QDateTime> m_trackStartTimes; ///< 批次号到航迹开始时间的映射
    QMap<quint64, PointInfo> m_pendingTrackUpdates;  ///< 等待批量刷新的总航迹更新
    QMap<unsigned int, PointInfo> m_pendingDroneTrackUpdates;  ///< 等待批量刷新的无人机航迹更新
    QTimer* m_trackTableRefreshTimer = nullptr;      ///< 航迹表批量刷新定时器
    FrozenColumnHelper* m_trackTableFrozenHelper = nullptr;    ///< 总航迹表格冻结列辅助类
    FrozenColumnHelper* m_droneTableFrozenHelper = nullptr;    ///< 无人机表格冻结列辅助类
    QTableWidget* m_tbdTrackTable = nullptr;                  ///< TBD航迹表格
    QTableWidget* m_cooperativeTrackTable = nullptr;          ///< 协同航迹表格
    FrozenColumnHelper* m_tbdTrackTableFrozenHelper = nullptr; ///< TBD表格冻结列辅助类
    FrozenColumnHelper* m_cooperativeTrackFrozenHelper = nullptr; ///< 协同表格冻结列辅助类

    /**
     * @brief 初始化航迹管理功能
     * @details 设置航迹表格列头、连接信号槽、配置表格属性
     */
    void setupTrackManagement();

    void updateTrackStatsWidgets();

    void setupTrackTable(QTableWidget* tableWidget, FrozenColumnHelper*& frozenHelper);

    /**
     * @brief 初始化时按可视宽度铺满航迹表列宽，之后仍允许用户手动拖拽
     */
    void fitTrackTableColumnsToViewport(QTableWidget* tableWidget);

    void syncFrozenTrackTables();

    void scheduleTrackTableRefresh();

    void applyTrackListUpdate(const PointInfo& info, bool& sortMain, bool& sortTbd,
                              bool& sortCooperative);

    void applyDroneTrackListUpdate(const PointInfo& info, bool& sortDrone);

    void applyTrackTabDisplayMode();

    // simulateIncomingTracks (test helper) removed

    /**
     * @brief 添加或更新航迹表格中的行
     * @param tableWidget 目标表格
     * @param info 航迹信息
     * @param targetType 目标类型文本
     * @return 更新的行号
     */
    int addOrUpdateTrackRow(QTableWidget* tableWidget, const PointInfo& info, const QString& targetType,
                            bool* inserted = nullptr);

    void removeTrackRow(QTableWidget* tableWidget, unsigned type, unsigned int batch);

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

    /**
     * @brief 初始化日志信息组件
     * @details 设置日志文本框为只读，配置样式
     */
    void setupLogInfo();

    void scheduleLogFlush();

    /**
     * @brief 向界面日志区追加一行完整日志
     * @param line 已包含时间戳后的完整日志行
     */
    void appendLogLine(const QString& line);

    /**
     * @brief 记录命令日志
     * @param commandName 命令名称
     * @param parameters 命令参数描述
     * @details 将命令操作记录到logEdit中，显示时间戳、命令名和参数
     *          日志从顶部添加，自动限制最大行数
     */
    void logCommand(const QString &commandName, const QString &parameters);
    QString servoCommandText(unsigned char cmd) const;
    QString formatServoControlDetails(const ServoControlParam& param, const QString& source) const;

    /**
     * @brief 设置命令序列
     * @details 准备一键全数配置的命令列表
     */
    void setupCommands();

    /**
     * @brief 命令定时器超时槽函数
     * @details 定时器触发时依次执行命令列表中的命令
     */
    void cmdTimeOut();

    // 参数保存成员变量（对应旧框架中的参数）
    BatteryControlM m_batteryControlM;   ///< 阵地控制参数
    TranRecControl m_tranRecControlM;    ///< 发射接收控制参数
    DirGramScan m_freqControlM;          ///< 方向图扫描/频率控制参数
    BeamControl m_beamControl;           ///< 波束控制参数
    SigProParam m_sigProParam;           ///< 信号处理参数
    DataProParam m_dataProParam;         ///< 数据处理参数
    ScanRange m_scanRange;               ///< 扫描范围参数
    ServoControlParam m_servoControlParam;    ///< 伺服控制参数

    int m_maxLogLines;                   ///< 最大日志行数限制
    QStringList m_pendingLogLines;       ///< 等待批量刷新的日志行
    QTimer* m_logFlushTimer = nullptr;   ///< 日志批量刷新定时器

    // 一键全数配置相关成员
    QTimer* m_commandTimer;              ///< 命令执行定时器
    std::vector<std::function<void()>> m_commands; ///< 命令列表
    int m_commandIndex;                  ///< 当前命令索引

    // 健康管理相关成员
    bool m_systemNormal;                 ///< 系统整体健康状态（true=正常，false=异常）
    int m_sigProSta;                     ///< 信号处理软件状态
    int m_dataProSta;                    ///< 数据处理软件状态
    int m_beamConSta;                    ///< 波束调度软件状态
    int m_targetRecSta;                  ///< 目标识别软件状态
    BITReport m_lastBITReport;           ///< 最新的BIT上报信息

    // 健康管理窗口相关
    CusWindow* m_healthWindow;           ///< 雷达系统健康管理窗口指针
    QPushButton* m_sigProBtn;            ///< 信号处理状态按钮
    QPushButton* m_dataProBtn;           ///< 数据处理状态按钮
    QPushButton* m_beamConBtn;           ///< 波束调度状态按钮
    QPushButton* m_targetRecBtn;         ///< 目标识别状态按钮

    // 数据存储管理窗口相关
    CusWindow* m_dataStorageWindow;      ///< 数据存储管理窗口指针
    DataSaveUI* m_dataStorageDialog;     ///< 数据存储管理对话框指针

    // 录屏回放窗口相关
    CusWindow* m_recorderWindow;         ///< 录屏回放窗口指针
    ScreenRecorderWidget* m_recorderWidget; ///< 录屏回放组件指针
    QToolButton* m_offlineAdsbButton = nullptr;    ///< adsb.m离线点迹绘制按钮
    QToolButton* m_offlineCompareButton = nullptr; ///< compare.m离线点迹绘制按钮

    // BIT状态按钮
    QPushButton* m_btnTxOpen;            ///< 阵面发射开启状态
    QPushButton* m_btnDutyCycle;         ///< 占空比状态
    QPushButton* m_btnPulseWidth;        ///< 脉宽状态
    QPushButton* m_btnRxOpen;            ///< 阵面接收状态
    QPushButton* m_btnFreqSrc;           ///< 频率源状态
    QPushButton* m_btnDigBoard;          ///< 收发板状态
    QPushButton* m_btnServo;             ///< 伺服状态
    QPushButton* m_btnBeidou;            ///< 北斗状态
    QPushButton* m_btnBluetooth;         ///< 蓝牙状态
    QPushButton* m_btnPowerBoard;        ///< 波控板电源状态

    // 温度和角度标签
    QLabel* m_tempLabel;                 ///< 温度信息标签
    QLabel* m_angleLabel;                ///< 角度信息标签

    // BIT更新控制
    QDateTime m_lastBITUpdateTime;       ///< 上次BIT信息更新到界面的时间
    static constexpr int BIT_UPDATE_INTERVAL_SEC = 60;  ///< BIT信息更新间隔（秒）

    /**
     * @brief 更新健康管理窗口显示
     * @details 根据最新的监控参数和BIT信息更新窗口控件
     */
    void updateHealthWindow();

    // 雷达控制状态成员
    bool m_isTransmitting;               ///< 发射状态（true=发射开启，false=发射关闭）
    bool m_isStandby;                    ///< 待机状态（true=待机模式，false=工作模式）

    /**
     * @brief 更新发射按钮的显示状态
     * @details 根据当前发射状态更新按钮的颜色和文本
     */
    void updateTransmitButton();

    /**
     * @brief 更新待机按钮的显示状态
     * @details 根据当前待机状态更新按钮的颜色和文本
     */
    void updateStandbyButton();

    /**
     * @brief 如果当前处于待机状态，则触发进入工作
     */
    void enterWorkingModeIfStandby();

    /**
     * @brief 按登录权限紧凑排列参数设置按钮
     */
    void arrangeParamSettingsButtons();
};

#endif // MAINOVERLAYOUT_H
