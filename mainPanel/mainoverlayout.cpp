/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-28 13:44:05
 * @Description: 
 */
#include "mainoverlayout.h"

#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Controller/RadarDataManager.h"
#include "Controller/controller.h"
#include "PointManager/detmanager.h"
#include "PointManager/trackmanager.h"
#include "PolarDisp/mousepositioninfo.h"
#include "PolarDisp/ppisscene.h"
#include "PolarDisp/ppiview.h"
#include "PolarDisp/pviewtopleft.h"
#include "azelrangewidget.h"
#include "cusWidgets/customcombobox.h"
#include "cusWidgets/custommessagebox.h"
#include "cusWidgets/cuswindow.h"
#include "cusWidgets/detachablewidget.h"
#include "cusWidgets/frozentablewidget.h"
// 参数配置对话框头文件
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSplitter>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "PolarDisp/sectorscene.h"
#include "PolarDisp/sectorwidget.h"
#include "PolarDisp/rangeazimuthchart.h"
#include "PolarDisp/rangeheightchart.h"
#include "PolarDisp/zoomview.h"
#include "paramWidget/batterycontrol.h"
#include "paramWidget/dataprocessui.h"
#include "paramWidget/datasaveui.h"
#include "paramWidget/freqcontrolui.h"
#include "paramWidget/photoelectricparam.h"
#include "paramWidget/scanrangeui.h"
#include "paramWidget/twsmodedialog.h"
#include "paramWidget/tasmodedialog.h"
#include "paramWidget/servocontrol.h"
#include "paramWidget/sigparamui.h"
#include "paramWidget/tranrecvui.h"
#include "paramWidget/waveandsample.h"
#include "screenrecorderwidget.h"
#include "Basic/authmanager.h"

namespace {
constexpr int kTrackTableRefreshIntervalMs = 50;
constexpr int kLogFlushIntervalMs = 50;
constexpr bool kDefaultEnableSectorDisplay = false;

quint64 makeTrackTableUpdateKey(unsigned type, unsigned int batch)
{
    return (static_cast<quint64>(type) << 32) | static_cast<quint64>(batch);
}
}

MainOverLayOut::MainOverLayOut(QWidget* parent) : QWidget(parent), ui(new Ui::MainOverLayOut) {
    ui->setupUi(this);
    setupResizableMainLayout();

    // ========== 屏幕自适应：动态覆盖 UI 中的固定尺寸 ==========
    applyScaledSizes();

    // 状态信息面板包含发射接收控制checkboxes
    ui->infoTab->setToolTip("信息面板");

    topRightSet();
    mainPView();
    setupRangeSettings();
    setupWorkModeSettings();
    setupTrackManagement();
    setupLogInfo();

    // 初始化最大日志行数
    m_maxLogLines = 200;

    // ========== 从ConfigManager加载保存的参数 ==========
    // 伺服控制参数
    m_servoControlParam.cmd = CF_INS.servoCmd(0);
    m_servoControlParam.speed = CF_INS.servoSpeed(3);
    m_servoControlParam.az = CF_INS.servoAz(0);

    // 扫描范围参数
    m_scanRange.workMode = CF_INS.scanRangeWorkMode(0);

    // 波束控制参数 - 完整初始化所有字段
    m_beamControl.freqID = CF_INS.beamFreqID(5);
    m_beamControl.type = CF_INS.beamType(2);
    m_beamControl.aziStart = CF_INS.beamAziStart(-4500);
    m_beamControl.aziEnd = CF_INS.beamAziEnd(4500);
    m_beamControl.aziStep = CF_INS.beamAziStep(400);
    m_beamControl.scene = CF_INS.beamScene(0);
    m_beamControl.flagNum = CF_INS.beamFlagNum(2);
    m_beamControl.pulseNum = CF_INS.beamPulseNum(128);

    // 波形1参数
    m_beamControl.beam1Flag = CF_INS.beamBeam1Flag(1);
    m_beamControl.beam1Code = CF_INS.beamBeam1Code(6);
    m_beamControl.sampleStart1 = CF_INS.beamSampleStart1(30);
    m_beamControl.sampleEnd1 = CF_INS.beamSampleEnd1(130);
    m_beamControl.elestart1 = CF_INS.beamElestart1(0);
    m_beamControl.eleend1 = CF_INS.beamEleend1(3000);
    m_beamControl.elestep1 = CF_INS.beamElestep1(400);

    // 波形2参数
    m_beamControl.beam2Flag = CF_INS.beamBeam2Flag(1);
    m_beamControl.beam2Code = CF_INS.beamBeam2Code(9);
    m_beamControl.sampleStart2 = CF_INS.beamSampleStart2(120);
    m_beamControl.sampleEnd2 = CF_INS.beamSampleEnd2(490);
    m_beamControl.elestart2 = CF_INS.beamElestart2(0);
    m_beamControl.eleend2 = CF_INS.beamEleend2(1500);
    m_beamControl.elestep2 = CF_INS.beamElestep2(400);

    // 波形3参数
    m_beamControl.beam3Flag = CF_INS.beamBeam3Flag(0);
    m_beamControl.beam3Code = CF_INS.beamBeam3Code(11);
    m_beamControl.sampleStart3 = CF_INS.beamSampleStart3(270);
    m_beamControl.sampleEnd3 = CF_INS.beamSampleEnd3(1730);
    m_beamControl.elestart3 = CF_INS.beamElestart3(0);
    m_beamControl.eleend3 = CF_INS.beamEleend3(400);
    m_beamControl.elestep3 = CF_INS.beamElestep3(400);

    // 信号处理参数
    m_sigProParam.noise = CF_INS.sigProNoise(350);
    m_sigProParam.thresh1 = CF_INS.sigProThresh1(90);
    m_sigProParam.thresh2 = CF_INS.sigProThresh2(150);
    m_sigProParam.clutterThresh = CF_INS.sigProClutterThresh(170);
    m_sigProParam.clutterMapFlaseRate = CF_INS.sigProClutterMapFlaseRate(4);
    m_sigProParam.CFARType = CF_INS.sigProCFARType(0);
    m_sigProParam.disProWin = CF_INS.sigProDisProWin(2);
    m_sigProParam.disRefWin = CF_INS.sigProDisRefWin(16);
    m_sigProParam.dopProWin = CF_INS.sigProDopProWin(2);
    m_sigProParam.dopRefWin = CF_INS.sigProDopRefWin(16);
    m_sigProParam.MTDWinType = CF_INS.sigProMTDWinType(1);
    m_sigProParam.clutterMode = CF_INS.sigProClutterMode(1);
    m_sigProParam.clutterChannelWidth = CF_INS.sigProClutterChannelWidth(5);
    m_sigProParam.clutterUnitWin = CF_INS.sigProClutterUnitWin(1);
    m_sigProParam.clutterIter = CF_INS.sigProClutterIter(19);
    m_sigProParam.algorithmSwitch = CF_INS.sigProAlgorithmSwitch(7);

    // 数据处理参数
    m_dataProParam.startWinLen = CF_INS.dataProStartWinLen(4);
    m_dataProParam.startPoint = CF_INS.dataProStartPoint(3);
    m_dataProParam.endWinLen = CF_INS.dataProEndWinLen(3);
    m_dataProParam.endPoint = CF_INS.dataProEndPoint(3);
    m_dataProParam.noiseVar = CF_INS.dataProNoiseVar(1);
    m_dataProParam.trackDisLower = CF_INS.dataProTrackDisLower(10);
    m_dataProParam.trackDisUpper = CF_INS.dataProTrackDisUpper(300);
    m_dataProParam.trackAziThresh = CF_INS.dataProTrackAziThresh(15);
    m_dataProParam.trackEleThresh = CF_INS.dataProTrackEleThresh(20);
    m_dataProParam.trackVelThresh = CF_INS.dataProTrackVelThresh(200);
    m_dataProParam.trackStatThresh = CF_INS.dataProTrackStatThresh(16);
    m_dataProParam.accuDisGate = CF_INS.dataProAccuDisGate(20);
    m_dataProParam.accuAziGate = CF_INS.dataProAccuAziGate(15);
    m_dataProParam.accuEleGate = CF_INS.dataProAccuEleGate(20);
    m_dataProParam.accuVelGate = CF_INS.dataProAccuVelGate(3);

    LOG_INFO("MainOverLayOut: Loaded params from ConfigManager");
    LOG_INFO(QString("  servo: cmd=%1 speed=%2 az=%3")
                 .arg(m_servoControlParam.cmd)
                 .arg(m_servoControlParam.speed)
                 .arg(m_servoControlParam.az));

    // 初始化一键全数配置相关成员
    m_commandTimer = new QTimer(this);
    m_commandIndex = 0;
    connect(m_commandTimer, &QTimer::timeout, this, &MainOverLayOut::cmdTimeOut);

        m_trackTableRefreshTimer = new QTimer(this);
        m_trackTableRefreshTimer->setSingleShot(true);
        m_trackTableRefreshTimer->setInterval(kTrackTableRefreshIntervalMs);
        connect(m_trackTableRefreshTimer, &QTimer::timeout,
            this, &MainOverLayOut::flushPendingTrackTableUpdates);

            m_logFlushTimer = new QTimer(this);
            m_logFlushTimer->setSingleShot(true);
            m_logFlushTimer->setInterval(kLogFlushIntervalMs);
            connect(m_logFlushTimer, &QTimer::timeout,
                this, &MainOverLayOut::flushPendingLogLines);

    // 初始化健康管理相关成员
    m_systemNormal = false;  // 默认异常状态
    m_sigProSta = 1;         // 默认异常
    m_dataProSta = 1;        // 默认异常
    m_beamConSta = 1;        // 默认异常
    m_targetRecSta = 1;      // 默认异常

    // 初始化健康管理窗口指针
    m_healthWindow = nullptr;
    m_sigProBtn = nullptr;
    m_dataProBtn = nullptr;
    m_beamConBtn = nullptr;
    m_targetRecBtn = nullptr;

    // 初始化数据存储管理窗口指针
    m_dataStorageWindow = nullptr;
    m_dataStorageDialog = nullptr;

    // 初始化录屏回放窗口指针
    m_recorderWindow = nullptr;
    m_recorderWidget = nullptr;

    m_btnTxOpen = nullptr;
    m_btnDutyCycle = nullptr;
    m_btnPulseWidth = nullptr;
    m_btnRxOpen = nullptr;
    m_btnFreqSrc = nullptr;
    m_btnDigBoard = nullptr;
    m_btnServo = nullptr;
    m_btnBeidou = nullptr;
    m_btnBluetooth = nullptr;
    m_btnPowerBoard = nullptr;
    m_tempLabel = nullptr;
    m_angleLabel = nullptr;

    // 初始化雷达控制状态
    m_isTransmitting = false;  // 默认发射关闭
    m_isStandby = true;        // 默认待机模式

    // 连接健康管理信号
    connect(CON_INS, &Controller::monitorParamSend, this, &MainOverLayOut::monitorParamRef);

    // 连接伺服回送与BIT上报
    connect(CON_INS, &Controller::servoCtrlRet, this, &MainOverLayOut::onServoCtrlRet);
    connect(CON_INS, &Controller::bitReport, this, &MainOverLayOut::onBITReport);

    // 连接雷达控制按钮
    connect(ui->btnBatteryControl, &QPushButton::clicked, this,
            &MainOverLayOut::onBatteryControlClicked);
    connect(ui->btnStartSoftware, &QPushButton::clicked, this,
            &MainOverLayOut::onStartSoftwareClicked);
    connect(ui->btnStopSoftware, &QPushButton::clicked, this,
            &MainOverLayOut::onStopSoftwareClicked);
    connect(ui->btnDataStorage, &QPushButton::clicked, this, &MainOverLayOut::onDataStorageClicked);
    connect(ui->btnTWSMode, &QPushButton::clicked, this, &MainOverLayOut::onTWSModeClicked);
    connect(ui->btnTASMode, &QPushButton::clicked, this, &MainOverLayOut::onTASModeClicked);
    connect(ui->btnServoControl, &QPushButton::clicked, this,
            &MainOverLayOut::onServoControlClicked);
    connect(ui->btnServoNorth, &QPushButton::clicked, this,
            &MainOverLayOut::onServoNorthClicked);
    connect(ui->btnTransmitControl, &QPushButton::clicked, this,
            &MainOverLayOut::onTransmitControlClicked);
    connect(ui->btnRadarStandby, &QPushButton::clicked, this,
            &MainOverLayOut::onRadarStandbyClicked);

    // 连接参数设置按钮
    connect(ui->btnDataProcess, &QPushButton::clicked, this, &MainOverLayOut::onDataProcessClicked);
    connect(ui->btnSignalProcess, &QPushButton::clicked, this,
            &MainOverLayOut::onSignalProcessClicked);
    connect(ui->btnFreqControl, &QPushButton::clicked, this, &MainOverLayOut::onFreqControlClicked);
    arrangeParamSettingsButtons();
    // 隐藏方向图扫描控制按钮，功能已集成到"范围设置"tab中
    // ui->btnScanRange->setVisible(false);
    connect(ui->btnScanRange, &QPushButton::clicked, this, &MainOverLayOut::onScanRangeClicked);
    // 光电系统控制按钮（如果UI中存在）
    // connect(ui->btnPhotoelectric, &QPushButton::clicked, this,
    //         &MainOverLayOut::onPhotoelectricClicked);

    // 连接雷达系统健康管理按钮
    connect(ui->radarsystem, &QToolButton::clicked, this, &MainOverLayOut::onRadarSystemClicked);

    // 连接记录回放按钮
    connect(ui->toolButton_3, &QToolButton::clicked, this, &MainOverLayOut::onRecordPlayClicked);

    // ========== 关键修复：连接航迹数据到表格显示 ==========
    // 从Controller接收航迹数据并更新表格
    connect(CON_INS, &Controller::traInfoProcess, this, &MainOverLayOut::updateTrackList);

    // 从Controller接收TBD航迹数据并更新表格
    if (CF_INS.iftbd(false)) {
        connect(CON_INS, &Controller::tbdInfoProcess, this, &MainOverLayOut::updateTrackList);
        LOG_INFO("[MainOverLayOut] Connected Controller::tbdInfoProcess -> updateTrackList");
    }

    if (CF_INS.ifxietong(false)) {
        connect(CON_INS, &Controller::cooperativeTrackProcess, this, &MainOverLayOut::updateTrackList);
        LOG_INFO("[MainOverLayOut] Connected Controller::cooperativeTrackProcess -> updateTrackList");
    }

    // 从Controller接收航迹数据并更新无人机表格
    connect(CON_INS, &Controller::traInfoProcess, this, &MainOverLayOut::updateDroneTrackList);

    // 从Controller接收TBD航迹数据并更新无人机表格
    if (CF_INS.iftbd(false)) {
        connect(CON_INS, &Controller::tbdInfoProcess, this, &MainOverLayOut::updateDroneTrackList);
        LOG_INFO("[MainOverLayOut] Connected Controller::tbdInfoProcess -> updateDroneTrackList");
    }
    if (CF_INS.ifxietong(false)) {
        connect(CON_INS, &Controller::cooperativeTrackProcess, this, &MainOverLayOut::updateDroneTrackList);
        LOG_INFO("[MainOverLayOut] Connected Controller::cooperativeTrackProcess -> updateDroneTrackList");
    }

    // ========== 数据存储管理全局反馈连接 ==========
    // 数据保存成功反馈（全局连接，不依赖窗口）
    connect(CON_INS, &Controller::dataSaveOK, this, [this](DataSaveOK info) {
        logCommand("数据存储成功", QString("ID:%1, 大小:%2GB").arg(info.dataID).arg(info.dataSize));
    });

    // 数据删除成功反馈（全局连接，不依赖窗口）
    connect(CON_INS, &Controller::dataDelOK, this, [this](DataDelOK info) {
        logCommand("数据删除成功", QString("ID:%1").arg(info.dataID));
    });

    // 离线处理状态反馈（全局连接，不依赖窗口）
    connect(CON_INS, &Controller::offLineStat, this, [this](OfflineStat info) {
        QString status = (info.delStat == 0) ? "正常处理" : "离线处理";
        logCommand("离线处理状态", QString("ID:%1, 状态:%2").arg(info.dataID).arg(status));

        // 更新静态变量
        if (info.delStat == 1) {
            DataSaveUI::ifCurrentoffline = true;
            DataSaveUI::offlineDataID = info.dataID;
        } else {
            DataSaveUI::ifCurrentoffline = false;
        }
    });

    // ========== 连接点迹可见性控制信号 ==========
    // 从MousePositionInfo获取可见性控制信号，同步到所有显示视图
    MousePositionInfo* posInfo = mView->getMousePositionInfo();
    if (posInfo) {
        // 连接检测点可见性控制
        connect(posInfo, &MousePositionInfo::detectionVisibilityChanged, this,
                [this](bool visible) {
            // 主PPI视图的检测管理器
            if (mScene && mScene->det()) {
                mScene->det()->setAllVisible(visible);
            }
            // 扇区视图的检测管理器
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->detManager()) {
                m_sectorWidget->scene()->detManager()->setAllVisible(visible);
            }
            // 距离-方位图表的检测点可见性
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setDetectionVisible(visible);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setDetectionVisible(visible);
            }
        });

        // 连接航迹可见性控制
        connect(posInfo, &MousePositionInfo::trackVisibilityChanged, this,
                [this](bool visible) {
            // 主PPI视图的航迹管理器
            if (mScene && mScene->track()) {
                mScene->track()->setTypeVisible(PointType::Track, visible);
            }
            // 扇区视图的航迹管理器
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                m_sectorWidget->scene()->trackManager()->setTypeVisible(PointType::Track, visible);
            }
            // 距离-方位图表的航迹可见性
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setTrackVisible(visible);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setTrackVisible(visible);
            }
        });

        if (CF_INS.iftbd(false)) {
            connect(posInfo, &MousePositionInfo::tbdTrackVisibilityChanged, this,
                    [this](bool visible) {
                if (mScene && mScene->track()) {
                    mScene->track()->setTypeVisible(PointType::TBDPointType, visible);
                }
                if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                    m_sectorWidget->scene()->trackManager()->setTypeVisible(PointType::TBDPointType, visible);
                }
                if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                    m_rangeAzimuthWidget->chart()->setTbdTrackVisible(visible);
                }
                if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                    m_rangeHeightWidget->chart()->setTbdTrackVisible(visible);
                }
            });
        }

        if (CF_INS.ifxietong(false)) {
            connect(posInfo, &MousePositionInfo::cooperativeTrackVisibilityChanged, this,
                    [this](bool visible) {
                if (mScene && mScene->track()) {
                    mScene->track()->setTypeVisible(PointType::CooperativeTrackPointType, visible);
                }
                if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                    m_sectorWidget->scene()->trackManager()->setTypeVisible(PointType::CooperativeTrackPointType, visible);
                }
                if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                    m_rangeAzimuthWidget->chart()->setCooperativeTrackVisible(visible);
                }
                if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                    m_rangeHeightWidget->chart()->setCooperativeTrackVisible(visible);
                }
            });
        }

        // ========== 连接点迹大小控制信号 ==========
        // 连接检测点大小控制
        connect(posInfo, &MousePositionInfo::detectionSizeChanged, this,
                [this](double ratio) {
            // 主PPI视图的检测管理器
            if (mScene && mScene->det()) {
                mScene->det()->setPointSizeRatio(ratio);
            }
            // 扇区视图的检测管理器
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->detManager()) {
                m_sectorWidget->scene()->detManager()->setPointSizeRatio(ratio);
            }
            // 距离-方位图表的检测点大小
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setDetectionSizeRatio(ratio);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setDetectionSizeRatio(ratio);
            }
        });

        // 连接航迹大小控制
        connect(posInfo, &MousePositionInfo::trackSizeChanged, this,
                [this](double ratio) {
            // 主PPI视图的航迹管理器
            if (mScene && mScene->track()) {
                mScene->track()->setPointSizeRatio(ratio);
            }
            // 扇区视图的航迹管理器
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                m_sectorWidget->scene()->trackManager()->setPointSizeRatio(ratio);
            }
            // 距离-方位图表的航迹大小
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setTrackSizeRatio(ratio);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setTrackSizeRatio(ratio);
            }
        });

        LOG_INFO("MainOverLayOut: Connected point visibility and size control signals");
    } else {
        LOG_WARNING("MainOverLayOut: Failed to get MousePositionInfo from PPIView");
    }

    // 初始化按钮状态显示
    updateTransmitButton();
    updateStandbyButton();
}

