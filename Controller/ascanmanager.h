/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:16:28
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 11:21:01
 * @Description: 
 */
/**
 * @file ascanmanager.h
 * @brief A显数据 UDP 接收管理器
 * @details 监听信号处理分系统发送的 A显数据帧（AScanFrame, 消息ID 0xEE10），
 *          解析后以信号形式转发给 AScanWidget。
 *          使用独立 QUdpSocket 绑定本机端口，readyRead 直接解析原始帧。
 */
#ifndef ASCANMANAGER_H
#define ASCANMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include "Basic/Protocol.h"

/**
 * @class AScanManager
 * @brief A显数据 UDP 接收与解析
 *
 * 集成到 Controller：
 * @code
 * // controller.cpp init()
 * m_ascanMgr = new AScanManager(this);
 * m_ascanMgr->init(CF_INS.ip("DISP_CTRL_IP","192.168.64.4"), DISP_GET_SIG_PORT3);
 * // 通过 Controller 信号转发
 * connect(m_ascanMgr, &AScanManager::ascanReceived,
 *         this, &Controller::ascanReceived);
 * @endcode
 */
class AScanManager : public QObject
{
    Q_OBJECT
public:
    explicit AScanManager(QObject* parent = nullptr);
    ~AScanManager() override = default;

    /**
     * @brief 初始化 UDP 监听
     * @param localIp   本机 IP（显控网口）
     * @param localPort 本机监听端口（建议 DISP_GET_SIG_PORT3 = 8005）
     */
    void init(const QString& localIp, quint16 localPort);

signals:
    /**
     * @brief A显帧接收完成
     * @param frame  帧头结构体
     * @param pcAmps PC后幅度数据（dB），长度 = frame.pointNum
     * @param mtdAmps MTD后幅度数据（dB），长度 = frame.pointNum（可为空）
     */
    void ascanReceived(const AScanFrame& frame,
                       const QVector<float>& pcAmps,
                       const QVector<float>& mtdAmps);

    void logMessage(const QString& msg);

private slots:
    void onReadyRead();

private:
    void processData(const QByteArray& data);

    QUdpSocket* m_socket = nullptr;
};

#endif // ASCANMANAGER_H
