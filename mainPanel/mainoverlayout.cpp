/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-26 15:06:48
 * @Description: 
 */
#include "mainoverlayout.h"

#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Controller/controller.h"
#include "PolarDisp/polaraxis.h"
#include "PolarDisp/ppisscene.h"
#include "PolarDisp/ppiview.h"
#include "cusWidgets/custommessagebox.h"
// 参数配置对话框头文件
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSlider>
#include <QSignalBlocker>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "PolarDisp/echorenderer.h"
#include "PolarDisp/colorbarwidget.h"
#include "PolarDisp/echolinechart.h"
#include "Basic/MarineProtocol.h"
#include "Controller/MarineRadarManager.h"

MainOverLayOut::MainOverLayOut(QWidget* parent) : QWidget(parent), ui(new Ui::MainOverLayOut) {
    ui->setupUi(this);

    // ========== 屏幕自适应：动态覆盖 UI 中的固定尺寸 ==========
    applyScaledSizes();

    topRightSet();
    mainPView();
    setupPPIOverlay();
    setupMarineControls();
    setupServoStatusPanel();
    setupAScopeToggle();
    setupColorBar();
    setupRangeSettings();
    setupWorkModeSettings();

    connect(CON_INS, &Controller::marineStatusUpdated,
            this, [this](const MarineRadarStatus& st) {
        if (m_gainCombo && m_gainCombo->currentIndex() != st.gain) {
            QSignalBlocker blocker(m_gainCombo);
            m_gainCombo->setCurrentIndex(qBound(0, static_cast<int>(st.gain), 3));
        }
        if (m_seaSlider && m_seaSlider->value() != st.seaVal) {
            QSignalBlocker blocker(m_seaSlider);
            m_seaSlider->setValue(st.seaVal);
        }
        if (m_rainSlider && m_rainSlider->value() != st.rainVal) {
            QSignalBlocker blocker(m_rainSlider);
            m_rainSlider->setValue(st.rainVal);
        }
        if (m_interferenceCombo && m_interferenceCombo->currentIndex() != st.ganRao) {
            QSignalBlocker blocker(m_interferenceCombo);
            m_interferenceCombo->setCurrentIndex(qBound(0, static_cast<int>(st.ganRao), 3));
        }
        if (m_levelSlider && m_levelSlider->value() != st.level) {
            QSignalBlocker blocker(m_levelSlider);
            m_levelSlider->setValue(st.level);
        }
        // 同步发射按钮
        if (m_btnTxToggle && m_btnTxToggle->isChecked() != st.txOn) {
            QSignalBlocker blocker(m_btnTxToggle);
            m_btnTxToggle->setChecked(st.txOn);
        }
        // 发射指示灯
        bool tx = st.txOn;
        if (m_lblTransmitIndicator) {
            m_lblTransmitIndicator->setText(tx ? QString::fromUtf8("\u25CF 发射开") : QString::fromUtf8("\u25CF 发射关"));
            m_lblTransmitIndicator->setStyleSheet(
                tx ? "color: #44ff44; font-size: 18px; font-family: 'Microsoft YaHei'; background: transparent;"
                   : "color: #ff4444; font-size: 18px; font-family: 'Microsoft YaHei'; background: transparent;");
        }
    });

}

