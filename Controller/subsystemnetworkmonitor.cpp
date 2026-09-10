#include "subsystemnetworkmonitor.h"

#include "Basic/ConfigManager.h"

#include <QByteArray>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QtGlobal>

namespace {
constexpr int kRadar = 0;
constexpr int kLaser = 1;

QString utf8Text(const char* hex)
{
    const QByteArray bytes = QByteArray::fromHex(hex);
    return QString::fromUtf8(bytes.constData(), bytes.size());
}

QString nameFor(int endpoint)
{
    return endpoint == kRadar ? utf8Text("E998B5E99DA2") : utf8Text("E6BF80E58589");
}
}

SubsystemNetworkMonitor::SubsystemNetworkMonitor(QObject* parent)
    : QObject(parent)
{
    for (int endpoint = 0; endpoint < 2; ++endpoint) {
        m_ping[endpoint] = new QProcess(this);
        m_timeout[endpoint] = new QTimer(this);
        m_timeout[endpoint]->setSingleShot(true);
        connect(m_timeout[endpoint], &QTimer::timeout, this, [this, endpoint] {
            finishPing(endpoint, false, utf8Text("50494E47E8B685E697B6"));
            m_ping[endpoint]->kill();
        });
        connect(m_ping[endpoint], QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this, endpoint](int code, QProcess::ExitStatus exitStatus) {
            const QByteArray standardOutput = m_ping[endpoint]->readAllStandardOutput();
            const QString standardError = QString::fromLocal8Bit(
                m_ping[endpoint]->readAllStandardError()).trimmed().left(160);
            // Windows ping can exit zero on "destination unreachable"; require an actual echo reply.
            const bool reachable = exitStatus == QProcess::NormalExit && code == 0
                && standardOutput.toLower().contains("ttl=");
            finishPing(endpoint, reachable, standardError.isEmpty()
                ? utf8Text("E697A049434D50E5BA94E7AD94EFBC88E8B685E697B6E38081E4B88DE58FAFE8BEBEE68896E8A2ABE8BF87E6BBA4EFBC89") : standardError);
        });
        connect(m_ping[endpoint], &QProcess::errorOccurred, this,
                [this, endpoint](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                finishPing(endpoint, false, m_ping[endpoint]->errorString());
            }
        });
    }

    m_pollTimer.setInterval(qMax(1000, CF_INS.healthNetworkPollIntervalMs(5000)));
    connect(&m_pollTimer, &QTimer::timeout, this, &SubsystemNetworkMonitor::poll);
}

SubsystemNetworkMonitor::~SubsystemNetworkMonitor()
{
    for (QProcess* process : m_ping) {
        if (process && process->state() != QProcess::NotRunning) {
            process->kill();
        }
    }
}

void SubsystemNetworkMonitor::start()
{
    poll();
    if (CF_INS.healthNetworkEnabled(true)) {
        m_pollTimer.start();
    }
}

QString SubsystemNetworkMonitor::localInterfaceError(const QString& ip) const
{
    const QHostAddress wanted(ip);
    if (ip.trimmed().isEmpty() || wanted.isNull()) {
        return utf8Text("E69CACE69CBA4950E69CAAE9858DE7BDAEE68896E6A0BCE5BC8FE697A0E69588");
    }

    bool found = false;
    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry& entry : interface.addressEntries()) {
            if (entry.ip() != wanted) {
                continue;
            }
            found = true;
            const QNetworkInterface::InterfaceFlags flags = interface.flags();
            if (flags.testFlag(QNetworkInterface::IsUp) && flags.testFlag(QNetworkInterface::IsRunning)) {
                return QString();
            }
        }
    }
    return found ? utf8Text("E5AFB9E5BA94E7BD91E58DA1E69CAAE590AFE794A8E68896E993BEE8B7AFE69CAAE8BF9EE68EA5")
                 : utf8Text("E69CACE69CBAE69CAAE9858DE7BDAEE8AFA549507634E59CB0E59D80");
}

