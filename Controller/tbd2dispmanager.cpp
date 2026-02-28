/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 16:19:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:30
 * @Description: 
 */
#include "tbd2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include "RadarDataManager.h"
#include <QThread>

Tbd2DispManager::Tbd2DispManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DATA_PRO_ID", DATA_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID", DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP),
                                   CF_INS.port("DISP_GET_DATA_PORT2", DISP_GET_DATA_PORT2));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();

    host = QHostAddress(CF_INS.ip("DATA_PRO_IP", DATA_PRO_IP));
    port = CF_INS.port("DATA_PRO_2_DISP2", DATA_PRO_2_DISP2);

    connect(socket, &ThreadedUdpSocket::tbdInfo, this, &Tbd2DispManager::tbdInfoDecode);
    connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);
    connect(this, &Tbd2DispManager::tbdInfoProcess, CON_INS, &Controller::tbdInfoProcess);
}

void Tbd2DispManager::tbdInfoDecode(QByteArray data)
{
    const char* raw = data.constData();
    // skip protocol frame
    raw += sizeof(ProtocolFrame);

    // need at least head
    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TBDTrackHead))) {
        return;
    }

    auto head = reinterpret_cast<const TBDTrackHead*>(raw);
    Q_UNUSED(head); // updateFlag reserved for UI if needed
    raw += sizeof(TBDTrackHead);

    const char* end = data.constData() + data.size();

    while (raw + static_cast<int>(sizeof(TBDTrackInfo)) <= end) {
        auto trackInfo = reinterpret_cast<const TBDTrackInfo*>(raw);
        raw += sizeof(TBDTrackInfo);

        // bound check for points
        const int pointsLenBytes = static_cast<int>(trackInfo->length) * static_cast<int>(sizeof(TBDPoint));
        if (raw + pointsLenBytes > end) {
            break; // malformed
        }

        for (unsigned i = 0; i < trackInfo->length; ++i) {
            auto pt = reinterpret_cast<const TBDPoint*>(raw);
            PointInfo info;
            info.type = TBDPointType;
            info.range = pt->dis;
            info.azimuth = pt->azi;   // 协议值就是度数，无需转换
            info.elevation = pt->ele; // 协议值就是度数，无需转换
            info.SNR = pt->SNR;
            info.speed = pt->vel;
            info.altitute = pt->altitute;
            info.amp = pt->amp;
            info.batch = trackInfo->batch;
            info.statMethod = 0;

            // 推入统一数据管理器并保持兼容的信号发射
            RADAR_DATA_MGR.processTrack(info);
            emit tbdInfoProcess(info);
            raw += sizeof(TBDPoint);
        }
    }
}

Tbd2DispManager::~Tbd2DispManager()
{
    thread->quit();
    thread->wait();
}
