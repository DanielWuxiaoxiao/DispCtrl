/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 17:16:07
 * @Description: 
 */
#include "data2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "Basic/log.h"
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

    LOG_INFO(QString("[Data2DispManager] listen=%1:%2 peer=%3:%4 mesID=0x%5")
             .arg(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP))
             .arg(CF_INS.port("DISP_GET_DATA_PORT", DISP_GET_DATA_PORT))
             .arg(host.toString())
             .arg(port)
             .arg(TRACK_INFO_MSG_ID, 4, 16, QChar('0')));

    connect(socket, &ThreadedUdpSocket::traInfo, this, &Data2DispManager::traInfoDecode);
    connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);
    connect(this,&Data2DispManager::traInfoProcess,CON_INS, &Controller::traInfoProcess);
}

void Data2DispManager::traInfoDecode(QByteArray data)
{
    if (data.isEmpty()) {
        LOG_WARNING("[Data2DispManager] Empty datagram received");
        return;
    }

    auto rawData = data.data();
    rawData += sizeof(ProtocolFrame); //去掉帧头
    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TrackResult))) {
        LOG_WARNING(QString("[Data2DispManager] Datagram too small: size=%1, need>=%2")
                    .arg(data.size())
                    .arg(sizeof(ProtocolFrame) + sizeof(TrackResult)));
        return;
    }

    const auto trackResult = reinterpret_cast<const TrackResult*>(rawData);
    if (trackResult->mesID != TRACK_INFO_MSG_ID) {
        LOG_WARNING(QString("[Data2DispManager] Unexpected mesID=0x%1, expected=0x%2")
                    .arg(trackResult->mesID, 4, 16, QChar('0'))
                    .arg(TRACK_INFO_MSG_ID, 4, 16, QChar('0')));
    }

    const int traNum = static_cast<int>(trackResult->trackNum);
    rawData += sizeof(TrackResult);

    const char* end = data.constData() + data.size();
    const int expectedBytes = traNum * static_cast<int>(sizeof(trackInfo));
    if (rawData + expectedBytes > end) {
        LOG_WARNING(QString("[Data2DispManager] Malformed frame: trackNum=%1 exceeds remaining bytes=%2")
                    .arg(traNum)
                    .arg(end - rawData));
        return;
    }

    PointInfo info;

    for(int i =0; i<traNum; i++)
    {
        const auto traPointInfo = reinterpret_cast<const trackInfo*>(rawData);
        info.type = 2; //类型为跟踪点
        info.range = traPointInfo->dis;
        info.azimuth = traPointInfo->azi;   // 协议值就是度数，无需转换
        info.elevation = traPointInfo->ele; // 协议值就是度数，无需转换
        info.SNR = traPointInfo->SNR;
        info.speed = traPointInfo->vel;
        info.altitute = traPointInfo->altitute;
        info.amp = traPointInfo->amp;
        info.batch = traPointInfo->batch;
        info.statMethod = traPointInfo->statMethod;
        info.targetRecResult = traPointInfo->targetRecResult;
        rawData += sizeof(trackInfo);

        // 使用新的统一数据管理器
        RADAR_DATA_MGR.processTrack(info);

        // 保持原有的信号发射以兼容现有代码
        emit traInfoProcess(info);
    }

    if (rawData != end) {
        LOG_WARNING(QString("[Data2DispManager] Trailing bytes detected: %1")
                    .arg(end - rawData));
    }

    LOG_INFO(QString("[Data2DispManager] Decoded normal track frame: tracks=%1 size=%2")
             .arg(traNum)
             .arg(data.size()));
}

Data2DispManager::~Data2DispManager() {
    thread->quit();
    thread->wait();
}
