/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-15 19:03:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:29
 * @Description: 
 */
#include "ntptimesync.h"

#include "Basic/log.h"

#include <QHostAddress>
#include <QUdpSocket>

#include <cerrno>
#include <cstring>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <time.h>
#endif

namespace {

constexpr int kNtpPacketSize = 48;
constexpr int kNtpReceiveTimestampOffset = 32;
constexpr int kNtpTransmitTimestampOffset = 40;
constexpr qint64 kNtpUnixEpochOffsetSeconds = 2208988800LL;
constexpr qint64 kDd31MinResyncIntervalMs = 60000;

quint32 readBigEndianU32(const QByteArray& bytes, int offset)
{
    return (static_cast<quint32>(static_cast<quint8>(bytes.at(offset))) << 24)
        | (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 1))) << 16)
        | (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 2))) << 8)
        | static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 3)));
}

void writeBigEndianU32(QByteArray& bytes, int offset, quint32 value)
{
    bytes[offset] = static_cast<char>((value >> 24) & 0xFFU);
    bytes[offset + 1] = static_cast<char>((value >> 16) & 0xFFU);
    bytes[offset + 2] = static_cast<char>((value >> 8) & 0xFFU);
    bytes[offset + 3] = static_cast<char>(value & 0xFFU);
}

bool ntpTimestampToUtc(const QByteArray& packet, int offset, QDateTime& utc)
{
    const quint32 seconds = readBigEndianU32(packet, offset);
    const quint32 fraction = readBigEndianU32(packet, offset + 4);
    if (seconds < static_cast<quint32>(kNtpUnixEpochOffsetSeconds)) {
        return false;
    }
    const qint64 milliseconds = (static_cast<quint64>(fraction) * 1000ULL + 0x80000000ULL) >> 32;
    utc = QDateTime::fromMSecsSinceEpoch(
        (static_cast<qint64>(seconds) - kNtpUnixEpochOffsetSeconds) * 1000LL + milliseconds,
        Qt::UTC);
    return utc.isValid();
}

void writeNtpTimestamp(QByteArray& packet, int offset, const QDateTime& utc)
{
    const qint64 milliseconds = utc.toMSecsSinceEpoch();
    const qint64 secondsSinceUnixEpoch = milliseconds / 1000LL;
    const qint64 millisecondRemainder = milliseconds % 1000LL;
    const quint32 ntpSeconds = static_cast<quint32>(secondsSinceUnixEpoch + kNtpUnixEpochOffsetSeconds);
    const quint32 ntpFraction = static_cast<quint32>((static_cast<quint64>(millisecondRemainder) << 32) / 1000ULL);
    writeBigEndianU32(packet, offset, ntpSeconds);
    writeBigEndianU32(packet, offset + 4, ntpFraction);
}

bool setSystemUtc(const QDateTime& utc, QString& error)
{
    if (!utc.isValid()) {
        error = QStringLiteral("NTP 返回的 UTC 时间无效");
        return false;
    }

#if defined(Q_OS_WIN)
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        error = QStringLiteral("无法打开进程令牌，错误码=%1").arg(GetLastError());
        return false;
    }
    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    if (!LookupPrivilegeValue(nullptr, SE_SYSTEMTIME_NAME, &privileges.Privileges[0].Luid)) {
        error = QStringLiteral("无法查询 SeSystemtimePrivilege，错误码=%1").arg(GetLastError());
        CloseHandle(token);
        return false;
    }
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    if (!AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr)
        || GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        error = QStringLiteral("未获得修改系统时间权限，请以管理员身份运行，错误码=%1").arg(GetLastError());
        CloseHandle(token);
        return false;
    }
    const QDate date = utc.date();
    const QTime time = utc.time();
    SYSTEMTIME systemTime{};
    systemTime.wYear = static_cast<WORD>(date.year());
    systemTime.wMonth = static_cast<WORD>(date.month());
    systemTime.wDay = static_cast<WORD>(date.day());
    systemTime.wHour = static_cast<WORD>(time.hour());
    systemTime.wMinute = static_cast<WORD>(time.minute());
    systemTime.wSecond = static_cast<WORD>(time.second());
    systemTime.wMilliseconds = static_cast<WORD>(time.msec());
    const BOOL success = SetSystemTime(&systemTime);
    const DWORD errorCode = success ? ERROR_SUCCESS : GetLastError();
    CloseHandle(token);
    if (!success) {
        error = QStringLiteral("SetSystemTime 调用失败，错误码=%1").arg(errorCode);
        return false;
    }
    return true;
