/*
 * @Description: 激光侦察上报实现，见 laserreportmanager.h
 */
#include "laserreportmanager.h"

#include "Basic/ConfigManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStringList>
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

    if (m_radarHost.isNull() || m_laserHost.isNull() || m_udpPort == 0) {
        emit logMessage(QString("[LASER][INIT][ERROR] invalid 9009 endpoint local=%1 peer=%2:%3")
                            .arg(m_radarHost.toString()).arg(m_laserHost.toString()).arg(m_udpPort));
        m_enabled = false;
        return false;
    }

    m_socket = new QUdpSocket(this);
    // 严格使用雷达端9009作为源端口；绑定失败不能退化为系统随机端口发送。
    if (!m_socket->bind(m_radarHost, m_udpPort, QUdpSocket::ShareAddress)) {
        emit logMessage(QString("[LASER][INIT][ERROR] bind %1:%2 failed: %3；9009上报未启动")
                            .arg(m_radarHost.toString()).arg(m_udpPort)
                            .arg(m_socket->errorString()));
        m_socket->deleteLater();
        m_socket = nullptr;
        m_enabled = false;
        return false;
    }
    emit logMessage(QString("[LASER][INIT] bind local=%1:%2 ok")
                        .arg(m_radarHost.toString()).arg(m_udpPort));

    // 本版本只要求雷达→激光端9009出站，不连接 readyRead，激光端不能反向控制本机雷达。

    // RadarAPP当前状态帧默认携带一个扫描范围，并以跟踪模式上报。
    LaserScanRangeInfo defaultRange{};
    defaultRange.rangeID = 1;
    defaultRange.startAzimuth = 45.0f;
    defaultRange.endAzimuth = 135.0f;
    defaultRange.startElevation = 0.0f;
    defaultRange.endElevation = 60.0f;
    m_scanRanges = {defaultRange};

    // 1s 周期定时器：启用后常驻运行，每拍发状态帧(心跳)；有活动目标时附带侦察帧。
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &LaserReportManager::onReportTick);
    m_timer->start(m_intervalMs);
    onReportTick();  // 立即发一拍状态帧，建立心跳

    emit logMessage(QStringLiteral("[LASER][INIT] ready (9009状态心跳已启动，待右键引导光电跟踪后附带侦察帧)"));
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
            sendReconForTargets({last}, 1, QStringLiteral("CANCEL"));
        }
        emit logMessage(QString("[LASER][CANCEL] batch=%1 已消批，停止上报").arg(batch));
        stopReport();
    }
}

void LaserReportManager::setAutoReport(bool on)
{
    if (!m_enabled) return;
    if (m_autoReportEnabled == on) return;
    m_autoReportEnabled = on;
    emit logMessage(QString("[LASER][AUTO] 自动上报(全部目标,最多%1) %2")
                        .arg(LASER_MAX_TARGETS)
                        .arg(on ? QStringLiteral("已开启") : QStringLiteral("已关闭")));
    if (on) onReportTick();  // 立即发一拍
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
        emit logMessage(QString("[LASER][START] 开启对 batch=%1 的引导光电持续跟踪(9009)").arg(batch));
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
    emit logMessage(QString("[LASER][STOP] 停止 batch=%1 的引导光电持续跟踪（状态帧心跳继续）").arg(m_activeBatch));
    m_activeBatch = -1;
    // 注意：不停止定时器——状态帧心跳在模块启用期间始终保持
}

void LaserReportManager::onReportTick()
{
    if (!m_enabled || !m_socket) return;

    // 1) 状态帧（隐含心跳）：每拍都发，与是否有目标无关
    sendStatusFrame();

    // 2) 侦察帧：优先级——自动上报开启→全部目标(最多10)；否则→单目标(若有)
    if (m_autoReportEnabled) {
        QVector<PointInfo> targets;
        for (auto it = m_latest.cbegin(); it != m_latest.cend() && targets.size() < LASER_MAX_TARGETS; ++it) {
            targets.append(it.value());
        }
        // 自动模式即使无目标也发空侦察帧（协议允许 targetCount=0）
        sendReconForTargets(targets, 0, QStringLiteral("AUTO"));
    } else if (m_activeBatch >= 0 && m_latest.contains(m_activeBatch)) {
        sendReconForTargets({m_latest.value(m_activeBatch)}, 0, QStringLiteral("RECON"));
    }
}

