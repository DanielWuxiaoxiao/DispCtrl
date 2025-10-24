/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:35
 * @Description: 
 */
#include "mainoverlayout.h"
#include "PolarDisp/ppiview.h"
#include "PolarDisp/ppisscene.h"
#include "cusWidgets/custommessagebox.h"
#include "cusWidgets/customcombobox.h"
#include "cusWidgets/cuswindow.h"
#include "Controller/controller.h"
#include "cusWidgets/detachablewidget.h"
#include "azelrangewidget.h"
#include "Basic/ConfigManager.h"
#include "PolarDisp/pviewtopleft.h"
#include "PointManager/trackmanager.h"
#include "Controller/RadarDataManager.h"
// 参数配置对话框头文件
#include "paramWidget/batterycontrol.h"
#include "paramWidget/tranrecvui.h"
#include "paramWidget/datasaveui.h"
#include "paramWidget/dataprocessui.h"
#include "paramWidget/sigparamui.h"
#include "paramWidget/freqcontrolui.h"
#include "paramWidget/waveandsample.h"
#include "paramWidget/scanrangeui.h"
#include "paramWidget/photoelectricparam.h"
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>
#include <QDebug>
#include "PolarDisp/zoomview.h"
#include "PolarDisp/sectorwidget.h"
#include "PolarDisp/sectorscene.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"

MainOverLayOut::MainOverLayOut(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainOverLayOut)
{
    ui->setupUi(this);

    // 设置状态信息标签的简化tooltip
    ui->stalabel1->setToolTip("状态信息1");
    ui->stalabel2->setToolTip("状态信息2");
    ui->stalabel3->setToolTip("状态信息3");
    ui->stalabel4->setToolTip("状态信息4");
    ui->stalabel5->setToolTip("状态信息5");

    ui->stamsg1->setToolTip("状态数值1");
    ui->stamsg2->setToolTip("状态数值2");
    ui->stamsg3->setToolTip("状态数值3");
    ui->stamsg4->setToolTip("状态数值4");
    ui->stamsg5->setToolTip("状态数值5");

    ui->infoTab->setToolTip("信息面板");

    topRightSet();
    mainPView();
    setupRangeSettings();
    setupWorkModeSettings();
    setupTrackManagement();
    setupLogInfo();

    // 初始化最大日志行数
    m_maxLogLines = 200;

    // 初始化一键全数配置相关成员
    m_commandTimer = new QTimer(this);
    m_commandIndex = 0;
    connect(m_commandTimer, &QTimer::timeout, this, &MainOverLayOut::cmdTimeOut);

    // 初始化健康管理相关成员
    m_systemNormal = false;  // 默认异常状态
    m_sigProSta = 1;         // 默认异常
    m_dataProSta = 1;        // 默认异常
    m_beamConSta = 1;        // 默认异常

    // 连接健康管理信号
    connect(CON_INS, &Controller::monitorParamSend, this, &MainOverLayOut::monitorParamRef);

    // 连接雷达控制按钮
    connect(ui->btnBatteryControl, &QPushButton::clicked, this, &MainOverLayOut::onBatteryControlClicked);
    connect(ui->btnStartSoftware, &QPushButton::clicked, this, &MainOverLayOut::onStartSoftwareClicked);
    connect(ui->btnDataStorage, &QPushButton::clicked, this, &MainOverLayOut::onDataStorageClicked);
    connect(ui->btnTransmitControl, &QPushButton::clicked, this, &MainOverLayOut::onTransmitControlClicked);

    // 连接参数设置按钮
    connect(ui->btnDataProcess, &QPushButton::clicked, this, &MainOverLayOut::onDataProcessClicked);
    connect(ui->btnSignalProcess, &QPushButton::clicked, this, &MainOverLayOut::onSignalProcessClicked);
    connect(ui->btnFreqControl, &QPushButton::clicked, this, &MainOverLayOut::onFreqControlClicked);
    connect(ui->btnSetParam, &QPushButton::clicked, this, &MainOverLayOut::onSetParamClicked);
    // 隐藏方向图扫描控制按钮，功能已集成到"范围设置"tab中
    // ui->btnScanRange->setVisible(false);
    connect(ui->btnScanRange, &QPushButton::clicked, this, &MainOverLayOut::onScanRangeClicked);
    connect(ui->btnPhotoelectric, &QPushButton::clicked, this, &MainOverLayOut::onPhotoelectricClicked);

    // 连接雷达系统健康管理按钮
    connect(ui->radarsystem, &QToolButton::clicked, this, &MainOverLayOut::onRadarSystemClicked);

    // ========== 关键修复：连接航迹数据到表格显示 ==========
    // 从Controller接收航迹数据并更新表格
    connect(CON_INS, &Controller::traInfoProcess,
            this, &MainOverLayOut::updateTrackList);

    // 从Controller接收航迹数据并更新无人机表格
    connect(CON_INS, &Controller::traInfoProcess,
            this, &MainOverLayOut::updateDroneTrackList);
}