#elif defined(Q_OS_UNIX)
    const qint64 milliseconds = utc.toMSecsSinceEpoch();
    timespec value{};
    value.tv_sec = static_cast<time_t>(milliseconds / 1000LL);
    value.tv_nsec = static_cast<long>((milliseconds % 1000LL) * 1000000LL);
    if (clock_settime(CLOCK_REALTIME, &value) != 0) {
        error = QStringLiteral("clock_settime 调用失败：%1；请以 root 或具 CAP_SYS_TIME 权限运行")
                    .arg(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }
    return true;
#else
    error = QStringLiteral("当前平台未实现系统时间更新");
    return false;
#endif
}

}  // namespace

NtpTimeSync::NtpTimeSync(QObject* parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    m_periodicTimer.setSingleShot(false);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &NtpTimeSync::onTimeout);
    connect(&m_periodicTimer, &QTimer::timeout, this, &NtpTimeSync::onPeriodicTimeout);
}

void NtpTimeSync::start(const CommandControlSettings& settings)
{
    m_settings = settings;
    if (!m_settings.timeSyncEnabled) {
        setStatus(QStringLiteral("NTP 授时已由配置关闭"));
        return;
    }
    if (!m_socket) {
        m_socket = new QUdpSocket(this);
        connect(m_socket, &QUdpSocket::readyRead, this, &NtpTimeSync::processPendingDatagrams);
    }
    m_periodicTimer.start(m_settings.timeSyncIntervalMs);
    requestSync(m_settings.timeServerIp, QStringLiteral("启动总控通信前"));
}

