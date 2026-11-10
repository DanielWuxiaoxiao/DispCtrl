#include "Basic/targetrecognition.h"
#include "Basic/uithemeruntime.h"
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-20 18:52:02
 * @Description: 
 */
#include "mainoverlayout.h"

#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/offlinerae.h"
#include "Controller/RadarDataManager.h"
#include "Controller/controller.h"
#include "CommandControl/commandcontrolmodule.h"
#include "ElectromagneticDevice/devicemodule.h"
#include "Controller/laserreportmanager.h"
#include "PhotoelectricV2/module.h"
#include "ZhoushanOptical/zhoushanmodule.h"
#include "RadarTrackApi/radartrackapi.h"
#include "Controller/raedatasetreader.h"
#include "Controller/subsystemnetworkmonitor.h"
#include "PointManager/detmanager.h"
#include "PointManager/trackmanager.h"
#include "PolarDisp/mousepositioninfo.h"
#include "PolarDisp/polaraxis.h"
#include "PolarDisp/ppisscene.h"
#include "PolarDisp/ppiview.h"
#include "PolarDisp/pviewtopleft.h"
#include "azelrangewidget.h"
#include "cusWidgets/customcombobox.h"
#include "cusWidgets/custommessagebox.h"
#include <QMenu>
#include "cusWidgets/cuswindow.h"
#include "cusWidgets/developerlogindialog.h"
#include "cusWidgets/detachablewidget.h"
#include "cusWidgets/frozentablewidget.h"
// 参数配置对话框头文件
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDoubleValidator>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <utility>

#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "PolarDisp/sectorscene.h"
#include "PolarDisp/sectorwidget.h"
#include "PolarDisp/rangeazimuthchart.h"
#include "PolarDisp/rangeheightchart.h"
#include "PolarDisp/track3dwidget.h"
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
#include "Basic/tascontrolformat.h"
#include "Basic/uifontscale.h"
#include "Basic/uitheme.h"
#include "healthstatuswidgets.h"
#include "classichealthwidgets.h"
#include "Basic/classicui.h"
#include <QScrollArea>
#include <QScreen>

namespace {
constexpr int kTrackTableRefreshIntervalMs = 50;
constexpr int kLogFlushIntervalMs = 50;
constexpr bool kDefaultEnableSectorDisplay = false;
constexpr int kTrackExternalReportingRole = Qt::UserRole + 1;
constexpr int kTrackReportingBoundaryRole = Qt::UserRole + 2;

void setStyleSheetIfChanged(QWidget* widget, const QString& style)
{
    UiTheme::setStyle(widget, [style] { return UiFontScale::style(style); });
}

enum TrackTableColumn {
    TrackIdColumn = 0,
    TrackRangeColumn,
    TrackAzimuthColumn,
    TrackHeightColumn,
    TrackSpeedColumn,
    TrackTypeColumn,
    TrackSnrColumn,
    TrackAmplitudeColumn,
    TrackElevationColumn,
    TrackTableColumnCount
};

int trackRangeColumnMinimumWidth(QTableWidget* tableWidget)
{
    // 距离保留米和一位小数；粗体用于覆盖上报行，样式尺寸包含单元格 padding。
    tableWidget->ensurePolished();
    QFont font = tableWidget->font();
    font.setBold(true);
    const QFontMetrics metrics(font);
    QChar widestDigit = QLatin1Char('0');
    for (ushort digit = '1'; digit <= '9'; ++digit) {
        if (metrics.horizontalAdvance(QChar(digit)) > metrics.horizontalAdvance(widestDigit))
            widestDigit = QChar(digit);
    }
    QStyleOptionViewItem option;
    option.initFrom(tableWidget);
    option.widget = tableWidget;
    option.font = font;
    option.fontMetrics = metrics;
    option.features = QStyleOptionViewItem::HasDisplay;
    option.text = QString(qMax(8, tableWidget->property("trackRangeTextCapacity").toInt()), widestDigit);
    return tableWidget->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), tableWidget).width();
}

bool isExternalControlStatus(const QString& status)
{
    return status != QStringLiteral("否");
}

class TrackTableItemDelegate final : public QStyledItemDelegate
{
public:
    explicit TrackTableItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem itemOption(option);
        initStyleOption(&itemOption, index);
        const QStyle* style = itemOption.widget ? itemOption.widget->style()
                                                : QApplication::style();
        const bool isHeightColumn = index.column() == TrackHeightColumn;
        if (isHeightColumn) {
            // 高度列由本代理统一绘制文字。先让 Qt 绘制底色、边框和选中态，
            // 但清空默认文字，避免与后面的绿色文字重叠。
            QStyleOptionViewItem backgroundOption(itemOption);
            backgroundOption.text.clear();
            style->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, painter,
                               backgroundOption.widget);
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }

        painter->save();
        const QRect cellRect = option.rect.adjusted(1, 1, -1, -1);
        const bool externallyReporting = index.data(kTrackExternalReportingRole).toBool();
        if (externallyReporting) {
            // 保持黑色主题底色，仅以细框强调，避免破坏普通行和交替行的可读性。
            QPen reportingPen{(UiTheme::classicEnabled ? QColor(0, 222, 166) : QColor(UiTheme::Accent))};
            reportingPen.setWidth(2);
            painter->setPen(reportingPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(cellRect);
        }

        if (index.data(kTrackReportingBoundaryRole).toBool()) {
            // 第一条未上报航迹上方的粗分割线，将上报与未上报两组清晰分开。
            QPen boundaryPen{(UiTheme::classicEnabled ? QColor(0, 255, 200) : QColor(UiTheme::Accent))};
            boundaryPen.setWidth(3);
            painter->setPen(boundaryPen);
            painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        }

        if (isHeightColumn) {
            // QSS 会覆盖 QTableWidgetItem 的前景色；高度文字只在这里绘制一次。
            QFont heightFont = itemOption.font;
            heightFont.setBold(true);
            painter->setFont(heightFont);
            painter->setPen((UiTheme::classicEnabled ? QColor(90, 255, 185) : QColor(UiTheme::Normal)));
            const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText,
                                                         &itemOption, itemOption.widget);
            painter->drawText(textRect, itemOption.displayAlignment,
                              index.data(Qt::DisplayRole).toString());
        }
        painter->restore();
    }
};

quint64 makeTrackTableUpdateKey(unsigned type, unsigned int batch)
{
    return (static_cast<quint64>(type) << 32) | static_cast<quint64>(batch);
}
}