void MainOverLayOut::topRightSet()
{
    // 设置控件提示信息
    ui->minButton->setToolTip("最小化窗口");

    ui->CloseButton->setToolTip("关闭程序");

    ui->timeLabel->setToolTip("系统时间");

    ui->TitleLabel->setToolTip("系统标题");

    ui->SubtitleLabel->setToolTip("系统副标题");

    connect(ui->minButton, &QPushButton::clicked, CON_INS, &Controller::minimizeWindow);
    connect(ui->CloseButton, &QPushButton::clicked, this, [this]()
    {
        if (CustomMessageBox::showConfirm(this, "退出确认", "是否确认退出程序？")) {
            QApplication::quit();
        }
    });

    auto timeTimer = new QTimer(this);
    timeTimer->setInterval(1000); // 定时器间隔：1000毫秒 = 1秒
    // 连接定时器超时信号到时间更新槽函数
    connect(timeTimer, &QTimer::timeout, this, [this]()
    {
        // 获取当前系统时间
        QDateTime currentTime = QDateTime::currentDateTime();
        QString timeFormat = "yyyy-MM-dd ddd HH:mm:ss";
        QString currentTimeStr = currentTime.toString(timeFormat);
        // 更新标签显示文本
        ui->timeLabel->setText(currentTimeStr);
    });
    timeTimer->start();
}

void MainOverLayOut::mainPView()
{
    mView = new PPIView();
    mView->setObjectName("mainPview");
    mScene = new PPIScene(this);
    mView->setPPIScene(mScene);
    QVBoxLayout *layout = new QVBoxLayout(ui->viewWidget);
    layout->setContentsMargins(20,0,20,0);
    layout->addWidget(mView);
    connect(mView, &PPIView::viewResized, mScene, &PPIScene::updateSceneSize);

    QVBoxLayout *layout1 = new QVBoxLayout(ui->pviewFitW);
    layout1->setContentsMargins(0,0,0,0);
    m_zoomView = new ZoomViewWidget();
    // 设置窗口属性
    layout1->addWidget(new DetachableWidget("P显", m_zoomView, QIcon(":/resources/icon/scan.png"), this));

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

    // 添加独立的扇区显示（和 pviewFitW 一样的风格）到 pviewZoomW
    m_sectorWidget = new SectorWidget();
    // 默认与主场景同步（如果需要独立场景，可以删除下面一行）
    // m_sectorWidget->setSector(-30, 30, 0, 5000); // 可按需初始化
    QVBoxLayout *layout2 = new QVBoxLayout(ui->pviewSectorW);
    layout2->setContentsMargins(0,0,0,0);
    layout2->addWidget(new DetachableWidget("扇区显示", m_sectorWidget, QIcon(":/resources/icon/scan.png"), this));

    // ========== 关键修复：连接扇区显示数据流 ==========
    // 从Controller接收检测点数据并添加到扇区DetManager
    connect(CON_INS, &Controller::detInfoProcess,
            m_sectorWidget->scene()->detManager(), &SectorDetManager::addDetPoint);

    // 从Controller接收航迹数据并添加到扇区TrackManager
    connect(CON_INS, &Controller::traInfoProcess,
            m_sectorWidget->scene()->trackManager(), &SectorTrackManager::addTrackPoint);

}

