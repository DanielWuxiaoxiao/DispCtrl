/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-12-25 16:19:34
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
    // 基本长度校验：帧头+msgID+控制表(512)+雷达ID+数量字段
    const int kMinSize = static_cast<int>(sizeof(ProtocolFrame) + sizeof(unsigned short) + 512
                                          + sizeof(unsigned char) + sizeof(unsigned short));
    if (data.size() < kMinSize) {
        return;
    }

    const char* rawData = data.constData();
    // 跳过协议帧与消息ID
    rawData += sizeof(ProtocolFrame) + sizeof(unsigned short);
    // 跳过 512B 控制表并读取雷达ID
    rawData += 512;
    auto radarId = *reinterpret_cast<const unsigned char*>(rawData);
    Q_UNUSED(radarId);
    rawData += sizeof(unsigned char);
    // 读取点迹数量
    auto detNum = *reinterpret_cast<const unsigned short*>(rawData);
    rawData += sizeof(unsigned short);
    // 根据剩余长度修正 detNum，防止越界
    const auto remaining = data.size() - static_cast<int>(rawData - data.constData());
    const auto maxDet = remaining / static_cast<int>(sizeof(detInfo));
    if (detNum > maxDet) {
        detNum = static_cast<unsigned short>(maxDet);
    }
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

        // 使用新的统一数据管理器
        RADAR_DATA_MGR.processDetection(info);

        // 保持原有的信号发射以兼容现有代码
        emit detInfoProcess(info);

        rawData += sizeof(detInfo);
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