void MainOverLayOut::topRightSet() {
    // 设置控件提示信息
    ui->minButton->setToolTip("最小化窗口");

    ui->CloseButton->setToolTip("关闭程序");

    ui->timeLabel->setToolTip("系统时间");

    ui->TitleLabel->setToolTip("系统标题");

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

void MainOverLayOut::setupPPIOverlay() {
    // 创建SIMRAD风格PPI左上角浮动覆盖层
    m_ppiOverlay = new QWidget(ui->viewWidget);
    m_ppiOverlay->setObjectName("ppiOverlay");
    m_ppiOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_ppiOverlay->setStyleSheet("background: transparent;");

    auto* overlayLayout = new QVBoxLayout(m_ppiOverlay);
    overlayLayout->setContentsMargins(12, 8, 0, 0);
    overlayLayout->setSpacing(2);

    // 量程大字（公里制）
    m_lblOverlayRange = new QLabel(QString::fromUtf8("量程  2.0 km"), m_ppiOverlay);
    m_lblOverlayRange->setObjectName("ppiOverlayLabel");
    m_lblOverlayRange->setStyleSheet(
        "color: #ff8800; font-size: 32px; font-weight: bold; "
        "font-family: 'Microsoft YaHei', 'Consolas', monospace; background: transparent;");

    // 船首向上 模式
    auto* lblHU = new QLabel(QString::fromUtf8("船首向上"), m_ppiOverlay);
    lblHU->setStyleSheet("color: #44ff44; font-size: 22px; font-weight: bold; "
                         "font-family: 'Microsoft YaHei'; background: transparent;");

    // 相对运动
    auto* lblRM = new QLabel(QString::fromUtf8("相对运动"), m_ppiOverlay);
    lblRM->setStyleSheet("color: #44ff44; font-size: 22px; font-weight: bold; "
                         "font-family: 'Microsoft YaHei'; background: transparent;");

    // 发射状态指示
    m_lblTransmitIndicator = new QLabel(QString::fromUtf8("\u25CF 发射关"), m_ppiOverlay);
    m_lblTransmitIndicator->setObjectName("ppiOverlayLabel");
    m_lblTransmitIndicator->setStyleSheet("color: #ff4444; font-size: 18px; "
                                          "font-family: 'Microsoft YaHei'; background: transparent;");

    // 距标圈指示
    m_lblRangeRing = new QLabel(QString::fromUtf8("\u25CF 距标圈"), m_ppiOverlay);
    m_lblRangeRing->setObjectName("ppiOverlayLabel");
    m_lblRangeRing->setStyleSheet("color: #44ff44; font-size: 18px; "
                                  "font-family: 'Microsoft YaHei'; background: transparent;");

    // 雷达型号
    auto* lblModel = new QLabel(QString::fromUtf8("西电船用"), m_ppiOverlay);
    lblModel->setStyleSheet("color: #888; font-size: 16px; "
                            "font-family: 'Microsoft YaHei'; background: transparent;");

    overlayLayout->addWidget(m_lblOverlayRange);
    overlayLayout->addWidget(lblHU);
    overlayLayout->addWidget(lblRM);
    overlayLayout->addSpacing(4);
    overlayLayout->addWidget(m_lblTransmitIndicator);
    overlayLayout->addWidget(m_lblRangeRing);
    overlayLayout->addSpacing(4);
    overlayLayout->addWidget(lblModel);
    overlayLayout->addStretch();

    m_ppiOverlay->setFixedSize(260, 280);
    m_ppiOverlay->move(0, 0);
    m_ppiOverlay->show();
    m_ppiOverlay->raise();

    // 同步量程变化到覆盖层标签（公里制）
    QTimer::singleShot(200, this, [this]() {
        if (mScene && mScene->axis()) {
            connect(mScene->axis(), &PolarAxis::rangeChanged,
                    this, [this](double /*min*/, double max) {
                if (m_lblOverlayRange) {
                    double km = max / 1000.0;
                    if (km >= 1.0)
                        m_lblOverlayRange->setText(QString::fromUtf8("量程  %1 km").arg(km, 0, 'f', 1));
                    else
                        m_lblOverlayRange->setText(QString::fromUtf8("量程  %1 m").arg(static_cast<int>(max)));
                }
            });
        }
    });

    // ===== PPI左下角 导航信息覆盖层 =====
    m_ppiNavOverlay = new QWidget(ui->viewWidget);
    m_ppiNavOverlay->setObjectName("ppiNavOverlay");
    m_ppiNavOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_ppiNavOverlay->setStyleSheet("background: transparent;");

    auto* navLayout = new QVBoxLayout(m_ppiNavOverlay);
    navLayout->setContentsMargins(12, 4, 0, 10);
    navLayout->setSpacing(3);

    // 位置 标题
    auto* lblPosTitle = new QLabel(QString::fromUtf8("位置"), m_ppiNavOverlay);
    lblPosTitle->setStyleSheet(
        "color: #888888; font-size: 16px; font-family: 'Microsoft YaHei'; background: transparent;");

    // 经纬度（度°分.小数' 格式）
    double lat = CF_INS.latitude();
    double lon = CF_INS.longitude();
    auto formatCoord = [](double val, bool isLat) -> QString {
        char hemi = isLat ? (val >= 0 ? 'N' : 'S') : (val >= 0 ? 'E' : 'W');
        val = qAbs(val);
        int deg = static_cast<int>(val);
        double min = (val - deg) * 60.0;
        return QString("%1 %2\xC2\xB0%3'")
            .arg(hemi).arg(deg).arg(min, 0, 'f', 3);
    };
    m_lblNavPos = new QLabel(m_ppiNavOverlay);
    m_lblNavPos->setText(QString("%1\n%2").arg(formatCoord(lat, true), formatCoord(lon, false)));
    m_lblNavPos->setStyleSheet(
        "color: #cccccc; font-size: 18px; font-family: 'Consolas', 'Microsoft YaHei', monospace; "
        "background: transparent; line-height: 1.4;");

    // 光标距离 + 方位
    m_lblNavCursor = new QLabel(QString::fromUtf8("光标  --.- km  --- °T"), m_ppiNavOverlay);
    m_lblNavCursor->setStyleSheet(
        "color: #ff8800; font-size: 18px; font-family: 'Microsoft YaHei', 'Consolas', monospace; "
        "background: transparent;");

    navLayout->addStretch();
    navLayout->addWidget(lblPosTitle);
    navLayout->addWidget(m_lblNavPos);
    navLayout->addSpacing(4);
    navLayout->addWidget(m_lblNavCursor);

    m_ppiNavOverlay->setFixedSize(280, 150);
    m_ppiNavOverlay->show();
    m_ppiNavOverlay->raise();

    // 初始定位 + resize跟随
    auto positionNavOverlay = [this]() {
        if (ui->viewWidget && m_ppiNavOverlay) {
            int y = ui->viewWidget->height() - m_ppiNavOverlay->height() - 8;
            m_ppiNavOverlay->move(8, qMax(0, y));
        }
    };
    QTimer::singleShot(100, this, positionNavOverlay);
    if (mView) {
        connect(mView, &PPIView::viewResized, this, positionNavOverlay);
    }

    // 连接光标位置更新
    if (mView) {
        connect(mView, &PPIView::cursorPositionChanged,
                this, [this](double distKm, double bearDeg) {
            if (m_lblNavCursor) {
                m_lblNavCursor->setText(
                    QString::fromUtf8("光标  %1 km  %2 °T")
                        .arg(distKm, 0, 'f', 2)
                        .arg(static_cast<int>(bearDeg + 0.5) % 360, 3, 10, QChar('0')));
            }
        });
    }
}

void MainOverLayOut::mainPView() {
    mView = new PPIView(this);  // 添加父对象，确保正确释放
    mView->setObjectName("mainPview");
    mScene = new PPIScene(this);
    mView->setPPIScene(mScene);
    QVBoxLayout* layout = new QVBoxLayout(ui->viewWidget);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->addWidget(mView);
    connect(mView, &PPIView::viewResized, mScene, &PPIScene::updateSceneSize);

}

void MainOverLayOut::setupRangeSettings() {
}

void MainOverLayOut::setupWorkModeSettings() {
    // 船用雷达模式：扫描范围参数联动已移除（无需TopLeft控件）
}

void MainOverLayOut::sendScanRangeParams() {
    // 船用雷达模式：扫描范围参数下发已移除
}

MainOverLayOut::~MainOverLayOut() {
    if (CON_INS) {
        disconnect(CON_INS, nullptr, this, nullptr);
    }

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

void MainOverLayOut::clearTrackTables() {
    // Legacy X576 track-table UI was removed; keep this slot for PPI clear callers.
}


// ============ 雷达控制槽函数实现 ============

/**
 * @brief 打开TAS模式设置对话框
 * @details 配置TAS（Target Alert System）模式参数
 */
// ============ 参数设置槽函数实现 ============

/**
 * @brief 记录命令日志
 * @param commandName 命令名称
 * @param parameters 命令参数描述
 * @details 写入统一日志系统。船用界面已删除旧 X576 文本日志面板。
 */
void MainOverLayOut::logCommand(const QString& commandName, const QString& parameters) {
    const QString detail = parameters.isEmpty()
        ? commandName
        : QString("%1: %2").arg(commandName, parameters);
    LOG_INFO(detail);
}


/**
 * @brief 根据屏幕分辨率动态设置面板宽度、按钮高度等尺寸
 * @details 覆盖 .ui 文件中的固定像素值，使界面在 1366×768 ~ 3840×2160 范围内自适应
 */
void MainOverLayOut::applyScaledSizes() {
    const int rightW = ScaleHelper::rightPanelWidth();

    ui->rightPanelWidget->setMinimumWidth(rightW);
    ui->rightPanelWidget->setMaximumWidth(rightW + 80);

    qInfo() << "ScaleHelper applied: factor=" << ScaleHelper::factor()
            << "rightPanel=" << rightW;
}

// ========== 船用雷达控件 ==========

void MainOverLayOut::setupMarineControls()
{
    setupSimradNavPanel();

    auto* cmdCombo = ui->marineCmdCombo;
    cmdCombo->clear();
    cmdCombo->addItem(QString::fromUtf8(u8"\u4ec5\u53c2\u6570"), MarineCmdParamsOnly);
    cmdCombo->addItem(QString::fromUtf8(u8"\u4f4d\u7f6e"), MarineCmdPosition);
    cmdCombo->addItem(QString::fromUtf8(u8"\u901f\u5ea6"), MarineCmdSpeed);
    const int defaultCmd = qBound(0, CF_INS.marineControl("cmd_num", MarineCmdParamsOnly), 4);
    const int defaultCmdIndex = cmdCombo->findData(defaultCmd);
    cmdCombo->setCurrentIndex(defaultCmdIndex >= 0 ? defaultCmdIndex : 0);

    auto* azimuthSpin = ui->marineAzimuthSpin;
    azimuthSpin->setRange(0.0, 359.99);
    azimuthSpin->setDecimals(2);
    azimuthSpin->setSingleStep(1.0);
    azimuthSpin->setSuffix(QString::fromUtf8(u8"\u00b0"));
    azimuthSpin->setValue(qBound(0.0, CF_INS.marineControlDouble("azimuth_deg", 0.0), 359.99));

    m_rangeCombo = ui->marineRangeCombo;
    m_rangeCombo->clear();
    for (int i = 0; i < MARINE_RANGE_TABLE_SIZE; ++i) {
        m_rangeCombo->addItem(marineRangeLabel(i), i);
    }
    const int defaultRangeIdx = CF_INS.marineControl("range", 7);
    m_rangeCombo->setCurrentIndex(qBound(0, defaultRangeIdx, MARINE_RANGE_TABLE_SIZE - 1));

    m_gainCombo = ui->marineGainCombo;
    m_gainCombo->clear();
    m_gainCombo->addItems({QString::fromUtf8(u8"\u5173"), QString::fromUtf8(u8"\u4f4e"),
                           QString::fromUtf8(u8"\u4e2d"), QString::fromUtf8(u8"\u9ad8")});
    m_gainCombo->setCurrentIndex(qBound(0, CF_INS.marineControl("gain", 0), 3));

    m_interferenceCombo = ui->marineInterferenceCombo;
    m_interferenceCombo->clear();
    m_interferenceCombo->addItems({QString::fromUtf8(u8"\u5173"), QString::fromUtf8(u8"\u4f4e"),
                                   QString::fromUtf8(u8"\u4e2d"), QString::fromUtf8(u8"\u9ad8")});
    m_interferenceCombo->setCurrentIndex(qBound(0, CF_INS.marineControl("interference", 0), 3));

    m_levelSlider = ui->marineLevelSlider;
    m_levelSlider->setRange(0, 255);
    m_levelSlider->setValue(CF_INS.marineControl("level", 0));
    m_levelValLabel = ui->marineLevelValLabel;
    m_levelValLabel->setText(QString::number(m_levelSlider->value()));

    m_seaSlider = ui->marineSeaSlider;
    m_seaSlider->setRange(0, 255);
    m_seaSlider->setValue(CF_INS.marineControl("sea_clutter", 0));
    m_seaValLabel = ui->marineSeaValLabel;
    m_seaValLabel->setText(m_seaSlider->value() == 0 ? QString::fromUtf8(u8"\u81ea\u52a8")
                                                       : QString::number(m_seaSlider->value()));

    m_rainSlider = ui->marineRainSlider;
    m_rainSlider->setRange(0, 255);
    m_rainSlider->setValue(CF_INS.marineControl("rain_clutter", 0));
    m_rainValLabel = ui->marineRainValLabel;
    m_rainValLabel->setText(m_rainSlider->value() == 0 ? QString::fromUtf8(u8"\u81ea\u52a8")
                                                         : QString::number(m_rainSlider->value()));

    m_btnTxToggle = ui->marineTxToggle;
    m_btnTxToggle->setCheckable(true);
    m_btnTxToggle->setChecked(CF_INS.marineControlBool("tx_on", false));
    m_btnTxToggle->setText(m_btnTxToggle->isChecked() ? QString::fromUtf8(u8"\u53d1\u5c04 \u5f00")
                                                       : QString::fromUtf8(u8"\u53d1\u5c04 \u5173"));

    m_servoCombo = ui->marineServoCombo;
    m_servoCombo->clear();
    for (int gear = 0; gear <= MARINE_SERVO_MAX_GEAR; ++gear) {
        m_servoCombo->addItem(QString::fromUtf8(u8"%1 \u6863 (%2 rpm)")
                                  .arg(gear)
                                  .arg(marineServoGearRpm(static_cast<uint8_t>(gear)), 0, 'f', 2),
                              gear);
    }
    const int defaultServo = CF_INS.marineControl("servo_gear", CF_INS.marineControl("servo_speed", 0));
    m_servoCombo->setCurrentIndex(qBound(0, defaultServo, static_cast<int>(MARINE_SERVO_MAX_GEAR)));
    auto* servoRpmLabel = ui->marineServoRpmLabel;
    servoRpmLabel->setText(QString::number(marineServoGearRpm(
                           static_cast<uint8_t>(m_servoCombo->currentData().toInt())), 'f', 2));

    m_marineCtrlPanel = ui->marineCtrlPanel;
    m_btnSendMarineControl = ui->sendMarineControlButton;
    m_btnSendMarineControl->setText(QString::fromUtf8(u8"\u4e0b\u53d1\u53c2\u6570"));
    auto* startButton = ui->marineStartButton;
    auto* stopButton = ui->marineStopButton;
    startButton->setText(QString::fromUtf8(u8"\u542f\u52a8"));
    stopButton->setText(QString::fromUtf8(u8"\u505c\u6b62"));

    const QString buttonStyle =
        "QPushButton { background: #1a1a1a; color: #ff8800; border: 1px solid #ff8800; "
        "border-radius: 4px; padding: 6px 10px; font-size: 14px; font-weight: bold; "
        "font-family: 'Microsoft YaHei'; }"
        "QPushButton:hover { background: #2a1a0a; }"
        "QPushButton:pressed { background: #3a260d; }";
    const QString txStyle =
        "QPushButton { background: #1a1a1a; color: #44ff44; border: 1px solid #333; "
        "border-radius: 4px; padding: 6px 10px; font-size: 14px; font-weight: bold; "
        "font-family: 'Microsoft YaHei'; }"
        "QPushButton:checked { background: #4a1010; color: #ff4444; border-color: #ff4444; }"
        "QPushButton:disabled { background: #070b0a; color: #56635f; border: 1px solid #24302d; }"
        "QPushButton:hover { background: #2a2a2a; }";
    m_btnSendMarineControl->setStyleSheet(buttonStyle);
    startButton->setStyleSheet(buttonStyle);
    stopButton->setStyleSheet(buttonStyle);
    m_btnTxToggle->setStyleSheet(txStyle);
    m_marineCtrlPanel->setStyleSheet(
        "QWidget#marineCtrlPanel { background: #0a0a0a; border: 1px solid #333; border-radius: 4px; }"
        "QLabel { color: #999; font-size: 13px; font-family: 'Microsoft YaHei'; background: transparent; }"
        "QLabel:disabled { color: #4d5a56; }"
        "QComboBox, QDoubleSpinBox { background: #111; color: #fff; border: 1px solid #555; "
        "border-radius: 3px; padding: 4px; min-height: 22px; }"
        "QComboBox:disabled, QDoubleSpinBox:disabled { background: #070b0a; color: #56635f; "
        "border: 1px solid #24302d; }"
        "QComboBox::drop-down:disabled { border-left: 1px solid #24302d; }"
        "QSlider::groove:horizontal { height: 5px; background: #003f3a; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 14px; margin: -5px 0; background: #4aa3ff; border-radius: 7px; }"
        "QSlider::groove:horizontal:disabled { background: #14211f; }"
        "QSlider::handle:horizontal:disabled { background: #3d4946; }");

    auto* toggleBtn = ui->marineToggleBtn;
    toggleBtn->setCheckable(true);
    toggleBtn->setChecked(false);
    toggleBtn->setText(QString::fromUtf8(u8"\u25bc \u96f7\u8fbe\u63a7\u5236"));
    toggleBtn->setStyleSheet(
        "QPushButton { background: #1a1a1a; color: #ff8800; border: 1px solid #333; "
        "border-radius: 3px; padding: 6px 10px; font-size: 14px; font-family: 'Microsoft YaHei'; }"
        "QPushButton:hover { background: #2a2a2a; }");
    m_marineCtrlPanel->setVisible(false);

    connect(toggleBtn, &QPushButton::toggled, this, [this, toggleBtn](bool checked) {
        m_marineCtrlPanel->setVisible(checked);
        toggleBtn->setText(checked ? QString::fromUtf8(u8"\u25b2 \u96f7\u8fbe\u63a7\u5236")
                                   : QString::fromUtf8(u8"\u25bc \u96f7\u8fbe\u63a7\u5236"));
    });

    connect(m_btnTxToggle, &QPushButton::toggled, this, [this](bool checked) {
        m_btnTxToggle->setText(checked ? QString::fromUtf8(u8"\u53d1\u5c04 \u5f00")
                                       : QString::fromUtf8(u8"\u53d1\u5c04 \u5173"));
        if (m_lblTransmitIndicator) {
            m_lblTransmitIndicator->setText(checked ? QString::fromUtf8(u8"\u25cf \u53d1\u5c04\u5f00")
                                                    : QString::fromUtf8(u8"\u25cf \u53d1\u5c04\u5173"));
            m_lblTransmitIndicator->setStyleSheet(
                QString("color: %1; font-size: 14px; font-weight: bold; background: transparent;")
                    .arg(checked ? "#44ff44" : "#ff4444"));
        }
    });

    connect(m_levelSlider, &QSlider::valueChanged, this, [this](int v) {
        m_levelValLabel->setText(QString::number(v));
    });
    connect(m_seaSlider, &QSlider::valueChanged, this, [this](int v) {
        m_seaValLabel->setText(v == 0 ? QString::fromUtf8(u8"\u81ea\u52a8") : QString::number(v));
    });
    connect(m_rainSlider, &QSlider::valueChanged, this, [this](int v) {
        m_rainValLabel->setText(v == 0 ? QString::fromUtf8(u8"\u81ea\u52a8") : QString::number(v));
    });
    connect(m_servoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, servoRpmLabel](int) {
        const auto gear = static_cast<uint8_t>(qBound(0, m_servoCombo->currentData().toInt(),
                                                      static_cast<int>(MARINE_SERVO_MAX_GEAR)));
        servoRpmLabel->setText(QString::number(marineServoGearRpm(gear), 'f', 2));
    });

    auto setEnabled = [](QWidget* widget, bool enabled) {
        if (widget) {
            widget->setEnabled(enabled);
        }
    };
    auto updateControlState = [=]() {
        const int cmd = cmdCombo->currentData().toInt();
        const bool positionMode = (cmd == MarineCmdPosition);
        const bool speedMode = (cmd == MarineCmdSpeed);
        const bool paramsMode = (cmd == MarineCmdParamsOnly);

        setEnabled(ui->lblMarineAzimuth, positionMode);
        setEnabled(azimuthSpin, positionMode);
        setEnabled(ui->lblMarineServo, speedMode);
        setEnabled(m_servoCombo, speedMode);
        setEnabled(servoRpmLabel, speedMode);

        setEnabled(ui->lblMarineRange, paramsMode);
        setEnabled(m_rangeCombo, paramsMode);
        setEnabled(ui->lblMarineGain, paramsMode);
        setEnabled(m_gainCombo, paramsMode);
        setEnabled(ui->lblMarineInterference, paramsMode);
        setEnabled(m_interferenceCombo, paramsMode);
        setEnabled(ui->lblMarineLevel, paramsMode);
        setEnabled(m_levelSlider, paramsMode);
        setEnabled(m_levelValLabel, paramsMode);
        setEnabled(ui->lblMarineSea, paramsMode);
        setEnabled(m_seaSlider, paramsMode);
        setEnabled(m_seaValLabel, paramsMode);
        setEnabled(ui->lblMarineRain, paramsMode);
        setEnabled(m_rainSlider, paramsMode);
        setEnabled(m_rainValLabel, paramsMode);
        setEnabled(ui->lblMarineTx, paramsMode);
        setEnabled(m_btnTxToggle, paramsMode);
    };
    connect(cmdCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [updateControlState](int) { updateControlState(); });
    updateControlState();

    connect(m_btnSendMarineControl, &QPushButton::clicked,
            this, &MainOverLayOut::sendMarineControlFromUi);
    connect(startButton, &QPushButton::clicked, this, [this]() {
        sendMarineControlCommandFromUi(MarineCmdStart);
    });
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        sendMarineControlCommandFromUi(MarineCmdStop);
    });

    connect(m_rangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) { syncMarineRange(idx); });
    syncMarineRange(m_rangeCombo->currentIndex());

    setupSimradTheme();
}