void MainOverLayOut::setupRangeSettings()
{
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

    // 连接信号槽，当角度范围改变时下发到雷达控制系统
    connect(m_azElRangeWidget, &AzElRangeWidget::azRangeChanged,
            this, [this](int minAz, int maxAz) {
        qDebug() << "方位角范围变更:" << minAz << "°到" << maxAz << "°";

        // 构造ScanRange参数并下发
        ScanRange param;
        // 保持其他参数为默认值或从当前设置获取
        param.place = 0;     // 默认水平放置
        param.method = 0;    // 默认先列后行
        param.workMode = 0;  // 默认TWS模式

        // 计算扫描范围中心点（方位角）
        int azCenter;
        if (maxAz >= minAz) {
            azCenter = (minAz + maxAz) / 2;
        } else {
            // 跨越0度的情况
            azCenter = ((minAz + maxAz + 360) / 2) % 360;
        }
        param.azi = azCenter * 100;  // 转换为0.01度单位

        // 俯仰角保持当前设置（可以从成员变量获取）
        param.ele = 1500;  // 默认15度

        // 下发参数到Controller
        CON_INS->sendSRParam(param);
        qDebug() << "下发方位角扫描范围: center=" << azCenter << "°";
    });

    connect(m_azElRangeWidget, &AzElRangeWidget::elRangeChanged,
            this, [this](int minEl, int maxEl) {
        qDebug() << "俯仰角范围变更:" << minEl << "°到" << maxEl << "°";

        // 构造ScanRange参数并下发
        ScanRange param;
        param.place = 0;
        param.method = 0;
        param.workMode = 0;

        // 方位角保持当前设置（可以从成员变量获取）
        param.azi = 2000;  // 默认20度

        // 计算俯仰角中心点
        int elCenter = (minEl + maxEl) / 2;
        param.ele = elCenter * 100;  // 转换为0.01度单位

        // 下发参数到Controller
        CON_INS->sendSRParam(param);
        qDebug() << "下发俯仰角扫描范围: center=" << elCenter << "°";
    });

    // 连接"设置"按钮点击信号，打开详细设置对话框
    connect(m_azElRangeWidget, &AzElRangeWidget::settingsButtonClicked,
            this, &MainOverLayOut::onScanRangeClicked);

    // 设置默认的扫描范围（从配置文件读取）
    int defaultAzMin = CF_INS.azimuthRange("min", 30);    // 从配置读取，默认30°
    int defaultAzMax = CF_INS.azimuthRange("max", 120);   // 从配置读取，默认120°
    int defaultElMin = CF_INS.elevationRange("min", -10); // 从配置读取，默认-10°
    int defaultElMax = CF_INS.elevationRange("max", 45);  // 从配置读取，默认45°

    m_azElRangeWidget->setAzRange(defaultAzMin, defaultAzMax);
    m_azElRangeWidget->setElRange(defaultElMin, defaultElMax);
}

void MainOverLayOut::setupWorkModeSettings()
{
    // 设置组合框默认值
    ui->placementCombo->setCurrentIndex(0);  // 默认水平放置
    ui->scanMethodCombo->setCurrentIndex(0); // 默认先列后行
    ui->workModeCombo->setCurrentIndex(0);   // 默认TWS

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
                    connect(yawEdit, &QLineEdit::returnPressed, this, &MainOverLayOut::sendScanRangeParams);
                    qDebug() << "已连接topleft偏航输入框";
                }
                if (rollEdit) {
                    connect(rollEdit, &QLineEdit::returnPressed, this, &MainOverLayOut::sendScanRangeParams);
                    qDebug() << "已连接topleft倾角输入框";
                }
            }
        }
    });
}

void MainOverLayOut::sendScanRangeParams()
{
    // 创建ScanRange参数结构
    ScanRange scanParam;

    // 设置阵面摆放方式 (0水平放置 1竖直放置)
    scanParam.place = static_cast<unsigned char>(ui->placementCombo->currentIndex());

    // 设置扫描方式 (0先列后行 1先行后列)
    scanParam.method = static_cast<unsigned char>(ui->scanMethodCombo->currentIndex());

    // 设置工作方式 (0 TWS 1 TAS)
    scanParam.workMode = static_cast<unsigned char>(ui->workModeCombo->currentIndex());

    // 使用topleft控件中的阵面指北角和倾角
    double aziValue = 0.0;
    double eleValue = 0.0;

    if (m_topLeftWidget) {
        QLineEdit* yawEdit = m_topLeftWidget->findChild<QLineEdit*>("yaw");
        QLineEdit* rollEdit = m_topLeftWidget->findChild<QLineEdit*>("roll");

        if (yawEdit && !yawEdit->text().isEmpty()) {
            aziValue = yawEdit->text().toDouble();
            qDebug() << "阵面指北角值:" << aziValue;
        }

        if (rollEdit && !rollEdit->text().isEmpty()) {
            eleValue = rollEdit->text().toDouble();
            qDebug() << "阵面倾角值:" << eleValue;
        }
    }

    // 设置方位角和俯仰角 (0.01°量化)
    scanParam.azi = static_cast<short>(aziValue * 100);  // 转换为0.01°量化
    scanParam.ele = static_cast<short>(eleValue * 100);  // 转换为0.01°量化

    // 通过Controller发送ScanRange参数
    if (CON_INS) {
        CON_INS->sendSRParam(scanParam);
        qDebug() << QString("发送扫描范围参数: 摆放方式=%1, 扫描方式=%2, 工作方式=%3, 阵面指北角=%.2f°, 阵面倾角=%.2f°")
                    .arg(scanParam.place)
                    .arg(scanParam.method)
                    .arg(scanParam.workMode)
                    .arg(aziValue)
                    .arg(eleValue);
    } else {
        qWarning() << "Controller实例不可用，无法发送扫描范围参数";
    }
}

