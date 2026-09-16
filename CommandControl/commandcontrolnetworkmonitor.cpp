/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-16 17:23:11
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:29
 * @Description: 
 */
#include "commandcontrolnetworkmonitor.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStandardPaths>

namespace {

constexpr int kControlPeer = 0;
constexpr int kNtpServer = 1;

QString nameFor(int endpoint)
{
    return endpoint == kControlPeer ? QStringLiteral("对端主控 IP")
                                    : QStringLiteral("NTP 服务器 IP");
}

}  // namespace

CommandControlNetworkMonitor::CommandControlNetworkMonitor(QObject* parent)
    : QObject(parent)
{
    for (int endpoint = 0; endpoint < 2; ++endpoint) {
        m_ping[endpoint] = new QProcess(this);
        m_timeout[endpoint] = new QTimer(this);
        m_timeout[endpoint]->setSingleShot(true);
        connect(m_timeout[endpoint], &QTimer::timeout, this, [this, endpoint] {
            finishPing(endpoint, false, QStringLiteral("Ping 超时"));
            m_ping[endpoint]->kill();
        });
        connect(m_ping[endpoint], QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this, endpoint](int code, QProcess::ExitStatus exitStatus) {
            const QByteArray output = m_ping[endpoint]->readAllStandardOutput();
            const QString error = QString::fromLocal8Bit(m_ping[endpoint]->readAllStandardError())
                                      .trimmed().left(160);
            // Windows 的 ping 在“目标不可达”时也可能返回 0，必须同时确认回显 TTL。
            const bool reachable = exitStatus == QProcess::NormalExit && code == 0
                && output.toLower().contains("ttl=");
            finishPing(endpoint, reachable, error.isEmpty()
                ? QStringLiteral("无 ICMP 回显（设备可能禁用 Ping 或被防火墙拦截）") : error);
        });
        connect(m_ping[endpoint], &QProcess::errorOccurred, this,
                [this, endpoint](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                finishPing(endpoint, false, m_ping[endpoint]->errorString());
            }
        });
    }

    m_pollTimer.setSingleShot(false);
    connect(&m_pollTimer, &QTimer::timeout, this, &CommandControlNetworkMonitor::poll);
}

CommandControlNetworkMonitor::~CommandControlNetworkMonitor()
{
    stop();
}

void CommandControlNetworkMonitor::start(const CommandControlSettings& settings)
{
    stop();
    m_settings = settings;
    m_targets[kControlPeer] = m_settings.expectedControlIp.trimmed();
    m_targets[kNtpServer] = m_settings.timeServerIp.trimmed();
    poll();
    m_pollTimer.start(m_settings.networkStatusPollIntervalMs);
}

void CommandControlNetworkMonitor::stop()
{
    m_pollTimer.stop();
    for (int endpoint = 0; endpoint < 2; ++endpoint) {
        m_timeout[endpoint]->stop();
        m_pingRunning[endpoint] = false;
        if (m_ping[endpoint]->state() != QProcess::NotRunning) {
            m_ping[endpoint]->kill();
        }
    }
}

QString CommandControlNetworkMonitor::localInterfaceError() const
{
    const QHostAddress expected(m_settings.localIp);
    if (m_settings.localIp.trimmed().isEmpty() || expected.protocol() != QAbstractSocket::IPv4Protocol) {
        return QStringLiteral("本机 IP 未配置或格式无效");
    }

    bool found = false;
    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry& entry : interface.addressEntries()) {
            if (entry.ip() != expected) {
                continue;
            }
            found = true;
            const QNetworkInterface::InterfaceFlags flags = interface.flags();
            if (flags.testFlag(QNetworkInterface::IsUp) && flags.testFlag(QNetworkInterface::IsRunning)) {
                return QString();
            }
        }
    }
    return found ? QStringLiteral("对应网卡未启用或链路未连接")
                 : QStringLiteral("本机未配置该 IPv4 地址");
}

void CommandControlNetworkMonitor::poll()
{
    const QString localError = localInterfaceError();
    m_localReady = localError.isEmpty();
    emit statusChanged(LocalInterface, m_localReady,
                       QStringLiteral("本机 IP %1：%2").arg(m_settings.localIp,
                           m_localReady ? QStringLiteral("本机网卡正常") : localError));

    for (int endpoint = 0; endpoint < 2; ++endpoint) {
        if (!m_localReady) {
            publishPeerUnavailable(endpoint, QStringLiteral("本机 IP 不可用"));
        } else if (m_targets[endpoint].isEmpty()
                   || QHostAddress(m_targets[endpoint]).protocol() != QAbstractSocket::IPv4Protocol) {
            publishPeerUnavailable(endpoint, QStringLiteral("IP 未配置或格式无效"));
        } else {
            ping(endpoint);
        }
    }
}

void CommandControlNetworkMonitor::ping(int endpoint)
{
    if (m_pingRunning[endpoint] || m_ping[endpoint]->state() != QProcess::NotRunning) {
        return;
    }

    const QString program = QStandardPaths::findExecutable(QStringLiteral("ping"));
    if (program.isEmpty()) {
        m_pingRunning[endpoint] = true;
        finishPing(endpoint, false, QStringLiteral("未找到 ping 程序"));
        return;
    }

    m_pingRunning[endpoint] = true;
#ifdef Q_OS_WIN
    const QStringList arguments = {"-4", "-n", "1", "-w", "1000", "-S",
                                   m_settings.localIp, m_targets[endpoint]};
#else
    const QStringList arguments = {"-4", "-n", "-c", "1", "-W", "1", "-I",
                                   m_settings.localIp, m_targets[endpoint]};
#endif
    m_timeout[endpoint]->start(3000);
    m_ping[endpoint]->start(program, arguments);
}

void CommandControlNetworkMonitor::finishPing(int endpoint, bool reachable, const QString& reason)
{
    if (!m_pingRunning[endpoint]) {
        return;
    }
    m_pingRunning[endpoint] = false;
    m_timeout[endpoint]->stop();

    if (reachable) {
        m_consecutiveFailures[endpoint] = 0;
        emit statusChanged(endpoint + 1, true,
                           QStringLiteral("%1 %2：Ping 正常")
                               .arg(nameFor(endpoint), m_targets[endpoint]));
        return;
    }

    // 连续三次失败后才稳定显示红色，避免单次 ICMP 丢包造成状态闪烁。
    m_consecutiveFailures[endpoint] = qMin(3, m_consecutiveFailures[endpoint] + 1);
    const bool stableFailure = m_consecutiveFailures[endpoint] >= 3;
    emit statusChanged(endpoint + 1, false,
                       stableFailure
                           ? QStringLiteral("%1 %2：不可达（%3）")
                                 .arg(nameFor(endpoint), m_targets[endpoint], reason)
                           : QStringLiteral("%1 %2：检测中（失败 %3/3）")
                                 .arg(nameFor(endpoint), m_targets[endpoint])
                                 .arg(m_consecutiveFailures[endpoint]));
}

void CommandControlNetworkMonitor::publishPeerUnavailable(int endpoint, const QString& reason)
{
    m_consecutiveFailures[endpoint] = 0;
    if (m_pingRunning[endpoint]) {
        m_pingRunning[endpoint] = false;
        m_timeout[endpoint]->stop();
        m_ping[endpoint]->kill();
    }
    const QString target = m_targets[endpoint].isEmpty() ? QStringLiteral("未配置") : m_targets[endpoint];
    emit statusChanged(endpoint + 1, false,
                       QStringLiteral("%1 %2：%3").arg(nameFor(endpoint), target, reason));
}