void MainOverLayOut::setupResizableMainLayout()
{
    if (m_mainSplitter || !ui->horizontalLayout || !ui->verticalLayout || !ui->verticalLayout_5) {
        return;
    }

    m_leftSidebar = new QWidget(this);
    m_leftSidebar->setObjectName("LeftSidebar");
    m_leftSidebar->setContentsMargins(0, 0, 0, 0);
    m_rightSidebar = new QWidget(this);
    m_rightSidebar->setObjectName("RightSidebar");
    m_rightSidebar->setContentsMargins(0, 0, 0, 0);
    ui->viewWidget->setContentsMargins(0, 0, 0, 0);

    const auto rootMargins = ui->verticalLayout_2->contentsMargins();
    ui->verticalLayout_2->setContentsMargins(0, rootMargins.top(), 0, rootMargins.bottom());
    const auto leftMargins = ui->verticalLayout->contentsMargins();
    ui->verticalLayout->setContentsMargins(0, leftMargins.top(), 0, leftMargins.bottom());
    const auto rightMargins = ui->verticalLayout_5->contentsMargins();
    ui->verticalLayout_5->setContentsMargins(0, rightMargins.top(), 0, rightMargins.bottom());

    ui->horizontalLayout->removeItem(ui->verticalLayout);
    ui->verticalLayout->setParent(nullptr);
    m_leftSidebar->setLayout(ui->verticalLayout);

    ui->horizontalLayout->removeWidget(ui->viewWidget);
    ui->viewWidget->setParent(nullptr);

    ui->horizontalLayout->removeItem(ui->verticalLayout_5);
    ui->verticalLayout_5->setParent(nullptr);
    m_rightSidebar->setLayout(ui->verticalLayout_5);

    ui->verticalLayout_2->removeItem(ui->horizontalLayout);
    ui->horizontalLayout->setParent(nullptr);
    delete ui->horizontalLayout;
    ui->horizontalLayout = nullptr;

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setObjectName("MainContentSplitter");
    m_mainSplitter->setContentsMargins(0, 0, 0, 0);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setHandleWidth(2);
    m_mainSplitter->addWidget(m_leftSidebar);
    m_mainSplitter->addWidget(ui->viewWidget);
    m_mainSplitter->addWidget(m_rightSidebar);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);

    ui->verticalLayout_2->addWidget(m_mainSplitter);
}

void MainOverLayOut::topRightSet() {
    // 设置控件提示信息
    ui->minButton->setToolTip("最小化窗口");

    ui->CloseButton->setToolTip("关闭程序");

    ui->timeLabel->setToolTip("系统时间");

    ui->TitleLabel->setToolTip("系统标题");

    ui->SubtitleLabel->setToolTip("系统副标题");

    connect(ui->minButton, &QPushButton::clicked, CON_INS, &Controller::minimizeWindow);
    connect(ui->CloseButton, &QPushButton::clicked, this, [this]() {
        // 使用 nullptr 作为父窗口，避免在 quit 时对话框与主窗口的释放顺序冲突
        if (CustomMessageBox::showConfirm(nullptr, "退出确认", "是否确认退出程序？")) {
            // 延迟退出，确保对话框完全关闭后再退出
            QTimer::singleShot(0, qApp, &QApplication::quit);
        }
    });

    auto timeTimer = new QTimer(this);
    timeTimer->setInterval(1000);  // 定时器间隔：1000毫秒 = 1秒
    // 连接定时器超时信号到时间更新槽函数
    connect(timeTimer, &QTimer::timeout, this, [this]() {
        // 获取当前系统时间
        QDateTime currentTime = QDateTime::currentDateTime();
        QString timeFormat = "yyyy-MM-dd ddd HH:mm:ss";
        QString currentTimeStr = currentTime.toString(timeFormat);
        // 更新标签显示文本
        ui->timeLabel->setText(currentTimeStr);
    });
    timeTimer->start();
}

void MainOverLayOut::mainPView() {
    mView = new PPIView(this);  // 添加父对象，确保正确释放
    mView->setObjectName("mainPview");
    mScene = new PPIScene(this);
    mView->setPPIScene(mScene);
    QVBoxLayout* layout = new QVBoxLayout(ui->viewWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mView);
    connect(mView, &PPIView::viewResized, mScene, &PPIScene::updateSceneSize);

    QVBoxLayout* layout1 = new QVBoxLayout(ui->pviewFitW);
    layout1->setContentsMargins(0, 0, 0, 0);
    m_zoomView = new ZoomViewWidget(this);  // 添加父对象
    // 设置窗口属性
    layout1->addWidget(
        new DetachableWidget("P显", m_zoomView, QIcon(":/resources/icon/scan.png"), this));

    // 如果场景已设置，同步场景
    m_zoomView->setPPIScene(mScene);
    // 连接信号
    connect(mView, &PPIView::areaSelected, [this](const QRectF& rect) {
        if (m_zoomView) {
            m_zoomView->showArea(rect);
            if (!m_zoomView->isVisible()) {
                m_zoomView->show();
            }
        }
    });

    // 添加独立的扇区显示和距离-方位显示（使用Tab组织）
    const bool sectorDisplayEnabled = CF_INS.displayFlag("sector_display_enabled",
                                                        kDefaultEnableSectorDisplay);
    if (sectorDisplayEnabled) {
        m_sectorWidget = new SectorWidget(this);  // 添加父对象
        m_sectorWidget->setVisible(false);
    } else {
        m_sectorWidget = nullptr;
    }
    m_rangeAzimuthWidget = new RangeAzimuthChartWidget(this);  // 新的直角坐标系图表显示
    m_rangeHeightWidget = new RangeHeightChartWidget(this);    // 新的距离-高度图表显示

    // 从配置文件读取并应用初始最大检测点数量
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        int initMaxPoints = CF_INS.displayConfig("max_points", 1000);
        int initMaxTrackPoints = CF_INS.displayConfig("max_track_points", 200);
        m_rangeAzimuthWidget->chart()->setMaxDetectionPoints(initMaxPoints);
        m_rangeAzimuthWidget->chart()->setMaxTrackPoints(initMaxTrackPoints);
        LOG_INFO(QString("[MainOverLayOut] B显初始最大检测点数量: %1").arg(initMaxPoints));
        if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
            m_rangeHeightWidget->chart()->setMaxDetectionPoints(initMaxPoints);
            m_rangeHeightWidget->chart()->setMaxTrackPoints(initMaxTrackPoints);
            LOG_INFO(QString("[MainOverLayOut] H显初始最大检测点数量: %1").arg(initMaxPoints));
        }
    }

    // 创建Tab Widget来容纳扇区显示和距离-方位显示
    QTabWidget* displayTabWidget = new QTabWidget(this);
    displayTabWidget->setObjectName("DisplayTabWidget");

    // 设置Tab标签页稍微加宽，与darkstyle.qss样式保持一致
    displayTabWidget->setStyleSheet(
        "QTabWidget#DisplayTabWidget QTabBar::tab { "
        "    padding: 4px 12px; "
        "}"
    );

    // 将扇区显示包装为可分离的widget
    // DetachableWidget* sectorDetachable = new DetachableWidget(
    //     "扇区显示", m_sectorWidget, QIcon(":/resources/icon/scan.png"), this);

    // 将距离-方位显示包装为可分离的widget
    DetachableWidget* rangeAzDetachable = new DetachableWidget(
        "B显", m_rangeAzimuthWidget, QIcon(":/resources/icon/radararray.png"), this);
    DetachableWidget* rangeHeightDetachable = new DetachableWidget(
        "高显", m_rangeHeightWidget, QIcon(":/resources/icon/radararray.png"), this);

    // 添加到TabWidget
    //displayTabWidget->addTab(sectorDetachable, "扇区显示");
    displayTabWidget->addTab(rangeAzDetachable, "B显");
    displayTabWidget->addTab(rangeHeightDetachable, "H显");

    // 将TabWidget添加到布局
    QVBoxLayout* layout2 = new QVBoxLayout(ui->pviewSectorW);
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(displayTabWidget);

        // ========== 扇区显示数据流连接 ==========
        // 当前UI未展示扇区页时，不创建/不驱动隐藏扇区场景，避免无意义的离屏刷新开销
    const bool sectorDataEnabled = CF_INS.displayFlag("sector_display_data_enabled", false);
    if (sectorDataEnabled && m_sectorWidget && m_sectorWidget->scene()) {
        connect(CON_INS, &Controller::detInfoProcess, m_sectorWidget->scene()->detManager(),
            &SectorDetManager::addDetPoint);

        connect(CON_INS, &Controller::traInfoProcess, m_sectorWidget->scene()->trackManager(),
            &SectorTrackManager::addTrackPoint);

        if (CF_INS.iftbd(false)) {
            connect(CON_INS, &Controller::tbdInfoProcess, m_sectorWidget->scene()->trackManager(),
                &SectorTrackManager::addTrackPoint);
        }

        if (CF_INS.ifxietong(false)) {
            connect(CON_INS, &Controller::cooperativeTrackProcess, m_sectorWidget->scene()->trackManager(),
                &SectorTrackManager::addTrackPoint);
        }
    }

    // ========== 距离-方位图表数据流连接 ==========
    // 连接检测点数据到距离-方位图表
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        connect(CON_INS, &Controller::detInfoProcess,
                this, [this](PointInfo info) {
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->addPointInfo(info);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->addPointInfo(info);
            }
        });

        // 连接航迹数据到距离-方位图表
        connect(CON_INS, &Controller::traInfoProcess,
                this, [this](PointInfo info) {
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->addPointInfo(info);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->addPointInfo(info);
            }
        });

        // 连接TBD航迹数据到距离-方位图表
        if (CF_INS.iftbd(false)) {
            connect(CON_INS, &Controller::tbdInfoProcess,
                    this, [this](PointInfo info) {
                if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                    m_rangeAzimuthWidget->chart()->addPointInfo(info);
                }
                if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                    m_rangeHeightWidget->chart()->addPointInfo(info);
                }
            });
        }

        if (CF_INS.ifxietong(false)) {
            connect(CON_INS, &Controller::cooperativeTrackProcess,
                    this, [this](PointInfo info) {
                if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                    m_rangeAzimuthWidget->chart()->addPointInfo(info);
                }
                if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                    m_rangeHeightWidget->chart()->addPointInfo(info);
                }
            });
        }
    }

    // ========== 连接PPIView的清除信号到距离-方位图表 ==========
    // 当PPIVisualSettings的"显清"按钮被点击时，同时清除RangeAzimuthChart的数据
    if (mView && m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        connect(mView, &PPIView::clearDisplayTriggered,
                m_rangeAzimuthWidget->chart(), &RangeAzimuthChart::clearRadarData);
        if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
            connect(mView, &PPIView::clearDisplayTriggered,
                    m_rangeHeightWidget->chart(), &RangeHeightChart::clearRadarData);
        }

        // ========== 连接最大检测点数量变化到距离-方位图表 ==========
        // 当用户修改最大检测点数量时，同步更新 RangeAzimuthChart 的限制
        connect(mView, &PPIView::maxPointsSettingChanged,
                m_rangeAzimuthWidget->chart(), &RangeAzimuthChart::setMaxDetectionPoints);
        if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
            connect(mView, &PPIView::maxPointsSettingChanged,
                    m_rangeHeightWidget->chart(), &RangeHeightChart::setMaxDetectionPoints);
        }

        connect(mView, &PPIView::maxPointsSettingChanged,
            this, [this](int maxPoints) {
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->detManager()) {
            m_sectorWidget->scene()->detManager()->setMaxPoints(maxPoints);
            }
        });

        connect(mView, &PPIView::maxTrackPointsSettingChanged,
                this, [this](int maxPoints) {
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                m_sectorWidget->scene()->trackManager()->setMaxPointsPerBatch(maxPoints);
            }
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setMaxTrackPoints(maxPoints);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setMaxTrackPoints(maxPoints);
            }
        });
    }

    // ========== 同步主视图的距离范围到所有辅助视图 ==========
    if (mScene && mScene->axis()) {
        // 同步到距离-方位图表显示
        connect(mScene->axis(), &PolarAxis::rangeChanged,
                this, [this](double min, double max) {
            if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
                m_rangeAzimuthWidget->chart()->setRangeFromMain(min, max);
            }
            if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
                m_rangeHeightWidget->chart()->setRangeFromMain(min, max);
            }
        });

        // 同步到扇区显示
        connect(mScene->axis(), &PolarAxis::rangeChanged,
                this, [this](double min, double max) {
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->axis()) {
                m_sectorWidget->scene()->axis()->setRange(min, max);
                if (m_sectorWidget->scene()->detManager()) {
                    m_sectorWidget->scene()->detManager()->refreshAll();
                }
                if (m_sectorWidget->scene()->trackManager()) {
                    m_sectorWidget->scene()->trackManager()->refreshAll();
                }
            }
        });

        // 初始化时同步一次距离范围
        double minR = mScene->axis()->minRange();
        double maxR = mScene->axis()->maxRange();
        if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
            m_rangeAzimuthWidget->chart()->setRangeFromMain(minR, maxR);
        }
        if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
            m_rangeHeightWidget->chart()->setRangeFromMain(minR, maxR);
        }
        if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->axis()) {
            m_sectorWidget->scene()->axis()->setRange(minR, maxR);
        }
    }
}