void LaserReportManager::fillTargetInfo(LaserTargetInfo& t, const PointInfo& info, unsigned char cancelFlag)
{
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
}

QByteArray LaserReportManager::buildReconFrame(const QVector<PointInfo>& targets, unsigned char cancelFlag)
{
    const int count = qMin(targets.size(), LASER_MAX_TARGETS);

    // 内容域 = LaserReconData(12) + count×LaserTargetInfo(64)
    LaserReconData recon;
    std::memset(&recon, 0, sizeof(recon));
    recon.typeID      = LASER_RECON_TYPE_ID;
    recon.targetCount = static_cast<unsigned short>(count);
    recon.dataSeq     = ++m_dataSeq;
    // contentLen 仅统计目标域字节数（= targetCount × 64），与 RadarAPP 一致（不含12字节头）
    recon.contentLen  = static_cast<unsigned int>(count * sizeof(LaserTargetInfo));

    const unsigned short contentBytes =
        static_cast<unsigned short>(sizeof(LaserReconData) + count * sizeof(LaserTargetInfo));

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
    for (int i = 0; i < count; ++i) {
        LaserTargetInfo t;
        fillTargetInfo(t, targets.at(i), cancelFlag);
        frame.append(reinterpret_cast<const char*>(&t), sizeof(t));
    }

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

bool LaserReportManager::sendReconForTargets(const QVector<PointInfo>& targets, unsigned char cancelFlag, const QString& tag)
{
    if (!m_socket) return false;
    const QByteArray frame = buildReconFrame(targets, cancelFlag);
    const qint64 written = m_socket->writeDatagram(frame, m_laserHost, m_udpPort);
    if (written < 0) {
        emit logMessage(QString("[LASER][%1][ERROR] writeDatagram failed target=%2:%3 err=%4")
                            .arg(tag).arg(m_laserHost.toString()).arg(m_udpPort)
                            .arg(m_socket->errorString()));
        return false;
    }
    const int count = qMin(targets.size(), LASER_MAX_TARGETS);
    QStringList batches;
    for (int i = 0; i < count; ++i) batches << QString::number(targets.at(i).batch);
    emit logMessage(QString("[LASER][%1] seq=%2 count=%3 cancel=%4 batches=[%5] bytes=%6 -> %7:%8")
                        .arg(tag).arg(m_dataSeq).arg(count).arg(cancelFlag)
                        .arg(batches.join(','))
                        .arg(written)
                        .arg(m_laserHost.toString()).arg(m_udpPort));
    return true;
}

QByteArray LaserReportManager::buildStatusFrame()
{
    // 状态帧内容 = LaserStatusData(13)（无扫描区，scanAreaCount=0）
    LaserStatusData status;
    const unsigned short scanCount = static_cast<unsigned short>(m_scanRanges.size());
    std::memset(&status, 0, sizeof(status));
    status.typeID        = 1;            // 1=雷达设备状态
    status.statusSeq     = ++m_statusSeq;
    status.workState     = m_workState;  // 受激光端 0x0201 更新（默认0x0F全开）
    status.faultState    = m_faultState; // 默认全部正常
    status.workMode      = m_workMode;   // 默认搜索
    status.scanAreaCount = scanCount;    // 受激光端 0x0104 更新
    const unsigned short contentBytes = static_cast<unsigned short>(
        sizeof(LaserStatusData) + scanCount * sizeof(LaserScanRangeInfo));
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
    for (unsigned short i = 0; i < scanCount; ++i) {
        frame.append(reinterpret_cast<const char*>(&m_scanRanges[i]), sizeof(LaserScanRangeInfo));
    }

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
    // 与 RadarAPP 的分类映射保持一致；当前X576常规来源通常只给出0/1，
    // 但若后续识别链路提供行人/车辆/鸟/其它，也不应被错误降成普通目标。
    switch (info.targetRecResult) {
    case 1: return 1; // 无人机
    case 4: return 3; // 鸟
    case 2:            // 行人
    case 3:            // 车辆
    case 5: return 4; // 其它
    default: return 0; // 未知/普通
    }
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

float LaserReportManager::bswapFloat(float v)
{
    unsigned int u;
    std::memcpy(&u, &v, sizeof(u));
    u = ((u & 0x000000FFu) << 24) | ((u & 0x0000FF00u) << 8) |
        ((u & 0x00FF0000u) >> 8)  | ((u & 0xFF000000u) >> 24);
    float r;
    std::memcpy(&r, &u, sizeof(r));
    return r;
}

void LaserReportManager::onIncomingDatagram()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(static_cast<int>(m_socket->pendingDatagramSize()));
        m_socket->readDatagram(buf.data(), buf.size());
        parseControlFrame(buf);
    }
}

