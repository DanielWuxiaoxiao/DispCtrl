/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-12-25 16:19:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:19
 * @Description: 
 */
#include "tbd2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "Basic/log.h"
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

    LOG_INFO(QString("[Tbd2DispManager] listen=%1:%2 peer=%3:%4 mesID=0x%5")
             .arg(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP))
             .arg(CF_INS.port("DISP_GET_DATA_PORT2", DISP_GET_DATA_PORT2))
             .arg(host.toString())
             .arg(port)
             .arg(TBD_TRACK_MSG_ID, 4, 16, QChar('0')));

    connect(socket, &ThreadedUdpSocket::tbdInfo, this, &Tbd2DispManager::tbdInfoDecode);
    connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);
    connect(this, &Tbd2DispManager::tbdInfoProcess, CON_INS, &Controller::tbdInfoProcess);
}

void Tbd2DispManager::tbdInfoDecode(QByteArray data)
{
    if (data.isEmpty()) {
        LOG_WARNING("[Tbd2DispManager] Empty datagram received");
        return;
    }

    const char* raw = data.constData();
    raw += sizeof(ProtocolFrame);

    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TrackResult))) {
        LOG_WARNING(QString("[Tbd2DispManager] Datagram too small: size=%1, need>=%2")
                    .arg(data.size())
                    .arg(sizeof(ProtocolFrame) + sizeof(TrackResult)));
        return;
    }

    const auto trackResult = reinterpret_cast<const TrackResult*>(raw);
    if (trackResult->mesID != TBD_TRACK_MSG_ID) {
        LOG_WARNING(QString("[Tbd2DispManager] Unexpected mesID=0x%1, expected=0x%2")
                    .arg(trackResult->mesID, 4, 16, QChar('0'))
                    .arg(TBD_TRACK_MSG_ID, 4, 16, QChar('0')));
    }
    raw += sizeof(TrackResult);

    const char* end = data.constData() + data.size();
    const int trackCount = static_cast<int>(trackResult->trackNum);
    const int expectedBytes = trackCount * static_cast<int>(sizeof(trackInfo));
    if (raw + expectedBytes > end) {
        LOG_WARNING(QString("[Tbd2DispManager] Malformed frame: trackNum=%1 exceeds remaining bytes=%2")
                    .arg(trackCount)
                    .arg(end - raw));
        return;
    }

    ++m_frameCount;
    m_pointCount += static_cast<quint64>(trackCount);

    static quint64 s_tbdFrameLogCount = 0;
    ++s_tbdFrameLogCount;
    const bool shouldLogFrame = (s_tbdFrameLogCount <= 3) || (s_tbdFrameLogCount % 100 == 0);

    if (trackCount <= 0 && shouldLogFrame) {
        LOG_WARNING(QString("[Tbd2DispManager] Empty TBD track frame: frame=%1 size=%2 mesID=0x%3")
                    .arg(m_frameCount)
                    .arg(data.size())
                    .arg(trackResult->mesID, 4, 16, QChar('0')));
    } else if (shouldLogFrame) {
        const auto firstTrack = reinterpret_cast<const trackInfo*>(raw);
        const auto lastTrack = reinterpret_cast<const trackInfo*>(raw + (trackCount - 1) * static_cast<int>(sizeof(trackInfo)));
        LOG_INFO(QString("[Tbd2DispManager] Frame #%1 decoded: tracks=%2 totalTracks=%3 size=%4 firstBatch=%5 lastBatch=%6 firstStat=%7 firstRange=%8 firstAz=%9 targetRec=%10")
                 .arg(m_frameCount)
                 .arg(trackCount)
                 .arg(m_pointCount)
                 .arg(data.size())
                 .arg(firstTrack->batch)
                 .arg(lastTrack->batch)
                 .arg(firstTrack->statMethod)
                 .arg(firstTrack->dis, 0, 'f', 1)
             .arg(firstTrack->azi, 0, 'f', 2)
                 .arg(firstTrack->targetRecResult));
    }

    PointInfo info;
    for (int i = 0; i < trackCount; ++i)
    {
        const auto traPointInfo = reinterpret_cast<const trackInfo*>(raw);
        info.type = TBDPointType;
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

        if (info.statMethod == 2) {
            LOG_INFO(QString("[Tbd2DispManager] Removal track received: batch=%1 range=%2 azimuthDeg=%3")
                     .arg(info.batch)
                     .arg(info.range, 0, 'f', 1)
                     .arg(info.azimuth, 0, 'f', 2));
        }

        RADAR_DATA_MGR.processTrack(info);
        emit tbdInfoProcess(info);
        raw += sizeof(trackInfo);
    }

    const qsizetype trailingBytes = end - raw;
    if (trailingBytes > 0 && trailingBytes != static_cast<qsizetype>(sizeof(ProtocolEnd))) {
        LOG_WARNING(QString("[Tbd2DispManager] Unexpected trailing bytes: %1")
                    .arg(trailingBytes));
    }

    if (shouldLogFrame) {
        LOG_INFO(QString("[Tbd2DispManager] Frame #%1 dispatched to Controller::tbdInfoProcess, tracks=%2")
                 .arg(m_frameCount)
                 .arg(trackCount));
    }
}

Tbd2DispManager::~Tbd2DispManager()
{
    thread->quit();
    thread->wait();
}