void NtpTimeSync::requestSync(const QString& serverIp, const QString& reason)
{
    if (!m_settings.timeSyncEnabled) {
        return;
    }
    const QString requestedServer = serverIp.trimmed();
    const QHostAddress address(requestedServer);
    if (address.protocol() != QAbstractSocket::IPv4Protocol) {
        complete(false, QStringLiteral("NTP 服务器 IP 无效：%1").arg(requestedServer));
        return;
    }
    if (m_requestActive) {
        return;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (reason == QStringLiteral("收到 DD31 组播")
        && nowMs - m_lastRequestUtcMs < kDd31MinResyncIntervalMs) {
        return;
    }
    m_serverIp = requestedServer;
    m_reason = reason;
    m_remainingRetries = m_settings.timeSyncRetryCount;
    beginRequest();
}

void NtpTimeSync::requestManualSync()
{
    if (!m_settings.timeSyncEnabled) {
        setStatus(QStringLiteral("NTP 授时已由配置关闭，不能执行手动校时"));
        return;
    }
    if (m_requestActive) {
        setStatus(QStringLiteral("NTP 授时正在进行，请等待当前请求完成"));
        return;
    }
    requestSync(m_settings.timeServerIp, QStringLiteral("手动校时"));
}

void NtpTimeSync::beginRequest()
{
    if (!m_socket) {
        complete(false, QStringLiteral("NTP UDP 套接字未初始化"));
        return;
    }
    const QHostAddress address(m_serverIp);
    QByteArray request(kNtpPacketSize, '\0');
    // LI=0、Version=4、Mode=3(client)；NTP 线上字节序固定为大端。
    request[0] = static_cast<char>(0x23);
    m_requestUtc = QDateTime::currentDateTimeUtc();
    writeNtpTimestamp(request, kNtpTransmitTimestampOffset, m_requestUtc);
    m_lastRequestUtcMs = m_requestUtc.toMSecsSinceEpoch();
    m_requestActive = true;
    const qint64 written = m_socket->writeDatagram(request, address, m_settings.timeServerPort);
    if (written != request.size()) {
        complete(false, QStringLiteral("NTP 请求发送失败：%1").arg(m_socket->errorString()));
        return;
    }
    setStatus(QStringLiteral("NTP 授时中：%1:%2（%3）")
                  .arg(m_serverIp).arg(m_settings.timeServerPort).arg(m_reason));
    m_timeoutTimer.start(m_settings.timeSyncTimeoutMs);
}

void NtpTimeSync::processPendingDatagrams()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QHostAddress sender;
        quint16 senderPort = 0;
        QByteArray packet;
        packet.resize(static_cast<int>(m_socket->pendingDatagramSize()));
        if (m_socket->readDatagram(packet.data(), packet.size(), &sender, &senderPort) < 0) {
            continue;
        }
        if (!m_requestActive || sender.toString() != m_serverIp || senderPort != m_settings.timeServerPort) {
            continue;
        }
        if (packet.size() < kNtpPacketSize) {
            complete(false, QStringLiteral("NTP 应答长度异常：%1").arg(packet.size()));
            return;
        }
        const quint8 mode = static_cast<quint8>(packet.at(0)) & 0x07U;
        const quint8 stratum = static_cast<quint8>(packet.at(1));
        if ((mode != 4 && mode != 5) || stratum == 0 || stratum > 15) {
            complete(false, QStringLiteral("NTP 应答无效：mode=%1 stratum=%2").arg(mode).arg(stratum));
            return;
        }
        QDateTime serverReceiveUtc;
        QDateTime serverTransmitUtc;
        if (!ntpTimestampToUtc(packet, kNtpTransmitTimestampOffset, serverTransmitUtc)) {
            complete(false, QStringLiteral("NTP 应答缺少有效发送时间戳"));
            return;
        }
        const bool hasReceiveTimestamp = ntpTimestampToUtc(packet, kNtpReceiveTimestampOffset, serverReceiveUtc);
        const QDateTime localReceiveUtc = QDateTime::currentDateTimeUtc();
        const qint64 offsetMs = hasReceiveTimestamp
            ? (m_requestUtc.msecsTo(serverReceiveUtc) + localReceiveUtc.msecsTo(serverTransmitUtc)) / 2
            : localReceiveUtc.msecsTo(serverTransmitUtc);
        const QDateTime correctedUtc = localReceiveUtc.addMSecs(offsetMs);
        QString error;
        if (!setSystemUtc(correctedUtc, error)) {
            complete(false, QStringLiteral("NTP 已响应但系统时间未更新：%1").arg(error));
            return;
        }
        complete(true, QStringLiteral("NTP 授时成功：服务器=%1:%2，层级=%3，校正=%4ms，系统UTC=%5")
                           .arg(m_serverIp)
                           .arg(m_settings.timeServerPort)
                           .arg(stratum)
                           .arg(offsetMs)
                           .arg(correctedUtc.toString(Qt::ISODateWithMs)));
        return;
    }
}

void NtpTimeSync::onTimeout()
{
    if (!m_requestActive) {
        return;
    }
    if (m_remainingRetries-- > 0) {
        LOG_WARNING(QString("[CommandControl][NTP] %1:%2 无响应，剩余 %3 次重试")
                    .arg(m_serverIp).arg(m_settings.timeServerPort).arg(m_remainingRetries));
        beginRequest();
        return;
    }
    complete(false, QStringLiteral("NTP 授时超时：%1:%2").arg(m_serverIp).arg(m_settings.timeServerPort));
}

void NtpTimeSync::onPeriodicTimeout()
{
    requestSync(m_settings.timeServerIp, QStringLiteral("周期重校"));
}

void NtpTimeSync::complete(bool success, const QString& detail)
{
    m_timeoutTimer.stop();
    m_requestActive = false;
    setStatus(detail);
    if (success) {
        LOG_INFO(QString("[CommandControl][NTP] %1").arg(detail));
    } else {
        LOG_WARNING(QString("[CommandControl][NTP] %1").arg(detail));
    }
}

void NtpTimeSync::setStatus(const QString& status)
{
    if (m_statusText == status) {
        return;
    }
    m_statusText = status;
    emit statusChanged(status);
}
