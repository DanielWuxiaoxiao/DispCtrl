/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 16:58:32
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:49
 * @Description: 
 */
#ifndef GCSMANAGER_H
#define GCSMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QtGlobal>
#include "Basic/Protocol.h"

/**
 * @brief GCS链路管理器
 *
 * 负责与GCS地面站之间的UDP通信：
 *  - 接收并回复心跳帧（0xA4）
 *  - 向GCS发送目标下发帧（0x52）
 *
 * 典型用法：
 * @code
 *   GCSManager* mgr = new GCSManager(this);
 *   mgr->init();
 *   mgr->sendTargetAssignment(params);
 * @endcode
 */
class GCSManager : public QObject
{
    Q_OBJECT
public:
    explicit GCSManager(QObject* parent = nullptr);
    ~GCSManager() override;

    /**
     * @brief 初始化UDP套接字并绑定本地端口
     * @return 绑定成功返回true
     */
    bool init();

    /**
     * @brief 向GCS发送目标下发帧（命令0x52）
     * @param params GCS目标参数结构体
     */
    void sendTargetAssignment(const GcsTargetParams& params);

signals:
    /** @brief 收到GCS心跳帧时发出，已自动回复 */
    void heartbeatReceived();

    /** @brief 通信日志，供主界面输出调试信息 */
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();

private:
    /** @brief 构造完整GCS帧（含帧头+校验） */
    QByteArray buildFrame(quint8 src, quint8 dst, quint8 cmd, const QByteArray& params);

    /** @brief 解析收到的帧，校验合法性 */
    bool parseFrame(const QByteArray& data, quint8& outCmd, QByteArray& outParams);

    QUdpSocket*  m_socket  = nullptr;
    QHostAddress m_gcsHost;
    quint16      m_dstPort = 0;
    quint16      m_srcPort = 0;
    QHostAddress m_lastPeerHost;
    quint16      m_lastPeerPort = 0;
    qint64       m_lastHeartbeatLogMs = 0;
    quint32      m_suppressedHeartbeatCount = 0;
};

#endif // GCSMANAGER_H