void SubsystemNetworkMonitor::poll()
{
    if (!CF_INS.healthNetworkEnabled(true)) {
        for (int endpoint = 0; endpoint < 2; ++endpoint) {
            emit statusChanged(endpoint, false,
                               utf8Text("E69CACE69CBA25314950EFBC9AE7BD91E7BB9CE6A380E6B58BE5B7B2E585B3E997AD").arg(nameFor(endpoint)));
            publishPeerUnavailable(endpoint, utf8Text("E7BD91E7BB9CE6A380E6B58BE5B7B2E585B3E997AD"));
        }
        return;
    }

    m_localIp[kRadar] = CF_INS.healthRadarLocalIp("192.168.64.4");
    m_peerIp[kRadar] = CF_INS.healthRadarPeerIp().trimmed();
    m_localIp[kLaser] = CF_INS.laserRadarIp("192.168.101.9");
    m_peerIp[kLaser] = CF_INS.laserCtrlIp("192.168.101.10").trimmed();

    for (int endpoint = 0; endpoint < 2; ++endpoint) {
        const QString localError = localInterfaceError(m_localIp[endpoint]);
        m_localReady[endpoint] = localError.isEmpty();
        emit statusChanged(endpoint, m_localReady[endpoint],
            utf8Text("E69CACE69CBA25314950202532EFBC9A2533")
                .arg(nameFor(endpoint), m_localIp[endpoint],
                     m_localReady[endpoint] ? utf8Text("E6ADA3E5B8B8") : localError));

        if (!m_localReady[endpoint]) {
            publishPeerUnavailable(endpoint, utf8Text("E69CACE69CBA4950E4B88DE58FAFE794A8"));
        } else if (m_peerIp[endpoint].isEmpty()) {
            publishPeerUnavailable(endpoint, utf8Text("E69CAAE9858DE7BDAEE5AFB9E7ABAF4950"));
        } else {
            ping(endpoint);
        }
    }
}

void SubsystemNetworkMonitor::ping(int endpoint)
{
    if (m_pingRunning[endpoint] || m_ping[endpoint]->state() != QProcess::NotRunning) {
        return;
    }

    const QString program = QStandardPaths::findExecutable(QStringLiteral("ping"));
    if (program.isEmpty()) {
        m_pingRunning[endpoint] = true;
        finishPing(endpoint, false, utf8Text("E689BEE4B88DE588B070696E67E7A88BE5BA8F"));
        return;
    }

    m_pingRunning[endpoint] = true;
#ifdef Q_OS_WIN
    const QStringList arguments = {"-4", "-n", "1", "-w", "1000", "-S",
                                   m_localIp[endpoint], m_peerIp[endpoint]};
#else
    const QStringList arguments = {"-4", "-n", "-c", "1", "-W", "1", "-I",
                                   m_localIp[endpoint], m_peerIp[endpoint]};
#endif
    m_timeout[endpoint]->start(3000);
    m_ping[endpoint]->start(program, arguments);
}

void SubsystemNetworkMonitor::finishPing(int endpoint, bool reachable, const QString& reason)
{
    if (!m_pingRunning[endpoint]) {
        return;
    }
    m_pingRunning[endpoint] = false;
    m_timeout[endpoint]->stop();

    if (reachable) {
        m_consecutiveFailures[endpoint] = 0;
        emit statusChanged(endpoint + 2, true,
            utf8Text("2531E8BF9EE68EA5202532EFBC9A50494E47E6ADA3E5B8B8")
                .arg(nameFor(endpoint), m_peerIp[endpoint]));
        return;
    }

    // 与 RadarAPP 保持相同的三次失败去抖，避免一次ICMP丢包把状态翻红。
    m_consecutiveFailures[endpoint] = qMin(3, m_consecutiveFailures[endpoint] + 1);
    const bool stableFailure = m_consecutiveFailures[endpoint] >= 3;
    emit statusChanged(endpoint + 2, false,
        stableFailure
            ? utf8Text("2531E8BF9EE68EA5202532EFBC9AE4B88DE58FAFE8BEBEEFBC882533EFBC89")
                  .arg(nameFor(endpoint), m_peerIp[endpoint], reason)
            : utf8Text("2531E8BF9EE68EA5202532EFBC9AE6A380E6B58BE4B8ADEFBC88E5A4B1E8B4A525332F33EFBC89")
                  .arg(nameFor(endpoint), m_peerIp[endpoint])
                  .arg(m_consecutiveFailures[endpoint]));
}

void SubsystemNetworkMonitor::publishPeerUnavailable(int endpoint, const QString& reason)
{
    m_consecutiveFailures[endpoint] = 0;
    if (m_pingRunning[endpoint]) {
        m_pingRunning[endpoint] = false;
        m_timeout[endpoint]->stop();
        m_ping[endpoint]->kill();
    }
    const QString peer = m_peerIp[endpoint].isEmpty() ? utf8Text("E69CAAE9858DE7BDAE") : m_peerIp[endpoint];
    emit statusChanged(endpoint + 2, false,
        utf8Text("2531E8BF9EE68EA5202532EFBC9A2533")
            .arg(nameFor(endpoint), peer, reason));
}