MainOverLayOut::~MainOverLayOut()
{
    delete ui;
}

void MainOverLayOut::setupTrackManagement()
{
    // 初始化总航迹表格
    QTableWidget* trackTable = ui->tableWidget;
    QStringList headers;
    headers << "批次号" << "方位(°)" << "俯仰(°)" << "高度(m)" << "距离(m)" << "速度(m/s)" << "SNR(dB)" << "目标类型";

    trackTable->setColumnCount(headers.size());
    trackTable->setHorizontalHeaderLabels(headers);
    trackTable->horizontalHeader()->setVisible(true);  // 强制显示表头
    trackTable->verticalHeader()->setVisible(false);
    trackTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    trackTable->setAlternatingRowColors(true);
    trackTable->setShowGrid(true);  // 显示网格线

    // 设置列宽
    QHeaderView* headerView = trackTable->horizontalHeader();
    headerView->setStretchLastSection(true);
    for (int i = 0; i < headers.size() - 1; ++i) {
        headerView->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    // 初始化无人机航迹表格
    QTableWidget* droneTable = ui->droneTableWidget;
    droneTable->setColumnCount(headers.size());
    droneTable->setHorizontalHeaderLabels(headers);
    droneTable->horizontalHeader()->setVisible(true);  // 强制显示表头
    droneTable->verticalHeader()->setVisible(false);
    droneTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    droneTable->setAlternatingRowColors(true);
    droneTable->setShowGrid(true);  // 显示网格线

    // 设置无人机表格列宽
    QHeaderView* droneHeaderView = droneTable->horizontalHeader();
    droneHeaderView->setStretchLastSection(true);
    for (int i = 0; i < headers.size() - 1; ++i) {
        droneHeaderView->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    // 连接RadarDataManager的信号
    connect(&RADAR_DATA_MGR, &RadarDataManager::trackReceived,
            this, &MainOverLayOut::updateTrackList);
    connect(&RADAR_DATA_MGR, &RadarDataManager::trackReceived,
            this, &MainOverLayOut::updateDroneTrackList);
    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared,
            this, &MainOverLayOut::clearAllTracks);

    // 连接Controller的目标分类信号
    if (CON_INS) {
        connect(CON_INS, &Controller::targetClaRes,
                this, [this](TargetClaRes res) {
                    updateTargetClassification(res.batchID, res.claRes);
                });
    }

    qDebug() << "航迹管理功能初始化完成";
}

void MainOverLayOut::updateTrackList(const PointInfo& info)
{
    QString targetType = getTargetTypeText(m_targetTypes.value(info.batch, 0));
    addOrUpdateTrackRow(ui->tableWidget, info, targetType);
    sortTrackTable(ui->tableWidget);
}

void MainOverLayOut::updateDroneTrackList(const PointInfo& info)
{
    // 只显示无人机类型的航迹
    if (m_targetTypes.value(info.batch, 0) == 1) { // 1 = 无人机
        QString targetType = getTargetTypeText(1);
        addOrUpdateTrackRow(ui->droneTableWidget, info, targetType);
        sortTrackTable(ui->droneTableWidget);
    }
}

void MainOverLayOut::updateTargetClassification(unsigned short batchID, int targetType)
{
    m_targetTypes[batchID] = targetType;

    // 更新总航迹表格中的目标类型
    QTableWidget* trackTable = ui->tableWidget;
    for (int row = 0; row < trackTable->rowCount(); ++row) {
        if (trackTable->item(row, 0) && static_cast<unsigned short>(trackTable->item(row, 0)->text().toInt()) == batchID) {
            if (trackTable->item(row, 7)) {
                trackTable->item(row, 7)->setText(getTargetTypeText(targetType));
            }
            break;
        }
    }

    // 如果是无人机类型，添加到无人机表格；否则从无人机表格中移除
    if (targetType == 1) { // 无人机
        // 从总表格中找到该批次的数据，添加到无人机表格
        for (int row = 0; row < trackTable->rowCount(); ++row) {
            if (trackTable->item(row, 0) && static_cast<unsigned short>(trackTable->item(row, 0)->text().toInt()) == batchID) {
                PointInfo info;
                info.batch = batchID;
                info.azimuth = trackTable->item(row, 1)->text().toFloat();
                info.elevation = trackTable->item(row, 2)->text().toFloat();
                info.altitute = trackTable->item(row, 3)->text().toFloat();
                info.range = trackTable->item(row, 4)->text().toFloat();
                info.speed = trackTable->item(row, 5)->text().toFloat();
                info.SNR = trackTable->item(row, 6)->text().toFloat();

                addOrUpdateTrackRow(ui->droneTableWidget, info, getTargetTypeText(targetType));
                break;
            }
        }
    } else {
        // 从无人机表格中移除非无人机目标
        QTableWidget* droneTable = ui->droneTableWidget;
        for (int row = droneTable->rowCount() - 1; row >= 0; --row) {
            if (droneTable->item(row, 0) && static_cast<unsigned short>(droneTable->item(row, 0)->text().toInt()) == batchID) {
                droneTable->removeRow(row);
                break;
            }
        }
    }

    // 重新排序两个表格
    sortTrackTable(ui->tableWidget);
    sortTrackTable(ui->droneTableWidget);
}

void MainOverLayOut::clearAllTracks()
{
    ui->tableWidget->setRowCount(0);
    ui->droneTableWidget->setRowCount(0);
    m_targetTypes.clear();
    m_trackStartTimes.clear();
}

int MainOverLayOut::addOrUpdateTrackRow(QTableWidget* tableWidget, const PointInfo& info, const QString& targetType)
{
    int row = -1;

    // 查找是否已存在该批次
    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        if (tableWidget->item(i, 0) && static_cast<unsigned short>(tableWidget->item(i, 0)->text().toInt()) == info.batch) {
            row = i;
            break;
        }
    }

    // 如果不存在，创建新行
    if (row == -1) {
        row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        m_trackStartTimes[info.batch] = QDateTime::currentDateTime(); // 记录航迹开始时间
    }

    // 更新行数据
    tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(info.batch)));
    tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(info.azimuth, 'f', 1)));
    tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(info.elevation, 'f', 1)));
    tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(info.altitute, 'f', 1)));
    tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(info.range, 'f', 1)));
    tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(info.speed, 'f', 1)));
    tableWidget->setItem(row, 6, new QTableWidgetItem(QString::number(info.SNR, 'f', 1)));
    tableWidget->setItem(row, 7, new QTableWidgetItem(targetType));

    // 设置所有项为不可编辑
    for (int col = 0; col < 8; ++col) {
        if (tableWidget->item(row, col)) {
            tableWidget->item(row, col)->setFlags(tableWidget->item(row, col)->flags() & ~Qt::ItemIsEditable);
        }
    }

    return row;
}

