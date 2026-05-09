/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-05-09 09:43:04
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 11:28:41
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

    LOG_INFO(QString("[CollabTrack2DispManager] listen=%1:%2 peer=%3:%4")
             .arg(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP))
             .arg(CF_INS.port("DISP_GET_DATA_PORT3", DISP_GET_DATA_PORT3))
             .arg(host.toString())
             .arg(port));

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

    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TBDTrackHead))) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Datagram too small: size=%1, need>=%2")
                    .arg(data.size())
                    .arg(sizeof(ProtocolFrame) + sizeof(TBDTrackHead)));
        return;
    }

    auto head = reinterpret_cast<const TBDTrackHead*>(raw);
    if (head->mesID != COOPERATIVE_TRACK_MSG_ID) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Unexpected mesID=0x%1, expected=0x%2")
                    .arg(head->mesID, 4, 16, QChar('0'))
                    .arg(COOPERATIVE_TRACK_MSG_ID, 4, 16, QChar('0')));
    }
    raw += sizeof(TBDTrackHead);

    const char* end = data.constData() + data.size();
    int decodedTrackCount = 0;
    int decodedPointCount = 0;

    while (raw + static_cast<int>(sizeof(TBDTrackInfo)) <= end) {
        auto trackInfo = reinterpret_cast<const TBDTrackInfo*>(raw);
        raw += sizeof(TBDTrackInfo);

        if (trackInfo->length == 0) {
            LOG_WARNING(QString("[CollabTrack2DispManager] Batch %1 has zero points").arg(trackInfo->batch));
            continue;
        }

        const int pointsLenBytes = static_cast<int>(trackInfo->length) * static_cast<int>(sizeof(TBDPoint));
        if (raw + pointsLenBytes > end) {
            LOG_WARNING(QString("[CollabTrack2DispManager] Malformed frame: batch=%1 length=%2 exceeds remaining bytes=%3")
                        .arg(trackInfo->batch)
                        .arg(trackInfo->length)
                        .arg(end - raw));
            break;
        }

        ++decodedTrackCount;

        for (unsigned i = 0; i < trackInfo->length; ++i) {
            auto pt = reinterpret_cast<const TBDPoint*>(raw);
            PointInfo info;
            info.type = CooperativeTrackPointType;
            info.range = pt->dis;
            info.azimuth = pt->azi;
            info.elevation = pt->ele;
            info.SNR = pt->SNR;
            info.speed = pt->vel;
            info.altitute = pt->altitute;
            info.amp = pt->amp;
            info.batch = trackInfo->batch;
            info.statMethod = 0;
            info.targetRecResult = 0;

            RADAR_DATA_MGR.processTrack(info);
            emit cooperativeTrackProcess(info);
            raw += sizeof(TBDPoint);
            ++decodedPointCount;
        }
    }

    if (raw != end) {
        LOG_WARNING(QString("[CollabTrack2DispManager] Trailing bytes detected: %1")
                    .arg(end - raw));
    }

    LOG_INFO(QString("[CollabTrack2DispManager] Decoded cooperative frame: tracks=%1 points=%2 size=%3")
             .arg(decodedTrackCount)
             .arg(decodedPointCount)
             .arg(data.size()));
}

CollabTrack2DispManager::~CollabTrack2DispManager()
{
    thread->quit();
    thread->wait();
}