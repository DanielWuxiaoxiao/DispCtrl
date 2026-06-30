/*
 * @Description: 激光侦察上报实现，见 laserreportmanager.h
 */
#include "laserreportmanager.h"

#include "Basic/ConfigManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <cstring>

LaserReportManager::LaserReportManager(QObject* parent)
    : QObject(parent)
{
}

LaserReportManager::~LaserReportManager()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (m_socket) {
        m_socket->close();
    }
}

bool LaserReportManager::init()
{
    m_enabled = CF_INS.laserReportEnabled(false);
    if (!m_enabled) {
        emit logMessage(QStringLiteral("[LASER][INIT] disabled by config"));
        return false;
    }

    m_radarHost   = QHostAddress(CF_INS.laserRadarIp("192.168.101.9"));
    m_laserHost   = QHostAddress(CF_INS.laserCtrlIp("192.168.101.10"));
    m_udpPort     = CF_INS.laserUdpPort(9009);
    m_intervalMs  = qMax(100, CF_INS.laserReportIntervalMs(1000));
    m_saveTxt     = CF_INS.laserSaveTxt(true);
    m_saveDir     = CF_INS.laserSaveDir("LaserReportLog");

    emit logMessage(QString("[LASER][INIT] enabled radar=%1 laser=%2 port=%3 interval=%4ms saveTxt=%5 dir=%6")
                        .arg(m_radarHost.toString())
                        .arg(m_laserHost.toString())
                        .arg(m_udpPort)
                        .arg(m_intervalMs)
                        .arg(m_saveTxt)
                        .arg(m_saveDir));

    if (m_laserHost.isNull() || m_udpPort == 0) {
        emit logMessage(QString("[LASER][INIT][ERROR] invalid laser endpoint %1:%2")
                            .arg(m_laserHost.toString()).arg(m_udpPort));
        m_enabled = false;
        return false;
    }

    m_socket = new QUdpSocket(this);
    // 雷达端本地绑定（收发共用单端口）。绑定失败不致命：仍可用OS选端口发送。
    if (!m_radarHost.isNull()) {
        if (!m_socket->bind(m_radarHost, m_udpPort, QUdpSocket::ShareAddress)) {
            emit logMessage(QString("[LASER][INIT][WARN] bind %1:%2 failed: %3 （改用OS分配源地址继续）")
                                .arg(m_radarHost.toString()).arg(m_udpPort)
                                .arg(m_socket->errorString()));
        } else {
            emit logMessage(QString("[LASER][INIT] bind local=%1:%2 ok")
                                .arg(m_radarHost.toString()).arg(m_udpPort));
        }
    }

    // 1s 周期定时器：启用后常驻运行，每拍发状态帧(心跳)；有活动目标时附带侦察帧。
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &LaserReportManager::onReportTick);
    m_timer->start(m_intervalMs);
    onReportTick();  // 立即发一拍状态帧，建立心跳

    emit logMessage(QStringLiteral("[LASER][INIT] ready (周期状态帧心跳已启动，待右键开启目标后附带侦察帧)"));
    return true;
}

void LaserReportManager::reportTrackPoint(const PointInfo& info)
{
    if (!m_enabled) return;
    if (info.type != PointType::Track) return;   // 仅常规航迹
    m_latest.insert(static_cast<int>(info.batch), info);
}

void LaserReportManager::removeTrackPoint(int batch)
{
    if (!m_enabled) return;

    const bool wasActive = (batch == m_activeBatch);
    PointInfo last;
    const bool hadInfo = m_latest.contains(batch);
    if (hadInfo) last = m_latest.value(batch);
    m_latest.remove(batch);

    if (wasActive) {
        // 消批：若有最后已知点，补发一帧 cancelFlag=1 通知激光端目标消失
        if (hadInfo && m_socket) {
            sendFrame(buildReconFrame(last, 1), QStringLiteral("CANCEL"), last);
        }
        emit logMessage(QString("[LASER][CANCEL] batch=%1 已消批，停止上报").arg(batch));
        stopReport();
    }
}