void MainOverLayOut::setupRangeSettings() {
#if 0
    // 创建方位角和俯仰角范围控制组件
    m_azElRangeWidget = new AzElRangeWidget(this);

    // 获取范围设置tab（tab_3）并为其设置布局
    QWidget* rangeTab = ui->tab_3;  // "范围设置"标签页

    // 创建垂直布局来容纳AzElRangeWidget
    QVBoxLayout* rangeLayout = new QVBoxLayout(rangeTab);
    rangeLayout->setContentsMargins(3, 3, 3, 3);  // 减少边距给更多空间
    rangeLayout->setSpacing(0);

    // 设置AzElRangeWidget的尺寸策略，让它占据更多空间
    m_azElRangeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    //m_azElRangeWidget->setMinimumSize(300, 250);  // 设置最小尺寸

    rangeLayout->addWidget(m_azElRangeWidget, 1);  // 给widget更多空间权重

    // 不添加弹性空间，让widget充分利用可用空间
    // rangeLayout->addStretch();

    // 设置tab的样式，与其他tab保持一致的背景
    rangeTab->setStyleSheet(R"(
        QWidget {
            background: rgba(16, 24, 24, 0.8);
            border-radius: 8px;
        }
    )");

    // 连接"设置"按钮点击信号，打开详细设置对话框
    connect(m_azElRangeWidget, &AzElRangeWidget::settingsButtonClicked,
            this, &MainOverLayOut::onScanRangeClicked);

    // 设置默认的扫描范围（从配置文件读取）
    int defaultAzMin = CF_INS.azimuthRange("min", 30);    // 从配置读取，默认30°
    int defaultAzMax = CF_INS.azimuthRange("max", 120);   // 从配置读取，默认120°
    int defaultElMin = CF_INS.elevationRange("min", -10); // 从配置读取，默认-10°
    int defaultElMax = CF_INS.elevationRange("max", 45);  // 从配置读取，默认45°

    // 初始化时静默更新，避免自动下发
    m_azElRangeWidget->setSignalMuted(true);
    m_azElRangeWidget->setAzRange(defaultAzMin, defaultAzMax);
    m_azElRangeWidget->setElRange(defaultElMin, defaultElMax);
    m_azElRangeWidget->setSignalMuted(false);
#endif
}

void MainOverLayOut::setupWorkModeSettings() {
    // 设置组合框默认值

    // 寻找并连接PpiView topleft的输入框（用于联动）
    // 这里需要在mainPView()方法创建后才能找到topleft控件
    // 延迟执行连接
    QTimer::singleShot(100, this, [this]() {
        // 尝试找到PpiView中的topleft控件
        if (mView) {
            m_topLeftWidget = mView->findChild<mainviewTopLeft*>();
            if (m_topLeftWidget) {
                // 找到topleft控件中的偏航和倾角输入框
                QLineEdit* yawEdit = m_topLeftWidget->findChild<QLineEdit*>("yaw");
                QLineEdit* rollEdit = m_topLeftWidget->findChild<QLineEdit*>("roll");

                if (yawEdit) {
                    connect(yawEdit, &QLineEdit::returnPressed, this,
                            &MainOverLayOut::sendScanRangeParams);
                }
                if (rollEdit) {
                    connect(rollEdit, &QLineEdit::returnPressed, this,
                            &MainOverLayOut::sendScanRangeParams);
                }
            }
        }
    });
}

void MainOverLayOut::sendScanRangeParams() {
    // 创建ScanRange参数结构
    ScanRange scanParam;

    // 使用topleft控件中的阵面指北角和倾角
    double aziValue = 0.0;
    double eleValue = 0.0;

    if (m_topLeftWidget) {
        QLineEdit* yawEdit = m_topLeftWidget->findChild<QLineEdit*>("yaw");
        QLineEdit* rollEdit = m_topLeftWidget->findChild<QLineEdit*>("roll");
    }
}

MainOverLayOut::~MainOverLayOut() {
    if (m_commandTimer) {
        m_commandTimer->stop();
    }
    if (CON_INS) {
        disconnect(CON_INS, nullptr, this, nullptr);
        // 断开连接到扇区显示子对象的信号，避免析构时回调已释放对象
        if (m_sectorWidget && m_sectorWidget->scene()) {
            disconnect(CON_INS, nullptr, m_sectorWidget->scene()->detManager(), nullptr);
            disconnect(CON_INS, nullptr, m_sectorWidget->scene()->trackManager(), nullptr);
        }
        // 距离-方位图表：信号连接在 lambda 中，会随着 this 的销毁自动断开
        // 不需要手动断开连接
    }
    // 断开 RadarDataManager 的信号，避免销毁后仍然回调到已释放对象
    disconnect(&RADAR_DATA_MGR, nullptr, this, nullptr);

    // ui 对象由 Qt 的 setupUi 创建，其中的 widget 都是 this 的子对象
    // Qt 会自动删除子对象，所以这里不需要手动 delete ui
    // 但是 Ui::MainOverLayOut 本身是在堆上分配的，需要手动删除
    // 为了避免双重释放，先检查是否仍然有效
    if (ui) {
        // ui->setupUi(this) 会将所有 widget 设为 this 的子对象
        // 所以 delete ui 只删除 Ui 类本身，不删除 widget
        delete ui;
        ui = nullptr;
    }
}

void MainOverLayOut::setupTrackManagement() {
    LOG_INFO(QString("[MainOverLayOut::setupTrackManagement] iftbd=%1 ifxietong=%2")
             .arg(CF_INS.iftbd(false))
             .arg(CF_INS.ifxietong(false)));

    setupTrackTable(ui->tableWidget, m_trackTableFrozenHelper);
    setupTrackTable(ui->droneTableWidget, m_droneTableFrozenHelper);

    if (CF_INS.iftbd(false)) {
        QWidget* tbdTab = new QWidget(ui->trackTab);
        QVBoxLayout* layout = new QVBoxLayout(tbdTab);
        layout->setSpacing(0);
        layout->setContentsMargins(4, 4, 4, 4);
        m_tbdTrackTable = new QTableWidget(tbdTab);
        m_tbdTrackTable->setObjectName("tbdTrackTableWidget");
        layout->addWidget(m_tbdTrackTable);
        ui->trackTab->addTab(tbdTab, tr("TBD航迹"));
        setupTrackTable(m_tbdTrackTable, m_tbdTrackTableFrozenHelper);
        LOG_INFO("[MainOverLayOut::setupTrackManagement] Created TBD track table tab");
    }

    if (CF_INS.ifxietong(false)) {
        QWidget* cooperativeTab = new QWidget(ui->trackTab);
        QVBoxLayout* layout = new QVBoxLayout(cooperativeTab);
        layout->setSpacing(0);
        layout->setContentsMargins(4, 4, 4, 4);
        m_cooperativeTrackTable = new QTableWidget(cooperativeTab);
        m_cooperativeTrackTable->setObjectName("cooperativeTrackTableWidget");
        layout->addWidget(m_cooperativeTrackTable);
        ui->trackTab->addTab(cooperativeTab, tr("协同航迹"));
        setupTrackTable(m_cooperativeTrackTable, m_cooperativeTrackFrozenHelper);
        LOG_INFO("[MainOverLayOut::setupTrackManagement] Created cooperative track table tab");
    }

    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared, this, &MainOverLayOut::clearAllTracks);
    connect(ui->trackTab, &QTabWidget::currentChanged, this, &MainOverLayOut::onTrackTabChanged);

    // 连接Controller的目标分类信号
    if (CON_INS) {
        connect(CON_INS, &Controller::targetClaRes, this,
                [this](TargetClaRes res) { updateTargetClassification(res.batchID, res.claRes); });
    }

    applyTrackTabDisplayMode();
    updateTrackStatsWidgets();

    // (no test simulators scheduled)
}

void MainOverLayOut::updateTrackStatsWidgets()
{
    const int totalTrackCount = ui->tableWidget ? ui->tableWidget->rowCount() : 0;
    const int droneTrackCount = ui->droneTableWidget ? ui->droneTableWidget->rowCount() : 0;
    const int tbdTrackCount = m_tbdTrackTable ? m_tbdTrackTable->rowCount() : 0;
    const int cooperativeTrackCount = m_cooperativeTrackTable ? m_cooperativeTrackTable->rowCount() : 0;

    if (ui->trackTab) {
        const int totalTabIndex = ui->trackTab->indexOf(ui->tab_5);
        if (totalTabIndex >= 0) {
            const QString totalTabText = tr("航迹管理(%1)").arg(totalTrackCount);
            ui->trackTab->setTabText(totalTabIndex, totalTabText);
            ui->trackTab->setTabToolTip(totalTabIndex, tr("总航迹批数: %1").arg(totalTrackCount));
        }

        const int droneTabIndex = ui->trackTab->indexOf(ui->droneTrackTab);
        if (droneTabIndex >= 0) {
            const QString droneTabText = tr("无人机航迹(%1)").arg(droneTrackCount);
            ui->trackTab->setTabText(droneTabIndex, droneTabText);
            ui->trackTab->setTabToolTip(droneTabIndex, tr("无人机航迹批数: %1").arg(droneTrackCount));
        }

        if (m_tbdTrackTable) {
            const int tbdTabIndex = ui->trackTab->indexOf(m_tbdTrackTable->parentWidget());
            if (tbdTabIndex >= 0) {
                const QString tbdTabText = tr("TBD航迹(%1)").arg(tbdTrackCount);
                ui->trackTab->setTabText(tbdTabIndex, tbdTabText);
                ui->trackTab->setTabToolTip(tbdTabIndex, tr("TBD航迹批数: %1").arg(tbdTrackCount));
            }
        }

        if (m_cooperativeTrackTable) {
            const int cooperativeTabIndex = ui->trackTab->indexOf(m_cooperativeTrackTable->parentWidget());
            if (cooperativeTabIndex >= 0) {
                const QString cooperativeTabText = tr("协同航迹(%1)").arg(cooperativeTrackCount);
                ui->trackTab->setTabText(cooperativeTabIndex, cooperativeTabText);
                ui->trackTab->setTabToolTip(cooperativeTabIndex, tr("协同航迹批数: %1").arg(cooperativeTrackCount));
            }
        }
    }
}

