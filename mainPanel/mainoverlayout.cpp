/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:18
 * @Description: 
 */
#include "mainoverlayout.h"
#include "PolarDisp/ppiview.h"
#include "PolarDisp/ppisscene.h"
#include "cusWidgets/custommessagebox.h"
#include "Controller/controller.h"
#include "cusWidgets/detachablewidget.h"
#include <QTimer>
#include <QDateTime>
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

MainOverLayOut::~MainOverLayOut()
{
    delete ui;
}
