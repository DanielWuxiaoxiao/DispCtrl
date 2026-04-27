/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 16:58:32
 * @Description: 
 */
/**
 * @file beamschedulemanager.h
 * @brief 波束调度报告 UDP 接收管理器
 * @details 监听资源调度分系统发送的波束调度报告帧（BeamScheduleReport, 消息ID 0xEE11），
 *          解析后以信号形式转发给 BeamScheduleWidget 显示甘特图。
 *          使用独立 QUdpSocket 绑定本机端口，直接解析原始帧格式。
 */
#ifndef BEAMSCHEDULEMANAGER_H
#define BEAMSCHEDULEMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include "Basic/Protocol.h"

/**
 * @class BeamScheduleManager
 * @brief 波束调度报告 UDP 接收与解析
 *
 * ### 典型用法
 * @code
 * m_beamMgr = new BeamScheduleManager(this);
 * m_beamMgr->init(CF_INS.ip("DISP_CTRL_IP","192.168.64.4"), DISP_GET_RES_PORT);
 * connect(m_beamMgr, &BeamScheduleManager::beamScheduleReceived,
 *         m_beamWidget, &BeamScheduleWidget::onBeamSchedule);
 * @endcode
 */
class BeamScheduleManager : public QObject
{
    Q_OBJECT
public:
    explicit BeamScheduleManager(QObject* parent = nullptr);
    ~BeamScheduleManager() override = default;

    /**
     * @brief 初始化 UDP 监听
     * @param localIp   本机 IP（显控网口，空字符串=AnyIPv4）
     * @param localPort 本机监听端口（建议 DISP_GET_RES_PORT = 8013）
     */
    void init(const QString& localIp, quint16 localPort);

signals:
    /**
     * @brief 波束调度报告接收完成
     * @param header  帧头结构体
     * @param slots   本帧所有波束时间槽
     */
    void beamScheduleReceived(const BeamScheduleReport& header,
                              const QVector<BeamSlot>&   beamSlots);

    void logMessage(const QString& msg);

private slots:
    void onReadyRead();

private:
    void processData(const QByteArray& data);

    QUdpSocket* m_socket = nullptr;
};

#endif // BEAMSCHEDULEMANAGER_H
