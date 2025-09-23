/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
#include "data2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include "RadarDataManager.h"  // 雷达数据管理器头文件
#include <QThread>

Data2DispManager::Data2DispManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DATA_PRO_ID",DATA_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_GET_DATA_PORT",DISP_GET_DATA_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    host = QHostAddress(CF_INS.ip("SIG_PRO_IP",SIG_PRO_IP));
    port = CF_INS.port("DATA_PRO_2_DISP",DATA_PRO_2_DISP);
    connect(socket, &ThreadedUdpSocket::traInfo, this, &Data2DispManager::traInfoDecode);
    connect(this,&Data2DispManager::traInfoProcess,CON_INS, &Controller::traInfoProcess);
}

void Data2DispManager::traInfoDecode(QByteArray data)
{
    auto rawData = data.data();
    rawData += sizeof(ProtocolFrame); //去掉帧头
    auto trackResult = *((TrackResult*)(rawData));
    auto traNum = trackResult.trackNum;
    rawData += sizeof(TrackResult);

    PointInfo info;

    for(int i =0; i<traNum; i++)
    {
        auto traPointInfo = (trackInfo*)rawData;
        info.type = 2; //类型为跟踪点
        info.range = traPointInfo->dis;
        info.azimuth = traPointInfo->azi;
        info.elevation = traPointInfo->ele;
        info.SNR = traPointInfo->SNR;
        info.speed = traPointInfo->vel;
        info.altitute = traPointInfo->altitute;
        info.amp = traPointInfo->amp;
        info.batch = traPointInfo->batch;
        info.statMethod = traPointInfo->statMethod;
        rawData += sizeof(trackInfo);
        
        // 使用新的统一数据管理器
        RADAR_DATA_MGR.processTrack(info);

        // 保持原有的信号发射以兼容现有代码
        emit traInfoProcess(info);
    }
}

Data2DispManager::~Data2DispManager() {
    thread->quit();
    thread->wait();
}