void LaserReportManager::startReport(int batch)
{
    if (!m_enabled) return;

    if (m_activeBatch == batch) {
        return;  // 已在上报该目标
    }

    const int prev = m_activeBatch;
    m_activeBatch = batch;
    if (prev >= 0) {
        emit logMessage(QString("[LASER][SWITCH] 由 batch=%1 切换到 batch=%2").arg(prev).arg(batch));
    } else {
        emit logMessage(QString("[LASER][START] 开启对 batch=%1 的持续激光上报").arg(batch));
    }

    // 每次“下发一个新目标”保存一条 txt（用当前缓存的最新点）
    if (m_latest.contains(batch)) {
        saveTxtRecord(m_latest.value(batch));
    } else {
        emit logMessage(QString("[LASER][START][WARN] batch=%1 暂无缓存航迹点，txt记录将待首帧后由上报日志体现").arg(batch));
    }

    // 定时器在 init() 中已常驻运行（状态帧心跳）；此处立即发一拍，附带该目标侦察帧即时反馈
    onReportTick();
}

void LaserReportManager::stopReport()
{
    if (m_activeBatch < 0) return;
    emit logMessage(QString("[LASER][STOP] 停止 batch=%1 的激光上报（状态帧心跳继续）").arg(m_activeBatch));
    m_activeBatch = -1;
    // 注意：不停止定时器——状态帧心跳在模块启用期间始终保持
}

void LaserReportManager::onReportTick()
{
    if (!m_enabled || !m_socket) return;

    // 1) 状态帧（隐含心跳）：每拍都发，与是否有目标无关
    sendStatusFrame();

    // 2) 侦察帧：仅当有活动目标且有缓存航迹点时附带
    if (m_activeBatch < 0) return;
    if (!m_latest.contains(m_activeBatch)) {
        return;  // 目标暂无新点，本周期跳过侦察帧
    }
    const PointInfo info = m_latest.value(m_activeBatch);
    sendFrame(buildReconFrame(info, 0), QStringLiteral("RECON"), info);
}

bool LaserReportManager::sendFrame(const QByteArray& frame, const QString& tag, const PointInfo& info)
{
    if (!m_socket) return false;
    const qint64 written = m_socket->writeDatagram(frame, m_laserHost, m_udpPort);
    if (written < 0) {
        emit logMessage(QString("[LASER][%1][ERROR] writeDatagram failed target=%2:%3 err=%4")
                            .arg(tag).arg(m_laserHost.toString()).arg(m_udpPort)
                            .arg(m_socket->errorString()));
        return false;
    }
    emit logMessage(QString("[LASER][%1] seq=%2 batch=%3 dist=%4 az=%5 el=%6 spd=%7 type=%8 q=%9 bytes=%10 -> %11:%12")
                        .arg(tag)
                        .arg(m_dataSeq)
                        .arg(info.batch)
                        .arg(static_cast<double>(info.range), 0, 'f', 1)
                        .arg(static_cast<double>(normalizedAzimuth(info.azimuth)), 0, 'f', 2)
                        .arg(static_cast<double>(info.elevation), 0, 'f', 2)
                        .arg(static_cast<double>(info.speed), 0, 'f', 1)
                        .arg(mapTargetType(info))
                        .arg(mapTrackQuality(info))
                        .arg(written)
                        .arg(m_laserHost.toString()).arg(m_udpPort));
    return true;
}

