/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:47
 * @Description: 
 */
#include "data2dispmanager.h"
#include "UDP/threadudpsocket.h"
#include "Basic/log.h"
#include "controller.h"
#include "RadarDataManager.h"  // 雷达数据管理器头文件
#include <QPointer>
#include <QThread>

namespace {

bool decodeTrackFrame(const QByteArray& data, QList<PointInfo>& tracks)
{
    if (data.isEmpty()) {
        LOG_WARNING("[Data2DispManager] Empty datagram received");
        return false;
    }

    if (data.size() < static_cast<int>(sizeof(ProtocolFrame) + sizeof(TrackResult))) {
        LOG_WARNING(QString("[Data2DispManager] Datagram too small: size=%1, need>=%2")
                    .arg(data.size())
                    .arg(sizeof(ProtocolFrame) + sizeof(TrackResult)));
        return false;
    }

    const char* rawData = data.constData() + sizeof(ProtocolFrame);
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
        return false;
    }

    tracks.clear();
    tracks.reserve(traNum);

    for (int i = 0; i < traNum; ++i) {
        const auto traPointInfo = reinterpret_cast<const trackInfo*>(rawData);

        PointInfo info;
        info.type = PointType::Track;
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
        tracks.append(info);

        rawData += sizeof(trackInfo);
    }

    const qsizetype trailingBytes = end - rawData;
    if (trailingBytes > 0 && trailingBytes != static_cast<qsizetype>(sizeof(ProtocolEnd))) {
        LOG_WARNING(QString("[Data2DispManager] Unexpected trailing bytes: %1")
                    .arg(trailingBytes));
    }

    return true;
}

}  // namespace

Data2DispManager::Data2DispManager(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QList<PointInfo>>("QList<PointInfo>");

    src = CF_INS.id("DATA_PRO_ID",DATA_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP),CF_INS.port("DISP_GET_DATA_PORT",DISP_GET_DATA_PORT));
    socket->setSourceAndDestID(src, dst);

    thread = new QThread(this);
    socket->moveToThread(thread);
    connect(thread, &QThread::started, socket, &ThreadedUdpSocket::start);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    thread->start();
    host = QHostAddress(CF_INS.ip("DATA_PRO_IP",DATA_PRO_IP));
    port = CF_INS.port("DATA_PRO_2_DISP",DATA_PRO_2_DISP);

    LOG_INFO(QString("[Data2DispManager] listen=%1:%2 peer=%3:%4 mesID=0x%5")
             .arg(CF_INS.ip("DISP_CTRL_IP", DISP_CTRL_IP))
             .arg(CF_INS.port("DISP_GET_DATA_PORT", DISP_GET_DATA_PORT))
             .arg(host.toString())
             .arg(port)
             .arg(TRACK_INFO_MSG_ID, 4, 16, QChar('0')));

    QPointer<Data2DispManager> self(this);
    connect(socket, &ThreadedUdpSocket::traInfo, socket,
            [self](const QByteArray& data) {
        if (!self) {
            return;
        }

        QList<PointInfo> tracks;
        if (!decodeTrackFrame(data, tracks)) {
            return;
        }

        for (const PointInfo& info : tracks) {
            RADAR_DATA_MGR.processTrack(info);
        }

        QMetaObject::invokeMethod(self.data(), "dispatchTrackBatch", Qt::QueuedConnection,
                                  Q_ARG(QList<PointInfo>, tracks),
                                  Q_ARG(int, data.size()));
    });
    connect(socket, &ThreadedUdpSocket::servoCtrlRet, CON_INS, &Controller::servoCtrlRet);
    connect(socket, &ThreadedUdpSocket::bitReport, CON_INS, &Controller::onBITReport);
    connect(this,&Data2DispManager::traInfoProcess,CON_INS, &Controller::traInfoProcess);
}

void Data2DispManager::dispatchTrackBatch(const QList<PointInfo>& tracks, int packetSize)
{
    for (const PointInfo& info : tracks) {
        emit traInfoProcess(info);
    }

    static quint64 s_frameLogCount = 0;
    ++s_frameLogCount;
    const bool shouldLogFrame = (s_frameLogCount <= 3) || (s_frameLogCount % 100 == 0);

    if (!shouldLogFrame) {
        return;
    }

    if (!tracks.isEmpty()) {
        const PointInfo& first = tracks.first();
        const PointInfo& last = tracks.last();
        LOG_INFO(QString("[Data2DispManager] Decoded normal track frame: tracks=%1 size=%2 firstBatch=%3 lastBatch=%4 firstRange=%5 firstAz=%6 firstStat=%7 targetRec=%8")
                 .arg(tracks.size())
                 .arg(packetSize)
                 .arg(first.batch)
                 .arg(last.batch)
                 .arg(first.range, 0, 'f', 1)
                 .arg(first.azimuth, 0, 'f', 2)
                 .arg(first.statMethod)
                 .arg(first.targetRecResult));
    } else {
        LOG_INFO(QString("[Data2DispManager] Decoded normal track frame: tracks=0 size=%1")
                 .arg(packetSize));
    }
}

Data2DispManager::~Data2DispManager() {
    thread->quit();
    thread->wait();
}