void MainOverLayOut::setupTrackTable(QTableWidget* tableWidget, FrozenColumnHelper*& frozenHelper)
{
    if (!tableWidget) return;

    QStringList headers;
    headers << "批次号" << "方位" << "俯仰" << "高度" << "距离" << "速度" << "SNR" << "类型";

    tableWidget->setColumnCount(headers.size());
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->horizontalHeader()->setVisible(true);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setShowGrid(true);
    tableWidget->setWordWrap(false);
    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    QHeaderView* headerView = tableWidget->horizontalHeader();
    headerView->setStretchLastSection(false);
    headerView->setMinimumSectionSize(48);
    const QList<int> columnWidths = {64, 58, 58, 58, 58, 58, 54, 70};
    for (int i = 0; i < headers.size(); ++i) {
        headerView->setSectionResizeMode(i, QHeaderView::Interactive);
        tableWidget->setColumnWidth(i, columnWidths.value(i, 58));
    }

    frozenHelper = nullptr;
}

void MainOverLayOut::syncFrozenTrackTables()
{
    if (m_trackTableFrozenHelper) {
        m_trackTableFrozenHelper->syncFrozenContent();
    }
    if (m_droneTableFrozenHelper) {
        m_droneTableFrozenHelper->syncFrozenContent();
    }
    if (m_tbdTrackTableFrozenHelper) {
        m_tbdTrackTableFrozenHelper->syncFrozenContent();
    }
    if (m_cooperativeTrackFrozenHelper) {
        m_cooperativeTrackFrozenHelper->syncFrozenContent();
    }
}

void MainOverLayOut::scheduleTrackTableRefresh()
{
    if (m_trackTableRefreshTimer && !m_trackTableRefreshTimer->isActive()) {
        m_trackTableRefreshTimer->start();
    }
}

void MainOverLayOut::flushPendingTrackTableUpdates()
{
    if (m_pendingTrackUpdates.isEmpty() && m_pendingDroneTrackUpdates.isEmpty()) {
        return;
    }

    const QMap<quint64, PointInfo> pendingTrackUpdates = m_pendingTrackUpdates;
    const QMap<unsigned int, PointInfo> pendingDroneTrackUpdates = m_pendingDroneTrackUpdates;
    m_pendingTrackUpdates.clear();
    m_pendingDroneTrackUpdates.clear();

    bool sortMain = false;
    bool sortTbd = false;
    bool sortCooperative = false;
    bool sortDrone = false;

    for (auto it = pendingTrackUpdates.cbegin(); it != pendingTrackUpdates.cend(); ++it) {
        applyTrackListUpdate(it.value(), sortMain, sortTbd, sortCooperative);
    }

    for (auto it = pendingDroneTrackUpdates.cbegin(); it != pendingDroneTrackUpdates.cend(); ++it) {
        applyDroneTrackListUpdate(it.value(), sortDrone);
    }

    if (sortMain) {
        sortTrackTable(ui->tableWidget);
    }
    if (sortTbd && m_tbdTrackTable) {
        sortTrackTable(m_tbdTrackTable);
    }
    if (sortCooperative && m_cooperativeTrackTable) {
        sortTrackTable(m_cooperativeTrackTable);
    }
    if (sortDrone) {
        sortTrackTable(ui->droneTableWidget);
    }

    syncFrozenTrackTables();
    updateTrackStatsWidgets();
}

void MainOverLayOut::applyTrackListUpdate(const PointInfo& info, bool& sortMain, bool& sortTbd,
                                          bool& sortCooperative)
{
    if (info.statMethod == 2) {
        removeTrackRow(ui->tableWidget, info.type, info.batch);
        if (m_tbdTrackTable && info.type == PointType::TBDPointType) {
            removeTrackRow(m_tbdTrackTable, info.type, info.batch);
        }
        if (m_cooperativeTrackTable && info.type == PointType::CooperativeTrackPointType) {
            removeTrackRow(m_cooperativeTrackTable, info.type, info.batch);
        }
        return;
    }

    QString targetType = trackTypeLabel(info.type);
    if (info.type == PointType::Track) {
        targetType = getTargetTypeText(m_targetTypes.value(info.batch, 0));
    }

    bool insertedMain = false;
    addOrUpdateTrackRow(ui->tableWidget, info, targetType, &insertedMain);
    sortMain = sortMain || insertedMain;

    if (m_tbdTrackTable && info.type == PointType::TBDPointType) {
        bool inserted = false;
        addOrUpdateTrackRow(m_tbdTrackTable, info, targetType, &inserted);
        sortTbd = sortTbd || inserted;
    }

    if (m_cooperativeTrackTable && info.type == PointType::CooperativeTrackPointType) {
        bool inserted = false;
        addOrUpdateTrackRow(m_cooperativeTrackTable, info, targetType, &inserted);
        sortCooperative = sortCooperative || inserted;
    }
}

void MainOverLayOut::applyDroneTrackListUpdate(const PointInfo& info, bool& sortDrone)
{
    if (info.type != PointType::Track) {
        return;
    }

    if (info.statMethod == 2) {
        removeTrackRow(ui->droneTableWidget, info.type, info.batch);
        return;
    }

    const bool isDroneTrack = (info.targetRecResult == 1) || (m_targetTypes.value(info.batch, 0) == 1);
    if (isDroneTrack) {
        PointInfo droneInfo = info;
        droneInfo.targetRecResult = 1;
        bool inserted = false;
        addOrUpdateTrackRow(ui->droneTableWidget, droneInfo, getTargetTypeText(1), &inserted);
        sortDrone = sortDrone || inserted;
    } else {
        removeTrackRow(ui->droneTableWidget, info.type, info.batch);
    }
}

// simulateIncomingTracks removed

void MainOverLayOut::updateTrackList(const PointInfo& info) {
    m_pendingTrackUpdates[makeTrackTableUpdateKey(info.type, info.batch)] = info;
    scheduleTrackTableRefresh();
}

void MainOverLayOut::updateDroneTrackList(const PointInfo& info) {
    if (info.type != PointType::Track) {
        return;
    }

    m_pendingDroneTrackUpdates[info.batch] = info;
    scheduleTrackTableRefresh();
}

void MainOverLayOut::updateTargetClassification(unsigned int batchID, int targetType) {
    m_targetTypes[batchID] = targetType;

    // classification update (no debug log)

    // 不覆盖显示列 "类型"（该列用于显示识别结果），只更新内部映射 m_targetTypes
    QTableWidget* trackTable = ui->tableWidget;

    int ordinaryTrackRow = -1;
    for (int row = 0; row < trackTable->rowCount(); ++row) {
        QTableWidgetItem* batchItem = trackTable->item(row, 0);
        if (!batchItem) {
            continue;
        }
        if (batchItem->text().toUInt() == batchID &&
            batchItem->data(Qt::UserRole).toUInt() == PointType::Track) {
            ordinaryTrackRow = row;
            break;
        }
    }

    if (ordinaryTrackRow >= 0) {
        QString recResultStr = (targetType == 1) ? tr("无人机") : tr("其它");
        QTableWidgetItem* typeItem = trackTable->item(ordinaryTrackRow, 7);
        if (!typeItem) {
            typeItem = new QTableWidgetItem();
            trackTable->setItem(ordinaryTrackRow, 7, typeItem);
        }
        typeItem->setText(recResultStr);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
    }

    if (targetType == 1 && ordinaryTrackRow >= 0) {
        PointInfo info;
        info.batch = batchID;
        info.azimuth = trackTable->item(ordinaryTrackRow, 1)->text().toFloat();
        info.elevation = trackTable->item(ordinaryTrackRow, 2)->text().toFloat();
        info.altitute = trackTable->item(ordinaryTrackRow, 3)->text().toFloat();
        info.range = trackTable->item(ordinaryTrackRow, 4)->text().toFloat();
        info.speed = trackTable->item(ordinaryTrackRow, 5)->text().toFloat();
        info.SNR = trackTable->item(ordinaryTrackRow, 6)->text().toFloat();
        info.type = PointType::Track;
        info.targetRecResult = 1;

        addOrUpdateTrackRow(ui->droneTableWidget, info, getTargetTypeText(targetType));
    } else {
        removeTrackRow(ui->droneTableWidget, PointType::Track, batchID);
    }

    // 重新排序两个表格
    sortTrackTable(ui->tableWidget);
    sortTrackTable(ui->droneTableWidget);

    syncFrozenTrackTables();
    updateTrackStatsWidgets();
}

void MainOverLayOut::onTrackTabChanged(int index) {
    Q_UNUSED(index);
    applyTrackTabDisplayMode();
}

void MainOverLayOut::applyTrackTabDisplayMode() {
    const bool droneTabActive = ui->trackTab->currentWidget() == ui->droneTrackTab;

    if (mView) {
        mView->setOnlyRecognizedDroneTracksVisible(droneTabActive);
    }
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setOnlyRecognizedDroneTracksVisible(droneTabActive);
    }
    if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
        m_rangeHeightWidget->chart()->setOnlyRecognizedDroneTracksVisible(droneTabActive);
    }

    MousePositionInfo* posInfo = mView ? mView->getMousePositionInfo() : nullptr;
    const bool trackVisible = posInfo ? posInfo->isTrackVisible() : true;
    const bool tbdVisible = posInfo ? posInfo->isTbdTrackVisible() : false;
    const bool cooperativeVisible = posInfo ? posInfo->isCooperativeTrackVisible() : false;

    const bool normalTrackDisplay = droneTabActive ? true : trackVisible;
    const bool specialTrackDisplay = droneTabActive ? false : true;

    if (mScene && mScene->track()) {
        mScene->track()->setTypeVisible(PointType::Track, normalTrackDisplay);
        mScene->track()->setTypeVisible(PointType::TBDPointType,
                                        specialTrackDisplay && tbdVisible);
        mScene->track()->setTypeVisible(PointType::CooperativeTrackPointType,
                                        specialTrackDisplay && cooperativeVisible);
    }

    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setTrackVisible(normalTrackDisplay);
        m_rangeAzimuthWidget->chart()->setTbdTrackVisible(specialTrackDisplay && tbdVisible);
        m_rangeAzimuthWidget->chart()->setCooperativeTrackVisible(specialTrackDisplay && cooperativeVisible);
    }
    if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
        m_rangeHeightWidget->chart()->setTrackVisible(normalTrackDisplay);
        m_rangeHeightWidget->chart()->setTbdTrackVisible(specialTrackDisplay && tbdVisible);
        m_rangeHeightWidget->chart()->setCooperativeTrackVisible(specialTrackDisplay && cooperativeVisible);
    }
}

void MainOverLayOut::onTrackRemoved(int batchID) {
    LOG_INFO(QString("[MainOverLayOut::onTrackRemoved] CALLED batchID=%1, trackTable rows=%2, droneTable rows=%3")
             .arg(batchID).arg(ui->tableWidget->rowCount()).arg(ui->droneTableWidget->rowCount()));

    // 从总航迹表格中删除
    QTableWidget* trackTable = ui->tableWidget;

    for (int row = trackTable->rowCount() - 1; row >= 0; --row) {
        if (trackTable->item(row, 0)) {
            int rowBatchID = trackTable->item(row, 0)->text().toInt();

            if (rowBatchID == batchID) {
                trackTable->removeRow(row);
                LOG_INFO(QString("[MainOverLayOut::onTrackRemoved] Removed row %1 from trackTable (batch=%2)").arg(row).arg(batchID));

                // 同步冻结列
                if (m_trackTableFrozenHelper) {
                    m_trackTableFrozenHelper->syncFrozenContent();
                }
                break;
            }
        }
    }

    // 从无人机表格中删除
    QTableWidget* droneTable = ui->droneTableWidget;
    for (int row = droneTable->rowCount() - 1; row >= 0; --row) {
        if (droneTable->item(row, 0) && droneTable->item(row, 0)->text().toInt() == batchID) {
            droneTable->removeRow(row);
            LOG_INFO(QString("[MainOverLayOut::onTrackRemoved] Removed row %1 from droneTable (batch=%2)").arg(row).arg(batchID));

            // 同步冻结列
            if (m_droneTableFrozenHelper) {
                m_droneTableFrozenHelper->syncFrozenContent();
            }
            break;
        }
    }

    if (m_tbdTrackTable) {
        for (int row = m_tbdTrackTable->rowCount() - 1; row >= 0; --row) {
            if (m_tbdTrackTable->item(row, 0) && m_tbdTrackTable->item(row, 0)->text().toInt() == batchID) {
                m_tbdTrackTable->removeRow(row);
            }
        }
    }

    if (m_cooperativeTrackTable) {
        for (int row = m_cooperativeTrackTable->rowCount() - 1; row >= 0; --row) {
            if (m_cooperativeTrackTable->item(row, 0) && m_cooperativeTrackTable->item(row, 0)->text().toInt() == batchID) {
                m_cooperativeTrackTable->removeRow(row);
            }
        }
    }

    // 从内部映射中删除
    m_targetTypes.remove(batchID);
    m_trackStartTimes.remove(batchID);

    LOG_INFO(QString("[MainOverLayOut::onTrackRemoved] DONE batchID=%1, trackTable rows=%2, droneTable rows=%3")
             .arg(batchID).arg(ui->tableWidget->rowCount()).arg(ui->droneTableWidget->rowCount()));

    syncFrozenTrackTables();
    updateTrackStatsWidgets();
}

void MainOverLayOut::clearAllTracks() {
    ui->tableWidget->setRowCount(0);
    ui->droneTableWidget->setRowCount(0);
    if (m_tbdTrackTable) {
        m_tbdTrackTable->setRowCount(0);
    }
    if (m_cooperativeTrackTable) {
        m_cooperativeTrackTable->setRowCount(0);
    }
    m_targetTypes.clear();
    m_trackStartTimes.clear();
    syncFrozenTrackTables();
    updateTrackStatsWidgets();
}

/**
 * @brief 清除航迹表格数据
 * @details 响应"显清"按钮，清除航迹列表和无人机航迹列表的所有数据
 *          与clearAllTracks()功能相同，但作为公共槽函数供外部调用
 */
void MainOverLayOut::clearTrackTables() {
    clearAllTracks();
    //LOG_INFO("Track tables cleared by user request");
}