MainOverLayOut::MainOverLayOut(QWidget* parent) : QWidget(parent), ui(new Ui::MainOverLayOut) {
    ui->setupUi(this);
    UiTheme::applyClassicMainIcons(this);
    // Scope the slightly larger text to these pages, including dynamically added test buttons.
    // 字号微增后收紧按钮内留白，小屏侧栏仍保留完整操作文字。
    const auto controlButtonFont = [] { return UiFontScale::style(UiTheme::classicEnabled
        ? QStringLiteral("QPushButton { font-size:16px; }")
        : QStringLiteral("QPushButton { font-size:16px; padding:6px 14px; }")); };
    UiTheme::setStyle(ui->tab_4, controlButtonFont);
    UiTheme::setStyle(ui->tab, controlButtonFont);
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

    // 仅作为阵面开启控制弹窗的初始显示状态；程序启动不自动下发 AA01。
    const int defaultPanelCount = CF_INS.defaultPanelCount();
    m_batteryControlM.quadrant1 = defaultPanelCount >= 1;
    m_batteryControlM.quadrant2 = defaultPanelCount >= 2;
    m_batteryControlM.quadrant3 = defaultPanelCount >= 3;
    m_batteryControlM.quadrant4 = defaultPanelCount >= 4;

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
    m_sigProParam.distanceProcessUpperLimitM = CF_INS.sigProDistanceProcessUpperLimitM(5200);
    m_sigProParam.heightProcessLowerLimitM = CF_INS.sigProHeightProcessLowerLimitM(0);

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
    m_healthTabs = nullptr;
    m_activeHealthPanel = 0;
    m_sigProBtn = nullptr;
    m_dataProBtn = nullptr;
    m_beamConBtn = nullptr;
    m_targetRecBtn = nullptr;
    m_networkHealthOk.fill(false);
    m_networkHealthText.fill(QStringLiteral("网络状态检查中..."));

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

    // 初始化雷达控制状态
    m_isTransmitting = false;  // 默认发射关闭
    m_isStandby = true;        // 默认待机模式
    m_resourceStatusTimeoutMs = qBound(5000,
        CF_INS.displayConfig(QStringLiteral("resource_status_timeout_ms"), 15000), 300000);
    m_resourceStatusClock.start();
    // Use raw monitor events, not fault-latched five-second display snapshots.
    connect(CON_INS, &Controller::monitorParamSend, this, [this](const MonitorParam& report) {
        if (report.mesID != 0xCF01) return;
        m_resourceStatusClock.restart();
        if (report.beamConSta != 0 && report.beamConSta != 2)
            resetTransmitState(QStringLiteral("调度软件状态=%1").arg(report.beamConSta));
    });
    auto* resourceStatusTimer = new QTimer(this);
    resourceStatusTimer->setInterval(1000);
    connect(resourceStatusTimer, &QTimer::timeout, this, [this] {
        if (m_resourceStatusClock.hasExpired(m_resourceStatusTimeoutMs))
            resetTransmitState(QStringLiteral("监控状态超过%1秒未更新").arg(m_resourceStatusTimeoutMs / 1000.0));
    });
    resourceStatusTimer->start();

    // 连接健康管理信号
    connect(CON_INS, &Controller::radarPanelPositionRequested, this, [this](const QString& text) {
        logCommand(QStringLiteral("下发雷达阵面经纬高姿态"), text);
    });
    connect(CON_INS, &Controller::healthSoftwareSnapshot, this, &MainOverLayOut::monitorParamRef);
    connect(CON_INS, &Controller::healthFaultSummary, this, [this](const QString& text) {
        logCommand(QStringLiteral("健康异常"), text);
    });
    connect(CON_INS, &Controller::trackDataRateRequested, this, [this](const QString& text) {
        logCommand(QStringLiteral("调整跟踪数据率"), text);
    });
    connect(CON_INS, &Controller::healthWindowFlushed, this, [this] {
        flushStatusUi();
        if (m_healthUiDirty) updateHealthWindow();
    });

    // 连接伺服回送与BIT上报
    connect(CON_INS, &Controller::servoCtrlRet, this, &MainOverLayOut::onServoCtrlRet);
    connect(CON_INS, &Controller::healthBitSnapshot, this, &MainOverLayOut::onBITReport);

    m_subsystemNetworkMonitor = new SubsystemNetworkMonitor(this);
    connect(m_subsystemNetworkMonitor, &SubsystemNetworkMonitor::statusChanged, this,
            &MainOverLayOut::updateNetworkHealthStatus);
    m_subsystemNetworkMonitor->start();
    LOG_INFO(QStringLiteral("[UI_FEATURES] config=%1 laser=%2 legacyGcsReport=%3（总控入口以模块enabled为准）")
        .arg(CF_INS.getConfigFilePath()).arg(CF_INS.laserReportEnabled(true)).arg(CF_INS.gcsTargetReportEnabled(false)));

    m_commandControlButton = new QPushButton(ui->btnDataProcess->parentWidget());
    m_commandControlButton->setObjectName(QStringLiteral("commandControlButton"));
    m_commandControlButton->setText(QStringLiteral("总控通信"));
    m_commandControlButton->setToolTip(QStringLiteral("打开总控通信控制、组播设备状态和 DDA4 回放窗口"));
    m_commandControlButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_commandControlButton->setEnabled(false);
    connect(m_commandControlButton, &QPushButton::clicked, this, [this]() {
        if (m_commandControlModule) {
            m_commandControlModule->showControlWindow();
        }
    });

    if (CF_INS.testTrackEnabled(false)) {
        m_testTrackButton = new QPushButton(ui->btnDataProcess->parentWidget());
        m_testTrackButton->setObjectName(QStringLiteral("testTrackButton"));
        m_testTrackButton->setText(QStringLiteral("生成测试航迹"));
        m_testTrackButton->setToolTip(QStringLiteral("生成配置数量的本地普通航迹边界样本；生成中点击可终止并清除全部测试批次"));
        m_testTrackButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(m_testTrackButton, &QPushButton::clicked,
                this, &MainOverLayOut::onGenerateTestTracksClicked);
        connect(CON_INS, &Controller::testTrackGenerationStateChanged, this,
                [this](bool active) {
            if (!m_testTrackButton) {
                return;
            }
            m_testTrackButton->setEnabled(true);
            m_testTrackButton->setText(active ? QStringLiteral("终止并清除测试航迹")
                                               : QStringLiteral("生成测试航迹"));
            if (!active) {
                appendExternalLog(QStringLiteral("[测试航迹] 本轮已完成，所有测试批次已自动消批"));
            }
        });
    }

    if (CF_INS.testDetectionEnabled(false)) {
        m_testDetectionButton = new QPushButton(ui->btnDataProcess->parentWidget());
        m_testDetectionButton->setObjectName("testDetectionButton");
        m_testDetectionButton->setText(QStringLiteral("生成测试点迹"));
        m_testDetectionButton->setToolTip(QStringLiteral("通过检测点公共入口生成测试点迹；停止后已有点迹遵循显示数量上限淘汰"));
        connect(m_testDetectionButton, &QPushButton::clicked, this, [] {
            if (CON_INS->isTestDetectionGenerationRunning()) CON_INS->stopTestDetectionGeneration();
            else CON_INS->startTestDetectionGeneration();
        });
        connect(CON_INS, &Controller::testDetectionGenerationStateChanged, this, [this](bool active) {
            m_testDetectionButton->setText(active ? QStringLiteral("终止生成测试点迹") : QStringLiteral("生成测试点迹"));
        });
    }

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
    setupOfflineRaeButtons();

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
            if (m_track3DWidget) {
                m_track3DWidget->setTrackVisible(visible);
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
                if (m_track3DWidget) {
                    m_track3DWidget->setTbdTrackVisible(visible);
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
                if (m_track3DWidget) {
                    m_track3DWidget->setCooperativeTrackVisible(visible);
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
            if (m_track3DWidget) {
                m_track3DWidget->setTrackSizeRatio(ratio);
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

    // 副标题位置改为显示软件版本号（替代原英文名 "Radar Control Platform"）
    ui->SubtitleLabel->setText(APP_VERSION_STR);
    ui->SubtitleLabel->setToolTip("软件版本号");

    ui->verticalLayout_6->removeWidget(ui->SubtitleLabel);
    auto* modeRow = new QHBoxLayout;
    modeRow->setSpacing(10);
    modeRow->addWidget(ui->SubtitleLabel, 0, Qt::AlignVCenter);
    m_modeSwitchButton = new QPushButton(ui->SubtitleLabel->parentWidget());
    m_modeSwitchButton->setObjectName(QStringLiteral("modeSwitchButton"));
    m_modeSwitchButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    const auto modeStyle = [] { return UiFontScale::style(QString(
        (UiTheme::classicEnabled ? "QPushButton#modeSwitchButton { color:#66FFCC; background:#123630; border:1px solid #66CCAA; "
        "border-radius:5px; font-size:15px; padding:3px 10px; min-height:24px; }"
        "QPushButton#modeSwitchButton:hover { background:#195046; }" : "QPushButton#modeSwitchButton { color:#c4d3dd; background:rgba(32,46,55,150); border:1px solid #364a57; "
        "border-radius:5px; font-size:15px; padding:3px 10px; min-height:24px; }"
        "QPushButton#modeSwitchButton:hover { background:rgba(38,59,71,210); }"))); };
    UiTheme::setStyle(m_modeSwitchButton, modeStyle);
    modeRow->addWidget(m_modeSwitchButton, 0, Qt::AlignVCenter);
    m_themeSwitchButton = new QPushButton(QStringLiteral("换肤"), ui->SubtitleLabel->parentWidget());
    m_themeSwitchButton->setObjectName(QStringLiteral("themeSwitchButton"));
    m_themeSwitchButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    UiTheme::setStyle(m_themeSwitchButton, [modeStyle] {
        return modeStyle().replace(QStringLiteral("modeSwitchButton"), QStringLiteral("themeSwitchButton"));
    });
    modeRow->addWidget(m_themeSwitchButton, 0, Qt::AlignVCenter);
    m_themeSwitchButton->setToolTip(UiTheme::classicEnabled ? QStringLiteral("当前经典主题，点击切换石墨主题")
                                                         : QStringLiteral("当前石墨主题，点击切换经典主题"));
    connect(m_themeSwitchButton, &QPushButton::clicked, this, [this] {
        const QString next = UiTheme::classicEnabled ? QStringLiteral("graphite") : QStringLiteral("classic");
        m_themeSwitchButton->setEnabled(false);
        const bool changed = UiTheme::Runtime::instance().apply(next);
        if (changed && !CF_INS.saveUiTheme(next)) {
            LOG_WARNING("[UI_THEME] switched for this session; config.toml could not be saved");
            appendExternalLog(QStringLiteral("主题已切换，本地配置保存失败；下次启动将使用原配置主题。"));
        }
        m_themeSwitchButton->setEnabled(true);
    });
    UiTheme::onChanged(this, [this] { refreshTheme(); });
    modeRow->addStretch();
    ui->verticalLayout_6->addLayout(modeRow);
    connect(m_modeSwitchButton, &QPushButton::clicked, this, &MainOverLayOut::onModeSwitchClicked);
    connect(&AuthManager::instance(), &AuthManager::modeChanged, this, [this](bool) {
        refreshModeControls();
        arrangeParamSettingsButtons();
        LOG_INFO(QStringLiteral("[UI_MODE] switched to %1").arg(AuthManager::instance().isAdminMode() ? "developer" : "user"));
    });
    refreshModeControls();

    connect(ui->minButton, &QPushButton::clicked, CON_INS, &Controller::minimizeWindow);
    connect(ui->CloseButton, &QPushButton::clicked, this, [this]() {
        // 使用 nullptr 作为父窗口，避免在 quit 时对话框与主窗口的释放顺序冲突
        if (CustomMessageBox::showConfirm(nullptr, "退出确认", "是否确认退出程序？")) {
            // 延迟退出，确保对话框完全关闭后再退出
            LOG_INFO("[AppLifecycle] quit requested by main close button");
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
        new DetachableWidget("P显", m_zoomView, UiTheme::themedIcon(QStringLiteral(":/resources/icon/scan.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this));

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
    m_track3DWidget = new Track3DWidget(this);                  // 三维航迹显示
    if (mScene && mScene->track()) {
        connect(mScene->track(), &TrackManager::batchFocusChanged,
                m_track3DWidget, &Track3DWidget::setBatchFocused);
    }

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

    // 页签保留足够宽度，避免高 DPI 或较窄布局下中文标题被挤压。
    UiTheme::setStyle(displayTabWidget, [=] { return UiFontScale::style(
        "QTabWidget#DisplayTabWidget QTabBar::tab { "
        "    min-width: 58px; "
        "    padding: 4px 12px; "
        "}"
    ); });

    // 将扇区显示包装为可分离的widget
    // DetachableWidget* sectorDetachable = new DetachableWidget(
    //     "扇区显示", m_sectorWidget, QIcon(":/resources/icon/ui/radar.svg"), this);

    // 将距离-方位显示包装为可分离的widget
    DetachableWidget* rangeAzDetachable = new DetachableWidget(
        "B显", m_rangeAzimuthWidget, UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
    DetachableWidget* rangeHeightDetachable = new DetachableWidget(
        "高显", m_rangeHeightWidget, UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
    DetachableWidget* track3DDetachable = new DetachableWidget(
        "3D显", m_track3DWidget, UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);

    // 添加到TabWidget
    //displayTabWidget->addTab(sectorDetachable, "扇区显示");
    displayTabWidget->addTab(rangeAzDetachable, "B显");
    displayTabWidget->addTab(rangeHeightDetachable, "H显");
    displayTabWidget->addTab(track3DDetachable, "3D显");

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
            if (m_track3DWidget) {
                m_track3DWidget->addPointInfo(info);
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
                if (m_track3DWidget) {
                    m_track3DWidget->addPointInfo(info);
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
                if (m_track3DWidget) {
                    m_track3DWidget->addPointInfo(info);
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
        if (m_track3DWidget) {
            connect(mView, &PPIView::clearDisplayTriggered,
                    m_track3DWidget, &Track3DWidget::clearRadarData);
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
            if (m_track3DWidget) {
                m_track3DWidget->setMaxTrackPoints(maxPoints);
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
            if (m_track3DWidget) {
                m_track3DWidget->setRangeFromMain(min, max);
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
        if (m_track3DWidget) {
            m_track3DWidget->setRangeFromMain(minR, maxR);
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
    UiTheme::setStyle(rangeTab, [=] { return UiFontScale::style((UiTheme::classicEnabled ? R"(
        QWidget {
            background: rgba(16, 24, 24, 0.8);
            border-radius: 8px;
        }
    )" : R"(
        QWidget {
            background: transparent;
            border-radius: 8px;
        }
    )")); });

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
    LOG_INFO("[UI_LIFECYCLE][MainOverLayOut] destroy begin");
    disconnect(&AuthManager::instance(), nullptr, this, nullptr);
    if (m_commandControlModule) {
        disconnect(m_commandControlModule, nullptr, this, nullptr);
    }
    if (m_commandTimer) {
        m_commandTimer->stop();
    }
    // destroyed 回调会访问计时器和健康页缓存，必须在派生成员析构前执行。
    // 直接销毁也会移除待处理的 DeferredDelete，不运行嵌套事件循环。
    delete m_healthWindow;
    delete m_dataStorageWindow;
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
    LOG_INFO("[UI_LIFECYCLE][MainOverLayOut] destroy end");
}

void MainOverLayOut::setCommandControlModule(CommandControlModule* module)
{
    if (module && !module->isEnabled()) module = nullptr;
    if (m_commandControlModule) {
        disconnect(m_commandControlModule, &CommandControlModule::reportingStateChanged,
                   this, &MainOverLayOut::refreshExternalReportingIndicators);
    }
    m_commandControlModule = module;
    if (m_commandControlModule) {
        connect(m_commandControlModule, &CommandControlModule::reportingStateChanged,
                this, &MainOverLayOut::refreshExternalReportingIndicators);
    }
    if (m_commandControlButton) {
        m_commandControlButton->setEnabled(module != nullptr);
    }
    arrangeParamSettingsButtons();
    refreshExternalReportingIndicators();
    LOG_INFO(QStringLiteral("[UI_FEATURES] 总控通信入口=%1").arg(module ? "显示" : "隐藏"));
}

void MainOverLayOut::setElectromagneticDeviceModule(ElectromagneticDeviceModule* module)
{
    m_electromagneticModule = module;
    if (module) {
        module->setInitialSoftwareStatus(currentSoftwareStatus());
        connect(this, &MainOverLayOut::softwareStatusChanged, module,
                &ElectromagneticDeviceModule::processSoftwareReport, Qt::UniqueConnection);
        connect(this, &MainOverLayOut::electromagneticPanelStatusChanged, module,
                &ElectromagneticDeviceModule::processBitReport, Qt::UniqueConnection);
        if (m_bitReportTimes[0].isValid()) module->processBitReport(currentElectromagneticPanelStatus());
    }
    if (getPPIView()) getPPIView()->setElectromagneticDeviceModule(module);
    if (!module || !module->isUiEnabled()) return;
    if (!m_electromagneticButton) {
        m_electromagneticButton = new QPushButton(ui->btnDataProcess->parentWidget());
        m_electromagneticButton->setObjectName("electromagneticDeviceButton");
        m_electromagneticButton->setText(QStringLiteral("电磁设备通信"));
        connect(m_electromagneticButton, &QPushButton::clicked, this, [this] {
            if (m_electromagneticModule) m_electromagneticModule->showWindow(this);
        });
    }
    connect(module, &ElectromagneticDeviceModule::linkStateChanged, this, [this](bool ok, const QString& text) {
        m_electromagneticLinkOk = ok;
        m_electromagneticStatus = text;
        if (m_electromagneticButton) m_electromagneticButton->setToolTip(text);
        scheduleHealthWindowRefresh();
    });
    arrangeParamSettingsButtons();
}

void MainOverLayOut::applyElectromagneticControl(quint16 message, const QByteArray& payload)
{
    if (!ElectromagneticProtocol::validCommand(message, payload)) return;
    const quint8 value = quint8(payload[0]);
    switch (message) {
    case 1:
        // 外部协议固定阵面 0，保留其它阵面的当前开关。
        m_batteryControlM.quadrant1 = value;
        CON_INS->sendBCParam(m_batteryControlM);
        break;
    case 2:
        m_tranRecControlM.recv = value;
        m_tranRecControlM.tran = quint8(payload[1]);
        m_isTransmitting = m_tranRecControlM.tran != 0;
        CON_INS->sendTRParam(m_tranRecControlM);
        if (m_commandControlModule) m_commandControlModule->setRadiationStatus(m_isTransmitting);
        updateTransmitButton();
        break;
    case 3:
        m_scanRange.workMode = value;
        m_isStandby = false;
        CON_INS->sendSRParam(m_scanRange);
        updateStandbyButton();
        break;
    case 4:
        m_beamControl.scene = value;
        emit sig_SetBeamControlParam(m_beamControl);
        break;
    case 8: {
        StartSysParam command; command.sta = value;
        CON_INS->sendSysStart(command);
        break;
    }
    default: return;
    }
    logCommand(QStringLiteral("电磁设备控制"), QStringLiteral("编号:%1 参数:%2")
        .arg(message).arg(QString::fromLatin1(payload.toHex(' '))));
}

void MainOverLayOut::setLaserReportManager(LaserReportManager* module)
{
    if (m_laserReportManager) {
        disconnect(m_laserReportManager, &LaserReportManager::reportingStateChanged,
                   this, &MainOverLayOut::refreshExternalReportingIndicators);
    }
    m_laserReportManager = module;
    if (m_laserReportManager) {
        connect(m_laserReportManager, &LaserReportManager::reportingStateChanged,
                this, &MainOverLayOut::refreshExternalReportingIndicators);
    }
    refreshExternalReportingIndicators();
}

bool MainOverLayOut::isLaserNetworkReady() const
{
    return m_networkHealthOk[SubsystemNetworkMonitor::LaserLocal]
        && m_networkHealthOk[SubsystemNetworkMonitor::LaserPeer];
}

void MainOverLayOut::setPhotoelectricV2Module(PhotoelectricV2::Module* module)
{
    m_opticalV2 = module;
    if (!module) return;
    m_opticalStatus = module->statusText();
    connect(module, &PhotoelectricV2::Module::statusChanged, this, [this](bool ok, const QString& text) {
        m_opticalLinkOk = ok; m_opticalStatus = text; scheduleHealthWindowRefresh();
    });
    connect(module, &PhotoelectricV2::Module::reportingStateChanged, this, [this] {
        refreshExternalReportingIndicators();
    });
}

void MainOverLayOut::setZhoushanOpticalModule(ZhoushanOptical::Module* module)
{
    m_zhoushanOptical = module;
    if (!module) return;
    m_zhoushanStatus = module->statusText();
    connect(module, &ZhoushanOptical::Module::statusChanged, this, [this](bool ok, const QString& text) {
        m_zhoushanStatus = text; m_zhoushanLinkOk = ok; scheduleHealthWindowRefresh();
    });
    connect(module, &ZhoushanOptical::Module::reportingStateChanged,
            this, &MainOverLayOut::refreshExternalReportingIndicators);
}

void MainOverLayOut::setRadarTrackApiModule(RadarTrackApi::Module* module)
{
    m_radarTrackApi = module;
    if (!module) return;
    m_radarTrackApiButton = new QPushButton(QStringLiteral("平台航迹上报"), this);
    connect(m_radarTrackApiButton, &QPushButton::clicked, this, [this] { m_radarTrackApi->showWindow(this); });
    arrangeParamSettingsButtons();
}

QString MainOverLayOut::laserNetworkStatusText() const
{
    return QStringLiteral("本机激光网口：%1；激光终端：%2")
        .arg(m_networkHealthText[SubsystemNetworkMonitor::LaserLocal],
             m_networkHealthText[SubsystemNetworkMonitor::LaserPeer]);
}

void MainOverLayOut::onGenerateTestTracksClicked()
{
    if (CON_INS->isTestTrackGenerationRunning()) {
        if (CON_INS->stopTestTrackGeneration()) {
            appendExternalLog(QStringLiteral("[测试航迹] 已终止本轮并清除全部测试批次"));
        }
        return;
    }
    if (CON_INS->startTestTrackGeneration()) {
        appendExternalLog(QStringLiteral("[测试航迹] 已开始生成本地普通航迹；再次点击可终止并清除，完成后自动消批"));
        return;
    }
    appendExternalLog(QStringLiteral("[测试航迹] 无法启动：功能未启用或上一轮尚未结束"));
}

void MainOverLayOut::setupTrackManagement() {
    LOG_INFO(QString("[MainOverLayOut::setupTrackManagement] iftbd=%1 ifxietong=%2")
             .arg(CF_INS.iftbd(false))
             .arg(CF_INS.ifxietong(false)));

    setupTrackTable(ui->tableWidget, m_trackTableFrozenHelper);
    setupTrackTable(ui->droneTableWidget, m_droneTableFrozenHelper);
    for (auto* table : {ui->tableWidget, ui->droneTableWidget}) {
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table, &QTableWidget::customContextMenuRequested, this, [this, table](const QPoint& pos) {
            const int row = table->rowAt(pos.y());
            if (row < 0 || !table->item(row, 0)) return;
            bool ok = false;
            const int batch = table->item(row, 0)->text().toInt(&ok);
            if (!ok || batch < 0 || batch > 65535) return;
            QMenu menu(table);
            auto* rate = menu.addAction(tr("调整跟踪数据率 [批次: %1]").arg(batch));
            if (menu.exec(table->viewport()->mapToGlobal(pos)) != rate) return;
            int hz = 2;
            if (CustomMessageBox::getInteger(this, tr("调整跟踪数据率"),
                tr("航迹批号 %1：请输入数据率（Hz，1～100）").arg(batch), 1, 100, hz)
                && !CON_INS->requestTrackDataRate(batch, hz))
                CustomMessageBox::showWarning(this, tr("下发失败"), tr("参数无效或资源调度尚未初始化。"));
        });
    }

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
    if (UiTheme::classicEnabled) {
        if (!tableWidget) return;

        QStringList headers;
        headers << "ID" << "距离" << "方位" << "高度" << "速度" << "类型" << "SNR" << "幅度" << "俯仰";

        tableWidget->setColumnCount(headers.size());
        tableWidget->setHorizontalHeaderLabels(headers);
        tableWidget->horizontalHeader()->setVisible(true);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setAlternatingRowColors(true);
        tableWidget->setItemDelegate(new TrackTableItemDelegate(tableWidget));
        tableWidget->setShowGrid(true);
        tableWidget->setWordWrap(false);
        tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

        QHeaderView* headerView = tableWidget->horizontalHeader();
        // 初始列宽由 fitTrackTableColumnsToViewport 均匀分配；各列仍可手动调整。
        headerView->setStretchLastSection(false);
        headerView->setMinimumSectionSize(48);
        for (int i = 0; i < headers.size(); ++i) {
            headerView->setSectionResizeMode(i, QHeaderView::Interactive);
        }
        tableWidget->setProperty("trackColumnsNeedInitialFit", true);
        tableWidget->setProperty("trackColumnsUserResized", false);
        tableWidget->setProperty("trackColumnsFitting", false);
        connect(headerView, &QHeaderView::sectionResized, tableWidget,
                [tableWidget](int, int, int) {
                    if (!tableWidget->property("trackColumnsFitting").toBool()) {
                        tableWidget->setProperty("trackColumnsUserResized", true);
                    }
                });
        fitTrackTableColumnsToViewport(tableWidget);
        QTimer::singleShot(0, tableWidget, [this, tableWidget]() {
            if (tableWidget->viewport()->width() > 0) {
                fitTrackTableColumnsToViewport(tableWidget);
                tableWidget->setProperty("trackColumnsNeedInitialFit", false);
            }
        });

        frozenHelper = nullptr;
        return;
    }

    if (!tableWidget) return;

    QStringList headers;
    headers << "ID" << "距离" << "方位" << "高度" << "速度" << "类型" << "SNR" << "幅度" << "俯仰";

    tableWidget->setColumnCount(headers.size());
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->horizontalHeader()->setVisible(true);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setItemDelegate(new TrackTableItemDelegate(tableWidget));
    tableWidget->setShowGrid(true);
    tableWidget->setWordWrap(false);
    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    QHeaderView* headerView = tableWidget->horizontalHeader();
    // 初始列宽优先容纳完整距离，其余空间分给其他列；各列仍可手动调整。
    headerView->setStretchLastSection(false);
    headerView->setMinimumSectionSize(48);
    for (int i = 0; i < headers.size(); ++i) {
        headerView->setSectionResizeMode(i, QHeaderView::Interactive);
    }
    tableWidget->setProperty("trackColumnsNeedInitialFit", true);
    tableWidget->setProperty("trackColumnsUserResized", false);
    tableWidget->setProperty("trackColumnsFitting", false);
    tableWidget->setProperty("trackRangeTextCapacity", 8);
    connect(headerView, &QHeaderView::sectionResized, tableWidget,
            [tableWidget](int, int, int) {
                if (!tableWidget->property("trackColumnsFitting").toBool()) {
                    tableWidget->setProperty("trackColumnsUserResized", true);
                }
            });
    fitTrackTableColumnsToViewport(tableWidget);
    QTimer::singleShot(0, tableWidget, [this, tableWidget]() {
        if (tableWidget->viewport()->width() > 0) {
            fitTrackTableColumnsToViewport(tableWidget);
            tableWidget->setProperty("trackColumnsNeedInitialFit", false);
        }
    });

    frozenHelper = nullptr;
}

void MainOverLayOut::fitTrackTableColumnsToViewport(QTableWidget* tableWidget)
{
    if (UiTheme::classicEnabled) {
        if (!tableWidget || tableWidget->columnCount() <= 0) {
            return;
        }

        const int viewportWidth = tableWidget->viewport()->width();
        if (viewportWidth <= 0 || tableWidget->columnCount() != TrackTableColumnCount) {
            return;
        }

        // 前 8 列刚好铺满可视区；第 9 列“俯仰”固定放在右侧滚动区，默认不挤占主信息。
        const int idWidth = qBound(54, viewportWidth / 10, 78);
        const int remainingWidth = qMax(7 * 48, viewportWidth - idWidth);
        const int commonWidth = remainingWidth / 7;
        tableWidget->setProperty("trackColumnsFitting", true);
        tableWidget->setColumnWidth(TrackIdColumn, idWidth);
        int assignedWidth = idWidth;
        for (int column = TrackRangeColumn; column <= TrackAmplitudeColumn; ++column) {
            const int width = column == TrackAmplitudeColumn
                ? qMax(48, viewportWidth - assignedWidth) : qMax(48, commonWidth);
            tableWidget->setColumnWidth(column, width);
            assignedWidth += width;
        }
        tableWidget->setColumnWidth(TrackElevationColumn, 72);
        tableWidget->setProperty("trackColumnsFitting", false);
        return;
    }

    if (!tableWidget || tableWidget->columnCount() <= 0 ||
        tableWidget->property("trackColumnsUserResized").toBool()) {
        return;
    }

    const int viewportWidth = tableWidget->viewport()->width();
    if (viewportWidth <= 0 || tableWidget->columnCount() != TrackTableColumnCount) {
        return;
    }

    // 宽度足够时前 8 列铺满；窄屏优先完整显示距离，允许横向滚动，不挤压主 PPI。
    const int idWidth = qBound(54, viewportWidth / 10, 78);
    const int commonWidth = qMax(7 * 48, viewportWidth - idWidth) / 7;
    const int rangeWidth = qMax(trackRangeColumnMinimumWidth(tableWidget), commonWidth);
    // 增宽距离不能挤窄类型等其他列；额外宽度放入既有横向滚动区。
    const int amplitudeWidth = qMax(48, viewportWidth - idWidth - 6 * commonWidth);
    tableWidget->setProperty("trackColumnsFitting", true);
    tableWidget->setColumnWidth(TrackIdColumn, idWidth);
    tableWidget->setColumnWidth(TrackRangeColumn, rangeWidth);
    for (int column = TrackAzimuthColumn; column <= TrackAmplitudeColumn; ++column) {
        const int width = column == TrackAmplitudeColumn ? amplitudeWidth : commonWidth;
        tableWidget->setColumnWidth(column, width);
    }
    tableWidget->setColumnWidth(TrackElevationColumn, 72);
    tableWidget->setProperty("trackColumnsFitting", false);
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
    if (OfflineRae::isOffline(info)) {
        return;
    }

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
    if (OfflineRae::isOffline(info)) {
        return;
    }

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
        QString recResultStr = TargetRecognition::text(static_cast<unsigned>(targetType));
        QTableWidgetItem* typeItem = trackTable->item(ordinaryTrackRow, TrackTypeColumn);
        if (!typeItem) {
            typeItem = new QTableWidgetItem();
            trackTable->setItem(ordinaryTrackRow, TrackTypeColumn, typeItem);
        }
        typeItem->setText(recResultStr);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
    }

    if (targetType == 1 && ordinaryTrackRow >= 0) {
        PointInfo info;
        info.batch = batchID;
        info.range = trackTable->item(ordinaryTrackRow, TrackRangeColumn)->text().toFloat();
        info.azimuth = trackTable->item(ordinaryTrackRow, TrackAzimuthColumn)->text().toFloat();
        info.elevation = trackTable->item(ordinaryTrackRow, TrackElevationColumn)->text().toFloat();
        info.altitute = trackTable->item(ordinaryTrackRow, TrackHeightColumn)->text().toFloat();
        info.speed = trackTable->item(ordinaryTrackRow, TrackSpeedColumn)->text().toFloat();
        info.SNR = trackTable->item(ordinaryTrackRow, TrackSnrColumn)->text().toFloat();
        info.amp = trackTable->item(ordinaryTrackRow, TrackAmplitudeColumn)->text().toFloat();
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

    // 无人机页签初始不可见，首次切换后再按真实 viewport 宽度铺满列。
    if (ui->trackTab->currentWidget() == ui->droneTrackTab &&
        !ui->droneTableWidget->property("trackColumnsUserResized").toBool()) {
        QTimer::singleShot(0, ui->droneTableWidget, [this]() {
            if (ui->droneTableWidget->viewport()->width() > 0) {
                fitTrackTableColumnsToViewport(ui->droneTableWidget);
                ui->droneTableWidget->setProperty("trackColumnsNeedInitialFit", false);
            }
        });
    }
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
    if (m_track3DWidget) {
        m_track3DWidget->setOnlyRecognizedDroneTracksVisible(droneTabActive);
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
    if (m_track3DWidget) {
        m_track3DWidget->setTrackVisible(normalTrackDisplay);
        m_track3DWidget->setTbdTrackVisible(specialTrackDisplay && tbdVisible);
        m_track3DWidget->setCooperativeTrackVisible(specialTrackDisplay && cooperativeVisible);
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

    const QString rangeText = QString::number(info.range, 'f', 1);
    ensureItem(TrackRangeColumn)->setText(rangeText);
    // 仅首次遇到更长距离时扩容，不逐点测量字体或扫描整表，且尊重手动列宽。
    if (!UiTheme::classicEnabled && !tableWidget->property("trackColumnsUserResized").toBool() &&
        rangeText.size() > tableWidget->property("trackRangeTextCapacity").toInt()) {
        tableWidget->setProperty("trackRangeTextCapacity", rangeText.size());
        fitTrackTableColumnsToViewport(tableWidget);
    }
    ensureItem(TrackAzimuthColumn)->setText(QString::number(info.azimuth, 'f', 1));
    ensureItem(TrackHeightColumn)->setText(QString::number(info.altitute, 'f', 1));
    ensureItem(TrackSpeedColumn)->setText(QString::number(info.speed, 'f', 1));
    // 目标识别结果（来自数据处理上报）
    QString recResultStr = targetType;
    if (info.type == PointType::Track) {
        recResultStr = TargetRecognition::text(info);
    }
    ensureItem(TrackTypeColumn)->setText(recResultStr);
    ensureItem(TrackSnrColumn)->setText(QString::number(info.SNR, 'f', 1));
    ensureItem(TrackAmplitudeColumn)->setText(QString::number(info.amp, 'f', 1));
    const QString controlStatus = externalControlStatus(info);
    ensureItem(TrackElevationColumn)->setText(QString::number(info.elevation, 'f', 1));

    applyTrackRowAppearance(tableWidget, row, info.type, controlStatus);

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

    // 自定义排序：正在对外上报的航迹置顶，再按无人机和起批时间排列。
    std::sort(allRows.begin(), allRows.end(), [this](const TrackRowData& a, const TrackRowData& b) {
        unsigned int batchA = a.columns[0].toUInt();
        unsigned int batchB = b.columns[0].toUInt();

        int typeA = m_targetTypes.value(batchA, 0);
        int typeB = m_targetTypes.value(batchB, 0);
        PointInfo infoA;
        infoA.batch = batchA;
        infoA.type = a.type;
        PointInfo infoB;
        infoB.batch = batchB;
        infoB.type = b.type;
        // 上报状态直接从模块读取，不依赖表格显示列，保证上报行优先排序。
        const bool reportingA = isExternalControlStatus(externalControlStatus(infoA));
        const bool reportingB = isExternalControlStatus(externalControlStatus(infoB));
        if (reportingA != reportingB) {
            return reportingA;
        }

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
    bool previousRowReporting = false;
    for (int i = 0; i < allRows.size(); ++i) {
        tableWidget->insertRow(i);
        const TrackRowData& rowData = allRows[i];
        for (int col = 0; col < rowData.columns.size(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(rowData.columns[col]);
            if (col == 0) {
                item->setData(Qt::UserRole, rowData.type);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(i, col, item);
        }
        PointInfo info;
        info.batch = rowData.columns.value(TrackIdColumn).toUInt();
        info.type = rowData.type;
        const QString controlStatus = externalControlStatus(info);
        applyTrackRowAppearance(tableWidget, i, rowData.type, controlStatus);
        const bool reporting = isExternalControlStatus(controlStatus);
        const bool startsUnreportedGroup = i > 0 && previousRowReporting && !reporting;
        if (startsUnreportedGroup) {
            for (int column = 0; column < tableWidget->columnCount(); ++column) {
                if (QTableWidgetItem* item = tableWidget->item(i, column)) {
                    item->setData(kTrackReportingBoundaryRole, true);
                }
            }
        }
        previousRowReporting = reporting;
    }
}

QString MainOverLayOut::externalControlStatus(const PointInfo& info) const
{
    if (info.type != PointType::Track) {
        return QStringLiteral("否");
    }
    const int batch = static_cast<int>(info.batch);
    QString commandControlStatus;
    if (m_commandControlModule && m_commandControlModule->isManualReportActive(info.batch)) {
        commandControlStatus = QStringLiteral("手动");
    } else if (m_commandControlModule && m_commandControlModule->isAutoReportActive(info.batch)) {
        commandControlStatus = QStringLiteral("自动");
    }
    const bool laserReporting = m_laserReportManager
        && (m_laserReportManager->isReporting(batch)
            || m_laserReportManager->isAutoReportingTrack(batch));
    if (m_opticalV2 && m_opticalV2->isReporting(batch)) {
        return commandControlStatus.isEmpty() ? QStringLiteral("光电V2") : commandControlStatus + QStringLiteral("/光电V2");
    }
    if (m_zhoushanOptical && m_zhoushanOptical->isReporting(batch)) {
        return commandControlStatus.isEmpty() ? QStringLiteral("光电目指") : commandControlStatus + QStringLiteral("/光电目指");
    }
    if (!commandControlStatus.isEmpty() && laserReporting) {
        return commandControlStatus + QStringLiteral("/激光");
    }
    if (!commandControlStatus.isEmpty()) {
        return commandControlStatus;
    }
    if (laserReporting) {
        return QStringLiteral("激光");
    }
    return QStringLiteral("否");
}

void MainOverLayOut::applyTrackRowAppearance(QTableWidget* tableWidget, int row, unsigned type,
                                             const QString& controlStatus) const
{
    if (UiTheme::classicEnabled) {
        if (!tableWidget || row < 0 || row >= tableWidget->rowCount()) {
            return;
        }
        const bool externallyControlled = isExternalControlStatus(controlStatus);
        // 所有普通行都显式写入深色背景，避免交替行回退到系统白色调色板。
        const QBrush normalBackground(row % 2 == 0 ? QColor(16, 24, 24)
                                                    : QColor(25, 39, 37));
        // QSS 统一负责黑色主题底色；这里保留画刷作为无 QSS 场景的安全回退。
        // 实际的上报强调由代理绘制青色细框，避免使用浅色背景破坏原始主题。
        const QBrush background = externallyControlled
            ? QBrush(QColor(0, 104, 88))
            : (type != PointType::Track ? QBrush(trackTypeColor(type).darker(170)) : normalBackground);
        // 深色主题下显式指定浅色文字；空画刷会回退到系统调色板，导致黑灰字不可读。
        const QBrush foreground = externallyControlled ? QBrush(QColor(240, 255, 251))
                                                         : QBrush(QColor(238, 245, 245));
        const QBrush heightForeground(QColor(90, 255, 185));
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = tableWidget->item(row, col);
            if (!item) {
                continue;
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setBackground(background);
            item->setForeground(col == TrackHeightColumn ? heightForeground : foreground);
            item->setData(kTrackExternalReportingRole, externallyControlled);
            item->setData(kTrackReportingBoundaryRole, false);
            QFont font = item->font();
            font.setBold(externallyControlled || col == TrackHeightColumn);
            item->setFont(font);
        }
        return;
    }

    if (!tableWidget || row < 0 || row >= tableWidget->rowCount()) {
        return;
    }
    const bool externallyControlled = isExternalControlStatus(controlStatus);
    // 表格在全屏地图上使用半透明底；同时保留无 QSS 时的深色回退。
    const QBrush normalBackground(row % 2 == 0 ? (UiTheme::classicEnabled ? QColor(16, 24, 24) : UiTheme::color(QColor(18, 23, 28, 120)))
                                                : (UiTheme::classicEnabled ? QColor(25, 39, 37) : UiTheme::color(QColor(27, 34, 40, 140))));
    // QSS 统一负责黑色主题底色；这里保留画刷作为无 QSS 场景的安全回退。
    // 实际的上报强调由代理绘制青色细框，避免使用浅色背景破坏原始主题。
    QColor typeBackground = trackTypeColor(type).darker(170);
    typeBackground.setAlpha(140);
    const QBrush background = externallyControlled
        ? QBrush((UiTheme::classicEnabled ? QColor(0, 104, 88) : UiTheme::color(QColor(46, 75, 87, 160))))
        : (type != PointType::Track ? QBrush(typeBackground) : normalBackground);
    // 深色主题下显式指定浅色文字；空画刷会回退到系统调色板，导致黑灰字不可读。
    const QBrush foreground = externallyControlled ? QBrush((UiTheme::classicEnabled ? QColor(240, 255, 251) : QColor(UiTheme::Text)))
                                                     : QBrush((UiTheme::classicEnabled ? QColor(238, 245, 245) : QColor(UiTheme::Text)));
    const QBrush heightForeground{(UiTheme::classicEnabled ? QColor(90, 255, 185) : QColor(UiTheme::Normal))};
    for (int col = 0; col < tableWidget->columnCount(); ++col) {
        QTableWidgetItem* item = tableWidget->item(row, col);
        if (!item) {
            continue;
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setBackground(background);
        item->setForeground(col == TrackHeightColumn ? heightForeground : foreground);
        item->setData(kTrackExternalReportingRole, externallyControlled);
        item->setData(kTrackReportingBoundaryRole, false);
        QFont font = item->font();
        font.setBold(externallyControlled || col == TrackHeightColumn);
        item->setFont(font);
    }
}

void MainOverLayOut::refreshExternalReportingIndicators()
{
    const std::array<QTableWidget*, 4> tables = {
        ui ? ui->tableWidget : nullptr, ui ? ui->droneTableWidget : nullptr,
        m_tbdTrackTable, m_cooperativeTrackTable
    };
    for (QTableWidget* tableWidget : tables) {
        if (!tableWidget) {
            continue;
        }
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            QTableWidgetItem* idItem = tableWidget->item(row, TrackIdColumn);
            if (!idItem) {
                continue;
            }
            PointInfo info;
            info.batch = idItem->text().toUInt();
            info.type = idItem->data(Qt::UserRole).toUInt();
            const QString status = externalControlStatus(info);
            applyTrackRowAppearance(tableWidget, row, info.type, status);
        }
        sortTrackTable(tableWidget);
    }
    syncFrozenTrackTables();
}

QString MainOverLayOut::getTargetTypeText(int targetType) const {
    return TargetRecognition::text(static_cast<unsigned>(targetType));
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
void MainOverLayOut::bindHealthPanel(int panelId)
{
    if (UiTheme::classicEnabled) { bindClassicHealthPanel(panelId); return; }
    if (panelId < 0 || panelId >= kRadarPanelCount) {
        return;
    }

    const HealthPanelWidgets& widgets = m_healthPanelWidgets[panelId];
    m_activeHealthPanel = panelId;
    m_btnTxOpen = widgets.txOpen;
    m_btnDutyCycle = widgets.dutyCycle;
    m_btnPulseWidth = widgets.pulseWidth;
    m_btnRxOpen = widgets.rxOpen;
    m_btnFreqSrc = widgets.freqSrc;
    m_btnDigBoard = widgets.digBoard;
    m_btnServo = widgets.servo;
    m_btnBeidou = widgets.beidou;
    m_btnBluetooth = widgets.bluetooth;
    m_btnPowerBoard = widgets.powerBoard;
    m_lastBITReport = m_bitReports[panelId];
}



void MainOverLayOut::scheduleHealthWindowRefresh()
{
    m_healthUiDirty = true;
}

void MainOverLayOut::updateHealthWindow()
{
    if (UiTheme::classicEnabled) { updateClassicHealthWindow(); return; }
    // Periodic refresh comes only from Controller's complete 5-second window.
    // 如果窗口不存在或不可见，直接返回
    if (!m_healthWindow || !m_healthWindow->isVisible()) {
        return;
    }
    m_healthUiDirty = false;

    for (int index = 0; index < int(m_networkHealthText.size()); ++index) {
        applyNetworkHealthStatus(index);
    }
    if (m_opticalHealthButton) {
        m_opticalHealthButton->setText(m_opticalStatus);
        setStyleSheetIfChanged(m_opticalHealthButton, m_opticalLinkOk
            ? UiTheme::statusStyle(UiTheme::Status::Normal)
            : UiTheme::statusStyle(UiTheme::Status::Fault));
    }
    if (m_zhoushanHealthButton) {
        m_zhoushanHealthButton->setText(m_zhoushanStatus);
        setStyleSheetIfChanged(m_zhoushanHealthButton, m_zhoushanLinkOk
            ? UiTheme::statusStyle(UiTheme::Status::Normal)
            : UiTheme::statusStyle(UiTheme::Status::Fault));
    }
    if (m_electromagneticHealthButton) {
        const QString text = "电磁设备：" + m_electromagneticStatus;
        const QString style = m_electromagneticLinkOk
            ? UiTheme::statusStyle(UiTheme::Status::Normal)
            : UiTheme::statusStyle(UiTheme::Status::Fault);
        if (m_electromagneticHealthButton->text() != text)
            m_electromagneticHealthButton->setText(text);
        setStyleSheetIfChanged(m_electromagneticHealthButton, style);
    }

    using Status = UiTheme::Status;
    QStringList faults;
    auto software = [&faults, this](HealthUi::Field* field, const QString& name, int code) {
        if (!field) return;
        Status state = Status::Unknown;
        QString text = QStringLiteral("未上报");
        if (m_softwareStatusReceived) {
            switch (code) {
            case 0: state = Status::Normal; text = QStringLiteral("正常"); break;
            case 1: state = Status::Fault; text = QStringLiteral("运行异常"); break;
            case 2: state = Status::Normal; text = QStringLiteral("启动成功"); break;
            case 3: state = Status::Fault; text = QStringLiteral("启动失败"); break;
            case 4: state = Status::Inactive; text = QStringLiteral("已关闭"); break;
            case 5: state = Status::Fault; text = QStringLiteral("关闭失败"); break;
            default: state = Status::Fault; text = QStringLiteral("状态码异常（%1）").arg(code); break;
            }
        }
        field->setStatus(state, text);
        if (state == Status::Fault) faults.append(name + QStringLiteral(" · ") + text);
    };
    software(m_sigProBtn, QStringLiteral("信号处理"), m_sigProSta);
    software(m_dataProBtn, QStringLiteral("数据处理"), m_dataProSta);
    software(m_beamConBtn, QStringLiteral("波束调度"), m_beamConSta);
    software(m_targetRecBtn, QStringLiteral("目标识别"), m_targetRecSta);

    const bool hasPanelSnapshot = m_bitReportTimes[m_activeHealthPanel].isValid();
    const unsigned char bitGroup = m_lastBITReport.bitGroup;
    const unsigned char powerState = m_lastBITReport.powerState;
    auto bit = [hasPanelSnapshot, &faults](HealthUi::Field* field, const QString& name,
                                          bool normal, bool switchOnly = false) {
        if (!field) return;
        if (!hasPanelSnapshot) {
            field->setStatus(Status::Unknown, QStringLiteral("未上报"));
        } else if (switchOnly) {
            field->setStatus(normal ? Status::Normal : Status::Inactive,
                             normal ? QStringLiteral("开启") : QStringLiteral("关闭"));
        } else {
            field->setStatus(normal ? Status::Normal : Status::Fault,
                             normal ? QStringLiteral("正常") : QStringLiteral("! 异常"));
            if (!normal) faults.append(name + QStringLiteral("异常"));
        }
    };
    bit(m_btnTxOpen, QStringLiteral("阵面发射"), bitGroup & 0x80, true);
    bit(m_btnDutyCycle, QStringLiteral("占空比"), !(bitGroup & 0x40));
    bit(m_btnPulseWidth, QStringLiteral("脉宽"), !(bitGroup & 0x20));
    bit(m_btnRxOpen, QStringLiteral("阵面接收"), bitGroup & 0x10, true);
    bit(m_btnFreqSrc, QStringLiteral("频率源"), bitGroup & 0x08);
    bit(m_btnDigBoard, QStringLiteral("收发板"), bitGroup & 0x04);
    bit(m_btnServo, QStringLiteral("伺服"), bitGroup & 0x02);
    bit(m_btnBeidou, QStringLiteral("北斗"), bitGroup & 0x01);
    bit(m_btnBluetooth, QStringLiteral("蓝牙"), powerState & 0x02);
    bit(m_btnPowerBoard, QStringLiteral("波控板电源"), powerState & 0x01);

    const SubArrayPowerVisualState& visualState = m_subArrayPowerVisualStates[m_activeHealthPanel];
    HealthPanelWidgets& widgets = m_healthPanelWidgets[m_activeHealthPanel];
    int normalCount = 0;
    int faultyCount = 0;
    for (int powerIndex = 0; powerIndex < 36; ++powerIndex) {
        if (widgets.subArrayPowerCells[powerIndex]) {
            const Status state = !visualState.hasConfirmedStatus ? Status::Unknown
                : visualState.normal[powerIndex] ? Status::Normal : Status::Fault;
            widgets.subArrayPowerCells[powerIndex]->setStatus(state);
        }
        if (visualState.hasConfirmedStatus) {
            if (visualState.normal[powerIndex]) ++normalCount;
            else ++faultyCount;
        }
    }
    if (widgets.powerCountLabel) {
        widgets.powerCountLabel->setText(visualState.hasConfirmedStatus
            ? QStringLiteral("%1 / 36 正常").arg(normalCount) : QStringLiteral("36 路 · 待上报"));
        setStyleSheetIfChanged(widgets.powerCountLabel, visualState.hasConfirmedStatus ? QString()
            : QStringLiteral("color:%1;").arg(HealthUi::Unreported));
    }
    if (widgets.subArrayPowerSummaryLabel) {
        widgets.subArrayPowerSummaryLabel->setText(visualState.hasConfirmedStatus
            ? QStringLiteral("正常 %1 · 故障 %2\n5秒窗口 %3 帧 · 故障率 >90% 为故障")
                  .arg(normalCount).arg(faultyCount).arg(visualState.sampleCount)
            : QStringLiteral("尚未收到子阵电源 BIT 上报"));
        setStyleSheetIfChanged(widgets.subArrayPowerSummaryLabel, visualState.hasConfirmedStatus ? QString()
            : QStringLiteral("color:%1;").arg(HealthUi::Unreported));
    }
    if (faultyCount) faults.append(QStringLiteral("子阵电源 %1 路故障").arg(faultyCount));

    auto measurement = [hasPanelSnapshot](HealthUi::Field* field, const QString& value) {
        if (field) field->setStatus(hasPanelSnapshot ? Status::Inactive : Status::Unknown,
                                  hasPanelSnapshot ? value : QStringLiteral("未上报"), false);
    };
    measurement(widgets.panelTemp, QStringLiteral("%1 °C").arg(m_lastBITReport.panelTemp * 0.1, 0, 'f', 1));
    measurement(widgets.fpgaTemp, QStringLiteral("%1 °C").arg(m_lastBITReport.fpgaTemp * 0.1, 0, 'f', 1));
    measurement(widgets.yaw, QStringLiteral("%1°").arg(m_lastBITReport.yaw * 0.01, 0, 'f', 2));
    const QString updated = hasPanelSnapshot
        ? m_bitReportTimes[m_activeHealthPanel].toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) : QStringLiteral("—");
    if (m_healthFooterTime) m_healthFooterTime->setText(QStringLiteral("状态汇总时间　%1").arg(updated));
    if (m_healthFooterPanel) m_healthFooterPanel->setText(QStringLiteral("阵面 %1").arg(m_activeHealthPanel + 1));

    const std::array<HealthUi::Field*, 8> networkFields{{m_radarLocalNetworkBtn, m_laserLocalNetworkBtn,
        m_radarPeerNetworkBtn, m_laserPeerNetworkBtn, m_opticalLocalNetworkBtn, m_opticalPeerNetworkBtn,
        m_zhoushanLocalNetworkBtn, m_zhoushanPeerNetworkBtn}};
    int networkFaults = 0;
    for (std::size_t index = 0; index < networkFields.size(); ++index)
        if (networkFields[index] && m_networkHealthKnown[index] && !m_networkHealthOk[index]) ++networkFaults;
    if (networkFaults) faults.append(QStringLiteral("网络 %1 项异常").arg(networkFaults));
    if (m_opticalHealthButton && !m_opticalLinkOk) faults.append(QStringLiteral("光电状态异常"));
    if (m_zhoushanHealthButton && !m_zhoushanLinkOk) faults.append(QStringLiteral("光电目指状态异常"));
    if (m_electromagneticHealthButton && !m_electromagneticLinkOk) faults.append(QStringLiteral("电磁设备状态异常"));
    if (m_healthAlert) {
        const bool fault = !faults.isEmpty();
        const bool pending = !hasPanelSnapshot || !m_softwareStatusReceived || !visualState.hasConfirmedStatus;
        const QString text = fault ? QStringLiteral("!  %1").arg(faults.join(QStringLiteral("　/　")))
            : pending
                ? QStringLiteral("—  等待健康状态上报") : QStringLiteral("●  当前软件及阵面快照无异常");
        if (m_healthAlert->text() != text) m_healthAlert->setText(text);
        setStyleSheetIfChanged(m_healthAlert, QStringLiteral(
            "QLabel {background:%1;color:%2;border:none;border-left:3px solid %2;"
            "padding:14px 18px;font-size:16px;}")
            .arg(fault ? UiTheme::FaultSurface : UiTheme::Surface,
                 fault ? UiTheme::Fault : pending ? HealthUi::Unreported : UiTheme::Normal));
    }
}

void MainOverLayOut::updateNetworkHealthStatus(int index, bool ok, const QString& text)
{
    if (index < 0 || index >= static_cast<int>(m_networkHealthText.size())) {
        return;
    }
    m_networkHealthKnown[static_cast<size_t>(index)] = true;
    m_networkHealthOk[static_cast<size_t>(index)] = ok;
    m_networkHealthText[static_cast<size_t>(index)] = text;
    scheduleHealthWindowRefresh();
}

void MainOverLayOut::applyNetworkHealthStatus(int index)
{
    if (UiTheme::classicEnabled) { applyClassicNetworkHealthStatus(index); return; }
    HealthUi::Field* button = nullptr;
    switch (index) {
    case SubsystemNetworkMonitor::RadarLocal: button = m_radarLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::LaserLocal: button = m_laserLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::RadarPeer: button = m_radarPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::LaserPeer: button = m_laserPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::OpticalLocal: button = m_opticalLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::OpticalPeer: button = m_opticalPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::ZhoushanLocal: button = m_zhoushanLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::ZhoushanPeer: button = m_zhoushanPeerNetworkBtn; break;
    default: return;
    }
    if (!button) {
        return;
    }

    const bool known = m_networkHealthKnown[static_cast<size_t>(index)];
    const bool ok = m_networkHealthOk[static_cast<size_t>(index)];
    button->setStatus(!known ? UiTheme::Status::Unknown : ok ? UiTheme::Status::Normal : UiTheme::Status::Fault,
                      !known ? QStringLiteral("检查中") : ok ? QStringLiteral("正常") : QStringLiteral("异常"));
    button->setToolTip(m_networkHealthText[static_cast<size_t>(index)]);
    button->setAccessibleDescription(m_networkHealthText[static_cast<size_t>(index)]);
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
        resetTransmitState(QStringLiteral("手动关闭处理软件"));

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
    CusWindow* cusWin = new CusWindow(tr("TWS模式设置"), UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
    CusWindow* cusWin = new CusWindow(tr("TAS模式设置"), UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
        const QString details = TasControl::scanDetails(param);
        logCommand("TAS - 工作模式下发请求", details);
        LOG_INFO(QStringLiteral("[TAS][SUBMIT] %1").arg(details));
        // 发送工作模式配置到数据处理端
        emit sig_SetScanRangeParam(param);
    });

    connect(tasDialog, &TASModeDialog::setBeamControl, this, [this](const BeamControl param) {
        m_beamControl = param;  // 保存到内存
        const QString details = TasControl::beamDetails(param);
        logCommand("TAS - 波形采样下发请求", details);
        LOG_INFO(QStringLiteral("[TAS][SUBMIT] %1").arg(details));
        // 发送波形及采样控制配置
        emit sig_SetBeamControlParam(param);
    });

    connect(tasDialog, &TASModeDialog::setServoControl, this, [this](const ServoControlParam param) {
        m_servoControlParam = param;  // 保存到内存
        const QString details = TasControl::servoDetails(param);
        logCommand("TAS - 伺服下发请求", details);
        LOG_INFO(QStringLiteral("[TAS][SUBMIT] %1").arg(details));
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
    LOG_INFO("[UI_WINDOW][DataStorage] open entered");
    // 如果窗口已经存在，直接显示并激活
    if (m_dataStorageWindow && m_dataStorageDialog) {
        LOG_INFO("[UI_WINDOW][DataStorage] reuse; before show");
        m_dataStorageWindow->show();
        m_dataStorageWindow->raise();
        m_dataStorageWindow->activateWindow();
        LOG_INFO("[UI_WINDOW][DataStorage] shown");
        return;
    }

    // 创建新窗口
    m_dataStorageWindow =
        new CusWindow("数据存储管理", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
    // CusWindow 已启用 WA_DeleteOnClose，关闭时由 destroyed 回调清空持有指针。

    m_dataStorageDialog = new DataSaveUI(m_dataStorageWindow);
    m_dataStorageDialog->setWindowFlags(Qt::Widget);
    m_dataStorageWindow->setContentWidget(m_dataStorageDialog);
    LOG_INFO("[UI_WINDOW][DataStorage] content ready");

    // 连接数据存储参数发送
    connect(m_dataStorageDialog, &DataSaveUI::setParam, this, [this](DataSet param) {
        QString action;
        if (param.ifsave) {
            action = QString("数据存储 - ID:%1, 开关:%2, 确认波束:%3, 跟踪波束:%4, 搜索波束:%5, 检测点迹:%6")
                .arg(param.save.dataID)
                .arg(param.save.saveSwitch ? "开启" : "关闭")
                .arg(param.save.confirmBeam).arg(param.save.trackBeam)
                .arg(param.save.searchBeam).arg(param.save.detection);
            LOG_INFO("[CC01][SUBMIT][16B] " + action);
        } else if (param.ifdel) {
            action = QString("数据删除 - ID:%1").arg(param.del.dataID);
        } else if (param.ifoffline) {
            action = QString("离线处理 - ID:%1, 模式:%2")
                .arg(param.off.dataID)
                .arg(param.off.onSwitch ? "离线" : "正常");
            if (param.extendedOffline) {
                QStringList ids;
                for (int id = 0; id < 4; ++id) if (param.off.replayPanel[id]) ids << QString::number(id);
                action += QStringLiteral(", 回放阵面ID:[%1]").arg(ids.join(','));
            }
            LOG_INFO(QStringLiteral("[CC03][SUBMIT][%1B] ").arg(param.offlinePayloadSize()) + action);
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
        LOG_INFO("[UI_WINDOW][DataStorage] destroyed");
    });

    LOG_INFO("[UI_WINDOW][DataStorage] before show");
    m_dataStorageWindow->show();
    LOG_INFO("[UI_WINDOW][DataStorage] shown");
}

// ============ 参数设置槽函数实现 ============

/**
 * @brief 打开数据处理参数对话框
 * @details 配置数据处理相关参数
 */
void MainOverLayOut::onDataProcessClicked() {
    CusWindow* window =
        new CusWindow("数据处理参数", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
        new CusWindow("信号处理参数", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
    if (!AuthManager::instance().isAdminMode()) {
        LOG_WARNING("[UI_MODE] blocked developer-only waveform window in user mode");
        return;
    }
    // 创建自定义窗口，使用雷达图标
    CusWindow* window =
        new CusWindow("波形及采样控制", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
   * @brief 打开阵面开启控制对话框
   * @details 配置四个阵面的开启状态
   */
void MainOverLayOut::onBatteryControlClicked() {
    // 创建自定义窗口，使用雷达图标
    CusWindow* window =
        new CusWindow("阵面开启控制", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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

    // 记录四个阵面的实际下发状态
    connect(dialog, &BatteryControl::setParam, this, [this](const BatteryControlM param) {
        logCommand("阵面开启控制",
                   QString("阵面1=%1, 阵面2=%2, 阵面3=%3, 阵面4=%4")
                       .arg(param.quadrant1 ? QStringLiteral("开") : QStringLiteral("关"))
                       .arg(param.quadrant2 ? QStringLiteral("开") : QStringLiteral("关"))
                       .arg(param.quadrant3 ? QStringLiteral("开") : QStringLiteral("关"))
                       .arg(param.quadrant4 ? QStringLiteral("开") : QStringLiteral("关")));
    });

    window->show();
}

void MainOverLayOut::onServoControlClicked() {
    CusWindow* window = new CusWindow("伺服控制", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
        new CusWindow("工作模式控制", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
        new CusWindow("光电系统控制", UiTheme::themedIcon(QStringLiteral(":/resources/icon/radararray.png"), QStringLiteral(":/resources/icon/ui/radar.svg")), this);
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
    m_softwareStatusReceived = true; // 展示用：不把开机默认故障值当成已收到的快照。
    m_sigProSta = res.sigProSta;
    m_dataProSta = res.dataProSta;
    m_beamConSta = res.beamConSta;
    m_targetRecSta = res.targetRecSta;
    // This is already a 5-second snapshot; failed startup/shutdown are faults too.
    m_systemNormal = !HealthStateWindow::softwareFault(res.sigProSta) && !HealthStateWindow::softwareFault(res.dataProSta)
        && !HealthStateWindow::softwareFault(res.beamConSta) && !HealthStateWindow::softwareFault(res.targetRecSta);
    emit softwareStatusChanged(currentSoftwareStatus());
    m_softwareUiPending = true;
    scheduleHealthWindowRefresh();
}

void MainOverLayOut::flushStatusUi()
{
    // Called once after Controller has delivered the complete 5-second window.
    if (m_softwareUiPending) {
        setStyleSheetIfChanged(ui->radarsystem, m_systemNormal ? QString() : QString(
            (UiTheme::classicEnabled ? "QToolButton#radarsystem { background-color:rgba(255,0,0,0.3); border:2px solid #cc0000; border-radius:5px; }"
            "QToolButton#radarsystem:hover { background-color:rgba(255,0,0,0.5); border:2px solid #ff0000; }" : "QToolButton#radarsystem { background-color:rgba(53,37,45,190); color:#ea8c93; border:1px solid #d46574; border-radius:5px; }"
            "QToolButton#radarsystem:hover { background-color:rgba(67,41,50,220); border:1px solid #ea8c93; }")));
        m_softwareUiPending = false;
    }
    if (m_servoUiPending) {
        const auto& res = m_latestServoUi;
        if (!m_servoFaultLogClock.isValid()) m_servoFaultLogClock.start();
        // Do not use changing feedback angles in the key: drift is not a new failure.
        if (m_servoFaultLogGate.sample(0, quint64(res.cmd) + 1, m_servoFaultLogClock.elapsed())) {
            const QString text = QStringLiteral("5秒窗口 伺服失败 cmd=%1 speed=%2 az=%3°")
                .arg(servoCommandText(res.cmd)).arg(res.speed).arg(res.azCur * 0.01, 0, 'f', 2);
            logCommand(QStringLiteral("健康异常"), text);
            LOG_WARNING("[HEALTH][SERVO_FAULT] " + text);
        }
        m_servoUiPending = false;
    }
}

MonitorParam MainOverLayOut::currentSoftwareStatus() const
{
    MonitorParam status;
    status.dataProSta = static_cast<unsigned char>(m_dataProSta);
    status.beamConSta = static_cast<unsigned char>(m_beamConSta);
    status.sigProSta = static_cast<unsigned char>(m_sigProSta);
    status.targetRecSta = static_cast<unsigned char>(m_targetRecSta);
    return status;
}

void MainOverLayOut::onServoCtrlRet(ServoCtrlRet res) {
    if (res.result == 0) {
        m_latestServoUi = res;
        m_servoUiPending = true;
    }
}

void MainOverLayOut::onBITReport(BITReport res, bool powerConfirmed, quint32 powerSampleCount) {
    if (res.radarId < RADAR_ID_MIN || res.radarId > RADAR_ID_MAX) return;
    const int panelId = static_cast<int>(res.radarId);
    m_bitReports[panelId] = res;
    m_bitReportTimes[panelId] = QDateTime::currentDateTime();
    auto& power = m_subArrayPowerVisualStates[panelId];
    power.hasConfirmedStatus = powerConfirmed;
    power.sampleCount = powerSampleCount;
    for (int i = 0; i < 36; ++i) power.normal[i] = (res.subArrayPower[i / 8] & (1U << (i % 8))) != 0;
    if (panelId == 0) emit electromagneticPanelStatusChanged(currentElectromagneticPanelStatus());
    if (panelId == m_activeHealthPanel) m_lastBITReport = res;
    scheduleHealthWindowRefresh();
    // Health page refresh is driven only after the complete 5-second window is delivered.
}

BITReport MainOverLayOut::currentElectromagneticPanelStatus() const
{
    const auto& state = m_subArrayPowerVisualStates[0];
    return ElectromagneticProtocol::withConfirmedSubArrayPower(m_bitReports[0], state.normal, state.hasConfirmedStatus);
}

/**
 * @brief 打开雷达系统健康管理对话框
 * @details 显示信号处理、数据处理、波束调度、目标识别四个子系统的运行状态，实时更新
 */
void MainOverLayOut::onRadarSystemClicked() {
    if (UiTheme::classicEnabled) { createClassicHealthWindow(); return; }
    LOG_INFO("[UI_WINDOW][Health] open entered");
    // 如果窗口已存在，则直接显示
    if (m_healthWindow) {
        LOG_INFO("[UI_WINDOW][Health] reuse; before show");
        m_healthWindow->show();
        m_healthWindow->raise();
        m_healthWindow->activateWindow();
        LOG_INFO("[UI_WINDOW][Health] shown");
        updateHealthWindow();
        return;
    }

    // 创建自定义窗口
    m_healthWindow =
        new CusWindow("雷达系统健康管理", QIcon(":/resources/icon/ui/radar.svg"), this);
    m_healthWindow->setAttribute(Qt::WA_DeleteOnClose);
    // 默认按完整展开内容开窗；受屏幕限制时才使用滚动，不缩小文字或主屏 PPI。
    QScreen* healthScreen = QGuiApplication::screenAt(mapToGlobal(rect().center()));
    if (!healthScreen) healthScreen = QGuiApplication::primaryScreen();
    const QRect available = healthScreen ? healthScreen->availableGeometry() : QRect(0, 0, 1280, 800);
    const bool compactHealth = available.height() < ScaleHelper::uiScaled(1200);
    const QSize initialSize(qMin(ScaleHelper::uiScaled(1600), available.width()),
                            qMin(ScaleHelper::uiScaled(1360), available.height()));
    m_healthWindow->setMinimumSize(qMin(640, initialSize.width()), qMin(480, initialSize.height()));

    // 连接窗口关闭信号，清空指针
    connect(m_healthWindow, &QObject::destroyed, this, [this]() {
        m_healthWindow = nullptr;
        m_healthTabs = nullptr;
        m_activeHealthPanel = 0;
        m_healthAlert = nullptr;
        m_healthFooterTime = nullptr;
        m_healthFooterPanel = nullptr;
        m_sigProBtn = nullptr;
        m_dataProBtn = nullptr;
        m_beamConBtn = nullptr;
        m_targetRecBtn = nullptr;
        m_radarLocalNetworkBtn = nullptr;
        m_radarPeerNetworkBtn = nullptr;
        m_laserLocalNetworkBtn = nullptr;
        m_laserPeerNetworkBtn = nullptr;
        m_opticalLocalNetworkBtn = nullptr;
        m_opticalPeerNetworkBtn = nullptr;
        m_zhoushanLocalNetworkBtn = nullptr;
        m_zhoushanPeerNetworkBtn = nullptr;
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
        for (auto& widgets : m_healthPanelWidgets) {
            widgets = HealthPanelWidgets{};
        }
        LOG_INFO("[UI_WINDOW][Health] destroyed");
    });

    // 设计稿直接作为窗口内容，不再套一层限宽画布和外部标题栏。
    auto* healthScroll = new QScrollArea(m_healthWindow);
    healthScroll->setObjectName(QStringLiteral("HealthScrollArea"));
    healthScroll->viewport()->setObjectName(QStringLiteral("HealthViewport"));
    healthScroll->setFrameShape(QFrame::NoFrame);
    healthScroll->setWidgetResizable(true);
    auto* contentWidget = new QWidget;
    contentWidget->setObjectName(QStringLiteral("HealthContent"));
    contentWidget->setAttribute(Qt::WA_StyledBackground, true);
    contentWidget->setMinimumWidth(ScaleHelper::uiScaled(820));
    auto* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    HealthUi::applyStyle(contentWidget, QStringLiteral(
        "QWidget#HealthContent {background:#12171c;border:none;}"
        "QFrame#HealthFooter {background:#1b2228;border:none;}"
        "QLabel {background:transparent;border:none;color:#a6b2bd;font-size:15px;}"
        "QLabel[healthHeading=\"true\"] {color:#e8edf1;font-size:18px;font-weight:bold;}"
        "QTabWidget::pane {border:none;background:#12171c;}"
        "QTabBar::tab {font-size:15px;min-width:%1px;padding:8px 16px;margin:0 8px 0 0;}")
        .arg(ScaleHelper::uiScaled(76)));
    UiTheme::setStyle(healthScroll, [=] { return QStringLiteral(
        "QScrollArea#HealthScrollArea, QWidget#HealthViewport {background:#12171c;border:none;}"); });
    auto* header = new QFrame(m_healthWindow);
    header->setObjectName(QStringLiteral("HealthHeader"));
    HealthUi::applyStyle(header, QStringLiteral(
        "QFrame#HealthHeader {background:#1b2228;border:none;border-bottom:1px solid #303b44;}"
        "QLabel {background:transparent;border:none;color:#a6b2bd;font-size:15px;}"));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(ScaleHelper::uiScaled(28), ScaleHelper::uiScaled(compactHealth ? 16 : 22),
                                    ScaleHelper::uiScaled(28), ScaleHelper::uiScaled(compactHealth ? 16 : 22));
    headerLayout->setSpacing(ScaleHelper::uiScaled(16));
    auto* activity = new QLabel(header);
    activity->setPixmap(QIcon(QStringLiteral(":/resources/icon/ui/activity.svg"))
                            .pixmap(ScaleHelper::uiScaled(28), ScaleHelper::uiScaled(28)));
    headerLayout->addWidget(activity, 0, Qt::AlignVCenter);
    auto* headerText = new QVBoxLayout;
    headerText->setSpacing(ScaleHelper::uiScaled(5));
    auto* title = new QLabel(QStringLiteral("雷达系统健康管理"), header);
    HealthUi::applyStyle(title, QStringLiteral("QLabel {color:#e8edf1;font-size:26px;font-weight:bold;}"));
    headerText->addWidget(title);
    headerText->addWidget(new QLabel(QStringLiteral("设备运行与阵面状态"), header));
    headerLayout->addLayout(headerText, 1);
    m_healthWindow->setCustomTitleBar(header);

    auto* body = new QWidget(contentWidget);
    auto* bodyLayout = new QVBoxLayout(body);
    const int margin = ScaleHelper::uiScaled(compactHealth ? 20 : 28);
    bodyLayout->setContentsMargins(margin, margin, margin, margin);
    bodyLayout->setSpacing(ScaleHelper::uiScaled(compactHealth ? 12 : 22));
    m_healthAlert = new QLabel(body);
    m_healthAlert->setWordWrap(true);
    bodyLayout->addWidget(m_healthAlert);
    auto* softwareHeading = new QHBoxLayout;
    auto* softwareLabel = new QLabel(QStringLiteral("软件运行"), body);
    softwareLabel->setProperty("healthHeading", true);
    softwareHeading->addWidget(softwareLabel);
    softwareHeading->addStretch();
    softwareHeading->addWidget(new QLabel(QStringLiteral("4 个模块"), body));
    auto* softwareSection = new QVBoxLayout;
    softwareSection->setSpacing(ScaleHelper::uiScaled(12));
    softwareSection->addLayout(softwareHeading);
    m_sigProBtn = new HealthUi::Field(QStringLiteral("信号处理"), HealthUi::Field::Layout::Card, body, compactHealth);
    m_dataProBtn = new HealthUi::Field(QStringLiteral("数据处理"), HealthUi::Field::Layout::Card, body, compactHealth);
    m_beamConBtn = new HealthUi::Field(QStringLiteral("波束调度"), HealthUi::Field::Layout::Card, body, compactHealth);
    m_targetRecBtn = new HealthUi::Field(QStringLiteral("目标识别"), HealthUi::Field::Layout::Card, body, compactHealth);
    auto* softwareRow = new QHBoxLayout;
    softwareRow->setSpacing(ScaleHelper::uiScaled(12));
    for (auto* field : {m_sigProBtn, m_dataProBtn, m_beamConBtn, m_targetRecBtn}) softwareRow->addWidget(field, 1);
    softwareSection->addLayout(softwareRow);
    bodyLayout->addLayout(softwareSection);

    // 网络用轻量文字状态行；完整地址与诊断信息保留在工具提示和无障碍描述。
    auto* network = new QFrame(body);
    network->setObjectName(QStringLiteral("HealthNetwork"));
    UiTheme::setStyle(network, [=] { return QStringLiteral("QFrame#HealthNetwork {border:none;border-top:1px solid #303b44;border-bottom:1px solid #303b44;background:transparent;}"); });
    auto* networkGrid = new QGridLayout(network);
    networkGrid->setContentsMargins(0, ScaleHelper::uiScaled(compactHealth ? 8 : 10),
                                   0, ScaleHelper::uiScaled(compactHealth ? 8 : 10));
    networkGrid->setHorizontalSpacing(ScaleHelper::uiScaled(28));
    auto addNetwork = [network, networkGrid, compactHealth](const QString& name, int row, int col) {
        auto* field = new HealthUi::Field(name, HealthUi::Field::Layout::Inline, network, compactHealth);
        networkGrid->addWidget(field, row, col);
        return field;
    };
    m_radarLocalNetworkBtn = addNetwork(QStringLiteral("本机网络"), 0, 0);
    m_radarPeerNetworkBtn = addNetwork(QStringLiteral("阵面连接"), 0, 1);
    if (CF_INS.laserReportEnabled(true)) {
        m_laserLocalNetworkBtn = addNetwork(QStringLiteral("本机激光网络"), 1, 0);
        m_laserPeerNetworkBtn = addNetwork(QStringLiteral("激光连接"), 1, 1);
    }
    bodyLayout->addWidget(network);
    if (m_opticalV2) {
        m_opticalLocalNetworkBtn = addNetwork(QStringLiteral("本机光电网络"), 2, 0);
        m_opticalPeerNetworkBtn = addNetwork(QStringLiteral("光电连接"), 2, 1);
        m_opticalHealthButton = new QPushButton(m_opticalStatus, body);
        connect(m_opticalHealthButton.data(), &QPushButton::clicked, this, [this] { m_opticalV2->showWindow(this); });
        bodyLayout->addWidget(m_opticalHealthButton);
    }
    if (m_zhoushanOptical) {
        m_zhoushanLocalNetworkBtn = addNetwork(QStringLiteral("本机光电目指网络"), 3, 0);
        m_zhoushanPeerNetworkBtn = addNetwork(QStringLiteral("光电目指连接"), 3, 1);
        m_zhoushanHealthButton = new QPushButton(m_zhoushanStatus, body);
        connect(m_zhoushanHealthButton.data(), &QPushButton::clicked, this, [this] { m_zhoushanOptical->showWindow(this); });
        bodyLayout->addWidget(m_zhoushanHealthButton);
    }
    if (m_electromagneticModule && m_electromagneticModule->isUiEnabled()) {
        m_electromagneticHealthButton = new QPushButton(body);
        connect(m_electromagneticHealthButton.data(), &QPushButton::clicked, this, [this] { m_electromagneticModule->showWindow(this); });
        bodyLayout->addWidget(m_electromagneticHealthButton);
    }
    for (int index = 0; index < int(m_networkHealthText.size()); ++index) applyNetworkHealthStatus(index);

    m_healthTabs = new QTabWidget(body);
    m_healthTabs->setDocumentMode(true);
    m_healthTabs->tabBar()->setDrawBase(false);
    m_healthTabs->tabBar()->setExpanding(false);
    m_healthTabs->tabBar()->setElideMode(Qt::ElideNone);
    for (int panelId = 0; panelId < kRadarPanelCount; ++panelId) {
        auto* panelPage = new QWidget(m_healthTabs);
        panelPage->setProperty("healthPanelPage", true);
        panelPage->setAttribute(Qt::WA_StyledBackground, true);
        auto* panelLayout = new QHBoxLayout(panelPage);
        panelLayout->setContentsMargins(0, ScaleHelper::uiScaled(18), 0, 0);
        panelLayout->setSpacing(ScaleHelper::uiScaled(28));
        auto* details = new QVBoxLayout;
        details->setSpacing(0);
        auto* panelTitle = new QLabel(QStringLiteral("阵面 %1 · 状态摘要").arg(panelId + 1), panelPage);
        panelTitle->setProperty("healthHeading", true);
        details->addWidget(panelTitle);
        details->addSpacing(ScaleHelper::uiScaled(14));
        auto row = [details, panelPage, compactHealth](const QString& name) {
            auto* field = new HealthUi::Field(name, HealthUi::Field::Layout::Row, panelPage, compactHealth);
            details->addWidget(field);
            return field;
        };
        HealthPanelWidgets& widgets = m_healthPanelWidgets[panelId];
        widgets.dutyCycle = row(QStringLiteral("占空比"));
        widgets.freqSrc = row(QStringLiteral("频率源"));
        widgets.digBoard = row(QStringLiteral("收发板"));
        widgets.panelTemp = row(QStringLiteral("阵面温度"));
        widgets.fpgaTemp = row(QStringLiteral("FPGA 温度"));
        // 阵面状态始终完整列出，不再保留折叠入口或隐藏状态。
        widgets.txOpen = row(QStringLiteral("阵面发射"));
        widgets.rxOpen = row(QStringLiteral("阵面接收"));
        widgets.pulseWidth = row(QStringLiteral("脉宽"));
        widgets.servo = row(QStringLiteral("伺服"));
        widgets.beidou = row(QStringLiteral("北斗"));
        widgets.bluetooth = row(QStringLiteral("蓝牙"));
        widgets.powerBoard = row(QStringLiteral("波控板电源"));
        widgets.yaw = row(QStringLiteral("阵面偏航"));
        details->addStretch();
        panelLayout->addLayout(details, 5);

        auto* powerSection = new QVBoxLayout;
        powerSection->setSpacing(ScaleHelper::uiScaled(12));
        auto* powerHeading = new QHBoxLayout;
        auto* powerTitle = new QLabel(QStringLiteral("子阵电源"), panelPage);
        powerTitle->setProperty("healthHeading", true);
        powerHeading->addWidget(powerTitle);
        powerHeading->addStretch();
        widgets.powerCountLabel = new QLabel(panelPage);
        powerHeading->addWidget(widgets.powerCountLabel);
        powerSection->addLayout(powerHeading);
        auto* powerGrid = new QGridLayout;
        powerGrid->setSpacing(ScaleHelper::uiScaled(7));
        for (int powerIndex = 0; powerIndex < 36; ++powerIndex) {
            auto* powerCell = new HealthUi::PowerCell(powerIndex + 1, panelPage);
            const int row = powerIndex / 6;
            const int column = 5 - (powerIndex % 6);
            widgets.subArrayPowerCells[powerIndex] = powerCell;
            powerGrid->addWidget(powerCell, row, column);
        }
        powerSection->addLayout(powerGrid);
        powerSection->addWidget(new QLabel(QStringLiteral(
            "<span style='color:%1'>● 正常</span>　<span style='color:%2'>! 故障</span>　"
            "<span style='color:%3'>— 未上报</span>")
            .arg(UiTheme::Normal, UiTheme::Fault, HealthUi::Unreported), panelPage));
        widgets.subArrayPowerSummaryLabel = new QLabel(panelPage);
        widgets.subArrayPowerSummaryLabel->setWordWrap(true);
        HealthUi::applyStyle(widgets.subArrayPowerSummaryLabel,
            QStringLiteral("QLabel {color:#a6b2bd;font-size:13px;background:transparent;}"));
        powerSection->addWidget(widgets.subArrayPowerSummaryLabel);
        powerSection->addStretch();
        panelLayout->addLayout(powerSection, 7);
        m_bitReports[panelId].radarId = static_cast<unsigned char>(panelId);
        m_healthTabs->addTab(panelPage, QStringLiteral("阵面 %1").arg(panelId + 1));
    }
    bodyLayout->addWidget(m_healthTabs);
    connect(m_healthTabs, &QTabWidget::currentChanged, this, [this](int index) {
        bindHealthPanel(index);
        updateHealthWindow();
    });
    bindHealthPanel(0);
    mainLayout->addWidget(body, 1);
    auto* footer = new QFrame(contentWidget);
    footer->setObjectName(QStringLiteral("HealthFooter"));
    footer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(margin, ScaleHelper::uiScaled(compactHealth ? 12 : 16),
                                    margin, ScaleHelper::uiScaled(compactHealth ? 12 : 16));
    m_healthFooterTime = new QLabel(footer);
    m_healthFooterPanel = new QLabel(footer);
    HealthUi::applyStyle(m_healthFooterPanel, QStringLiteral("QLabel {color:#78cbd8;font-size:15px;}"));
    footerLayout->addWidget(m_healthFooterTime);
    footerLayout->addStretch();
    footerLayout->addWidget(m_healthFooterPanel);
    mainLayout->addWidget(footer);
    healthScroll->setWidget(contentWidget);
    m_healthWindow->setContentWidget(healthScroll);
    contentWidget->ensurePolished();
    mainLayout->activate();
    header->ensurePolished();
    headerLayout->activate();
    // 把内容与唯一标题区一起计入尺寸；外壳首次显示必须保留该几何值。
    const int requiredHeight = mainLayout->sizeHint().height() + header->sizeHint().height() + 8;
    const int openingHeight = qMin(available.height(), qMax(initialSize.height(), requiredHeight));
    const QSize openingSize(initialSize.width(), openingHeight);
    m_healthWindow->setInitialGeometry(QRect(
        available.center() - QPoint(openingSize.width() / 2, openingSize.height() / 2), openingSize));
    LOG_INFO("[UI_WINDOW][Health] content ready");

    LOG_INFO("[UI_WINDOW][Health] before show");
    m_healthWindow->show();
    LOG_INFO("[UI_WINDOW][Health] shown");

    // 首次更新显示（窗口显示后允许 updateHealthWindow 刷新缓存数据）
    updateHealthWindow();
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
    if (m_commandControlModule) {
        m_commandControlModule->setRadiationStatus(m_isTransmitting);
    }

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

void MainOverLayOut::onModeSwitchClicked()
{
    if (AuthManager::instance().isAdminMode()) {
        AuthManager::instance().setAdminMode(false);
        return;
    }
    DeveloperLoginDialog dialog(this);
    dialog.exec();
}

void MainOverLayOut::refreshModeControls()
{
    if (!m_modeSwitchButton) return;
    const bool developer = AuthManager::instance().isAdminMode();
    m_modeSwitchButton->setText(developer ? QStringLiteral("开发者模式") : QStringLiteral("用户模式"));
    m_modeSwitchButton->setToolTip(developer ? QStringLiteral("点击切换为用户模式") : QStringLiteral("点击验证并进入开发者模式"));
}

void MainOverLayOut::refreshTheme()
{
    UiTheme::applyClassicMainIcons(this);
    m_themeSwitchButton->setToolTip(UiTheme::classicEnabled ? QStringLiteral("当前经典主题，点击切换石墨主题")
                                                         : QStringLiteral("当前石墨主题，点击切换经典主题"));
    updateTransmitButton();
    updateStandbyButton();
    m_softwareUiPending = true;
    flushStatusUi();
    arrangeParamSettingsButtons();
    for (auto* table : {ui->tableWidget, ui->droneTableWidget, m_tbdTrackTable, m_cooperativeTrackTable}) {
        if (!table) continue;
        for (int row = 0; row < table->rowCount(); ++row) {
            auto* id = table->item(row, TrackIdColumn);
            if (!id) continue;
            const QVariant boundary = id->data(kTrackReportingBoundaryRole);
            PointInfo point{};
            point.batch = id->text().toUInt();
            point.type = id->data(Qt::UserRole).toUInt();
            for (int col = 0; col < table->columnCount(); ++col)
                if (auto* item = table->item(row, col)) item->setFont(table->font());
            applyTrackRowAppearance(table, row, point.type, externalControlStatus(point));
            for (int col = 0; col < table->columnCount(); ++col)
                if (auto* item = table->item(row, col)) item->setData(kTrackReportingBoundaryRole, boundary);
        }
    }
    // Replace only the health presentation; all published snapshots live in the overlay.
    if (m_healthWindow) {
        const bool visible = m_healthWindow->isVisible();
        const bool minimized = m_healthWindow->isMinimized();
        const QRect geometry = m_healthWindow->geometry();
        QPointer<QWidget> activeWindow = QApplication::activeWindow();
        const int panel = m_healthTabs ? m_healthTabs->currentIndex() : 0;
        delete m_healthWindow;
        m_healthUiDirty = true;
        if (visible) {
            onRadarSystemClicked();
            m_healthWindow->setInitialGeometry(geometry);
            if (m_healthTabs) m_healthTabs->setCurrentIndex(panel);
            if (minimized) m_healthWindow->showMinimized();
            if (activeWindow) activeWindow->activateWindow();
        }
    }
    QTimer::singleShot(0, this, [this] {
        if (m_trackTableFrozenHelper) m_trackTableFrozenHelper->syncFrozenContent();
        if (m_droneTableFrozenHelper) m_droneTableFrozenHelper->syncFrozenContent();
    });
    LOG_INFO(QStringLiteral("[UI_THEME] switched live to %1").arg(UiTheme::name()));
}

void MainOverLayOut::arrangeParamSettingsButtons()
{
    if (UiTheme::classicEnabled) {
        const bool multiPanelMode = CF_INS.defaultPanelCount() > 1;
        const QList<QPushButton*> radarButtons = {
            ui->btnStartSoftware, ui->btnStopSoftware, ui->btnTWSMode, ui->btnTASMode,
            ui->btnDataStorage, ui->btnServoNorth, ui->btnRadarStandby, ui->btnTransmitControl
        };
        for (auto* button : radarButtons) {
            ui->radarControlGrid->removeWidget(button);
            button->hide();
        }
        int visibleRadarCount = 0;
        for (auto* button : radarButtons) {
            if (multiPanelMode && (button == ui->btnTWSMode || button == ui->btnServoNorth)) continue;
            ui->radarControlGrid->addWidget(button, visibleRadarCount / 2, visibleRadarCount % 2);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            button->show();
            ++visibleRadarCount;
        }
        const int radarRows = (visibleRadarCount + 1) / 2;
        for (int row = 0; row < ui->radarControlGrid->rowCount(); ++row) {
            ui->radarControlGrid->setRowMinimumHeight(row, 0);
            ui->radarControlGrid->setRowStretch(row, row < radarRows ? 1 : 0);
        }
        for (int column = 0; column < ui->radarControlGrid->columnCount(); ++column) {
            ui->radarControlGrid->setColumnMinimumWidth(column, 0);
            ui->radarControlGrid->setColumnStretch(column, column < 2 ? 1 : 0);
        }
        const QList<QPushButton*> managedButtons = {
            ui->btnDataProcess,
            ui->btnSignalProcess,
            ui->btnServoControl,
            ui->btnScanRange,
            ui->btnFreqControl,
            ui->btnBatteryControl,
            m_commandControlButton
        };

        QList<QPushButton*> allManagedButtons = managedButtons;
        if (m_electromagneticButton) allManagedButtons.append(m_electromagneticButton);
        if (m_radarTrackApiButton) allManagedButtons.append(m_radarTrackApiButton);
        if (m_testDetectionButton) allManagedButtons.append(m_testDetectionButton);
        if (m_testTrackButton) {
            allManagedButtons.append(m_testTrackButton);
        }

        for (auto* btn : allManagedButtons) {
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
        visibleButtons.append(ui->btnBatteryControl);
        if (m_commandControlModule && m_commandControlModule->isEnabled())
            visibleButtons.append(m_commandControlButton);
        if (m_electromagneticButton) visibleButtons.append(m_electromagneticButton);
        if (m_radarTrackApiButton) visibleButtons.append(m_radarTrackApiButton);
        if (m_testDetectionButton) visibleButtons.append(m_testDetectionButton);
        if (m_testTrackButton) {
            visibleButtons.append(m_testTrackButton);
        }

        for (int i = 0; i < visibleButtons.size(); ++i) {
            auto* btn = visibleButtons.at(i);
            // 动态创建的“总控通信”与 .ui 中按钮采用相同的尺寸策略，避免末行被压缩。
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            btn->setMinimumHeight(56);
            btn->show();
            ui->paramSettingsGrid->addWidget(btn, i / 2, i % 2);
        }
        const int rowCount = (visibleButtons.size() + 1) / 2;
        // Remove stale empty-row space when a configuration/permission hides an entry.
        for (int row = rowCount; row < ui->paramSettingsGrid->rowCount(); ++row) {
            ui->paramSettingsGrid->setRowMinimumHeight(row, 0);
            ui->paramSettingsGrid->setRowStretch(row, 0);
        }
        for (int row = 0; row < rowCount; ++row) {
            ui->paramSettingsGrid->setRowMinimumHeight(row, 56);
            ui->paramSettingsGrid->setRowStretch(row, 1);
        }
        return;
    }

    const bool multiPanelMode = CF_INS.defaultPanelCount() > 1;
    const QList<QPushButton*> radarButtons = {
        ui->btnStartSoftware, ui->btnStopSoftware, ui->btnTWSMode, ui->btnTASMode,
        ui->btnDataStorage, ui->btnServoNorth, ui->btnRadarStandby, ui->btnTransmitControl
    };
    for (auto* button : radarButtons) {
        ui->radarControlGrid->removeWidget(button);
        button->hide();
    }
    int visibleRadarCount = 0;
    for (auto* button : radarButtons) {
        if (multiPanelMode && (button == ui->btnTWSMode || button == ui->btnServoNorth)) continue;
        ui->radarControlGrid->addWidget(button, visibleRadarCount / 2, visibleRadarCount % 2);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        button->show();
        ++visibleRadarCount;
    }
    const int radarRows = (visibleRadarCount + 1) / 2;
    for (int row = 0; row < ui->radarControlGrid->rowCount(); ++row) {
        ui->radarControlGrid->setRowMinimumHeight(row, 0);
        ui->radarControlGrid->setRowStretch(row, row < radarRows ? 1 : 0);
    }
    for (int column = 0; column < ui->radarControlGrid->columnCount(); ++column) {
        ui->radarControlGrid->setColumnMinimumWidth(column, 0);
        ui->radarControlGrid->setColumnStretch(column, column < 2 ? 1 : 0);
    }
    const QList<QPushButton*> managedButtons = {
        ui->btnDataProcess,
        ui->btnSignalProcess,
        ui->btnServoControl,
        ui->btnScanRange,
        ui->btnFreqControl,
        ui->btnBatteryControl,
        m_commandControlButton
    };

    QList<QPushButton*> allManagedButtons = managedButtons;
    if (m_electromagneticButton) allManagedButtons.append(m_electromagneticButton);
    if (m_radarTrackApiButton) allManagedButtons.append(m_radarTrackApiButton);
    if (m_testDetectionButton) allManagedButtons.append(m_testDetectionButton);
    if (m_testTrackButton) {
        allManagedButtons.append(m_testTrackButton);
    }

    for (auto* btn : allManagedButtons) {
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
    visibleButtons.append(ui->btnBatteryControl);
    if (m_commandControlModule && m_commandControlModule->isEnabled())
        visibleButtons.append(m_commandControlButton);
    if (m_electromagneticButton) visibleButtons.append(m_electromagneticButton);
    if (m_radarTrackApiButton) visibleButtons.append(m_radarTrackApiButton);
    if (m_testDetectionButton) visibleButtons.append(m_testDetectionButton);
    if (m_testTrackButton) {
        visibleButtons.append(m_testTrackButton);
    }

    for (int i = 0; i < visibleButtons.size(); ++i) {
        auto* btn = visibleButtons.at(i);
        // 动态创建的“总控通信”与 .ui 中按钮采用相同的尺寸策略，避免末行被压缩。
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setMinimumHeight(56);
        btn->show();
        ui->paramSettingsGrid->addWidget(btn, i / 2, i % 2);
    }
    const int rowCount = (visibleButtons.size() + 1) / 2;
    // Remove stale empty-row space when a configuration/permission hides an entry.
    for (int row = rowCount; row < ui->paramSettingsGrid->rowCount(); ++row) {
        ui->paramSettingsGrid->setRowMinimumHeight(row, 0);
        ui->paramSettingsGrid->setRowStretch(row, 0);
    }
    for (int row = 0; row < rowCount; ++row) {
        ui->paramSettingsGrid->setRowMinimumHeight(row, 56);
        ui->paramSettingsGrid->setRowStretch(row, 1);
    }
}

/**
 * @brief 更新发射按钮的显示状态
 * @details 根据当前发射状态更新按钮的颜色和文本
 */
void MainOverLayOut::updateTransmitButton()
{
    if (UiTheme::classicEnabled) {
        if (m_isTransmitting) {
            // 发射开启：绿色背景，文本为"关闭发射"
            ui->btnTransmitControl->setText("关闭发射");
            UiTheme::setStyle(ui->btnTransmitControl, [=] { return UiFontScale::style(
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
                "}"); });
        } else {
            // 发射关闭：红色背景，文本为"开启发射"
            ui->btnTransmitControl->setText("开启发射");
            UiTheme::setStyle(ui->btnTransmitControl, [=] { return UiFontScale::style(
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
                "}"); });
        }
        return;
    }

    ui->btnTransmitControl->setText(m_isTransmitting
        ? QStringLiteral("关闭发射\n本机状态：开启") : QStringLiteral("开启发射\n本机状态：关闭"));
    ui->btnTransmitControl->setToolTip(QStringLiteral("第一行为操作；第二行为本机控制状态，不代表设备回执。"));
    setStyleSheetIfChanged(ui->btnTransmitControl, QStringLiteral(
        "QPushButton {background:%1;color:%2;border:1px solid %3;"
        "border-radius:4px;padding:2px 6px;font-size:14px;}"
        "QPushButton:hover {background:%4;border-color:%2;}"
        "QPushButton:pressed {background:%4;border-color:#e8edf1;}"
        "QPushButton:disabled {background:rgba(25,35,43,190);color:#8495a1;border-color:#303b44;}")
        .arg(m_isTransmitting ? "rgba(40,78,63,218)" : "rgba(69,38,47,218)",
             m_isTransmitting ? "#9ce2bd" : "#f0abb0",
             m_isTransmitting ? "#78d4a5" : "#b86975",
             m_isTransmitting ? "rgba(49,95,76,235)" : "rgba(89,48,59,235)"));
}

void MainOverLayOut::resetTransmitState(const QString& reason)
{
    if (!m_isTransmitting && m_tranRecControlM.tran == 0) return;
    m_isTransmitting = false;
    m_tranRecControlM.tran = 0;
    if (m_commandControlModule) m_commandControlModule->setRadiationStatus(false);
    updateTransmitButton();
    const QString text = QStringLiteral("发射状态复位为关闭：%1（仅本机状态，不代表硬件回执）").arg(reason);
    LOG_WARNING(QStringLiteral("[TX_STATE][RESET] ") + text);
    logCommand(QStringLiteral("发射状态"), text);
}

/**
 * @brief 更新待机按钮的显示状态
 * @details 根据当前待机状态更新按钮的颜色和文本
 */
void MainOverLayOut::updateStandbyButton()
{
    if (UiTheme::classicEnabled) {
        if (m_isStandby) {
            // 待机模式：红色背景，文本为"进入工作"
            ui->btnRadarStandby->setText("进入工作");
            UiTheme::setStyle(ui->btnRadarStandby, [=] { return UiFontScale::style(
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
                "}"); });
        } else {
            // 工作模式：绿色背景，文本为"进入待机"
            ui->btnRadarStandby->setText("进入待机");
            UiTheme::setStyle(ui->btnRadarStandby, [=] { return UiFontScale::style(
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
                "}"); });
        }
        return;
    }

    ui->btnRadarStandby->setText(m_isStandby
        ? QStringLiteral("进入工作\n本机状态：待机") : QStringLiteral("进入待机\n本机状态：工作"));
    ui->btnRadarStandby->setToolTip(QStringLiteral("第一行为操作；第二行为本机控制状态，不代表设备回执。"));
    setStyleSheetIfChanged(ui->btnRadarStandby, QStringLiteral(
        "QPushButton {background:%1;color:%2;border:1px solid %3;"
        "border-radius:4px;padding:2px 6px;font-size:14px;}"
        "QPushButton:hover {background:%4;border-color:%2;}"
        "QPushButton:pressed {background:%4;border-color:#e8edf1;}"
        "QPushButton:disabled {background:rgba(25,35,43,190);color:#8495a1;border-color:#303b44;}")
        .arg(m_isStandby ? "rgba(65,54,37,218)" : "rgba(40,78,63,218)",
             m_isStandby ? "#dfbd82" : "#9ce2bd",
             m_isStandby ? "#b49968" : "#78d4a5",
             m_isStandby ? "rgba(85,70,46,235)" : "rgba(49,95,76,235)"));
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
        new CusWindow("记录回放", UiTheme::themedIcon(QStringLiteral(":/resources/icon/record.png"), QStringLiteral(":/resources/icon/ui/replay.svg")), this);
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

void MainOverLayOut::setupOfflineRaeButtons()
{
    if (!CF_INS.offlineRaeEnabled(false)) {
        LOG_INFO("[OfflineRAE] disabled by config");
        return;
    }

    auto createButton = [this](const QString& objectName,
                               const QString& text,
                               const QIcon& icon,
                               int column) -> QToolButton* {
        auto* btn = new QToolButton(ui->FuncWidget);
        btn->setObjectName(objectName);
        btn->setText(text);
        btn->setIcon(icon);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setProperty("buttonGroup", QStringLiteral("A"));
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setIconSize(ui->toolButton_3->iconSize());
        const bool compact = ScaleHelper::compactLayout();
        btn->setMinimumWidth(ScaleHelper::uiScaled(compact ? 70 : 78));
        btn->setMinimumHeight(ScaleHelper::uiScaled(compact ? 56 : 62));
        btn->setIconSize(QSize(ScaleHelper::uiScaled(compact ? 30 : 34),
                               ScaleHelper::uiScaled(compact ? 30 : 34)));
        ui->gridLayout_2->addWidget(btn, 1, column);
        return btn;
    };

    m_offlineAdsbButton = createButton(QStringLiteral("OfflineAdsbButton"),
                                       QStringLiteral("ADSB点迹"),
                                       (UiTheme::classicEnabled ? QIcon(QStringLiteral(":/resources/icon/data.png")) : QIcon(QStringLiteral(":/resources/icon/ui/awareness.svg"))),
                                       0);
    m_offlineCompareButton = createButton(QStringLiteral("OfflineCompareButton"),
                                          QStringLiteral("对比点迹"),
                                          (UiTheme::classicEnabled ? QIcon(QStringLiteral(":/resources/icon/radararray.png")) : QIcon(QStringLiteral(":/resources/icon/ui/radar.svg"))),
                                          1);

    connect(m_offlineAdsbButton, &QToolButton::clicked,
            this, &MainOverLayOut::onDrawOfflineAdsbClicked);
    connect(m_offlineCompareButton, &QToolButton::clicked,
            this, &MainOverLayOut::onDrawOfflineCompareClicked);

    LOG_INFO(QString("[OfflineRAE] enabled adsb_file=%1 compare_file=%2")
             .arg(CF_INS.offlineRaeFile("adsb_file", "data/adsb_rae.bin"),
                  CF_INS.offlineRaeFile("compare_file", "data/compare_rae.bin")));
}

QString MainOverLayOut::resolveRuntimePath(const QString& path) const
{
    if (path.isEmpty()) {
        return path;
    }
    QFileInfo info(path);
    if (info.isAbsolute()) {
        return QDir::cleanPath(path);
    }
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(path);
}

void MainOverLayOut::onDrawOfflineAdsbClicked()
{
    drawOfflineRaeDataset(resolveRuntimePath(CF_INS.offlineRaeFile("adsb_file", "data/adsb_rae.bin")),
                          QStringLiteral("ADSB点迹"));
}

void MainOverLayOut::onDrawOfflineCompareClicked()
{
    drawOfflineRaeDataset(resolveRuntimePath(CF_INS.offlineRaeFile("compare_file", "data/compare_rae.bin")),
                          QStringLiteral("对比点迹"));
}

void MainOverLayOut::drawOfflineRaeDataset(const QString& filePath, const QString& datasetName)
{
    const int maxRecords = CF_INS.offlineRaeMaxRecords(200000);
    const auto result = RaeDatasetReader::loadFile(filePath, maxRecords);
    if (!result.ok) {
        LOG_WARNING(QString("[OfflineRAE] %1 load failed: %2").arg(datasetName, result.message));
        logCommand(QStringLiteral("离线点迹绘制失败"), QString("%1: %2").arg(datasetName, result.message));
        CustomMessageBox::showWarning(this, QStringLiteral("离线点迹绘制失败"), result.message);
        return;
    }

    if (CF_INS.offlineRaeClearBeforeDraw(true) && mView) {
        mView->onClearDisplayRequested();
        clearAllTracks();
    }

    float maxRangeM = 0.0f;
    for (const auto& record : result.records) {
        maxRangeM = qMax(maxRangeM, record.point.range);
    }
    if (maxRangeM > 0.0f && mScene && mScene->axis() && maxRangeM > mScene->axis()->maxRange()) {
        const double nextMaxKm = std::ceil((static_cast<double>(maxRangeM) / 1000.0) * 1.05);
        mView->onMaxDistanceChanged(qMax(1.0, nextMaxKm));
        LOG_INFO(QString("[OfflineRAE] expanded display range to %1 km for %2")
                 .arg(qMax(1.0, nextMaxKm), 0, 'f', 0)
                 .arg(datasetName));
    }

    int detCount = 0;
    int trackCount = 0;
    int tbdCount = 0;
    int cooperativeCount = 0;

    for (const auto& record : result.records) {
        switch (record.pointType) {
            case PointType::Detection:
                if (mScene && mScene->det()) {
                    mScene->det()->addDetPoint(record.point);
                }
                ++detCount;
                break;
            case PointType::Track:
                if (mScene && mScene->track()) {
                    mScene->track()->addTrackPoint(record.point);
                }
                ++trackCount;
                break;
            case PointType::TBDPointType:
                if (mScene && mScene->track()) {
                    mScene->track()->addTrackPoint(record.point);
                }
                ++tbdCount;
                break;
            case PointType::CooperativeTrackPointType:
                if (mScene && mScene->track()) {
                    mScene->track()->addTrackPoint(record.point);
                }
                ++cooperativeCount;
                break;
            default:
                break;
        }

        if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
            m_rangeAzimuthWidget->chart()->addPointInfo(record.point);
        }
        if (m_rangeHeightWidget && m_rangeHeightWidget->chart()) {
            m_rangeHeightWidget->chart()->addPointInfo(record.point);
        }
        if (m_track3DWidget) {
            m_track3DWidget->addPointInfo(record.point);
        }
    }

    const QString summary = QString("%1 records=%2 det=%3 track=%4 tbd=%5 coop=%6 file=%7")
                                .arg(datasetName)
                                .arg(result.records.size())
                                .arg(detCount)
                                .arg(trackCount)
                                .arg(tbdCount)
                                .arg(cooperativeCount)
                                .arg(filePath);
    LOG_INFO(QString("[OfflineRAE] draw complete: %1").arg(summary));
    logCommand(QStringLiteral("离线点迹绘制"), summary);
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
    const int leftMinW = ScaleHelper::uiScaled(compact ? 300 : 340);
    const int leftMaxW = ScaleHelper::uiScaled(compact ? 430 : 560);
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
    UiTheme::setStyle(ui->setTab, [=] { return UiFontScale::style(QString(
        "QTabBar::tab { min-width: %1px; padding: 6px 10px; }"
    ).arg(ScaleHelper::uiScaled(compact ? 104 : 116))); });
    ui->trackTab->setMinimumWidth(leftMinW);
    ui->trackTab->setMaximumWidth(QWIDGETSIZE_MAX);
    UiTheme::setStyle(ui->trackTab, [=] { return UiFontScale::style(QString(
        "QTabBar::tab { min-width: %1px; padding: 6px 10px; }"
    ).arg(ScaleHelper::uiScaled(compact ? 142 : 156))); });

    // --- 右侧 P显/扇区显示面板 ---
    const int rightMinW = ScaleHelper::uiScaled(compact ? 300 : 340);
    const int rightMaxW = ScaleHelper::uiScaled(compact ? 470 : 560);
    const int topButtonW = ScaleHelper::uiScaled(compact ? 82 : 92);
    const int topButtonIcon = ScaleHelper::uiScaled(compact ? 18 : 20);
    const int funcButtonW = ScaleHelper::uiScaled(compact ? 70 : 78);
    const int funcButtonH = ScaleHelper::uiScaled(compact ? 56 : 62);
    const int funcIcon = ScaleHelper::uiScaled(compact ? 30 : 34);
    if (m_rightSidebar) {
        m_rightSidebar->setMinimumWidth(rightMinW);
        m_rightSidebar->setMaximumWidth(rightMaxW);
    }
    ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->horizontalSpacer_3->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui->LogoWidget->setMaximumWidth(qBound(ScaleHelper::uiScaled(230), rightW - ScaleHelper::uiScaled(120), ScaleHelper::uiScaled(300)));
    ui->CloseButton->setMinimumWidth(topButtonW);
    ui->CloseButton->setMaximumWidth(topButtonW);
    ui->minButton->setMinimumWidth(topButtonW);
    ui->minButton->setMaximumWidth(topButtonW);
    ui->CloseButton->setIconSize(QSize(topButtonIcon, topButtonIcon));
    ui->minButton->setIconSize(QSize(topButtonIcon, topButtonIcon));
    ui->CloseButton->setFocusPolicy(Qt::NoFocus);
    ui->minButton->setFocusPolicy(Qt::NoFocus);
    ui->timeLabel->setMaximumWidth(rightW);
    ui->FuncWidget->setMaximumWidth(rightW);
    const QList<QToolButton*> functionButtons = {
        ui->toolButton_3, ui->radarsystem, ui->FuncToolButton_2, ui->FuncToolButton,
        m_offlineAdsbButton, m_offlineCompareButton
    };
    for (auto* btn : functionButtons) {
        if (!btn) {
            continue;
        }
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
        ui->btnDataStorage,
        ui->btnServoNorth,
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
        ui->btnFreqControl,   ui->btnBatteryControl
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
