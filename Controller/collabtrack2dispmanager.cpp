/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-05-09 11:28:39
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 17:16:07
 * @Description: 
 */
#include "collabtrack2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "Basic/log.h"
#include "controller.h"
#include "RadarDataManager.h"
#include <QThread>

CollabTrack2DispManager::CollabTrack2DispManager(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("DATA_PRO_ID", DATA_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID", DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP),
                                   CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();

    host = QHostAddress(CF_INS.ip("DATA_PRO_IP", DATA_PRO_IP));
    port = CF_INS.port("DATA_PRO_2_DISP3", DATA_PRO_2_DISP3);

    LOG_INFO(QString("[CollabTrack2DispManager] listen=%1:%2 peer=%3:%4 mesID=0x%5")
             .arg(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP))
             .arg(CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3))
             .arg(host.toString())
             .arg(port)
             .arg(COOPERATIVE_TRACK_MSG_ID, 4, 16, QChar('0')));

    connect(socket, &ThreadedUdpSocket::cooperativeTrackInfo,
            this, &CollabTrack2DispManager::cooperativeTrackDecode);
    connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);
    connect(this, &CollabTrack2DispManager::cooperativeTrackProcess,
            CON_INS, &Controller::cooperativeTrackProcess);
}

void CollabTrack2DispManager::cooperativeTrackDecode(QByteArray data)
{
    if (data.isEmpty()) {
        LOG_WARNING("[CollabTrack2DispManager] Empty datagram received");
        return;
    }

    const char* raw = data.constData();
    raw += sizeof(ProtocolFrame);

    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TrackResult))) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Datagram too small: size=%1, need>=%2")
                    .arg(data.size())
                    .arg(sizeof(ProtocolFrame) + sizeof(TrackResult)));
        return;
    }

    const auto trackResult = reinterpret_cast<const TrackResult*>(raw);
    if (trackResult->mesID != COOPERATIVE_TRACK_MSG_ID) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Unexpected mesID=0x%1, expected=0x%2")
                    .arg(trackResult->mesID, 4, 16, QChar('0'))
                    .arg(COOPERATIVE_TRACK_MSG_ID, 4, 16, QChar('0')));
    }
    raw += sizeof(TrackResult);

    const char* end = data.constData() + data.size();
    const int trackCount = static_cast<int>(trackResult->trackNum);
    const int expectedBytes = trackCount * static_cast<int>(sizeof(trackInfo));
    if (raw + expectedBytes > end) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Malformed frame: trackNum=%1 exceeds remaining bytes=%2")
                    .arg(trackCount)
                    .arg(end - raw));
        return;
    }

    PointInfo info;
    for (int i = 0; i < trackCount; ++i)
    {
        const auto traPointInfo = reinterpret_cast<const trackInfo*>(raw);
        info.type = CooperativeTrackPointType;
        info.range = traPointInfo->dis;
        info.azimuth = traPointInfo->azi;
        info.elevation = traPointInfo->ele;
        info.SNR = traPointInfo->SNR;
        info.speed = traPointInfo->vel;
        info.altitute = traPointInfo->altitute;
        info.amp = traPointInfo->amp;
        info.batch = traPointInfo->batch;
        info.statMethod = traPointInfo->statMethod;
        info.targetRecResult = traPointInfo->targetRecResult;

        RADAR_DATA_MGR.processTrack(info);
        emit cooperativeTrackProcess(info);
        raw += sizeof(trackInfo);
    }

    if (raw != end) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Trailing bytes detected: %1")
                    .arg(end - raw));
    }

    LOG_INFO(QString("[CollabTrack2DispManager] Decoded cooperative frame: tracks=%1 size=%2")
             .arg(trackCount)
             .arg(data.size()));
}

CollabTrack2DispManager::~CollabTrack2DispManager()
{
    thread->quit();
    thread->wait();
}