void LaserReportManager::parseControlFrame(const QByteArray& datagram)
{
    const int totalLen = datagram.size();
    if (totalLen < static_cast<int>(sizeof(LaserFrameHeader) + sizeof(LaserFrameTail))) return;

    const char* data = datagram.constData();
    if (static_cast<unsigned char>(data[0]) != LASER_FRAME_HEAD0 ||
        static_cast<unsigned char>(data[1]) != LASER_FRAME_HEAD1) return;

    LaserFrameHeader header;
    std::memcpy(&header, data, sizeof(header));

    const unsigned short payloadLen = header.dataLen;
    const int expectedLen = static_cast<int>(sizeof(LaserFrameHeader) + payloadLen + sizeof(LaserFrameTail));
    if (totalLen < expectedLen) return;

    LaserFrameTail tail;
    std::memcpy(&tail, data + sizeof(LaserFrameHeader) + payloadLen, sizeof(tail));
    if (static_cast<unsigned char>(tail.frameTail[0]) != LASER_FRAME_TAIL0 ||
        static_cast<unsigned char>(tail.frameTail[1]) != LASER_FRAME_TAIL1) return;

    // 校验和（frameType起按字节累加低16位）
    unsigned int sum = 0;
    const int checkLen = static_cast<int>(sizeof(LaserFrameHeader)) - 2 + payloadLen;
    for (int i = 2; i < 2 + checkLen; ++i) {
        sum += static_cast<unsigned char>(data[i]);
    }
    if (static_cast<unsigned short>(sum & 0xFFFF) != tail.checkSum) {
        emit logMessage(QStringLiteral("[LASER][CTRL][WARN] 校验和不符，丢弃"));
        return;
    }
    // 仅接受来自激光端(5100)的控制帧
    if (header.senderID != LASER_RECEIVER_ID) {
        return;  // 发送方应为激光端 5100
    }
    if (header.frameType != LASER_FT_CONTROL) {
        return;  // 仅处理控制帧
    }
    if (payloadLen < sizeof(LaserControlHeader)) return;

    const char* payload = data + sizeof(LaserFrameHeader);
    LaserControlHeader ctrlHdr;
    std::memcpy(&ctrlHdr, payload, sizeof(ctrlHdr));
    const char* ctrlContent = payload + sizeof(LaserControlHeader);
    const unsigned short ctrlContentLen = static_cast<unsigned short>(payloadLen - sizeof(LaserControlHeader));

    switch (ctrlHdr.controlType) {
    case LASER_CT_TIME_SYNC: {  // 0x0101 系统授时
        if (ctrlContentLen >= sizeof(LaserTimeSync)) {
            LaserTimeSync ts;
            std::memcpy(&ts, ctrlContent, sizeof(ts));
            emit logMessage(QString("[LASER][CTRL] 授时 seq=%1 %2-%3-%4 %5:%6:%7.%8")
                                .arg(ctrlHdr.cmdSeq)
                                .arg(2000 + ts.syncTime.year).arg(ts.syncTime.month).arg(ts.syncTime.day)
                                .arg(ts.syncTime.hour).arg(ts.syncTime.minute).arg(ts.syncTime.second)
                                .arg(ts.syncTime.msecond));
            emit laserTimeSyncCommand(ts.syncTime);
            sendControlResponse(ctrlHdr.cmdSeq, 1);
        }
        break;
    }
    case LASER_CT_WORK_STATE: {  // 0x0201 工作状态设置
        if (ctrlContentLen >= sizeof(LaserWorkStateSet)) {
            LaserWorkStateSet ws;
            std::memcpy(&ws, ctrlContent, sizeof(ws));
            m_workState = ws.workState;  // 反映到后续状态帧
            emit logMessage(QString("[LASER][CTRL] 工作状态设置 seq=%1 workState=0x%2")
                                .arg(ctrlHdr.cmdSeq)
                                .arg(ws.workState, 2, 16, QChar('0')));
            emit laserWorkStateCommand(ws.workState);
            sendControlResponse(ctrlHdr.cmdSeq, 1);
        }
        break;
    }
    case LASER_CT_SEARCH_RANGE: {  // 0x0104 搜索范围设置（float为大端）
        if (ctrlContentLen >= sizeof(LaserSearchRange)) {
            LaserSearchRange sr;
            std::memcpy(&sr, ctrlContent, sizeof(sr));
            sr.startAzimuth   = bswapFloat(sr.startAzimuth);
            sr.endAzimuth     = bswapFloat(sr.endAzimuth);
            sr.startElevation = bswapFloat(sr.startElevation);
            sr.endElevation   = bswapFloat(sr.endElevation);

            // 更新本地扫描范围列表（用于状态帧上报）
            if (sr.setFlag == 0) {
                // 取消：移除同 rangeID
                for (int i = m_scanRanges.size() - 1; i >= 0; --i) {
                    if (m_scanRanges[i].rangeID == sr.rangeID) m_scanRanges.removeAt(i);
                }
            } else {
                LaserScanRangeInfo info;
                info.rangeID        = sr.rangeID;
                info.startAzimuth   = sr.startAzimuth;
                info.endAzimuth     = sr.endAzimuth;
                info.startElevation = sr.startElevation;
                info.endElevation   = sr.endElevation;
                bool replaced = false;
                for (int i = 0; i < m_scanRanges.size(); ++i) {
                    if (m_scanRanges[i].rangeID == sr.rangeID) { m_scanRanges[i] = info; replaced = true; break; }
                }
                if (!replaced) m_scanRanges.append(info);
            }
            emit logMessage(QString("[LASER][CTRL] 搜索范围 seq=%1 setFlag=%2 id=%3 az[%4,%5] el[%6,%7]")
                                .arg(ctrlHdr.cmdSeq).arg(sr.setFlag).arg(sr.rangeID)
                                .arg(sr.startAzimuth, 0, 'f', 2).arg(sr.endAzimuth, 0, 'f', 2)
                                .arg(sr.startElevation, 0, 'f', 2).arg(sr.endElevation, 0, 'f', 2));
            emit laserSearchRangeCommand(sr);
            sendControlResponse(ctrlHdr.cmdSeq, 1);
        }
        break;
    }
    default:
        emit logMessage(QString("[LASER][CTRL][WARN] 未知控制类别 0x%1 seq=%2")
                            .arg(ctrlHdr.controlType, 4, 16, QChar('0')).arg(ctrlHdr.cmdSeq));
        sendControlResponse(ctrlHdr.cmdSeq, 0);
        break;
    }
}