void MainOverLayOut::setupColorBar()
{
    m_colorBar = new ColorBarWidget(ui->viewWidget);
    m_colorBar->setFixedSize(80, 320);
    // 放置在PPI视图右下角
    QTimer::singleShot(100, this, [this]() {
        if (ui->viewWidget && m_colorBar) {
            int x = ui->viewWidget->width() - m_colorBar->width() - 10;
            int y = ui->viewWidget->height() - m_colorBar->height() - 10;
            m_colorBar->move(x, y);
            m_colorBar->show();
            m_colorBar->raise();
        }
    });

    // 从 EchoRenderer 获取颜色映射表
    if (mScene && mScene->echoRenderer()) {
        // SIMRAD Halo 高保真色阶（与 EchoRenderer::buildColorTableSimrad 一致）
        std::array<QRgb, 256> lut{};
        for (int i = 0; i < 16; ++i) lut[i] = qRgba(0, 0, 0, 0);
        for (int i = 16; i < 51; ++i) {
            double t = (i - 16) / 34.0;
            lut[i] = qRgb(0, static_cast<int>(t * 20), static_cast<int>(100 + t * 155));
        }
        for (int i = 51; i < 86; ++i) {
            double t = (i - 51) / 34.0;
            lut[i] = qRgb(0, static_cast<int>(20 + t * 220), static_cast<int>(255 - t * 60));
        }
        for (int i = 86; i < 121; ++i) {
            double t = (i - 86) / 34.0;
            lut[i] = qRgb(0, static_cast<int>(240 + t * 15), static_cast<int>(195 * (1.0 - t)));
        }
        for (int i = 121; i < 156; ++i) {
            double t = (i - 121) / 34.0;
            lut[i] = qRgb(static_cast<int>(t * 255), 255, 0);
        }
        for (int i = 156; i < 191; ++i) {
            double t = (i - 156) / 34.0;
            lut[i] = qRgb(255, static_cast<int>(255 - t * 120), 0);
        }
        for (int i = 191; i < 226; ++i) {
            double t = (i - 191) / 34.0;
            lut[i] = qRgb(255, static_cast<int>(135 * (1.0 - t)), 0);
        }
        for (int i = 226; i < 246; ++i) {
            double t = (i - 226) / 19.0;
            lut[i] = qRgb(255, static_cast<int>(t * 60), static_cast<int>(t * 30));
        }
        for (int i = 246; i <= 255; ++i) {
            double t = (i - 246) / 9.0;
            lut[i] = qRgb(255, static_cast<int>(60 + t * 195), static_cast<int>(30 + t * 225));
        }
        m_colorBar->setColorLUT(lut);
    }
}