void MainOverLayOut::sortTrackTable(QTableWidget* tableWidget)
{
    // 获取所有行数据
    QList<QStringList> allRows;
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = tableWidget->item(row, col);
            rowData << (item ? item->text() : QString());
        }
        allRows.append(rowData);
    }

    // 自定义排序：无人机优先，相同类别按时间排序
    std::sort(allRows.begin(), allRows.end(), [this](const QStringList& a, const QStringList& b) {
        unsigned short batchA = static_cast<unsigned short>(a[0].toInt());
        unsigned short batchB = static_cast<unsigned short>(b[0].toInt());

        int typeA = m_targetTypes.value(batchA, 0);
        int typeB = m_targetTypes.value(batchB, 0);

        // 无人机类型（1）优先
        if (typeA == 1 && typeB != 1) return true;
        if (typeA != 1 && typeB == 1) return false;

        // 相同类别按开始时间排序（新的在前）
        if (typeA == typeB) {
            QDateTime timeA = m_trackStartTimes.value(batchA, QDateTime::currentDateTime());
            QDateTime timeB = m_trackStartTimes.value(batchB, QDateTime::currentDateTime());
            return timeA > timeB; // 新的航迹在前
        }

        return typeA < typeB;
    });

    // 清空表格并重新填充
    tableWidget->setRowCount(0);
    for (int i = 0; i < allRows.size(); ++i) {
        tableWidget->insertRow(i);
        const QStringList& rowData = allRows[i];
        for (int col = 0; col < rowData.size(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(rowData[col]);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(i, col, item);
        }
    }
}

