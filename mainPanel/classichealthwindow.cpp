// 经典健康窗口的布局与样式取自 x576/f338142，数据读取共用当前的发布快照。
#include "mainoverlayout.h"
#include "classichealthwidgets.h"
#include "Basic/ConfigManager.h"
#include "Basic/log.h"
#include "Basic/uifontscale.h"
#include "Controller/subsystemnetworkmonitor.h"
#include "cusWidgets/cuswindow.h"
#include "ElectromagneticDevice/devicemodule.h"
#include "PhotoelectricV2/module.h"
#include "ZhoushanOptical/zhoushanmodule.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>

namespace {
void setStyleSheetIfChanged(QWidget* widget, const QString& style)
{
    const QString applied = UiFontScale::style(style);
    if (widget->styleSheet() != applied) widget->setStyleSheet(applied);
}
}

void MainOverLayOut::bindClassicHealthPanel(int panelId)
{
    if (!m_classicHealth) return;

    if (panelId < 0 || panelId >= kRadarPanelCount) {
        return;
    }

    const ClassicHealthWidgets::Panel& widgets = m_classicHealth->m_healthPanelWidgets[panelId];
    m_activeHealthPanel = panelId;
    m_classicHealth->m_btnTxOpen = widgets.txOpen;
    m_classicHealth->m_btnDutyCycle = widgets.dutyCycle;
    m_classicHealth->m_btnPulseWidth = widgets.pulseWidth;
    m_classicHealth->m_btnRxOpen = widgets.rxOpen;
    m_classicHealth->m_btnFreqSrc = widgets.freqSrc;
    m_classicHealth->m_btnDigBoard = widgets.digBoard;
    m_classicHealth->m_btnServo = widgets.servo;
    m_classicHealth->m_btnBeidou = widgets.beidou;
    m_classicHealth->m_btnBluetooth = widgets.bluetooth;
    m_classicHealth->m_btnPowerBoard = widgets.powerBoard;
    m_classicHealth->m_tempLabel = widgets.tempLabel;
    m_lastBITReport = m_bitReports[panelId];
}

