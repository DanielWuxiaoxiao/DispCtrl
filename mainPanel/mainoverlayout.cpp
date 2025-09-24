/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-24 11:22:47
 * @Description: 
 */
#include "mainoverlayout.h"
#include "PolarDisp/ppiview.h"
#include "PolarDisp/ppisscene.h"
#include "cusWidgets/custommessagebox.h"
#include "cusWidgets/customcombobox.h"
#include "Controller/controller.h"
#include "cusWidgets/detachablewidget.h"
#include "azelrangewidget.h"
#include "Basic/ConfigManager.h"
#include "PolarDisp/pviewtopleft.h"
#include "PointManager/trackmanager.h"
#include "Controller/RadarDataManager.h"
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
#include <QDebug>
#include "PolarDisp/zoomview.h"
#include "PolarDisp/sectorwidget.h"

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

    // 自动转发来自 Controller 的点到扇区面板

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

    // 连接信号槽，当角度范围改变时可以进行相应处理
    connect(m_azElRangeWidget, &AzElRangeWidget::azRangeChanged,
            this, [this](int minAz, int maxAz) {
        qDebug() << "方位角范围变更:" << minAz << "°到" << maxAz << "°";
        // 这里可以添加与雷达控制系统的接口调用
        // 例如：CON_INS->setAzimuthRange(minAz, maxAz);
    });

    connect(m_azElRangeWidget, &AzElRangeWidget::elRangeChanged,
            this, [this](int minEl, int maxEl) {
        qDebug() << "俯仰角范围变更:" << minEl << "°到" << maxEl << "°";
        // 这里可以添加与雷达控制系统的接口调用
        // 例如：CON_INS->setElevationRange(minEl, maxEl);
    });

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