QString MainOverLayOut::getTargetTypeText(int targetType) const
{
    switch (targetType) {
        case 0: return "未知";
        case 1: return "无人机";
        case 2: return "行人";
        case 3: return "车辆";
        case 4: return "鸟类";
        case 5: return "其他";
        default: return "未知";
    }
}

// ============ 雷达控制槽函数实现 ============

/**
 * @brief 打开一键全数配置对话框
 * @details 配置阵地控制参数
 */
void MainOverLayOut::onSetParamClicked()
{
    // 确保定时器没有在运行
    if (m_commandTimer->isActive()) {
        m_commandTimer->stop();
    }

    m_commandIndex = 0; // 重置命令索引
    setupCommands();    // 重新设置命令，确保每次启动都是从头开始

    // 设置定时器间隔为 1000 毫秒 (1秒)
    m_commandTimer->setInterval(1000);
    // 启动定时器
    m_commandTimer->start();

    logCommand("开始一键全数配置", "依次发送扫描范围、波形采样、信号处理、数据处理参数");
}

/**
 * @brief 打开处理软件启动对话框
 * @details 发送系统启动命令
 */
void MainOverLayOut::onStartSoftwareClicked()
{
    if (CustomMessageBox::showConfirm(this, "启动确认", "是否启动处理软件？")) {
        StartSysParam param;
        // 设置默认参数
        param.sta = 1;  // 启动状态

        logCommand("正在启动软件", "");

        emit CON_INS->sendSysStart(param);
    }
}

/**
 * @brief 打开数据存储/删除对话框
 * @details 配置数据保存和删除参数
 */