void MainOverLayOut::updateClassicHealthWindow()
{
    if (!m_classicHealth) return;

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
            ? QStringLiteral("QPushButton { background:#143c30; color:#66ffcc; border:1px solid #66ffcc; }")
            : QStringLiteral("QPushButton { background:#3c2020; color:#ff9090; border:1px solid #ff9090; }"));
    }
    if (m_zhoushanHealthButton) {
        m_zhoushanHealthButton->setText(m_zhoushanStatus);
        setStyleSheetIfChanged(m_zhoushanHealthButton, m_zhoushanLinkOk
            ? QStringLiteral("QPushButton { background:#143c30; color:#66ffcc; border:1px solid #66ffcc; }")
            : QStringLiteral("QPushButton { background:#3c2020; color:#ff9090; border:1px solid #ff9090; }"));
    }
    if (m_electromagneticHealthButton) {
        const QString text = "电磁设备：" + m_electromagneticStatus;
        const QString style = m_electromagneticLinkOk
            ? "QPushButton { background:#143c30; color:#66ffcc; border:1px solid #66ffcc; }"
            : "QPushButton { background:#3c2020; color:#ff9090; border:1px solid #ff9090; }";
        if (m_electromagneticHealthButton->text() != text)
            m_electromagneticHealthButton->setText(text);
        setStyleSheetIfChanged(m_electromagneticHealthButton, style);
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
    if (m_classicHealth->m_sigProBtn) {
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
        m_classicHealth->m_sigProBtn->setText(text);
        setStyleSheetIfChanged(m_classicHealth->m_sigProBtn, style);
    }

    // 数据处理软件
    if (m_classicHealth->m_dataProBtn) {
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
        m_classicHealth->m_dataProBtn->setText(text);
        setStyleSheetIfChanged(m_classicHealth->m_dataProBtn, style);
    }

    // 波束调度软件
    if (m_classicHealth->m_beamConBtn) {
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
        m_classicHealth->m_beamConBtn->setText(text);
        setStyleSheetIfChanged(m_classicHealth->m_beamConBtn, style);
    }

    // 目标识别软件
    if (m_classicHealth->m_targetRecBtn) {
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
        m_classicHealth->m_targetRecBtn->setText(text);
        setStyleSheetIfChanged(m_classicHealth->m_targetRecBtn, style);
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

    QString smallYellowStyle =
        "QPushButton { "
        "background-color: #ffff00; "
        "color: #101818; "
        "border: 1px solid #ffcc66; "
        "border-radius: 5px; "
        "padding: 8px; "
        "font-size: 13px; "
        "}";

    // 解析BIT状态位
    unsigned char bitGroup = m_lastBITReport.bitGroup;
    unsigned char powerState = m_lastBITReport.powerState;

    // 更新阵面发射状态
    if (m_classicHealth->m_btnTxOpen) {
        bool isOpen = bitGroup & 0x80;
        m_classicHealth->m_btnTxOpen->setText(isOpen ? "阵面发射开启" : "阵面发射关闭");
        setStyleSheetIfChanged(m_classicHealth->m_btnTxOpen, isOpen ? smallGreenStyle : smallRedStyle);
    }

    // 更新占空比状态
    if (m_classicHealth->m_btnDutyCycle) {
        bool hasAlarm = bitGroup & 0x40;
        m_classicHealth->m_btnDutyCycle->setText(hasAlarm ? "占空比报警" : "占空比正常");
        setStyleSheetIfChanged(m_classicHealth->m_btnDutyCycle, hasAlarm ? smallRedStyle : smallGreenStyle);
    }

    // 更新脉宽状态
    if (m_classicHealth->m_btnPulseWidth) {
        bool hasAlarm = bitGroup & 0x20;
        m_classicHealth->m_btnPulseWidth->setText(hasAlarm ? "脉宽报警" : "脉宽正常");
        setStyleSheetIfChanged(m_classicHealth->m_btnPulseWidth, hasAlarm ? smallRedStyle : smallGreenStyle);
    }

    // 更新阵面接收状态
    if (m_classicHealth->m_btnRxOpen) {
        bool isOpen = bitGroup & 0x10;
        m_classicHealth->m_btnRxOpen->setText(isOpen ? "阵面接收开启" : "阵面接收关闭");
        setStyleSheetIfChanged(m_classicHealth->m_btnRxOpen, isOpen ? smallGreenStyle : smallRedStyle);
    }

    // 更新频率源状态
    if (m_classicHealth->m_btnFreqSrc) {
        bool isNormal = bitGroup & 0x08;
        m_classicHealth->m_btnFreqSrc->setText(isNormal ? "频率源正常" : "频率源异常");
        setStyleSheetIfChanged(m_classicHealth->m_btnFreqSrc, isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新收发板状态
    if (m_classicHealth->m_btnDigBoard) {
        bool isLinked = bitGroup & 0x04;
        m_classicHealth->m_btnDigBoard->setText(isLinked ? "收发板建链" : "收发板断链");
        setStyleSheetIfChanged(m_classicHealth->m_btnDigBoard, isLinked ? smallGreenStyle : smallRedStyle);
    }

    // 更新伺服状态
    if (m_classicHealth->m_btnServo) {
        bool isNormal = bitGroup & 0x02;
        m_classicHealth->m_btnServo->setText(isNormal ? "伺服正常" : "伺服异常");
        setStyleSheetIfChanged(m_classicHealth->m_btnServo, isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新北斗状态
    if (m_classicHealth->m_btnBeidou) {
        bool isNormal = bitGroup & 0x01;
        m_classicHealth->m_btnBeidou->setText(isNormal ? "北斗正常" : "北斗异常");
        setStyleSheetIfChanged(m_classicHealth->m_btnBeidou, isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新蓝牙状态
    if (m_classicHealth->m_btnBluetooth) {
        bool isNormal = powerState & 0x02;
        m_classicHealth->m_btnBluetooth->setText(isNormal ? "蓝牙正常" : "蓝牙异常");
        setStyleSheetIfChanged(m_classicHealth->m_btnBluetooth, isNormal ? smallGreenStyle : smallRedStyle);
    }

    // 更新波控板电源状态
    if (m_classicHealth->m_btnPowerBoard) {
        bool isNormal = powerState & 0x01;
        m_classicHealth->m_btnPowerBoard->setText(isNormal ? "波控板电源正常" : "波控板电源异常");
        setStyleSheetIfChanged(m_classicHealth->m_btnPowerBoard, isNormal ? smallGreenStyle : smallRedStyle);
    }

    // ===== 更新 36 路子阵电源状态 =====
    // 协议 bit0 是 1 号电源；界面右上角放 1 号，向左递增，下一行右侧从 7 号开始。
    const SubArrayPowerVisualState& visualState =
        m_subArrayPowerVisualStates[m_activeHealthPanel];
    const bool hasBitReport = visualState.hasConfirmedStatus;
    ClassicHealthWidgets::Panel& activeWidgets = m_classicHealth->m_healthPanelWidgets[m_activeHealthPanel];
    int normalPowerCount = 0;
    int faultyPowerCount = 0;
    for (int powerIndex = 0; powerIndex < 36; ++powerIndex) {
        QPushButton* powerCell = activeWidgets.subArrayPowerCells[powerIndex];
        if (!powerCell) {
            continue;
        }

        if (!hasBitReport) {
            powerCell->setText(QString("电源%1\n未上报").arg(powerIndex + 1));
            setStyleSheetIfChanged(powerCell, smallYellowStyle);
            continue;
        }

        const bool isNormal = visualState.normal[powerIndex];
        powerCell->setText(QString("电源%1\n%2").arg(powerIndex + 1).arg(isNormal ? "正常" : "故障"));
        setStyleSheetIfChanged(powerCell, isNormal ? smallGreenStyle : smallRedStyle);
        if (isNormal) {
            ++normalPowerCount;
        } else {
            ++faultyPowerCount;
        }
    }

    if (activeWidgets.subArrayPowerSummaryLabel) {
        if (!hasBitReport) {
            activeWidgets.subArrayPowerSummaryLabel->setText(QStringLiteral("尚未收到子阵电源 BIT 上报"));
        } else {
            QString summary =
                QString("子阵电源：正常 %1 / 36，故障 %2 / 36")
                    .arg(normalPowerCount)
                    .arg(faultyPowerCount);
            summary += QStringLiteral("（5秒窗口 %1 帧，故障率>90%为故障）").arg(visualState.sampleCount);
            activeWidgets.subArrayPowerSummaryLabel->setText(summary);
        }
    }

    // ===== 更新温度和角度信息 =====
    if (m_classicHealth->m_tempLabel) {
        QString tempInfo = QString("FPGA温度: %1°C  |  阵面温度: %2°C  |  阵面偏航: %3°")
                               .arg(m_lastBITReport.fpgaTemp * 0.1, 0, 'f', 1)
                               .arg(m_lastBITReport.panelTemp * 0.1, 0, 'f', 1)
                               .arg(m_lastBITReport.yaw * 0.01, 0, 'f', 2);
        if (m_classicHealth->m_tempLabel->text() != tempInfo) m_classicHealth->m_tempLabel->setText(tempInfo);
    }

    QLabel* lastUpdateLabel = m_classicHealth->m_healthPanelWidgets[m_activeHealthPanel].lastUpdateLabel;
    if (lastUpdateLabel) {
        const QDateTime& reportTime = m_bitReportTimes[m_activeHealthPanel];
        if (reportTime.isValid()) {
            lastUpdateLabel->setText(QString("阵面 %1 状态汇总时间: %2")
                                     .arg(m_activeHealthPanel + 1)
                                     .arg(reportTime.toString("yyyy-MM-dd HH:mm:ss")));
        } else {
            lastUpdateLabel->setText(QString("阵面 %1 尚未收到 BIT 上报")
                                     .arg(m_activeHealthPanel + 1));
        }
    }
}

void MainOverLayOut::applyClassicNetworkHealthStatus(int index)
{
    if (!m_classicHealth) return;

    QPushButton* button = nullptr;
    switch (index) {
    case SubsystemNetworkMonitor::RadarLocal: button = m_classicHealth->m_radarLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::LaserLocal: button = m_classicHealth->m_laserLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::RadarPeer: button = m_classicHealth->m_radarPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::LaserPeer: button = m_classicHealth->m_laserPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::OpticalLocal: button = m_classicHealth->m_opticalLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::OpticalPeer: button = m_classicHealth->m_opticalPeerNetworkBtn; break;
    case SubsystemNetworkMonitor::ZhoushanLocal: button = m_classicHealth->m_zhoushanLocalNetworkBtn; break;
    case SubsystemNetworkMonitor::ZhoushanPeer: button = m_classicHealth->m_zhoushanPeerNetworkBtn; break;
    default: return;
    }
    if (!button) {
        return;
    }

    const bool ok = m_networkHealthOk[static_cast<size_t>(index)];
    const QString style = ok
        ? QStringLiteral("QPushButton { background:#00aa55; color:#ffffff; border:1px solid #66ffcc; border-radius:5px; padding:7px; font-size:13px; }")
        : QStringLiteral("QPushButton { background:#a02a2a; color:#ffffff; border:1px solid #ff6666; border-radius:5px; padding:7px; font-size:13px; }");
    button->setText(m_networkHealthText[static_cast<size_t>(index)]);
    button->setToolTip(m_networkHealthText[static_cast<size_t>(index)]);
    setStyleSheetIfChanged(button, style);
}

void MainOverLayOut::createClassicHealthWindow() {
    if (!m_classicHealth) m_classicHealth = std::make_unique<ClassicHealthWidgets>();

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
        new CusWindow("雷达系统健康管理", QIcon(":/resources/icon/radararray.png"), this);
    m_healthWindow->setAttribute(Qt::WA_DeleteOnClose);
    // 子阵电源状态以 6×6 阵列显示；固定最低 1200×1200，避免四阵面状态文字被压缩。
    m_healthWindow->setMinimumSize(1200, 1200);

    // 连接窗口关闭信号，清空指针
    connect(m_healthWindow, &QObject::destroyed, this, [this]() {
        m_healthWindow = nullptr;
        m_healthTabs = nullptr;
        m_activeHealthPanel = 0;
        m_classicHealth->m_sigProBtn = nullptr;
        m_classicHealth->m_dataProBtn = nullptr;
        m_classicHealth->m_beamConBtn = nullptr;
        m_classicHealth->m_targetRecBtn = nullptr;
        m_classicHealth->m_radarLocalNetworkBtn = nullptr;
        m_classicHealth->m_radarPeerNetworkBtn = nullptr;
        m_classicHealth->m_laserLocalNetworkBtn = nullptr;
        m_classicHealth->m_laserPeerNetworkBtn = nullptr;
        m_classicHealth->m_opticalLocalNetworkBtn = nullptr;
        m_classicHealth->m_opticalPeerNetworkBtn = nullptr;
        m_classicHealth->m_zhoushanLocalNetworkBtn = nullptr;
        m_classicHealth->m_zhoushanPeerNetworkBtn = nullptr;
        m_classicHealth->m_btnTxOpen = nullptr;
        m_classicHealth->m_btnDutyCycle = nullptr;
        m_classicHealth->m_btnPulseWidth = nullptr;
        m_classicHealth->m_btnRxOpen = nullptr;
        m_classicHealth->m_btnFreqSrc = nullptr;
        m_classicHealth->m_btnDigBoard = nullptr;
        m_classicHealth->m_btnServo = nullptr;
        m_classicHealth->m_btnBeidou = nullptr;
        m_classicHealth->m_btnBluetooth = nullptr;
        m_classicHealth->m_btnPowerBoard = nullptr;
        m_classicHealth->m_tempLabel = nullptr;
        for (auto& widgets : m_classicHealth->m_healthPanelWidgets) {
            widgets = ClassicHealthWidgets::Panel{};
        }
        LOG_INFO("[UI_WINDOW][Health] destroyed");
    });

    // 创建内容Widget
    QWidget* contentWidget = new QWidget(m_healthWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // ===== 软件状态区域 =====
    QLabel* softwareLabel = new QLabel("软件运行状态", contentWidget);
    softwareLabel->setStyleSheet(UiFontScale::style("font-size: 18px; font-weight: bold; color: #66ffcc;"));
    mainLayout->addWidget(softwareLabel);

    // 创建四个软件状态按钮
    m_classicHealth->m_sigProBtn = new QPushButton("信号处理", contentWidget);
    m_classicHealth->m_sigProBtn->setEnabled(false);  // 不可选
    m_classicHealth->m_sigProBtn->setMinimumHeight(50);

    m_classicHealth->m_dataProBtn = new QPushButton("数据处理", contentWidget);
    m_classicHealth->m_dataProBtn->setEnabled(false);  // 不可选
    m_classicHealth->m_dataProBtn->setMinimumHeight(50);

    m_classicHealth->m_beamConBtn = new QPushButton("波束调度", contentWidget);
    m_classicHealth->m_beamConBtn->setEnabled(false);  // 不可选
    m_classicHealth->m_beamConBtn->setMinimumHeight(50);

    m_classicHealth->m_targetRecBtn = new QPushButton("目标识别", contentWidget);
    m_classicHealth->m_targetRecBtn->setEnabled(false);
    m_classicHealth->m_targetRecBtn->setMinimumHeight(50);

    mainLayout->addWidget(m_classicHealth->m_sigProBtn);
    mainLayout->addWidget(m_classicHealth->m_dataProBtn);
    mainLayout->addWidget(m_classicHealth->m_beamConBtn);
    mainLayout->addWidget(m_classicHealth->m_targetRecBtn);

    // ===== 分系统网络状态 =====
    mainLayout->addSpacing(4);
    QLabel* networkLabel = new QLabel("分系统网络状态", contentWidget);
    networkLabel->setStyleSheet(UiFontScale::style("font-size: 18px; font-weight: bold; color: #66ffcc;"));
    mainLayout->addWidget(networkLabel);

    auto makeNetworkButton = [contentWidget]() {
        auto* button = new QPushButton(contentWidget);
        button->setEnabled(false);
        button->setMinimumHeight(38);
        return button;
    };
    m_classicHealth->m_radarLocalNetworkBtn = makeNetworkButton();
    m_classicHealth->m_radarPeerNetworkBtn = makeNetworkButton();
    m_classicHealth->m_laserLocalNetworkBtn = nullptr;
    m_classicHealth->m_laserPeerNetworkBtn = nullptr;
    QGridLayout* networkGrid = new QGridLayout();
    networkGrid->setSpacing(8);
    networkGrid->addWidget(m_classicHealth->m_radarLocalNetworkBtn, 0, 0);
    networkGrid->addWidget(m_classicHealth->m_radarPeerNetworkBtn, 0, 1);
    if (CF_INS.laserReportEnabled(true)) {
        m_classicHealth->m_laserLocalNetworkBtn = makeNetworkButton();
        m_classicHealth->m_laserPeerNetworkBtn = makeNetworkButton();
        networkGrid->addWidget(m_classicHealth->m_laserLocalNetworkBtn, 1, 0);
        networkGrid->addWidget(m_classicHealth->m_laserPeerNetworkBtn, 1, 1);
    }
    mainLayout->addLayout(networkGrid);
    if (m_opticalV2) {
        m_classicHealth->m_opticalLocalNetworkBtn = makeNetworkButton();
        m_classicHealth->m_opticalPeerNetworkBtn = makeNetworkButton();
        networkGrid->addWidget(m_classicHealth->m_opticalLocalNetworkBtn, 2, 0);
        networkGrid->addWidget(m_classicHealth->m_opticalPeerNetworkBtn, 2, 1);
        m_opticalHealthButton = new QPushButton(m_opticalStatus, contentWidget);
        m_opticalHealthButton->setMinimumHeight(38);
        connect(m_opticalHealthButton.data(), &QPushButton::clicked, this, [this] { m_opticalV2->showWindow(this); });
        mainLayout->addWidget(m_opticalHealthButton);
    }
    if (m_zhoushanOptical) {
        m_classicHealth->m_zhoushanLocalNetworkBtn = makeNetworkButton();
        m_classicHealth->m_zhoushanPeerNetworkBtn = makeNetworkButton();
        networkGrid->addWidget(m_classicHealth->m_zhoushanLocalNetworkBtn, 3, 0);
        networkGrid->addWidget(m_classicHealth->m_zhoushanPeerNetworkBtn, 3, 1);
        m_zhoushanHealthButton = new QPushButton(m_zhoushanStatus, contentWidget);
        m_zhoushanHealthButton->setMinimumHeight(38);
        connect(m_zhoushanHealthButton.data(), &QPushButton::clicked, this, [this] { m_zhoushanOptical->showWindow(this); });
        mainLayout->addWidget(m_zhoushanHealthButton);
    }
    if (m_electromagneticModule && m_electromagneticModule->isUiEnabled()) {
        m_electromagneticHealthButton = new QPushButton(contentWidget);
        m_electromagneticHealthButton->setText("电磁设备：" + m_electromagneticStatus);
        m_electromagneticHealthButton->setMinimumHeight(38);
        m_electromagneticHealthButton->setStyleSheet(UiFontScale::style(m_electromagneticLinkOk
            ? "QPushButton { background:#143c30; color:#66ffcc; border:1px solid #66ffcc; }"
            : "QPushButton { background:#3c2020; color:#ff9090; border:1px solid #ff9090; }"));
        connect(m_electromagneticHealthButton.data(), &QPushButton::clicked, this, [this] {
            m_electromagneticModule->showWindow(this);
        });
        mainLayout->addWidget(m_electromagneticHealthButton);
    }
    for (int index = 0; index < int(m_networkHealthText.size()); ++index) {
        applyNetworkHealthStatus(index);
    }

    // ===== 四个阵面 BIT 状态页 =====
    mainLayout->addSpacing(20);
    QLabel* bitLabel = new QLabel("阵面健康状态", contentWidget);
    bitLabel->setStyleSheet(UiFontScale::style("font-size: 18px; font-weight: bold; color: #66ffcc;"));
    mainLayout->addWidget(bitLabel);

    m_healthTabs = new QTabWidget(contentWidget);
    m_healthTabs->setDocumentMode(true);
    m_healthTabs->setMinimumHeight(430);
    m_healthTabs->setStyleSheet(UiFontScale::style(
        "QTabBar::tab { min-width: 72px; padding: 6px 10px; }"
    ));

    for (int panelId = 0; panelId < kRadarPanelCount; ++panelId) {
        QWidget* panelPage = new QWidget(m_healthTabs);
        QVBoxLayout* panelLayout = new QVBoxLayout(panelPage);
        panelLayout->setContentsMargins(12, 12, 12, 12);
        panelLayout->setSpacing(10);

        QLabel* panelTitle = new QLabel(QString("阵面 %1 BIT 状态").arg(panelId + 1), panelPage);
        panelTitle->setStyleSheet(UiFontScale::style("font-size: 16px; font-weight: bold; color: #66ffcc;"));
        panelLayout->addWidget(panelTitle);

        QGridLayout* bitGrid = new QGridLayout();
        bitGrid->setSpacing(10);
        auto makeBitButton = [panelPage](const QString& text) {
            auto* button = new QPushButton(text, panelPage);
            button->setEnabled(false);
            button->setMinimumHeight(42);
            return button;
        };

        ClassicHealthWidgets::Panel& widgets = m_classicHealth->m_healthPanelWidgets[panelId];
        widgets.txOpen = makeBitButton("阵面发射");
        widgets.dutyCycle = makeBitButton("占空比");
        widgets.pulseWidth = makeBitButton("脉宽");
        widgets.rxOpen = makeBitButton("阵面接收");
        widgets.freqSrc = makeBitButton("频率源");
        widgets.digBoard = makeBitButton("收发板");
        widgets.servo = makeBitButton("伺服");
        widgets.beidou = makeBitButton("北斗");
        widgets.bluetooth = makeBitButton("蓝牙");
        widgets.powerBoard = makeBitButton("波控板电源");

        bitGrid->addWidget(widgets.txOpen, 0, 0);
        bitGrid->addWidget(widgets.dutyCycle, 0, 1);
        bitGrid->addWidget(widgets.pulseWidth, 1, 0);
        bitGrid->addWidget(widgets.rxOpen, 1, 1);
        bitGrid->addWidget(widgets.freqSrc, 2, 0);
        bitGrid->addWidget(widgets.digBoard, 2, 1);
        bitGrid->addWidget(widgets.servo, 3, 0);
        bitGrid->addWidget(widgets.beidou, 3, 1);
        bitGrid->addWidget(widgets.bluetooth, 4, 0);
        bitGrid->addWidget(widgets.powerBoard, 4, 1);
        // 子阵电源按 6×6 物理排布显示：右上角为 1 号（bit0），向左递增；
        // 第二行右侧为 7 号，左下角为 36 号（bit35）。
        QVBoxLayout* subArrayLayout = new QVBoxLayout();
        subArrayLayout->setSpacing(6);
        QLabel* subArrayTitle = new QLabel("子阵电源状态（6×6）", panelPage);
        subArrayTitle->setStyleSheet(UiFontScale::style("font-size: 14px; font-weight: bold; color: #66ffcc;"));
        subArrayLayout->addWidget(subArrayTitle);

        QGridLayout* subArrayGrid = new QGridLayout();
        subArrayGrid->setSpacing(5);
        for (int powerIndex = 0; powerIndex < 36; ++powerIndex) {
            auto* powerCell = new QPushButton(QString("电源%1\n未上报").arg(powerIndex + 1), panelPage);
            powerCell->setEnabled(false);
            powerCell->setMinimumSize(58, 42);
            const int row = powerIndex / 6;
            const int column = 5 - (powerIndex % 6);
            powerCell->setToolTip(QString("子阵电源%1：协议第%2字节 D%3")
                                      .arg(powerIndex + 1)
                                      .arg(powerIndex / 8)
                                      .arg(powerIndex % 8));
            widgets.subArrayPowerCells[powerIndex] = powerCell;
            subArrayGrid->addWidget(powerCell, row, column);
        }
        subArrayLayout->addLayout(subArrayGrid);
        widgets.subArrayPowerSummaryLabel = new QLabel("未收到子阵电源 BIT 上报", panelPage);
        widgets.subArrayPowerSummaryLabel->setStyleSheet(UiFontScale::style("font-size: 12px; color: #999999;"));
        subArrayLayout->addWidget(widgets.subArrayPowerSummaryLabel);

        QHBoxLayout* panelStatusLayout = new QHBoxLayout();
        panelStatusLayout->setSpacing(18);
        panelStatusLayout->addLayout(bitGrid, 1);
        panelStatusLayout->addLayout(subArrayLayout, 2);
        panelLayout->addLayout(panelStatusLayout);

        widgets.tempLabel = new QLabel("温度/偏航信息加载中...", panelPage);
        widgets.tempLabel->setStyleSheet(UiFontScale::style("font-size: 14px; color: #ffffff; padding: 8px;"));
        panelLayout->addWidget(widgets.tempLabel);

        widgets.lastUpdateLabel = new QLabel("等待 BIT 上报...", panelPage);
        widgets.lastUpdateLabel->setStyleSheet(UiFontScale::style("font-size: 12px; color: #999999; padding: 8px;"));
        panelLayout->addWidget(widgets.lastUpdateLabel);
        panelLayout->addStretch();

        m_bitReports[panelId].radarId = static_cast<unsigned char>(panelId);
        m_healthTabs->addTab(panelPage, QString("阵面 %1").arg(panelId + 1));
    }

    mainLayout->addWidget(m_healthTabs);
    connect(m_healthTabs, &QTabWidget::currentChanged, this, [this](int index) {
        bindHealthPanel(index);
        updateHealthWindow();
    });
    bindHealthPanel(0);

    mainLayout->addStretch();

    // 设置内容
    m_healthWindow->setContentWidget(contentWidget);
    LOG_INFO("[UI_WINDOW][Health] content ready");

    LOG_INFO("[UI_WINDOW][Health] before show");
    m_healthWindow->show();
    LOG_INFO("[UI_WINDOW][Health] shown");

    // 首次更新显示（窗口显示后允许 updateHealthWindow 刷新缓存数据）
    updateHealthWindow();
}
