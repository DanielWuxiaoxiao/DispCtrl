/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 17:09:15
 * @Description: 
 */
#include "sig2dispmanager.h"
#include "Basic/log.h"
#include "UDP/threadudpsocket.h"
#include "controller.h"
#include "RadarDataManager.h"  // 雷达数据管理器头文件
#include <QThread>
#include <optional>

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
    connect(this,&sig2dispmanager::headingUpdated,CON_INS, &Controller::updateHeadingFromCtrlTable);
}

sig2dispmanager2::sig2dispmanager2(QObject *parent) : QObject(parent)
{
    src = CF_INS.id("SIG_PRO_ID",SIG_PRO_ID);
    dst = CF_INS.id("DISP_CTRL_ID",DISP_CTRL_ID);
    // 第二路信号接收应使用独立端口，避免与 sig2dispmanager 共用 DISP_GET_SIG_PORT1
    socket = new ThreadedUdpSocket(CF_INS.ip("DISP_CTRL_IP",DISP_CTRL_IP), CF_INS.port("DISP_GET_SIG_PORT2",DISP_GET_SIG_PORT2));
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
    connect(socket, &ThreadedUdpSocket::geoLocationReport, CON_INS, &Controller::geoLocationUpdated);
}

void sig2dispmanager::detInfoDecode(QByteArray data)
{
    // 基本长度校验：帧头+msgID+控制表(512)+雷达ID+数量字段
    const int kMinSize = static_cast<int>(sizeof(ProtocolFrame) + sizeof(unsigned short) + 512
                                          + sizeof(unsigned char) + sizeof(unsigned short)
                                          + sizeof(ProtocolEnd));
    if (data.size() < kMinSize) {
        return;
    }

    const char* rawData = data.constData();
    // 跳过协议帧与消息ID
    rawData += sizeof(ProtocolFrame) + sizeof(unsigned short);
    // 解析 512B 控制表，提取航向角（阵面偏航，0.01°）
    const int controlLen = 512;
    const char* controlTable = rawData;

    auto extractHeadingDeg = [](const char* buf, int len) -> std::optional<double> {
        if (!buf || len < 2) return std::nullopt;
        // 在控制表中查找 0xAA04（工作模式/阵面偏航）结构
        for (int off = 0; off <= len - static_cast<int>(sizeof(ScanRange)); ++off) {

        }
        return std::nullopt;
    };

    if (auto heading = extractHeadingDeg(controlTable, controlLen)) {
        emit headingUpdated(*heading);
    }

    // 跳过控制表并读取雷达ID
    rawData += controlLen;
    auto radarId = *reinterpret_cast<const unsigned char*>(rawData);
    if (radarId < RADAR_ID_MIN || radarId > RADAR_ID_MAX) {
        LOG_WARNING(QString("[Sig2DispManager] Invalid radarId=%1 in detection frame")
                    .arg(radarId));
    }
    rawData += sizeof(unsigned char);
    // 读取点迹数量
    auto detNum = *reinterpret_cast<const unsigned short*>(rawData);
    rawData += sizeof(unsigned short);
    // 根据剩余长度修正 detNum，防止越界
    const char* end = data.constData() + data.size() - sizeof(ProtocolEnd);
    const auto remaining = end - rawData;
    const auto maxDet = remaining / static_cast<int>(sizeof(detInfo));
    if (detNum > maxDet) {
        detNum = static_cast<unsigned short>(maxDet);
    }
    const detInfo* firstDet = detNum > 0
        ? reinterpret_cast<const detInfo*>(rawData)
        : nullptr;
    PointInfo info;

    for(int i =0; i<detNum; i++)
    {
    auto detPointInfo = reinterpret_cast<const detInfo*>(rawData);

        info.type = PointType::Detection;
        info.range = detPointInfo->dis;
        info.azimuth = detPointInfo->azi;
        info.elevation = detPointInfo->ele;
        info.SNR = detPointInfo->CFARSNR;
        info.speed = detPointInfo->vel;
        info.altitute = detPointInfo->altitute;
        info.amp = detPointInfo->amp;
        info.targetConfidence = detPointInfo->targetConfidence;
        info.targetRecResult = detPointInfo->targetRecResult;
        info.radarId = radarId;

        // 使用新的统一数据管理器
        RADAR_DATA_MGR.processDetection(info);

        // 保持原有的信号发射以兼容现有代码
        emit detInfoProcess(info);

        rawData += sizeof(detInfo);
    }

    static quint64 s_detFrameLogCount = 0;
    ++s_detFrameLogCount;
    if (s_detFrameLogCount <= 3 || s_detFrameLogCount % 100 == 0) {
        LOG_INFO(QString("[Sig2DispManager] Decoded detection frame: radarId=%1 detections=%2 size=%3 firstConfidence=%4 firstTargetRec=%5")
                 .arg(radarId)
                 .arg(detNum)
                 .arg(data.size())
                 .arg(firstDet ? firstDet->targetConfidence : 0.0f, 0, 'f', 3)
                 .arg(firstDet ? firstDet->targetRecResult : 0));
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