int MainOverLayOut::addOrUpdateTrackRow(QTableWidget* tableWidget, const PointInfo& info,
                                        const QString& targetType, bool* inserted) {
    int row = -1;
    bool rowInserted = false;

    // 查找是否已存在该批次
    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        if (!tableWidget->item(i, 0)) continue;
        if (tableWidget->item(i, 0)->text().toUInt() == info.batch &&
            tableWidget->item(i, 0)->data(Qt::UserRole).toUInt() == info.type) {
            row = i;
            break;
        }
    }

    // 如果不存在，创建新行
    if (row == -1) {
        row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        m_trackStartTimes[info.batch] = QDateTime::currentDateTime();  // 记录航迹开始时间
        rowInserted = true;
    }

    auto ensureItem = [tableWidget, row](int column) -> QTableWidgetItem* {
        QTableWidgetItem* item = tableWidget->item(row, column);
        if (!item) {
            item = new QTableWidgetItem();
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(row, column, item);
        }
        return item;
    };

    QTableWidgetItem* batchItem = ensureItem(0);
    batchItem->setData(Qt::UserRole, info.type);
    batchItem->setText(QString::number(info.batch));

    ensureItem(1)->setText(QString::number(info.azimuth, 'f', 1));
    ensureItem(2)->setText(QString::number(info.elevation, 'f', 1));
    ensureItem(3)->setText(QString::number(info.altitute, 'f', 1));
    ensureItem(4)->setText(QString::number(info.range, 'f', 1));
    ensureItem(5)->setText(QString::number(info.speed, 'f', 1));
    ensureItem(6)->setText(QString::number(info.SNR, 'f', 1));
    // 目标识别结果（来自数据处理上报）
    QString recResultStr = targetType;
    if (info.type == PointType::Track) {
        recResultStr = (info.targetRecResult == 1) ? QStringLiteral("无人机") : QStringLiteral("其它");
    }
    // 显示识别结果在"类型"列（列索引7），不使用额外列
    ensureItem(7)->setText(recResultStr);

    // 设置所有项为不可编辑
    for (int col = 0; col < tableWidget->columnCount(); ++col) {
        QTableWidgetItem* item = tableWidget->item(row, col);
        if (!item) continue;
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (info.type != PointType::Track) {
            item->setBackground(QBrush(trackTypeColor(info.type).lighter(150)));
        } else {
            item->setBackground(QBrush());
        }
    }

    if (inserted) {
        *inserted = rowInserted;
    }

    return row;
}

void MainOverLayOut::removeTrackRow(QTableWidget* tableWidget, unsigned type, unsigned int batch)
{
    if (!tableWidget) return;

    for (int row = tableWidget->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem* batchItem = tableWidget->item(row, 0);
        if (!batchItem) continue;
        if (batchItem->text().toUInt() == batch && batchItem->data(Qt::UserRole).toUInt() == type) {
            tableWidget->removeRow(row);
            break;
        }
    }
}