void MainOverLayOut::syncMarineRange(int rangeIndex)
{
    double rangeM = marineRangeMeters(rangeIndex);

    // 同步到 EchoRenderer
    if (mScene && mScene->echoRenderer()) {
        mScene->echoRenderer()->setRange(rangeM);
    }

    // 同步到 PPI 场景。量程变化必须重算 pixelsPerMeter，保证圆形铺满显示区。
    if (mScene) {
        mScene->setRange(0.0f, static_cast<float>(rangeM));
    }

    // 同步到覆盖层标签
    if (m_lblOverlayRange) {
        QString label = marineRangeLabel(rangeIndex);
        m_lblOverlayRange->setText(QString("量程  %1").arg(label));
    }

    // 同步到 MarineRadarManager
    qInfo() << "[Marine] Range changed to index:" << rangeIndex
            << "=" << marineRangeLabel(rangeIndex) << "(" << rangeM << "m)";
}


MarineControlFrame MainOverLayOut::buildMarineControlFrameFromUi() const
{
    MarineControlFrame frame;
    if (auto* cmdCombo = findChild<QComboBox*>("marineCmdCombo")) {
        frame.cmdNum = static_cast<uint8_t>(qBound(0, cmdCombo->currentData().toInt(), 4));
    }
    if (auto* azimuthSpin = findChild<QDoubleSpinBox*>("marineAzimuthSpin")) {
        frame.azimuth = marineEncodeAzimuth(azimuthSpin->value());
    }
    if (m_rangeCombo) {
        frame.rangeVal = static_cast<uint8_t>(qBound(0, m_rangeCombo->currentIndex(), MARINE_RANGE_TABLE_SIZE - 1));
    }
    if (m_gainCombo) {
        frame.gain = static_cast<uint8_t>(qBound(0, m_gainCombo->currentIndex(), 3));
    }
    if (m_interferenceCombo) {
        frame.ganRao = static_cast<uint8_t>(qBound(0, m_interferenceCombo->currentIndex(), 3));
    }
    if (m_levelSlider) {
        frame.level = static_cast<uint8_t>(qBound(0, m_levelSlider->value(), 255));
    }
    if (m_seaSlider) {
        frame.seaVal = static_cast<uint8_t>(qBound(0, m_seaSlider->value(), 255));
    }
    if (m_rainSlider) {
        frame.rainVal = static_cast<uint8_t>(qBound(0, m_rainSlider->value(), 255));
    }
    frame.txCtrl = (m_btnTxToggle && m_btnTxToggle->isChecked()) ? 1 : 0;
    frame.servo = m_servoCombo
        ? static_cast<uint8_t>(qBound(0, m_servoCombo->currentData().toInt(), static_cast<int>(MARINE_SERVO_MAX_GEAR)))
        : 0;
    frame.updateChecksum();
    return frame;
}

