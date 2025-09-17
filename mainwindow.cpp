#include "mainwindow.h"
#include "Basic/DispBasci.h"
#include <QVBoxLayout>
#include "mapDisp/mapprox.h"
#include "mainPanel/mainoverlayout.h"
#include "Controller/controller.h"

FramelessMainWindow::FramelessMainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 关键：设置无边框窗口标志
    // 设置窗口大小和位置
    setWindowTitle(APP_NAME);
    setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    showFullScreen();
    setupCentralView();
    setupOverlayUI();
    connect(CON_INS,&Controller::minimizeWindow,this,&QMainWindow::showMinimized);
}

void FramelessMainWindow::setupCentralView()
{
    QWidget *central = new QWidget(this);  //整个底层widget是map
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0,0,0,0);
    m_map = new MapProxyWidget();
    layout->addWidget(m_map->getView());
    m_map->chooseMap(1);  //默认地图显示
    setCentralWidget(central);
}

// ====== 悬浮控件层 ======
void FramelessMainWindow::setupOverlayUI()
{
    m_overlayWidget = new MainOverLayOut(this);
    m_overlayWidget->setAttribute(Qt::WA_TranslucentBackground,false);
    m_overlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_overlayWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_overlayWidget->setGeometry(0, 0, width(), height());
    m_overlayWidget->raise(); // 保证悬浮层在最上面
    m_overlayWidget->show();
}