void LaserReportManager::sendControlResponse(unsigned int cmdSeq, unsigned short result)
{
    if (!m_socket) return;

    LaserControlResponse resp;
    resp.cmdSeq = cmdSeq;
    resp.result = result;

    const unsigned short contentBytes = static_cast<unsigned short>(sizeof(LaserControlResponse));
    LaserFrameHeader hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.frameHead[0] = LASER_FRAME_HEAD0;
    hdr.frameHead[1] = LASER_FRAME_HEAD1;
    hdr.frameType    = LASER_FT_CTRL_RESP;
    hdr.senderID     = LASER_SENDER_ID;
    hdr.receiverID   = LASER_RECEIVER_ID;
    hdr.timeStamp    = nowLaserTime();
    hdr.dataLen      = contentBytes;

    QByteArray frame;
    frame.append(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    frame.append(reinterpret_cast<const char*>(&resp), sizeof(resp));
    unsigned int sum = 0;
    for (int i = 2; i < frame.size(); ++i) sum += static_cast<unsigned char>(frame.at(i));
    LaserFrameTail tail;
    tail.checkSum     = static_cast<unsigned short>(sum & 0xFFFF);
    tail.frameTail[0] = LASER_FRAME_TAIL0;
    tail.frameTail[1] = LASER_FRAME_TAIL1;
    frame.append(reinterpret_cast<const char*>(&tail), sizeof(tail));

    const qint64 written = m_socket->writeDatagram(frame, m_laserHost, m_udpPort);
    emit logMessage(QString("[LASER][CTRL_RESP] seq=%1 result=%2 bytes=%3 -> %4:%5")
                        .arg(cmdSeq).arg(result).arg(written)
                        .arg(m_laserHost.toString()).arg(m_udpPort));
}