void MainOverLayOut::sendMarineControlFromUi()
{
    const MarineControlFrame frame = buildMarineControlFrameFromUi();
    sendMarineControlCommandFromUi(frame.cmdNum);
}

void MainOverLayOut::sendMarineControlCommandFromUi(uint8_t command)
{
    if (!CON_INS || !CON_INS->marineMgr()) {
        qWarning() << "[Marine] send control ignored: manager not ready";
        return;
    }

    MarineControlFrame frame = buildMarineControlFrameFromUi();
    frame.cmdNum = static_cast<uint8_t>(qBound(0, static_cast<int>(command), 4));
    frame.updateChecksum();
    CON_INS->marineMgr()->setCurrentControl(frame);
    CON_INS->marineMgr()->sendControl(frame);

    qInfo() << "[Marine] Manual control sent"
            << "cmd" << static_cast<int>(frame.cmdNum)
            << "azimuth" << frame.azimuthDegrees()
            << "range" << static_cast<int>(frame.rangeVal)
            << "gain" << static_cast<int>(frame.gain)
            << "interference" << static_cast<int>(frame.ganRao)
            << "level" << static_cast<int>(frame.level)
            << "sea" << static_cast<int>(frame.seaVal)
            << "rain" << static_cast<int>(frame.rainVal)
            << "tx" << static_cast<int>(frame.txCtrl)
            << "servoGear" << static_cast<int>(frame.servo)
            << "servoRpm" << frame.servoRpm();
}

