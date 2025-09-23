/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:01
 * @Description: 
 */
#include "sig2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include "RadarDataManager.h"  // 雷达数据管理器头文件
#include <QThread>

sig2dispmanager::sig2dispmanager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("SIG_PRO_ID",SIG_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_GET_SIG_PORT1",DISP_GET_SIG_PORT1));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    host = QHostAddress(CF_INS.ip("SIG_PRO_IP",SIG_PRO_IP));
    port = CF_INS.port("SIG_2_DISP_PORT1",SIG_2_DISP_PORT1);
    connect(socket, &ThreadedUdpSocket::detInfo, this, &sig2dispmanager::detInfoDecode);
    connect(this,&sig2dispmanager::detInfoProcess,CON_INS, &Controller::detInfoProcess);
}

sig2dispmanager2::sig2dispmanager2(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("SIG_PRO_ID",SIG_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP), CF_INS.port("DISP_GET_SIG_PORT1",DISP_GET_SIG_PORT1));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    host = QHostAddress(CF_INS.ip("SIG_PRO_IP",SIG_PRO_IP));
    port = CF_INS.port("SIG_2_DISP_PORT2",SIG_2_DISP_PORT2);
    connect(socket, &ThreadedUdpSocket::dataSaveOK, CON_INS, &Controller::dataSaveOK);
    connect(socket, &ThreadedUdpSocket::dataDelOK, CON_INS, &Controller::dataDelOK);
    connect(socket, &ThreadedUdpSocket::offLineStat, CON_INS, &Controller::offLineStat);
}

void sig2dispmanager::detInfoDecode(QByteArray data)
{
    auto rawData = data.data();
    rawData += sizeof(unsigned short)+sizeof(SigData) + sizeof(ProtocolFrame) + sizeof(unsigned char); //跳过帧头和控制表以及雷达ID
    auto detNum = *((unsigned short*)(rawData)); //获取检测点数量
    rawData += sizeof(unsigned short); //跳过检测点内存指针
    PointInfo info;

    for(int i =0; i<detNum; i++)
    {
        auto detPointInfo = (detInfo*)rawData;

        info.type = PointType::Detection;
        info.range = detPointInfo->dis;
        info.azimuth = detPointInfo->azi;
        info.elevation = detPointInfo->ele;
        info.SNR = detPointInfo->CFARSNR;
        info.speed = detPointInfo->vel;
        info.altitute = detPointInfo->altitute;
        info.amp = detPointInfo->amp;
        
        // 使用新的统一数据管理器
        RADAR_DATA_MGR.processDetection(info);
        
        // 保持原有的信号发射以兼容现有代码
        emit detInfoProcess(info);
    }
}

sig2dispmanager::~sig2dispmanager() {
    thread->quit();
    thread->wait();
}

sig2dispmanager2::~sig2dispmanager2() {
    thread->quit();
    thread->wait();
}

