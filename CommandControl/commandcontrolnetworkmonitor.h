/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-16 17:23:11
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:29
 * @Description: 
 */
#ifndef COMMANDCONTROL_NETWORK_MONITOR_H
#define COMMANDCONTROL_NETWORK_MONITOR_H

#include "commandcontrolconfig.h"

#include <QObject>
#include <QProcess>
#include <QTimer>

/*
 * 低频异步网络监视。
 *
 * UDP 本身没有连接握手，因此主控与 NTP 的绿色仅表示从配置本机 IP 的 ICMP
 * 可达性。DD33/DD34 登录结果和 NTP 授时结果必须分别由协议状态显示。
 */
class CommandControlNetworkMonitor final : public QObject
{
    Q_OBJECT
public:
    enum StatusIndex {
        LocalInterface = 0,
        ControlPeer,
        NtpServer,
        StatusCount
    };

    explicit CommandControlNetworkMonitor(QObject* parent = nullptr);
    ~CommandControlNetworkMonitor() override;

    void start(const CommandControlSettings& settings);
    void stop();

signals:
    void statusChanged(int index, bool ok, const QString& text);

private:
    void poll();
    void ping(int endpoint);
    void finishPing(int endpoint, bool reachable, const QString& reason);
    QString localInterfaceError() const;
    void publishPeerUnavailable(int endpoint, const QString& reason);

    CommandControlSettings m_settings;
    QProcess* m_ping[2] = {nullptr, nullptr};
    QTimer* m_timeout[2] = {nullptr, nullptr};
    QTimer m_pollTimer;
    QString m_targets[2];
    bool m_localReady = false;
    bool m_pingRunning[2] = {false, false};
    int m_consecutiveFailures[2] = {0, 0};
};

#endif  // COMMANDCONTROL_NETWORK_MONITOR_H