void MainOverLayOut::setupSimradNavPanel()
{
    m_navPanel = ui->simradNavPanel;
    m_lblSOGVal = ui->lblSOGVal;
    m_lblHDGVal = ui->lblHDGVal;
    m_lblCOGVal = ui->lblCOGVal;
    m_lblTURNVal = ui->lblTURNVal;
    m_lblPOSLat = ui->lblPOSLat;
    m_lblPOSLon = ui->lblPOSLon;
    m_lblDepthVal = ui->lblDepthVal;
    m_lblNavTime = ui->lblNavTime;
    m_lblNavDate = ui->lblNavDate;

    m_navPanel->setStyleSheet(
        "#simradNavPanel { background: #0a0a0a; border-left: 2px solid #ff8800; }"
        "QLabel { color: #888; font-size: 14px; font-family: 'Microsoft YaHei'; background: transparent; }");

    auto styleValue = [](QLabel* label, const QColor& color, int fontSize) {
        if (!label) {
            return;
        }
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold; "
                                     "font-family: 'Consolas', 'Courier New', monospace; background: transparent;")
                                 .arg(color.name()).arg(fontSize));
    };
    auto styleText = [](QLabel* label, int fontSize = 20) {
        if (!label) {
            return;
        }
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setStyleSheet(QString("color: #fff; font-size: %1px; font-family: 'Consolas', monospace; background: transparent;")
                                 .arg(fontSize));
    };

    styleValue(m_lblSOGVal, QColor(0xff, 0x88, 0x00), 42);
    styleValue(m_lblHDGVal, QColor(0xff, 0xff, 0xff), 42);
    styleValue(m_lblCOGVal, QColor(0xff, 0xff, 0xff), 38);
    styleValue(m_lblTURNVal, QColor(0xff, 0xff, 0xff), 34);
    styleText(m_lblPOSLat, 20);
    styleText(m_lblPOSLon, 20);
    styleValue(m_lblDepthVal, QColor(0x44, 0xff, 0x44), 46);
    styleValue(m_lblNavTime, QColor(0xff, 0x88, 0x00), 28);
    styleValue(m_lblNavDate, QColor(0xff, 0x88, 0x00), 18);

    auto* navTimer = new QTimer(this);
    connect(navTimer, &QTimer::timeout, this, [this]() {
        const QDateTime now = QDateTime::currentDateTime();
        if (m_lblNavTime) {
            m_lblNavTime->setText(now.toString("HH:mm:ss"));
        }
        if (m_lblNavDate) {
            m_lblNavDate->setText(now.toString("dd/MM/yyyy"));
        }
    });
    navTimer->start(1000);

    const QDateTime now = QDateTime::currentDateTime();
    if (m_lblNavTime) {
        m_lblNavTime->setText(now.toString("HH:mm:ss"));
    }
    if (m_lblNavDate) {
        m_lblNavDate->setText(now.toString("dd/MM/yyyy"));
    }
}