void MainOverLayOut::sortTrackTable(QTableWidget* tableWidget) {
    // 获取所有行数据
    struct TrackRowData {
        QStringList columns;
        unsigned type = PointType::Track;
    };

    QList<TrackRowData> allRows;
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        TrackRowData rowData;
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = tableWidget->item(row, col);
            rowData.columns << (item ? item->text() : QString());
            if (col == 0 && item) {
                rowData.type = item->data(Qt::UserRole).toUInt();
            }
        }
        allRows.append(rowData);
    }

    // 自定义排序：无人机优先，相同类别按时间排序
    std::sort(allRows.begin(), allRows.end(), [this](const TrackRowData& a, const TrackRowData& b) {
        unsigned int batchA = a.columns[0].toUInt();
        unsigned int batchB = b.columns[0].toUInt();

        int typeA = m_targetTypes.value(batchA, 0);
        int typeB = m_targetTypes.value(batchB, 0);

        // 无人机类型（1）优先
        if (typeA == 1 && typeB != 1) return true;
        if (typeA != 1 && typeB == 1) return false;

        // 相同类别按开始时间排序（新的在前）
        if (typeA == typeB) {
            QDateTime timeA = m_trackStartTimes.value(batchA, QDateTime::currentDateTime());
            QDateTime timeB = m_trackStartTimes.value(batchB, QDateTime::currentDateTime());
            return timeA > timeB;  // 新的航迹在前
        }

        return typeA < typeB;
    });

    // 清空表格并重新填充
    tableWidget->setRowCount(0);
    for (int i = 0; i < allRows.size(); ++i) {
        tableWidget->insertRow(i);
        const TrackRowData& rowData = allRows[i];
        for (int col = 0; col < rowData.columns.size(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(rowData.columns[col]);
            if (col == 0) {
                item->setData(Qt::UserRole, rowData.type);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            if (rowData.type != PointType::Track) {
                item->setBackground(QBrush(trackTypeColor(rowData.type).lighter(150)));
            }
            tableWidget->setItem(i, col, item);
        }
    }
}

QString MainOverLayOut::getTargetTypeText(int targetType) const {
    switch (targetType) {
        case 0:
            return "未知";
        case 1:
            return "无人机";
        case 2:
            return "行人";
        case 3:
            return "车辆";
        case 4:
            return "鸟类";
        case 5:
            return "其他";
        default:
            return "未知";
    }
}

// ============ 雷达控制槽函数实现 ============

/**
 * @brief 打开处理软件启动对话框
 * @details 发送系统启动命令
 */
void MainOverLayOut::onStartSoftwareClicked() {
    if (CustomMessageBox::showConfirm(this, "启动确认", "是否启动处理软件？")) {
        StartSysParam param;
        // 设置默认参数
        param.sta = 1;  // 启动状态

        logCommand("正在启动软件", "");

        emit CON_INS->sendSysStart(param);
    }
}

/**
 * @brief 更新健康管理窗口的实时显示
 * @details 根据最新的 MonitorParam 和 BITReport 数据更新窗口内容
 */
void MainOverLayOut::updateHealthWindow()
{
    // 如果窗口不存在或不可见，直接返回
    if (!m_healthWindow || !m_healthWindow->isVisible()) {
        return;
    }

    // 定义按钮样式
    QString greenStyle =
        "QPushButton { "
        "background-color: #00ff00; "
        "color: #101818; "
        "border: 2px solid #66ffcc; "
        "border-radius: 8px; "
        "padding: 15px; "
        "font-size: 16px; "
        "font-weight: bold; "
        "}";

    QString redStyle =
        "QPushButton { "
        "background-color: #ff0000; "
        "color: #ffffff; "
        "border: 2px solid #ff6666; "
        "border-radius: 8px; "
        "padding: 15px; "
        "font-size: 16px; "
        "font-weight: bold; "
        "}";

    QString yellowStyle =
        "QPushButton { "
        "background-color: #ffff00; "
        "color: #101818; "
        "border: 2px solid #ffcc66; "
        "border-radius: 8px; "
        "padding: 15px; "
        "font-size: 16px; "
        "font-weight: bold; "
        "}";

    // ===== 更新软件状态 =====
    // 信号处理软件
    if (m_sigProBtn) {
        QString text;
        QString style;
        switch (m_sigProSta) {
            case 0: text = "信号处理 - 运行正常"; style = greenStyle; break;
            case 1: text = "信号处理 - 运行异常"; style = redStyle; break;
            case 2: text = "信号处理 - 启动成功"; style = greenStyle; break;
            case 3: text = "信号处理 - 启动失败"; style = redStyle; break;
            case 4: text = "信号处理 - 关闭成功"; style = yellowStyle; break;
            case 5: text = "信号处理 - 关闭失败"; style = redStyle; break;
            default: text = "信号处理 - 未知状态"; style = redStyle; break;
        }
        m_sigProBtn->setText(text);
        m_sigProBtn->setStyleSheet(style);
    }

    // 数据处理软件
    if (m_dataProBtn) {
        QString text;
        QString style;
        switch (m_dataProSta) {
            case 0: text = "数据处理 - 运行正常"; style = greenStyle; break;
            case 1: text = "数据处理 - 运行异常"; style = redStyle; break;
            case 2: text = "数据处理 - 启动成功"; style = greenStyle; break;
            case 3: text = "数据处理 - 启动失败"; style = redStyle; break;
            case 4: text = "数据处理 - 关闭成功"; style = yellowStyle; break;
            case 5: text = "数据处理 - 关闭失败"; style = redStyle; break;
            default: text = "数据处理 - 未知状态"; style = redStyle; break;
        }
        m_dataProBtn->setText(text);
        m_dataProBtn->setStyleSheet(style);
    }

    // 波束调度软件
    if (m_beamConBtn) {
        QString text;
        QString style;
        switch (m_beamConSta) {
            case 0: text = "波束调度 - 运行正常"; style = greenStyle; break;
            case 1: text = "波束调度 - 运行异常"; style = redStyle; break;
            case 2: text = "波束调度 - 启动成功"; style = greenStyle; break;
            case 3: text = "波束调度 - 启动失败"; style = redStyle; break;
            case 4: text = "波束调度 - 关闭成功"; style = yellowStyle; break;
            case 5: text = "波束调度 - 关闭失败"; style = redStyle; break;
            default: text = "波束调度 - 未知状态"; style = redStyle; break;
        }
        m_beamConBtn->setText(text);
        m_beamConBtn->setStyleSheet(style);
    }

    // 目标识别软件
    if (m_targetRecBtn) {
        QString text;
        QString style;
        switch (m_targetRecSta) {
            case 0: text = "目标识别 - 运行正常"; style = greenStyle; break;
            case 1: text = "目标识别 - 运行异常"; style = redStyle; break;
            case 2: text = "目标识别 - 启动成功"; style = greenStyle; break;
            case 3: text = "目标识别 - 启动失败"; style = redStyle; break;
            case 4: text = "目标识别 - 关闭成功"; style = yellowStyle; break;
            case 5: text = "目标识别 - 关闭失败"; style = redStyle; break;
            default: text = "目标识别 - 未知状态"; style = redStyle; break;
        }
        m_targetRecBtn->setText(text);
        m_targetRecBtn->setStyleSheet(style);
    }

    // ===== 更新 BIT 状态 =====
    // 定义小按钮样式
    QString smallGreenStyle =
        "QPushButton { "
        "background-color: #00ff00; "
        "color: #101818; "
        "border: 1px solid #66ffcc; "
        "border-radius: 5px; "
        "padding: 8px; "
        "font-size: 13px; "
        "}";

    QString smallRedStyle =
        "QPushButton { "
        "background-color: #ff0000; "
        "color: #ffffff; "
        "border: 1px solid #ff6666; "
        "border-radius: 5px; "
        "padding: 8px; "
        "font-size: 13px; "
        "}";

    // 解析BIT状态位
    unsigned char bitGroup = m_lastBITReport.bitGroup;
    unsigned char powerState = m_lastBITReport.powerState;

    // 更新阵面发射状态
    if (m_btnTxOpen) {
        bool isOpen = bitGroup & 0x80;
        m_btnTxOpen->setText(isOpen ? "阵面发射开启" : "阵面发射关闭");
        m_btnTxOpen->setStyleSheet(isOpen ? smallGreenStyle : smallRedStyle);
    }

    // 更新占空比状态
    if (m_btnDutyCycle) {
        bool hasAlarm = bitGroup & 0x40;
        m_btnDutyCycle->setText(hasAlarm ? "占空比报警" : "占空比正常");
        m_btnDutyCycle->setStyleSheet(hasAlarm ? smallRedStyle : smallGreenStyle);
    }

    // 更新脉宽状态
    if (m_btnPulseWidth) {
        bool hasAlarm = bitGroup & 0x20;
        m_btnPulseWidth->setText(hasAlarm ? "脉宽报警" : "脉宽正常");
        m_btnPulseWidth->setStyleSheet(hasAlarm ? smallRedStyle : smallGreenStyle);
    }

    // 更新阵面接收状态
    if (m_btnRxOpen) {
        bool isOpen = bitGroup & 0x10;
        m_btnRxOpen->setText(isOpen ? "阵面接收开启" : "阵面接收关闭");
        m_btnRxOpen->setStyleSheet(isOpen ? smallGreenStyle : smallRedStyle);
    }

    // 更新频率源状态
    if (m_btnFreqSrc) {
        bool isNormal = bitGroup & 0x08;
        m_btnFreqSrc->setText(isNormal ? "频率源正常" : "频率源异常");
        m_btnFreqSrc->setStyleSheet(isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新收发板状态
    if (m_btnDigBoard) {
        bool isLinked = bitGroup & 0x04;
        m_btnDigBoard->setText(isLinked ? "收发板建链" : "收发板断链");
        m_btnDigBoard->setStyleSheet(isLinked ? smallGreenStyle : smallRedStyle);
    }

    // 更新伺服状态
    if (m_btnServo) {
        bool isNormal = bitGroup & 0x02;
        m_btnServo->setText(isNormal ? "伺服正常" : "伺服异常");
        m_btnServo->setStyleSheet(isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新北斗状态
    if (m_btnBeidou) {
        bool isNormal = bitGroup & 0x01;
        m_btnBeidou->setText(isNormal ? "北斗正常" : "北斗异常");
        m_btnBeidou->setStyleSheet(isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新蓝牙状态
    if (m_btnBluetooth) {
        bool isNormal = powerState & 0x02;
        m_btnBluetooth->setText(isNormal ? "蓝牙正常" : "蓝牙异常");
        m_btnBluetooth->setStyleSheet(isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新波控板电源状态
    if (m_btnPowerBoard) {
        bool isNormal = powerState & 0x01;
        m_btnPowerBoard->setText(isNormal ? "波控板电源正常" : "波控板电源异常");
        m_btnPowerBoard->setStyleSheet(isNormal ? smallGreenStyle : smallRedStyle);
    }

    // ===== 更新温度和角度信息 =====
    if (m_tempLabel) {
        QString tempInfo = QString("FPGA温度: %1°C  |  阵面温度: %2°C")
                               .arg(m_lastBITReport.fpgaTemp * 0.1, 0, 'f', 1)
                               .arg(m_lastBITReport.panelTemp * 0.1, 0, 'f', 1);
        m_tempLabel->setText(tempInfo);
    }

    if (m_angleLabel) {
        QString angleInfo = QString("阵面偏航: %1°  |  扫描角度: %2°")
                                .arg(m_lastBITReport.yaw * 0.01, 0, 'f', 2)
                                .arg(m_lastBITReport.scanAngle * 0.01, 0, 'f', 2);
        m_angleLabel->setText(angleInfo);
    }
}

/**
 * @brief 打开处理软件关闭对话框
 * @details 发送系统关闭命令，依次关闭各处理软件
 */
void MainOverLayOut::onStopSoftwareClicked() {
    if (CustomMessageBox::showConfirm(this, "关闭确认", "是否关闭处理软件？\n将依次关闭信号处理、数据处理和目标识别软件。")) {
        StartSysParam param;
        // 设置关闭参数
        param.sta = 0;  // 关闭状态

        logCommand("正在关闭软件", "");

        emit CON_INS->sendSysStart(param);
    }
}

/**
 * @brief 打开TWS模式设置对话框
 * @details 配置TWS（Track-While-Scan）模式参数，整合工作模式、波形采样和伺服控制
 */
void MainOverLayOut::onTWSModeClicked() {
    logCommand("TWS模式设置", "打开TWS模式设置对话框");

    // 先创建CusWindow容器
    CusWindow* cusWin = new CusWindow(tr("TWS模式设置"), QIcon(":/resources/icon/radararray.png"), this);
    cusWin->setAttribute(Qt::WA_DeleteOnClose);

    // 创建TWS模式对话框，传入cusWin作为父对象
    TWSModeDialog* twsDialog = new TWSModeDialog(cusWin);
    twsDialog->setWindowFlags(Qt::Widget);  // 作为普通widget嵌入

    // 恢复保存的参数（使用现有成员变量组合）
    TWSModeParam savedParam;
    savedParam.scanRange = m_scanRange;
    savedParam.beamControl = m_beamControl;
    savedParam.servoControl = m_servoControlParam;
    twsDialog->restoreParam(savedParam);

    // 连接信号 - 按顺序发送三个配置，同时保存到内存
    connect(twsDialog, &TWSModeDialog::setScanRange, this, [this](const ScanRange param) {
        m_scanRange = param;  // 保存到内存
        logCommand("TWS - 工作模式设置", QString("工作模式: %1").arg(param.workMode == 0 ? "TWS" : "TAS"));
        // 发送工作模式配置到数据处理端
        emit sig_SetScanRangeParam(param);
    });

    connect(twsDialog, &TWSModeDialog::setBeamControl, this, [this](const BeamControl param) {
        m_beamControl = param;  // 保存到内存
        logCommand("TWS - 波形采样设置", QString("频率: 9.%1GHz, 方位: %2°~%3°")
            .arg(param.freqID)
            .arg(param.aziStart * 0.01)
            .arg(param.aziEnd * 0.01));
        // 发送波形及采样控制配置
        emit sig_SetBeamControlParam(param);
    });

    connect(twsDialog, &TWSModeDialog::setServoControl, this, [this](const ServoControlParam param) {
        m_servoControlParam = param;  // 保存到内存
        QString cmdText;
        switch(param.cmd) {
            case 0: cmdText = "方位停转"; break;
            case 1: cmdText = "方位转动"; break;
            case 2: cmdText = "方位寻位"; break;
            case 3: cmdText = "方位归北"; break;
            default: cmdText = "未知"; break;
        }
        logCommand("TWS - 伺服控制", QString("指令: %1, 速度: %2秒/转, 角度: %3°")
            .arg(cmdText)
            .arg(param.speed)
            .arg(param.az * 0.01));
        // 发送伺服控制配置
        emit sig_SetServoControlParam(param);
        enterWorkingModeIfStandby();
    });

    // 设置对话框作为内容
    cusWin->setContentWidget(twsDialog);
    cusWin->show();
}

/**
 * @brief 打开TAS模式设置对话框
 * @details 配置TAS（Target Alert System）模式参数
 */
/**
 * @brief 打开TAS模式设置对话框
 * @details 配置TAS（Track After Scan）模式参数
 *          TAS模式特点：
 *          1. 用户输入方位空域范围（不超过90°）
 *          2. 自动计算空域中心角度
 *          3. 自动生成伺服控制参数（方位寻位，3秒/转，中心角度）
 */
void MainOverLayOut::onTASModeClicked() {
    logCommand("TAS模式设置", "打开TAS模式设置对话框");

    // 先创建CusWindow容器
    CusWindow* cusWin = new CusWindow(tr("TAS模式设置"), QIcon(":/resources/icon/radararray.png"), this);
    cusWin->setAttribute(Qt::WA_DeleteOnClose);

    // 创建TAS模式对话框，传入cusWin作为父对象
    TASModeDialog* tasDialog = new TASModeDialog(cusWin);
    tasDialog->setWindowFlags(Qt::Widget);  // 作为普通widget嵌入

    // 从保存的参数恢复对话框状态
    TASModeParam tasParam;
    tasParam.scanRange = m_scanRange;
    tasParam.beamControl = m_beamControl;
    tasParam.servoControl = m_servoControlParam;
    tasDialog->restoreParam(tasParam);

    // 连接信号 - 按顺序发送三个配置，同时保存到内存
    connect(tasDialog, &TASModeDialog::setScanRange, this, [this](const ScanRange param) {
        m_scanRange = param;  // 保存到内存
        logCommand("TAS - 工作模式设置", QString("工作模式: %1").arg(param.workMode == 0 ? "TWS" : "TAS"));
        // 发送工作模式配置到数据处理端
        emit sig_SetScanRangeParam(param);
    });

    connect(tasDialog, &TASModeDialog::setBeamControl, this, [this](const BeamControl param) {
        m_beamControl = param;  // 保存到内存
        logCommand("TAS - 波形采样设置", QString("频率: 9.%1GHz, 方位中心: %2°")
            .arg(param.freqID)
            .arg(param.aziStart * 0.01));
        // 发送波形及采样控制配置
        emit sig_SetBeamControlParam(param);
    });

    connect(tasDialog, &TASModeDialog::setServoControl, this, [this](const ServoControlParam param) {
        m_servoControlParam = param;  // 保存到内存
        QString cmdText;
        switch(param.cmd) {
            case 0: cmdText = "方位停转"; break;
            case 1: cmdText = "方位转动"; break;
            case 2: cmdText = "方位寻位"; break;
            case 3: cmdText = "方位归北"; break;
            default: cmdText = "未知"; break;
        }
        logCommand("TAS - 伺服控制", QString("指令: %1, 速度: %2秒/转, 角度: %3°")
            .arg(cmdText)
            .arg(param.speed)
            .arg(param.az * 0.01));
        // 发送伺服控制配置
        emit sig_SetServoControlParam(param);
        enterWorkingModeIfStandby();
    });

    // 设置对话框作为内容
    cusWin->setContentWidget(tasDialog);
    cusWin->show();
}

/**
 * @brief 打开数据存储管理对话框
 * @details 配置数据保存和删除参数
 */
void MainOverLayOut::onDataStorageClicked() {
    // 如果窗口已经存在，直接显示并激活
    if (m_dataStorageWindow && m_dataStorageDialog) {
        m_dataStorageWindow->show();
        m_dataStorageWindow->raise();
        m_dataStorageWindow->activateWindow();
        return;
    }

    // 创建新窗口
    m_dataStorageWindow =
        new CusWindow("数据存储管理", QIcon(":/resources/icon/radararray.png"), this);
    // 不设置Qt::WA_DeleteOnClose，使用手动管理

    m_dataStorageDialog = new DataSaveUI(m_dataStorageWindow);
    m_dataStorageDialog->setWindowFlags(Qt::Widget);
    m_dataStorageWindow->setContentWidget(m_dataStorageDialog);

    // 连接数据存储参数发送
    connect(m_dataStorageDialog, &DataSaveUI::setParam, this, [this](DataSet param) {
        QString action;
        if (param.ifsave) {
            action = QString("数据存储 - ID:%1, 开关:%2")
                .arg(param.save.dataID)
                .arg(param.save.saveSwitch ? "开启" : "关闭");
        } else if (param.ifdel) {
            action = QString("数据删除 - ID:%1").arg(param.del.dataID);
        } else if (param.ifoffline) {
            action = QString("离线处理 - ID:%1, 模式:%2")
                .arg(param.off.dataID)
                .arg(param.off.onSwitch ? "离线" : "正常");
        }
        logCommand("数据存储管理", action);
    });

    connect(m_dataStorageDialog, &DataSaveUI::setParam, CON_INS, &Controller::sendDSParam);

    // 连接窗口内的UI更新（现在连接到持久化指针）
    connect(CON_INS, &Controller::dataSaveOK, m_dataStorageDialog, &DataSaveUI::dataSaveOK);
    connect(CON_INS, &Controller::dataDelOK, m_dataStorageDialog, &DataSaveUI::dataDelOK);
    connect(CON_INS, &Controller::offLineStat, m_dataStorageDialog, [this](OfflineStat offlinestat) {
        // 窗口内部状态更新（如果需要）
        // 全局状态已在构造函数中处理
    });

    // 窗口关闭时清空指针
    connect(m_dataStorageWindow, &CusWindow::destroyed, this, [this]() {
        m_dataStorageWindow = nullptr;
        m_dataStorageDialog = nullptr;
    });

    m_dataStorageWindow->show();
}

// ============ 参数设置槽函数实现 ============

/**
 * @brief 打开数据处理参数对话框
 * @details 配置数据处理相关参数
 */
void MainOverLayOut::onDataProcessClicked() {
    CusWindow* window =
        new CusWindow("数据处理参数", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    DataProcessUI* dialog = new DataProcessUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_dataProParam);

    // 保存参数
    connect(dialog, &DataProcessUI::setParam, this,
            [this](const DataProParam param) { m_dataProParam = param; });

    // 发送参数到控制器
    connect(dialog, &DataProcessUI::setParam, CON_INS, &Controller::sendDPParam);

    // 记录日志
    connect(dialog, &DataProcessUI::setParam, this, [this]() { logCommand("数据处理参数", ""); });

    window->show();
} /**
   * @brief 打开信号处理参数对话框
   * @details 配置信号处理相关参数
   */
void MainOverLayOut::onSignalProcessClicked() {
    CusWindow* window =
        new CusWindow("信号处理参数", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    sigParamUI* dialog = new sigParamUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_sigProParam);

    // 保存参数
    connect(dialog, &sigParamUI::setParam, this,
            [this](const SigProParam param) { m_sigProParam = param; });

    // 发送参数到控制器
    connect(dialog, &sigParamUI::setParam, CON_INS, &Controller::sendSPParam);

    // 记录日志
    connect(dialog, &sigParamUI::setParam, this, [this]() { logCommand("信号处理参数", ""); });

    window->show();
} /**
   * @brief 打开波形及采样控制对话框
   * @details 配置频率控制和波形参数
   */
void MainOverLayOut::onFreqControlClicked() {
    // 创建自定义窗口，使用雷达图标
    CusWindow* window =
        new CusWindow("波形及采样控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 创建对话框作为内容
    waveAndSample* waveControl = new waveAndSample(window);
    waveControl->setWindowFlags(Qt::Widget);  // 作为普通widget嵌入
    window->setContentWidget(waveControl);

    // 恢复上次参数
    waveControl->restoreParam(m_beamControl);

    // 保存参数
    connect(waveControl, &waveAndSample::setParam, this,
            [this](const BeamControl param) { m_beamControl = param; });

    // 发送参数
    connect(waveControl, &waveAndSample::setParam, CON_INS, &Controller::sendWCParam);

    // 记录日志
    connect(waveControl, &waveAndSample::setParam, this,
            [this]() { logCommand("波形采样参数", ""); });

    window->show();
} /**
   * @brief 打开电调控制对话框
   * @details 配置波束控制参数
   */
void MainOverLayOut::onBatteryControlClicked() {
    // 创建自定义窗口，使用雷达图标
    CusWindow* window =
        new CusWindow("阵面开启控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 创建对话框作为内容
    BatteryControl* dialog = new BatteryControl(window);
    dialog->setWindowFlags(Qt::Widget);  // 作为普通widget嵌入
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_batteryControlM);

    // 保存参数
    connect(dialog, &BatteryControl::setParam, this,
            [this](const BatteryControlM param) { m_batteryControlM = param; });

    // 连接参数设置信号到Controller
    connect(dialog, &BatteryControl::setParam, CON_INS, &Controller::sendBCParam);

    // 记录日志
    connect(dialog, &BatteryControl::setParam, this, [this]() { logCommand("阵面开启控制", ""); });

    window->show();
}

void MainOverLayOut::onServoControlClicked() {
    CusWindow* window = new CusWindow("伺服控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    ServoControl* dialog = new ServoControl(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    dialog->restoreParam(m_servoControlParam);

    connect(dialog, &ServoControl::setParam, this, [this](const ServoControlParam param) {
        m_servoControlParam = param;
        logCommand(QString::fromUtf8(u8"\u624b\u52a8\u4f3a\u670d\u63a7\u5236"),
                   formatServoControlDetails(param, QString::fromUtf8(u8"\u624b\u52a8\u4e0b\u53d1")));
    });

    connect(dialog, &ServoControl::setParam, CON_INS, &Controller::sendServoControl);

    window->show();
}

void MainOverLayOut::onServoNorthClicked()
{
    ServoControlParam northParam;
    northParam.cmd = 3;
    northParam.speed = m_servoControlParam.speed;
    northParam.az = 0;

    m_servoControlParam = northParam;
    CON_INS->sendServoControl(northParam);
    logCommand(QString::fromUtf8(u8"\u4e00\u952e\u4f3a\u670d\u5f52\u5317"),
               formatServoControlDetails(northParam, QString::fromUtf8(u8"\u7b2c1\u6b65/2: \u5f52\u53170\u00b0")));

    ServoControlParam seekParam = northParam;
    seekParam.cmd = 2;

    QTimer::singleShot(2000, this, [this, seekParam]() {
        m_servoControlParam = seekParam;
        CON_INS->sendServoControl(seekParam);
        logCommand(QString::fromUtf8(u8"\u4e00\u952e\u4f3a\u670d\u5f52\u5317"),
                   formatServoControlDetails(seekParam, QString::fromUtf8(u8"\u7b2c2\u6b65/2: 2s\u540e\u5bfb\u4f4d0\u00b0")));
    });
}
/**
 * @brief 打开方向图扫描控制对话框
 * @details 配置扫描范围参数
 */
void MainOverLayOut::onScanRangeClicked() {
    CusWindow* window =
        new CusWindow("工作模式控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    ScanRangeUI* dialog = new ScanRangeUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_scanRange);

    // 保存参数
    connect(dialog, &ScanRangeUI::setParam, this,
            [this](const ScanRange param) { m_scanRange = param; });

    // 发送参数到控制器
    connect(dialog, &ScanRangeUI::setParam, CON_INS, &Controller::sendSRParam);

    // 记录日志
    connect(dialog, &ScanRangeUI::setParam, this, [this]() { logCommand("工作模式控制", ""); });

    window->show();
} /**
   * @brief 打开光电系统控制对话框
   * @details 配置光电参数
   */
void MainOverLayOut::onPhotoelectricClicked() {
    CusWindow* window =
        new CusWindow("光电系统控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    PhotoElectricParam* dialog = new PhotoElectricParam(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 发送参数到控制器
    connect(dialog, &PhotoElectricParam::setParam, CON_INS, &Controller::sendPEParam);
    connect(dialog, &PhotoElectricParam::setParam2, CON_INS, &Controller::sendPEParam2);

    // 记录日志
    connect(dialog, &PhotoElectricParam::setParam, this, [this]() { logCommand("光电追踪", ""); });
    connect(dialog, &PhotoElectricParam::setParam2, this, [this]() { logCommand("光电追踪", ""); });

    window->show();
} /**
   * @brief 初始化日志信息组件
   * @details 设置日志文本框为只读
   */
void MainOverLayOut::setupLogInfo() {
    ui->logEdit->setReadOnly(true);
    // 日志文本框的样式已在darkstyle.qss中配置
}

void MainOverLayOut::scheduleLogFlush()
{
    if (m_logFlushTimer && !m_logFlushTimer->isActive()) {
        m_logFlushTimer->start();
    }
}

void MainOverLayOut::appendExternalLog(const QString& text)
{
    if (text.isEmpty()) return;

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    appendLogLine(QString("[%1] %2\n").arg(timestamp, text));
}

void MainOverLayOut::appendLogLine(const QString& line)
{
    if (line.isEmpty()) {
        return;
    }

    m_pendingLogLines.prepend(line);
    scheduleLogFlush();
}

void MainOverLayOut::flushPendingLogLines()
{
    if (m_pendingLogLines.isEmpty()) {
        return;
    }

    QStringList lines;
    for (const QString& pendingLine : m_pendingLogLines) {
        const QStringList splitLines = pendingLine.split('\n', Qt::SkipEmptyParts);
        for (const QString& splitLine : splitLines) {
            lines.append(splitLine);
        }
    }
    m_pendingLogLines.clear();

    const QStringList existingLines = ui->logEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    lines.append(existingLines);

    if (lines.count() > m_maxLogLines) {
        lines = lines.mid(0, m_maxLogLines);
    }

    QString updatedText;
    if (!lines.isEmpty()) {
        updatedText = lines.join('\n') + '\n';
    }

    ui->logEdit->setPlainText(updatedText);
    ui->logEdit->verticalScrollBar()->setValue(0);
}

/**
 * @brief 记录命令日志
 * @param commandName 命令名称
 * @param parameters 命令参数描述
 * @details 将命令操作记录到logEdit中，显示时间戳、命令名和参数
 *          日志从顶部添加，自动限制最大行数
 */
void MainOverLayOut::logCommand(const QString& commandName, const QString& parameters) {
    // 获取当前时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 格式化日志信息
    QString newLogEntry;
    if (parameters.isEmpty()) {
        newLogEntry = QString("[%1] %2\n").arg(timestamp, commandName);
    } else {
        newLogEntry = QString("[%1] %2: %3\n").arg(timestamp).arg(commandName).arg(parameters);
    }

    appendLogLine(newLogEntry);
}

// Servo control logging helpers. Keep protocol send paths unchanged; these only format UI log text.
QString MainOverLayOut::servoCommandText(unsigned char cmd) const
{
    switch (cmd) {
    case 0:
        return QString::fromUtf8(u8"\u65b9\u4f4d\u505c\u8f6c");
    case 1:
        return QString::fromUtf8(u8"\u65b9\u4f4d\u8f6c\u52a8");
    case 2:
        return QString::fromUtf8(u8"\u65b9\u4f4d\u5bfb\u4f4d");
    case 3:
        return QString::fromUtf8(u8"\u65b9\u4f4d\u5f52\u5317");
    default:
        return QString::fromUtf8(u8"\u672a\u77e5\u6307\u4ee4");
    }
}

QString MainOverLayOut::formatServoControlDetails(const ServoControlParam& param, const QString& source) const
{
    const QString mesId = QStringLiteral("0x")
        + QString("%1").arg(param.mesID, 4, 16, QLatin1Char('0')).toUpper();
    const double azDeg = static_cast<double>(param.az) / 100.0;

    return QString::fromUtf8(u8"%1, mesID=%2, cmd=%3(%4), speed=%5\u79d2/\u8f6c, azRaw=%6, az=%7\u00b0")
        .arg(source)
        .arg(mesId)
        .arg(static_cast<unsigned int>(param.cmd))
        .arg(servoCommandText(param.cmd))
        .arg(static_cast<unsigned int>(param.speed))
        .arg(param.az)
        .arg(azDeg, 0, 'f', 2);
}

/**
 * @brief 设置命令序列
 * @details 准备一键全数配置的命令列表
 */
void MainOverLayOut::setupCommands() {
    // 清空现有命令
    m_commands.clear();

    // 命令1: 发送扫描范围参数
    m_commands.push_back([this]() {
        sendScanRangeParams();
        logCommand("设置工作模式", "发送扫描范围参数");
    });

    // 命令2: 发送波形及采样控制参数
    m_commands.push_back([this]() {
        CON_INS->sendWCParam(m_beamControl);
        logCommand("波形及采样控制", "");
    });

    // 命令3: 发送信号处理参数
    m_commands.push_back([this]() {
        CON_INS->sendSPParam(m_sigProParam);
        logCommand("信号处理参数", "");
    });

    // 命令4: 发送数据处理参数
    m_commands.push_back([this]() {
        CON_INS->sendDPParam(m_dataProParam);
        logCommand("数据处理参数", "");
    });
}

/**
 * @brief 命令定时器超时槽函数
 * @details 定时器触发时依次执行命令列表中的命令
 */
void MainOverLayOut::cmdTimeOut() {
    if (m_commandIndex < static_cast<int>(m_commands.size())) {
        // 执行当前命令
        m_commands[m_commandIndex]();
        m_commandIndex++;
    } else {
        // 所有命令执行完毕，停止定时器
        m_commandTimer->stop();
        logCommand("一键全数配置完成", QString("共执行 %1 条命令").arg(m_commands.size()));
    }
}

/**
 * @brief 监控参数刷新槽函数
 * @param res 监控参数结构
 * @details 接收来自Controller的监控参数，更新系统健康状态和UI显示
 */
void MainOverLayOut::monitorParamRef(MonitorParam res) {
    bool normal = true;

    // 处理信号处理软件状态
    m_sigProSta = res.sigProSta;
    switch (res.sigProSta) {
        case 0:  // 正常
            break;
        case 1:  // 异常
            normal = false;
            break;
        case 2:  // 启动成功
            logCommand("信号处理软件启动成功", "");
            break;
        case 3:  // 启动失败
            logCommand("信号处理软件启动失败", "");
            break;
        case 4:  // 关闭成功
            logCommand("信号处理软件关闭成功", "");
            break;
        case 5:  // 关闭失败
            logCommand("信号处理软件关闭失败", "");
            break;
    }

    // 处理数据处理软件状态
    m_dataProSta = res.dataProSta;
    switch (res.dataProSta) {
        case 0:  // 正常
            break;
        case 1:  // 异常
            normal = false;
            break;
        case 2:  // 启动成功
            logCommand("数据处理软件启动成功", "");
            break;
        case 3:  // 启动失败
            logCommand("数据处理软件启动失败", "");
            break;
        case 4:  // 关闭成功
            logCommand("数据处理软件关闭成功", "");
            break;
        case 5:  // 关闭失败
            logCommand("数据处理软件关闭失败", "");
            break;
    }

    // 处理波束调度软件状态
    m_beamConSta = res.beamConSta;
    switch (res.beamConSta) {
        case 0:  // 正常
            break;
        case 1:  // 异常
            normal = false;
            break;
        case 2:  // 启动成功
            logCommand("波束调度软件启动成功", "");
            break;
        case 3:  // 启动失败
            logCommand("波束调度软件启动失败", "");
            break;
        case 4:  // 关闭成功
            logCommand("波束调度软件关闭成功", "");
            break;
        case 5:  // 关闭失败
            logCommand("波束调度软件关闭失败", "");
            break;
    }

    // 处理目标识别软件状态
    m_targetRecSta = res.targetRecSta;
    switch (res.targetRecSta) {
        case 0:  // 正常
            break;
        case 1:  // 异常
            normal = false;
            break;
        case 2:  // 启动成功
            logCommand("目标识别软件启动成功", "");
            break;
        case 3:  // 启动失败
            logCommand("目标识别软件启动失败", "");
            break;
        case 4:  // 关闭成功
            logCommand("目标识别软件关闭成功", "");
            break;
        case 5:  // 关闭失败
            logCommand("目标识别软件关闭失败", "");
            break;
    }

    // 更新系统整体状态
    m_systemNormal = normal;

    // 更新雷达系统按钮的样式
    if (normal) {
        // 正常状态 - 恢复原样式（清除自定义样式）
        ui->radarsystem->setStyleSheet("");
    } else {
        // 异常状态 - 半透明红色背景 + 深红边框
        ui->radarsystem->setStyleSheet(
            "QToolButton#radarsystem {"
            "    background-color: rgba(255, 0, 0, 0.3);"  // 半透明红色背景
            "    border: 2px solid #cc0000;"               // 深红色边框
            "    border-radius: 5px;"
            "}"
            "QToolButton#radarsystem:hover {"
            "    background-color: rgba(255, 0, 0, 0.5);"  // hover时稍微深一点
            "    border: 2px solid #ff0000;"
            "}");
    }

    // 如果健康管理窗口打开，更新显示
    updateHealthWindow();
}

void MainOverLayOut::onServoCtrlRet(ServoCtrlRet res) {
    auto resultText =
        QString("%1").arg(res.result == 0 ? "失败" : (res.result == 1 ? "成功" : "执行中"));
    QString cmdText;
    switch (res.cmd) {
        case 0:
            cmdText = "停转";
            break;
        case 1:
            cmdText = "转动";
            break;
        case 2:
            cmdText = "寻位";
            break;
        case 3:
            cmdText = "归北";
            break;
        default:
            cmdText = QString("未知(%1)").arg(res.cmd);
            break;
    }

    const double az = res.azCur / 100.0;  // 0.01°
    // 伺服信息不再显示到状态面板（已删除标签）
    // ui->stalabel1->setText("伺服命令");
    // ui->stamsg1->setText(QString("%1 | %2").arg(cmdText, resultText));
    // ui->stalabel2->setText("伺服速度(秒/转)");
    // ui->stamsg2->setText(QString::number(res.speed));
    // ui->stalabel3->setText("当前方位(°)");
    // ui->stamsg3->setText(QString::number(az, 'f', 2));

    logCommand("伺服回送", QString("cmd=%1 result=%2 speed=%3 az=%4°")
                               .arg(cmdText)
                               .arg(resultText)
                               .arg(res.speed)
                               .arg(QString::number(az, 'f', 2)));
}

void MainOverLayOut::onBITReport(BITReport res) {
    // 保存最新的BIT报告（始终保存，用于健康管理窗口实时显示）
    m_lastBITReport = res;

    // 检查是否需要更新日志和界面（1分钟更新一次）
    QDateTime currentTime = QDateTime::currentDateTime();
    bool shouldUpdate = false;

    if (!m_lastBITUpdateTime.isValid() ||
        m_lastBITUpdateTime.secsTo(currentTime) >= BIT_UPDATE_INTERVAL_SEC) {
        shouldUpdate = true;
        m_lastBITUpdateTime = currentTime;
    }

    // 仅在需要更新时记录日志
    if (shouldUpdate) {
        const double fpgaTemp = res.fpgaTemp / 10.0;    // 0.1°
        const double panelTemp = res.panelTemp / 10.0;  // 0.1°
        const double yaw = res.yaw / 100.0;             // 0.01°
        const QString powerState = res.powerState ? "正常" : "异常";

        QString subArrayInfo;
        for (int i = 0; i < 5; ++i) {
            subArrayInfo += QString("%1").arg(res.subArrayPower[i]);
            if (i != 4) subArrayInfo += ",";
        }

        logCommand("BIT上报", QString("power=%1 fpga=%2°C panel=%3°C yaw=%4° sub=%5")
                                  .arg(powerState)
                                  .arg(QString::number(fpgaTemp, 'f', 1))
                                  .arg(QString::number(panelTemp, 'f', 1))
                                  .arg(QString::number(yaw, 'f', 2))
                                  .arg(subArrayInfo));
    }

    // 如果健康管理窗口打开，实时更新显示
    updateHealthWindow();
}

/**
 * @brief 打开雷达系统健康管理对话框
 * @details 显示信号处理、数据处理、波束调度、目标识别四个子系统的运行状态，实时更新
 */
void MainOverLayOut::onRadarSystemClicked() {
    // 如果窗口已存在，则直接显示
    if (m_healthWindow) {
        m_healthWindow->show();
        m_healthWindow->raise();
        m_healthWindow->activateWindow();
        return;
    }

    // 创建自定义窗口
    m_healthWindow =
        new CusWindow("雷达系统健康管理", QIcon(":/resources/icon/radararray.png"), this);
    m_healthWindow->setAttribute(Qt::WA_DeleteOnClose);
    m_healthWindow->setMinimumSize(500, 900);

    // 连接窗口关闭信号，清空指针
    connect(m_healthWindow, &QObject::destroyed, this, [this]() {
        m_healthWindow = nullptr;
        m_sigProBtn = nullptr;
        m_dataProBtn = nullptr;
        m_beamConBtn = nullptr;
        m_targetRecBtn = nullptr;
        m_btnTxOpen = nullptr;
        m_btnDutyCycle = nullptr;
        m_btnPulseWidth = nullptr;
        m_btnRxOpen = nullptr;
        m_btnFreqSrc = nullptr;
        m_btnDigBoard = nullptr;
        m_btnServo = nullptr;
        m_btnBeidou = nullptr;
        m_btnBluetooth = nullptr;
        m_btnPowerBoard = nullptr;
        m_tempLabel = nullptr;
        m_angleLabel = nullptr;
    });

    // 创建内容Widget
    QWidget* contentWidget = new QWidget(m_healthWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // ===== 软件状态区域 =====
    QLabel* softwareLabel = new QLabel("软件运行状态", contentWidget);
    softwareLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #66ffcc;");
    mainLayout->addWidget(softwareLabel);

    // 创建四个软件状态按钮
    m_sigProBtn = new QPushButton("信号处理", contentWidget);
    m_sigProBtn->setEnabled(false);  // 不可选
    m_sigProBtn->setMinimumHeight(50);

    m_dataProBtn = new QPushButton("数据处理", contentWidget);
    m_dataProBtn->setEnabled(false);  // 不可选
    m_dataProBtn->setMinimumHeight(50);

    m_beamConBtn = new QPushButton("波束调度", contentWidget);
    m_beamConBtn->setEnabled(false);  // 不可选
    m_beamConBtn->setMinimumHeight(50);

    m_targetRecBtn = new QPushButton("目标识别", contentWidget);
    m_targetRecBtn->setEnabled(false);
    m_targetRecBtn->setMinimumHeight(50);

    mainLayout->addWidget(m_sigProBtn);
    mainLayout->addWidget(m_dataProBtn);
    mainLayout->addWidget(m_beamConBtn);
    mainLayout->addWidget(m_targetRecBtn);

    // ===== BIT状态区域 =====
    mainLayout->addSpacing(20);
    QLabel* bitLabel = new QLabel("BIT 状态信息", contentWidget);
    bitLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #66ffcc;");
    mainLayout->addWidget(bitLabel);

    // 创建BIT状态网格布局
    QGridLayout* bitGrid = new QGridLayout();
    bitGrid->setSpacing(10);

    // 创建BIT状态按钮
    m_btnTxOpen = new QPushButton("阵面发射", contentWidget);
    m_btnTxOpen->setEnabled(false);
    bitGrid->addWidget(m_btnTxOpen, 0, 0);

    m_btnDutyCycle = new QPushButton("占空比", contentWidget);
    m_btnDutyCycle->setEnabled(false);
    bitGrid->addWidget(m_btnDutyCycle, 0, 1);

    m_btnPulseWidth = new QPushButton("脉宽", contentWidget);
    m_btnPulseWidth->setEnabled(false);
    bitGrid->addWidget(m_btnPulseWidth, 1, 0);

    m_btnRxOpen = new QPushButton("阵面接收", contentWidget);
    m_btnRxOpen->setEnabled(false);
    bitGrid->addWidget(m_btnRxOpen, 1, 1);

    m_btnFreqSrc = new QPushButton("频率源", contentWidget);
    m_btnFreqSrc->setEnabled(false);
    bitGrid->addWidget(m_btnFreqSrc, 2, 0);

    m_btnDigBoard = new QPushButton("收发板", contentWidget);
    m_btnDigBoard->setEnabled(false);
    bitGrid->addWidget(m_btnDigBoard, 2, 1);

    m_btnServo = new QPushButton("伺服", contentWidget);
    m_btnServo->setEnabled(false);
    bitGrid->addWidget(m_btnServo, 3, 0);

    m_btnBeidou = new QPushButton("北斗", contentWidget);
    m_btnBeidou->setEnabled(false);
    bitGrid->addWidget(m_btnBeidou, 3, 1);

    m_btnBluetooth = new QPushButton("蓝牙", contentWidget);
    m_btnBluetooth->setEnabled(false);
    bitGrid->addWidget(m_btnBluetooth, 4, 0);

    m_btnPowerBoard = new QPushButton("波控板电源", contentWidget);
    m_btnPowerBoard->setEnabled(false);
    bitGrid->addWidget(m_btnPowerBoard, 4, 1);

    mainLayout->addLayout(bitGrid);

    // ===== 温度和角度信息 =====
    mainLayout->addSpacing(10);
    QLabel* infoLabel = new QLabel("温度和角度信息", contentWidget);
    infoLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #66ffcc;");
    mainLayout->addWidget(infoLabel);

    m_tempLabel = new QLabel("温度信息加载中...", contentWidget);
    m_tempLabel->setStyleSheet("font-size: 14px; color: #ffffff; padding: 10px;");
    mainLayout->addWidget(m_tempLabel);

    m_angleLabel = new QLabel("角度信息加载中...", contentWidget);
    m_angleLabel->setStyleSheet("font-size: 14px; color: #ffffff; padding: 10px;");
    mainLayout->addWidget(m_angleLabel);

    mainLayout->addStretch();

    // 设置内容
    m_healthWindow->setContentWidget(contentWidget);

    // 首次更新显示
    updateHealthWindow();

    m_healthWindow->show();
}

/**
 * @brief 处理发射开关按钮点击
 * @details 弹出确认对话框，确认后切换发射状态
 *
 * 功能说明：
 * - 根据当前状态显示相应的确认对话框
 * - 构造TranRecControl参数结构（接收默认为开启）
 * - 通过Controller发送命令
 * - 更新按钮状态和记录日志
 */
void MainOverLayOut::onTransmitControlClicked()
{
    // 根据当前状态显示不同的确认对话框
    QString title = m_isTransmitting ? "关闭发射确认" : "开启发射确认";
    QString message = m_isTransmitting
        ? "是否确定关闭发射？\n（接收仍保持开启）"
        : "是否确定开启发射？\n（接收将自动开启）";

    bool confirmed = CustomMessageBox::showConfirm(this, title, message);

    if (!confirmed) {
        return;
    }

    // 切换发射状态
    m_isTransmitting = !m_isTransmitting;

    // 构造参数：接收默认开启，发射根据状态设置
    TranRecControl param;
    param.recv = 1;  // 接收默认开启
    param.tran = m_isTransmitting ? 1 : 0;  // 根据状态设置发射

    // 发送参数到控制器
    CON_INS->sendTRParam(param);

    // 更新保存的参数
    m_tranRecControlM = param;

    // 更新按钮显示
    updateTransmitButton();

    // 记录日志
    logCommand("发射开关", QString("接收:开启 发射:%1")
               .arg(m_isTransmitting ? "开启" : "关闭"));
}

/**
 * @brief 处理雷达待机按钮点击
 * @details 切换雷达工作模式（待机 ↔ TWS）
 *
 * 功能说明：
 * - 根据当前状态切换工作模式
 * - 待机模式：workMode=2
 * - TWS工作模式：workMode=0
 * - 通过Controller发送命令
 * - 更新按钮状态和记录日志
 */
void MainOverLayOut::onRadarStandbyClicked()
{
    // 切换待机状态
    m_isStandby = !m_isStandby;

    // 构造参数
    ScanRange param;
    param.workMode = m_isStandby ? 2 : 0;  // 2:待机, 0:TWS

    // 发送参数到控制器
    CON_INS->sendSRParam(param);

    // 更新保存的参数
    m_scanRange = param;

    // 更新按钮显示
    updateStandbyButton();

    // 记录日志
    logCommand("雷达模式切换", QString("工作模式:%1")
               .arg(m_isStandby ? "待机" : "TWS工作"));
}

void MainOverLayOut::enterWorkingModeIfStandby()
{
    if (!m_isStandby) {
        return;
    }

    ui->btnRadarStandby->click();
}

void MainOverLayOut::arrangeParamSettingsButtons()
{
    const QList<QPushButton*> managedButtons = {
        ui->btnDataProcess,
        ui->btnSignalProcess,
        ui->btnServoControl,
        ui->btnScanRange,
        ui->btnFreqControl,
        ui->btnBatteryControl
    };

    for (auto* btn : managedButtons) {
        ui->paramSettingsGrid->removeWidget(btn);
        btn->hide();
    }

    QList<QPushButton*> visibleButtons = {
        ui->btnDataProcess,
        ui->btnSignalProcess,
        ui->btnServoControl,
        ui->btnScanRange
    };

    if (AuthManager::instance().isAdminMode()) {
        visibleButtons.append(ui->btnFreqControl);
    }

    for (int i = 0; i < visibleButtons.size(); ++i) {
        auto* btn = visibleButtons.at(i);
        btn->show();
        ui->paramSettingsGrid->addWidget(btn, i / 2, i % 2);
    }
}

/**
 * @brief 更新发射按钮的显示状态
 * @details 根据当前发射状态更新按钮的颜色和文本
 */
void MainOverLayOut::updateTransmitButton()
{
    if (m_isTransmitting) {
        // 发射开启：绿色背景，文本为"关闭发射"
        ui->btnTransmitControl->setText("关闭发射");
        ui->btnTransmitControl->setStyleSheet(
            "QPushButton {"
            "    background-color: #00ff00;"
            "    color: #101818;"
            "    border: 2px solid #66ffcc;"
            "    border-radius: 8px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #33ff33;"
            "    border: 2px solid #00ff88;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #00cc00;"
            "}");
    } else {
        // 发射关闭：红色背景，文本为"开启发射"
        ui->btnTransmitControl->setText("开启发射");
        ui->btnTransmitControl->setStyleSheet(
            "QPushButton {"
            "    background-color: #ff0000;"
            "    color: #ffffff;"
            "    border: 2px solid #ff6666;"
            "    border-radius: 8px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #ff3333;"
            "    border: 2px solid #ff8888;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #cc0000;"
            "}");
    }
}

/**
 * @brief 更新待机按钮的显示状态
 * @details 根据当前待机状态更新按钮的颜色和文本
 */
void MainOverLayOut::updateStandbyButton()
{
    if (m_isStandby) {
        // 待机模式：红色背景，文本为"进入工作"
        ui->btnRadarStandby->setText("进入工作");
        ui->btnRadarStandby->setStyleSheet(
            "QPushButton {"
            "    background-color: #ff0000;"
            "    color: #ffffff;"
            "    border: 2px solid #ff6666;"
            "    border-radius: 8px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #ff3333;"
            "    border: 2px solid #ff8888;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #cc0000;"
            "}");
    } else {
        // 工作模式：绿色背景，文本为"进入待机"
        ui->btnRadarStandby->setText("进入待机");
        ui->btnRadarStandby->setStyleSheet(
            "QPushButton {"
            "    background-color: #00ff00;"
            "    color: #101818;"
            "    border: 2px solid #66ffcc;"
            "    border-radius: 8px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #33ff33;"
            "    border: 2px solid #00ff88;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #00cc00;"
            "}");
    }
}

/**
 * @brief 打开录屏回放窗口
 * @details 显示屏幕录制与回放管理界面，使用CusWindow包装
 */
void MainOverLayOut::onRecordPlayClicked() {
    // 如果窗口已存在，则直接显示
    if (m_recorderWindow) {
        m_recorderWindow->show();
        m_recorderWindow->raise();
        m_recorderWindow->activateWindow();
        return;
    }

    // 创建自定义窗口
    m_recorderWindow =
        new CusWindow("记录回放", QIcon(":/resources/icon/record.png"), this);
    m_recorderWindow->setAttribute(Qt::WA_DeleteOnClose);
    m_recorderWindow->setMinimumSize(600, 700);

    // 创建录屏组件
    m_recorderWidget = new ScreenRecorderWidget(m_recorderWindow);
    m_recorderWidget->setRecordTarget(window()); // 录制顶级窗口

    m_recorderWindow->setContentWidget(m_recorderWidget);

    // 连接窗口关闭信号，清空指针
    connect(m_recorderWindow, &QObject::destroyed, this, [this]() {
        m_recorderWindow = nullptr;
        m_recorderWidget = nullptr;
    });

    m_recorderWindow->show();
    LOG_INFO("Screen recorder window opened");
}

/**
 * @brief 根据屏幕分辨率动态设置面板宽度、按钮高度等尺寸
 * @details 覆盖 .ui 文件中的固定像素值，使界面在 1366×768 ~ 3840×2160 范围内自适应
 */
void MainOverLayOut::applyScaledSizes() {
    int leftW   = ScaleHelper::leftPanelWidth();
    int rightW  = ScaleHelper::rightPanelWidth();
    const bool compact = ScaleHelper::compactLayout();
    int btnH    = ScaleHelper::buttonHeight();      // 基准40px缩放
    int setTabH = ScaleHelper::setTabMaxHeight();   // 基准600px缩放
    int logo    = ScaleHelper::logoSize();          // 基准50px缩放

    // --- 左侧面板 ---
    const int leftMinW = compact ? 300 : 340;
    const int leftMaxW = compact ? 430 : 560;
    if (m_leftSidebar) {
        m_leftSidebar->setMinimumWidth(leftMinW);
        m_leftSidebar->setMaximumWidth(leftMaxW);
    }
    ui->infoTab->setMinimumWidth(leftMinW);
    ui->infoTab->setMaximumWidth(QWIDGETSIZE_MAX);

    // --- 左侧设置面板 ---
    ui->setTab->setMinimumWidth(leftMinW);
    ui->setTab->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->setTab->setMaximumHeight(setTabH);
    ui->trackTab->setMinimumWidth(leftMinW);
    ui->trackTab->setMaximumWidth(QWIDGETSIZE_MAX);

    // --- 右侧 P显/扇区显示面板 ---
    const int rightMinW = compact ? 300 : 340;
    const int rightMaxW = compact ? 470 : 560;
    const int topButtonW = compact ? 82 : 92;
    const int funcButtonW = compact ? 70 : 78;
    const int funcButtonH = compact ? 56 : 62;
    const int funcIcon = compact ? 30 : 34;
    if (m_rightSidebar) {
        m_rightSidebar->setMinimumWidth(rightMinW);
        m_rightSidebar->setMaximumWidth(rightMaxW);
    }
    ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->horizontalSpacer_3->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->LogoWidget->setMaximumWidth(qBound(230, rightW - 120, 300));
    ui->CloseButton->setMinimumWidth(topButtonW);
    ui->CloseButton->setMaximumWidth(topButtonW);
    ui->minButton->setMinimumWidth(topButtonW);
    ui->minButton->setMaximumWidth(topButtonW);
    ui->CloseButton->setFocusPolicy(Qt::NoFocus);
    ui->minButton->setFocusPolicy(Qt::NoFocus);
    ui->timeLabel->setMaximumWidth(rightW);
    ui->FuncWidget->setMaximumWidth(rightW);
    const QList<QToolButton*> functionButtons = {
        ui->toolButton_3, ui->radarsystem, ui->FuncToolButton_2, ui->FuncToolButton
    };
    for (auto* btn : functionButtons) {
        btn->setMinimumWidth(funcButtonW);
        btn->setMinimumHeight(funcButtonH);
        btn->setIconSize(QSize(funcIcon, funcIcon));
        btn->setFocusPolicy(Qt::NoFocus);
    }
    ui->pviewFitW->setMinimumWidth(rightMinW);
    ui->pviewFitW->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->pviewSectorW->setMinimumWidth(rightMinW);
    ui->pviewSectorW->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->horizontalLayout_3->invalidate();
    ui->horizontalLayout_4->invalidate();
    ui->horizontalLayout_6->invalidate();
    ui->verticalLayout_5->invalidate();
    if (m_mainSplitter) {
        const int ppiDefaultW = qMax(700, ScaleHelper::logicalWidth() - leftW - rightW);
        QList<int> splitterSizes;
        splitterSizes << leftW << ppiDefaultW << rightW;
        m_mainSplitter->setSizes(splitterSizes);
    }

    // --- 雷达控制按钮 (4×2 grid) ---
    QList<QPushButton*> radarBtns = {
        ui->btnStartSoftware, ui->btnStopSoftware,
        ui->btnTWSMode,       ui->btnTASMode,
        ui->btnDataStorage,   ui->btnServoNorth,
        ui->btnRadarStandby,  ui->btnTransmitControl
    };
    for (auto* btn : radarBtns) {
        btn->setMinimumHeight(btnH);
        btn->setFocusPolicy(Qt::NoFocus);
    }

    // --- 参数设置按钮 ---
    QList<QPushButton*> paramBtns = {
        ui->btnDataProcess,   ui->btnSignalProcess,
        ui->btnServoControl,  ui->btnScanRange,
        ui->btnFreqControl
    };
    for (auto* btn : paramBtns) {
        btn->setMinimumHeight(btnH);
        btn->setFocusPolicy(Qt::NoFocus);
    }

    // --- Logo ---
    ui->label_12->setMaximumSize(logo, logo);

    LOG_INFO(QString("ScaleHelper applied: factor=%1 leftPanel=%2 rightPanel=%3 btnH=%4")
                 .arg(ScaleHelper::factor())
                 .arg(leftW)
                 .arg(rightW)
                 .arg(btnH));
}