QByteArray LaserReportManager::buildReconFrame(const PointInfo& info, unsigned char cancelFlag)
{
    // 内容域 = LaserReconData(12) + 1×LaserTargetInfo(64)
    LaserReconData recon;
    std::memset(&recon, 0, sizeof(recon));
    recon.typeID      = LASER_RECON_TYPE_ID;
    recon.targetCount = 1;
    recon.dataSeq     = ++m_dataSeq;
    // contentLen 仅统计目标域字节数（= targetCount × 64），与 RadarAPP 设备端一致（不含12字节ReconData头）
    recon.contentLen  = static_cast<unsigned int>(sizeof(LaserTargetInfo));

    LaserTargetInfo t;
    std::memset(&t, 0, sizeof(t));
    t.batchID        = info.batch;
    t.targetTime     = nowLaserTime();
    t.distance       = info.range;
    t.azimuth        = normalizedAzimuth(info.azimuth);
    t.elevation      = info.elevation;
    t.speed          = info.speed;
    t.azimuthSpeed   = 0.0f;
    t.elevationSpeed = 0.0f;
    t.radialSpeed    = info.speed;
    t.cancelFlag     = cancelFlag;
    t.targetType     = mapTargetType(info);
    t.trackQuality   = mapTrackQuality(info);

    const unsigned short contentBytes =
        static_cast<unsigned short>(sizeof(LaserReconData) + sizeof(LaserTargetInfo));

    LaserFrameHeader hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.frameHead[0] = LASER_FRAME_HEAD0;
    hdr.frameHead[1] = LASER_FRAME_HEAD1;
    hdr.frameType    = LASER_FT_RECON;
    hdr.senderID     = LASER_SENDER_ID;
    hdr.receiverID   = LASER_RECEIVER_ID;
    hdr.timeStamp    = nowLaserTime();
    hdr.dataLen      = contentBytes;

    QByteArray frame;
    frame.append(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    frame.append(reinterpret_cast<const char*>(&recon), sizeof(recon));
    frame.append(reinterpret_cast<const char*>(&t), sizeof(t));

    // 校验和：从 frameType 起（跳过2字节帧头）到数据内容止，按字节累加取低16位
    unsigned int sum = 0;
    for (int i = 2; i < frame.size(); ++i) {
        sum += static_cast<unsigned char>(frame.at(i));
    }
    LaserFrameTail tail;
    tail.checkSum     = static_cast<unsigned short>(sum & 0xFFFF);
    tail.frameTail[0] = LASER_FRAME_TAIL0;
    tail.frameTail[1] = LASER_FRAME_TAIL1;
    frame.append(reinterpret_cast<const char*>(&tail), sizeof(tail));

    return frame;
}

QByteArray LaserReportManager::buildStatusFrame()
{
    // 状态帧内容 = LaserStatusData(13)（无扫描区，scanAreaCount=0）
    LaserStatusData status;
    std::memset(&status, 0, sizeof(status));
    status.typeID        = 1;            // 1=雷达设备状态
    status.statusSeq     = ++m_statusSeq;
    status.workState     = 0x0F;         // 阵面全开（本工程不管理控制指令，固定缺省）
    status.faultState    = 0x0F;         // 全部正常
    status.workMode      = 0;            // 搜索
    status.scanAreaCount = 0;            // 不下发扫描区
    const unsigned short contentBytes = static_cast<unsigned short>(sizeof(LaserStatusData));
    status.contentLen    = static_cast<unsigned short>(
        contentBytes - sizeof(status.typeID) - sizeof(status.contentLen));

    LaserFrameHeader hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.frameHead[0] = LASER_FRAME_HEAD0;
    hdr.frameHead[1] = LASER_FRAME_HEAD1;
    hdr.frameType    = LASER_FT_STATUS;
    hdr.senderID     = LASER_SENDER_ID;
    hdr.receiverID   = LASER_RECEIVER_ID;
    hdr.timeStamp    = nowLaserTime();
    hdr.dataLen      = contentBytes;

    QByteArray frame;
    frame.append(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    frame.append(reinterpret_cast<const char*>(&status), sizeof(status));

    unsigned int sum = 0;
    for (int i = 2; i < frame.size(); ++i) {
        sum += static_cast<unsigned char>(frame.at(i));
    }
    LaserFrameTail tail;
    tail.checkSum     = static_cast<unsigned short>(sum & 0xFFFF);
    tail.frameTail[0] = LASER_FRAME_TAIL0;
    tail.frameTail[1] = LASER_FRAME_TAIL1;
    frame.append(reinterpret_cast<const char*>(&tail), sizeof(tail));

    return frame;
}

bool LaserReportManager::sendStatusFrame()
{
    if (!m_socket) return false;
    const QByteArray frame = buildStatusFrame();
    const qint64 written = m_socket->writeDatagram(frame, m_laserHost, m_udpPort);
    if (written < 0) {
        emit logMessage(QString("[LASER][STATUS][ERROR] writeDatagram failed target=%1:%2 err=%3")
                            .arg(m_laserHost.toString()).arg(m_udpPort).arg(m_socket->errorString()));
        return false;
    }
    // 心跳日志较频繁，降为概要（每帧一条），需要时可在配置里关日志
    emit logMessage(QString("[LASER][STATUS] seq=%1 bytes=%2 -> %3:%4 (heartbeat)")
                        .arg(m_statusSeq).arg(written)
                        .arg(m_laserHost.toString()).arg(m_udpPort));
    return true;
}

void LaserReportManager::saveTxtRecord(const PointInfo& info)
{
    if (!m_saveTxt) return;

    QDir dir(QDir::current());
    if (!dir.exists(m_saveDir)) {
        dir.mkpath(m_saveDir);
    }
    const QDateTime now = QDateTime::currentDateTime();
    const QString filePath = dir.filePath(
        QString("%1/LaserReport_CN_%2.txt").arg(m_saveDir, now.toString("yyyyMMdd")));

    QFile file(filePath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        emit logMessage(QString("[LASER][TXT][ERROR] 无法写入 %1: %2")
                            .arg(filePath, file.errorString()));
        return;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QString("时间：%1\n").arg(now.toString("yyyy-MM-dd HH:mm:ss.zzz"));
    out << QString("目标批次：%1\n").arg(info.batch);
    out << QString("目标类型：%1\n").arg(targetTypeText(info));
    out << QString("距离(m)：%1\n").arg(static_cast<double>(info.range), 0, 'f', 2);
    out << QString("方位(deg)：%1\n").arg(static_cast<double>(normalizedAzimuth(info.azimuth)), 0, 'f', 2);
    out << QString("俯仰(deg)：%1\n").arg(static_cast<double>(info.elevation), 0, 'f', 2);
    out << QString("速度(m/s)：%1\n").arg(static_cast<double>(info.speed), 0, 'f', 2);
    out << "----------------------------------------\n";
    file.close();

    emit logMessage(QString("[LASER][TXT] 已保存下发记录 batch=%1 -> %2").arg(info.batch).arg(filePath));
}

LaserDataTime LaserReportManager::nowLaserTime()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDate d = now.date();
    const QTime tm = now.time();
    LaserDataTime t;
    t.year    = static_cast<unsigned char>(d.year() % 100);
    t.month   = static_cast<unsigned char>(d.month());
    t.day     = static_cast<unsigned char>(d.day());
    t.hour    = static_cast<unsigned char>(tm.hour());
    t.minute  = static_cast<unsigned char>(tm.minute());
    t.second  = static_cast<unsigned char>(tm.second());
    t.msecond = static_cast<unsigned short>(tm.msec());
    return t;
}

unsigned short LaserReportManager::mapTargetType(const PointInfo& info)
{
    // 雷达侧仅有 targetRecResult(0其它/1无人机)；映射到激光协议类型 0普通/1无人机
    return (info.targetRecResult == 1) ? 1 : 0;
}

unsigned short LaserReportManager::mapTrackQuality(const PointInfo& info)
{
    // statMethod: 0(滤波)→7；1(外推)→3；其它→0
    switch (info.statMethod) {
    case 0:  return 7;
    case 1:  return 3;
    default: return 0;
    }
}

QString LaserReportManager::targetTypeText(const PointInfo& info)
{
    return (info.targetRecResult == 1) ? QStringLiteral("无人机") : QStringLiteral("其它");
}

float LaserReportManager::normalizedAzimuth(float azimuth)
{
    float v = std::fmod(azimuth, 360.0f);
    if (v < 0.0f) v += 360.0f;
    return v;
}