void MainOverLayOut::setupSimradTheme()
{
    // PPI 外围橙色边框
    ui->viewWidget->setStyleSheet(
        "QWidget#viewWidget { border: 2px solid #ff8800; background: #000; }");

    // 顶栏深色 + 橙色底线
    ui->topBarWidget->setStyleSheet(
        "QWidget#topBarWidget { background: #111; border-bottom: 2px solid #ff8800; }"
        "QLabel { color: #ccc; font-size: 15px; background: transparent; }");
    ui->TitleLabel->setStyleSheet(
        "color: #ff8800; font-size: 18px; font-weight: bold; background: transparent;");

    // 底栏深色 + 橙色顶线
    ui->bottomBarWidget->setStyleSheet(
        "QWidget#bottomBarWidget { background: #111; border-top: 2px solid #ff8800; }"
        "QLabel { color: #aaa; font-size: 12px; background: transparent; }"
        "QPushButton { background: #222; color: #ff8800; border: 1px solid #444; "
        "border-radius: 3px; padding: 2px 6px; min-width: 24px; min-height: 24px; "
        "font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #333; }");

    // 右侧面板背景
    ui->rightPanelWidget->setStyleSheet(
        "QWidget#rightPanelWidget { background: #0a0a0a; }");

    // 顶栏文字更新为 SIMRAD 风格
    ui->lblDisplayMode->setText("HU  RM");
    ui->lblDisplayMode->setStyleSheet(
        "color: #44ff44; font-size: 15px; font-weight: bold; background: transparent;");
    ui->TitleLabel->setText("西电船用");
}

// ============================================================================
// 伺服上行状态面板
// ============================================================================

void MainOverLayOut::setupServoStatusPanel()
{
    ui->servoStatusPanel->setStyleSheet(
        "#servoStatusPanel { background: #0a0a0a; border: 1px solid #333; border-radius: 4px; }"
        "QLabel { color: #888; font-size: 12px; font-family: 'Microsoft YaHei'; background: transparent; }");

    m_lblStRangeVal = ui->lblStRangeVal;
    m_lblStTxState = ui->lblStTxState;
    m_lblStGain = ui->lblStGain;
    m_lblStLevel = ui->lblStLevel;
    m_lblStSea = ui->lblStSea;
    m_lblStRain = ui->lblStRain;
    m_lblStGanRao = ui->lblStGanRao;
    m_lblStFreq = ui->lblStFreq;

    const auto applyValueStyle = [](QLabel* label, const QString& color = QStringLiteral("#44ff44")) {
        if (!label) {
            return;
        }
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold; "
                                     "font-family: 'Consolas', monospace; background: transparent;").arg(color));
    };
    applyValueStyle(m_lblStRangeVal);
    applyValueStyle(m_lblStTxState);
    applyValueStyle(m_lblStGain);
    applyValueStyle(m_lblStLevel);
    applyValueStyle(m_lblStSea);
    applyValueStyle(m_lblStRain);
    applyValueStyle(m_lblStGanRao);
    applyValueStyle(m_lblStFreq);

    connect(CON_INS, &Controller::marineStatusUpdated,
            this, &MainOverLayOut::onMarineStatusUpdated);
}

