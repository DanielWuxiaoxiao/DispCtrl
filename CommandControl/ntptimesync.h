/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-15 19:03:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-16 21:32:29
 * @Description: 
 */
#ifndef COMMANDCONTROL_NTP_TIME_SYNC_H
#define COMMANDCONTROL_NTP_TIME_SYNC_H

#include "commandcontrolconfig.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

class QUdpSocket;

/*
 * 低频非阻塞 NTP 客户端。
 *
 * 网络收发依赖 QUdpSocket 的 readyRead 信号，绝不在 PPI/GUI 线程等待超时。
 * 成功收到标准 NTP 应答后，以四时间戳公式估计当前 UTC 并调用平台接口更新系统时间。
 */
class NtpTimeSync final : public QObject
{
    Q_OBJECT
public:
    explicit NtpTimeSync(QObject* parent = nullptr);

    void start(const CommandControlSettings& settings);
    void requestSync(const QString& serverIp, const QString& reason);
    void requestManualSync();
    bool isEnabled() const { return m_settings.timeSyncEnabled; }
    QString statusText() const { return m_statusText; }

signals:
    void statusChanged(const QString& status);

private slots:
    void processPendingDatagrams();
    void onTimeout();
    void onPeriodicTimeout();

private:
    void beginRequest();
    void complete(bool success, const QString& detail);
    void setStatus(const QString& status);

    CommandControlSettings m_settings;
    QUdpSocket* m_socket = nullptr;
    QTimer m_timeoutTimer;
    QTimer m_periodicTimer;
    QString m_serverIp;
    QString m_reason;
    QString m_statusText;
    QDateTime m_requestUtc;
    qint64 m_lastRequestUtcMs = 0;
    int m_remainingRetries = 0;
    bool m_requestActive = false;
};

#endif  // COMMANDCONTROL_NTP_TIME_SYNC_H