void MainOverLayOut::onDataStorageClicked()
{
    CusWindow* window = new CusWindow("数据存储/删除", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    DataSaveUI* dialog = new DataSaveUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 记录日志
    connect(dialog, &DataSaveUI::setParam, this, [this]() {
        logCommand("数据存储参数", "");
    });

    connect(dialog, &DataSaveUI::setParam, CON_INS, &Controller::sendDSParam);

    // ========== 关键修复：连接数据存储状态反馈 ==========
    // 数据保存成功反馈
    connect(CON_INS, &Controller::dataSaveOK, dialog, &DataSaveUI::dataSaveOK);

    // 数据删除成功反馈
    connect(CON_INS, &Controller::dataDelOK, dialog, &DataSaveUI::dataDelOK);

    // 离线处理状态反馈
    connect(CON_INS, &Controller::offLineStat, dialog, [dialog](OfflineStat offlinestat) {
        if (offlinestat.delStat == 1) {
            // 离线处理开始
            DataSaveUI::offlineDataID = offlinestat.dataID;
        } else if (offlinestat.delStat == 0) {
            // 离线处理结束
        }
    });

    window->show();
}/**
 * @brief 打开发射接收控制对话框
 * @details 配置收发控制参数
 */
void MainOverLayOut::onTransmitControlClicked()
{
    CusWindow* window = new CusWindow("发射接收控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    TranRecvUI* dialog = new TranRecvUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_tranRecControlM);

    // 保存参数
    connect(dialog, &TranRecvUI::setParam, this, [this](const TranRecControl param) {
        m_tranRecControlM = param;
    });

    // 发送参数到控制器
    connect(dialog, &TranRecvUI::setParam, CON_INS, &Controller::sendTRParam);

    // 记录日志
    connect(dialog, &TranRecvUI::setParam, this, [this]() {
        logCommand("发射接收控制", "");
    });

    window->show();
}// ============ 参数设置槽函数实现 ============

/**
 * @brief 打开数据处理参数对话框
 * @details 配置数据处理相关参数
 */
void MainOverLayOut::onDataProcessClicked()
{
    CusWindow* window = new CusWindow("数据处理参数", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    DataProcessUI* dialog = new DataProcessUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_dataProParam);

    // 保存参数
    connect(dialog, &DataProcessUI::setParam, this, [this](const DataProParam param) {
        m_dataProParam = param;
    });

    // 发送参数到控制器
    connect(dialog, &DataProcessUI::setParam, CON_INS, &Controller::sendDPParam);

    // 记录日志
    connect(dialog, &DataProcessUI::setParam, this, [this]() {
        logCommand("数据处理参数", "");
    });

    window->show();
}/**
 * @brief 打开信号处理参数对话框
 * @details 配置信号处理相关参数
 */
void MainOverLayOut::onSignalProcessClicked()
{
    CusWindow* window = new CusWindow("信号处理参数", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    sigParamUI* dialog = new sigParamUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_sigProParam);

    // 保存参数
    connect(dialog, &sigParamUI::setParam, this, [this](const SigProParam param) {
        m_sigProParam = param;
    });

    // 发送参数到控制器
    connect(dialog, &sigParamUI::setParam, CON_INS, &Controller::sendSPParam);

    // 记录日志
    connect(dialog, &sigParamUI::setParam, this, [this]() {
        logCommand("信号处理参数", "");
    });

    window->show();
}/**
 * @brief 打开波形及采样控制对话框
 * @details 配置频率控制和波形参数
 */
void MainOverLayOut::onFreqControlClicked()
{
       // 创建自定义窗口，使用雷达图标
    CusWindow* window = new CusWindow("波形及采样控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 创建对话框作为内容
    waveAndSample* waveControl = new waveAndSample(window);
    waveControl->setWindowFlags(Qt::Widget); // 作为普通widget嵌入
    window->setContentWidget(waveControl);

    // 恢复上次参数
    waveControl->restoreParam(m_beamControl);

    // 保存参数
    connect(waveControl, &waveAndSample::setParam, this, [this](const BeamControl param) {
        m_beamControl = param;
    });

    // 发送参数
    connect(waveControl, &waveAndSample::setParam, CON_INS, &Controller::sendWCParam);

    // 记录日志
    connect(waveControl, &waveAndSample::setParam, this, [this]() {
        logCommand("波形采样参数", "");
    });

    window->show();
}/**
 * @brief 打开电调控制对话框
 * @details 配置波束控制参数
 */
void MainOverLayOut::onBatteryControlClicked()
{
    // 创建自定义窗口，使用雷达图标
    CusWindow* window = new CusWindow("象限电源控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 创建对话框作为内容
    BatteryControl* dialog = new BatteryControl(window);
    dialog->setWindowFlags(Qt::Widget); // 作为普通widget嵌入
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_batteryControlM);

    // 保存参数
    connect(dialog, &BatteryControl::setParam, this, [this](const BatteryControlM param) {
        m_batteryControlM = param;
    });

    // 连接参数设置信号到Controller
    connect(dialog, &BatteryControl::setParam, CON_INS, &Controller::sendBCParam);

    // 记录日志
    connect(dialog, &BatteryControl::setParam, this, [this]() {
        logCommand("象限电源控制", "");
    });

    window->show();
}/**
 * @brief 打开方向图扫描控制对话框
 * @details 配置扫描范围参数
 */
void MainOverLayOut::onScanRangeClicked()
{
    CusWindow* window = new CusWindow("搜索范围控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    ScanRangeUI* dialog = new ScanRangeUI(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 恢复上次参数
    dialog->restoreParam(m_scanRange);

    // 保存参数
    connect(dialog, &ScanRangeUI::setParam, this, [this](const ScanRange param) {
        m_scanRange = param;
    });

    // 发送参数到控制器
    connect(dialog, &ScanRangeUI::setParam, CON_INS, &Controller::sendSRParam);

    // 记录日志
    connect(dialog, &ScanRangeUI::setParam, this, [this]() {
        logCommand("扫描范围参数", "");
    });

    window->show();
}/**
 * @brief 打开光电系统控制对话框
 * @details 配置光电参数
 */
void MainOverLayOut::onPhotoelectricClicked()
{
    CusWindow* window = new CusWindow("光电系统控制", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);

    PhotoElectricParam* dialog = new PhotoElectricParam(window);
    dialog->setWindowFlags(Qt::Widget);
    window->setContentWidget(dialog);

    // 发送参数到控制器
    connect(dialog, &PhotoElectricParam::setParam, CON_INS, &Controller::sendPEParam);
    connect(dialog, &PhotoElectricParam::setParam2, CON_INS, &Controller::sendPEParam2);

    // 记录日志
    connect(dialog, &PhotoElectricParam::setParam, this, [this]() {
        logCommand("光电追踪", "");
    });
    connect(dialog, &PhotoElectricParam::setParam2, this, [this]() {
        logCommand("光电追踪", "");
    });

    window->show();
}/**
 * @brief 初始化日志信息组件
 * @details 设置日志文本框为只读
 */
void MainOverLayOut::setupLogInfo()
{
    ui->logEdit->setReadOnly(true);
    // 日志文本框的样式已在darkstyle.qss中配置
}

/**
 * @brief 记录命令日志
 * @param commandName 命令名称
 * @param parameters 命令参数描述
 * @details 将命令操作记录到logEdit中，显示时间戳、命令名和参数
 *          日志从顶部添加，自动限制最大行数
 */
void MainOverLayOut::logCommand(const QString &commandName, const QString &parameters)
{
    // 获取当前时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 格式化日志信息
    QString newLogEntry = QString("[%1] %2: %3\n").arg(timestamp).arg(commandName).arg(parameters);

    // 获取当前QTextEdit的文本
    QString currentText = ui->logEdit->toPlainText();

    // 将新日志添加到当前文本的最前面
    QString updatedText = newLogEntry + currentText;

    // 限制日志行数
    QStringList lines = updatedText.split('\n', Qt::SkipEmptyParts);
    if (lines.count() > m_maxLogLines) {
        // 如果超过最大行数，只取最新的 m_maxLogLines 行
        lines = lines.mid(0, m_maxLogLines);
        updatedText = lines.join('\n') + '\n';
    }

    // 更新QTextEdit的文本
    ui->logEdit->setPlainText(updatedText);

    // 确保滚动条在顶部
    ui->logEdit->verticalScrollBar()->setValue(0);
}

/**
 * @brief 设置命令序列
 * @details 准备一键全数配置的命令列表
 */
void MainOverLayOut::setupCommands()
{
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
void MainOverLayOut::cmdTimeOut()
{
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
void MainOverLayOut::monitorParamRef(MonitorParam res)
{
    bool normal = true;

    // 处理信号处理软件状态
    m_sigProSta = res.sigProSta;
    switch(res.sigProSta)
    {
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
    }

    // 处理数据处理软件状态
    m_dataProSta = res.dataProSta;
    switch(res.dataProSta)
    {
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
    }

    // 处理波束调度软件状态
    m_beamConSta = res.beamConSta;
    switch(res.beamConSta)
    {
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
            "}"
        );
    }
}

/**
 * @brief 打开雷达系统健康管理对话框
 * @details 显示信号处理、数据处理、波束调度三个子系统的运行状态
 */
void MainOverLayOut::onRadarSystemClicked()
{
    // 创建自定义窗口
    CusWindow* window = new CusWindow("雷达系统健康管理", QIcon(":/resources/icon/radararray.png"), this);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setMinimumSize(400, 300);

    // 创建内容Widget
    QWidget* contentWidget = new QWidget(window);
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 定义按钮样式
    QString greenStyle = "QPushButton { "
                         "background-color: #00ff00; "
                         "color: #101818; "
                         "border: 2px solid #66ffcc; "
                         "border-radius: 8px; "
                         "padding: 15px; "
                         "font-size: 16px; "
                         "font-weight: bold; "
                         "}";

    QString redStyle = "QPushButton { "
                       "background-color: #ff0000; "
                       "color: #ffffff; "
                       "border: 2px solid #ff6666; "
                       "border-radius: 8px; "
                       "padding: 15px; "
                       "font-size: 16px; "
                       "font-weight: bold; "
                       "}";

    // 创建三个状态按钮
    QPushButton* sigProBtn = new QPushButton("信号处理", contentWidget);
    sigProBtn->setEnabled(false);  // 不可选
    sigProBtn->setMinimumHeight(60);
    if (m_sigProSta == 0) {
        sigProBtn->setStyleSheet(greenStyle);
    } else {
        sigProBtn->setStyleSheet(redStyle);
    }

    QPushButton* dataProBtn = new QPushButton("数据处理", contentWidget);
    dataProBtn->setEnabled(false);  // 不可选
    dataProBtn->setMinimumHeight(60);
    if (m_dataProSta == 0) {
        dataProBtn->setStyleSheet(greenStyle);
    } else {
        dataProBtn->setStyleSheet(redStyle);
    }

    QPushButton* beamConBtn = new QPushButton("波束调度", contentWidget);
    beamConBtn->setEnabled(false);  // 不可选
    beamConBtn->setMinimumHeight(60);
    if (m_beamConSta == 0) {
        beamConBtn->setStyleSheet(greenStyle);
    } else {
        beamConBtn->setStyleSheet(redStyle);
    }

    // 添加到布局
    mainLayout->addWidget(sigProBtn);
    mainLayout->addWidget(dataProBtn);
    mainLayout->addWidget(beamConBtn);
    mainLayout->addStretch();

    // 设置内容
    window->setContentWidget(contentWidget);
    window->show();
}