void MainOverLayOut::onMarineStatusUpdated(const MarineRadarStatus& status)
{
    if (m_lblStRangeVal)
        m_lblStRangeVal->setText(marineRangeLabel(status.rangeCode));

    if (m_lblStTxState) {
        bool tx = status.txOn;
        m_lblStTxState->setText(tx ? QString::fromUtf8("\u5f00") : QString::fromUtf8("\u5173"));
        m_lblStTxState->setStyleSheet(
            QString("color: %1; font-size: 13px; font-weight: bold; "
                    "font-family: 'Consolas', monospace; background: transparent;")
                .arg(tx ? "#ff4444" : "#44ff44"));
    }

    auto gainText = [](uint8_t v) -> QString {
        switch (v) {
        case 0: return QString::fromUtf8("\u5173");
        case 1: return QString::fromUtf8("\u4f4e");
        case 2: return QString::fromUtf8("\u4e2d");
        case 3: return QString::fromUtf8("\u9ad8");
        default: return QString::number(v);
        }
    };

    if (m_lblStGain)
        m_lblStGain->setText(gainText(status.gain));
    if (m_lblStLevel)
        m_lblStLevel->setText(status.level == 0 ? QString::fromUtf8("\u81ea\u52a8") : QString::number(status.level));
    if (m_lblStSea)
        m_lblStSea->setText(status.seaVal == 0 ? QString::fromUtf8("\u81ea\u52a8") : QString::number(status.seaVal));
    if (m_lblStRain)
        m_lblStRain->setText(status.rainVal == 0 ? QString::fromUtf8("\u81ea\u52a8") : QString::number(status.rainVal));
    if (m_lblStGanRao)
        m_lblStGanRao->setText(gainText(status.ganRao));
    if (m_lblStFreq) {
        bool ok = (status.freqStatus == 0);
        m_lblStFreq->setText(ok ? QString::fromUtf8("\u6b63\u5e38") : QString::fromUtf8("\u5f02\u5e38"));
        m_lblStFreq->setStyleSheet(
            QString("color: %1; font-size: 13px; font-weight: bold; "
                    "font-family: 'Consolas', monospace; background: transparent;")
                .arg(ok ? "#44ff44" : "#ff4444"));
    }

    // 同步发射指示灯到PPI覆盖层
    if (m_lblTransmitIndicator) {
        bool tx = status.txOn;
        m_lblTransmitIndicator->setText(tx ? QString::fromUtf8("\u25CF \u53d1\u5c04\u5f00")
                                           : QString::fromUtf8("\u25CF \u53d1\u5c04\u5173"));
        m_lblTransmitIndicator->setStyleSheet(
            QString("color: %1; font-size: 14px; font-weight: bold; background: transparent;")
                .arg(tx ? "#44ff44" : "#ff4444"));
    }
}

// ============================================================================
// PPI ↔ A显 切换
// ============================================================================

void MainOverLayOut::setupAScopeToggle()
{
    // A显 折线图（初始隐藏，与PPI同区域）
    m_echoLineChart = new EchoLineChart(ui->viewWidget);
    m_echoLineChart->setVisible(false);

    // 切换按钮（放在PPI覆盖层右上角）
    m_btnToggleAScope = new QPushButton("A", ui->viewWidget);
    m_btnToggleAScope->setToolTip(QString::fromUtf8("PPI / A\u663e \u5207\u6362"));
    m_btnToggleAScope->setFixedSize(36, 36);
    m_btnToggleAScope->setStyleSheet(
        "QPushButton { background: #1a1a1a; color: #ff8800; border: 2px solid #ff8800; "
        "border-radius: 18px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #333; }"
        "QPushButton:checked { background: #ff8800; color: #000; }");
    m_btnToggleAScope->setCheckable(true);
    m_btnToggleAScope->setChecked(false);
    m_btnToggleAScope->raise();

    // 初始位置
    QTimer::singleShot(200, this, [this]() {
        if (ui->viewWidget && m_btnToggleAScope) {
            m_btnToggleAScope->move(ui->viewWidget->width() - 50, 10);
        }
    });

    // 切换逻辑
    connect(m_btnToggleAScope, &QPushButton::toggled, this, [this](bool checked) {
        m_showAScope = checked;
        if (checked) {
            // 显示A显，调整大小铺满viewWidget
            m_echoLineChart->setGeometry(0, 0, ui->viewWidget->width(), ui->viewWidget->height());
            m_echoLineChart->setVisible(true);
            m_echoLineChart->raise();
            m_btnToggleAScope->raise(); // 保持按钮在最上层
            if (m_colorBar) m_colorBar->setVisible(false);
        } else {
            m_echoLineChart->setVisible(false);
            if (m_colorBar) m_colorBar->setVisible(true);
        }
    });

    // 连接回波数据到A显
    connect(CON_INS, &Controller::marineEchoLine,
            m_echoLineChart, &EchoLineChart::updateEchoLine);

    // 量程同步到A显
    if (m_rangeCombo) {
        connect(m_rangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            if (m_echoLineChart)
                m_echoLineChart->setRangeMeters(marineRangeMeters(idx));
        });
        // 初始量程
        m_echoLineChart->setRangeMeters(marineRangeMeters(m_rangeCombo->currentIndex()));
    }
}
