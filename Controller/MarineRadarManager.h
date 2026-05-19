/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 15:27:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-19 10:10:44
 * @Description: 
 */
/**
 * @file MarineRadarManager.h
 * @brief 船用雷达UDP通信管理器
 * @details 负责：
 *   - 发送控制帧(16B)到伺服 (接口1)
 *   - 接收解析回波帧 (接口4)
 *   - 将解析结果以信号分发给渲染层
 */
#ifndef MARINERADARMANAGER_H
#define MARINERADARMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include "Basic/MarineProtocol.h"

/**
 * @class MarineRadarManager
 * @brief 船用雷达协议收发管理器
 *
 * 数据流:
 *   显控 ──(16B控制帧)──→ 伺服  (sendControl)
 *   伺服 ──(变长回波帧)──→ 显控  (echoLineReceived信号)
 */
class MarineRadarManager : public QObject
{
    Q_OBJECT
public:
    explicit MarineRadarManager(QObject* parent = nullptr);
    ~MarineRadarManager() override;

    /// 绑定接收端口, 设置发送目标
    void init(const QString& localIp, uint16_t localPort,
              const QString& servoIp, uint16_t servoPort);

    /// 发送控制帧
    void sendControl(const MarineControlFrame& frame);
    /// 仅更新当前控制帧缓存，不立即发送；用于启动时加载配置。
    void setCurrentControl(const MarineControlFrame& frame);

    // ---- 便捷发送 ----
    void setRange(uint8_t rangeVal);
    void setGain(uint8_t gain);
    void setSeaClutter(uint8_t val);
    void setRainClutter(uint8_t val);
    void setInterference(uint8_t val);
    void setLevel(uint8_t val);
    void setTxOn(bool on);
    void setServoSpeed(uint16_t speed);

    /// 获取当前控制帧(用于UI同步)
    const MarineControlFrame& currentControl() const { return m_ctrl; }

    /// 获取最新雷达状态(来自回波帧)
    const MarineRadarStatus& lastStatus() const { return m_status; }

    /// 控制帧发送周期(ms), 0=不自动发送
    void setAutoSendInterval(int ms);

signals:
    /**
     * @brief 收到一条扫描线回波
     * @param line 包含方位角和幅值数组
     */
    void echoLineReceived(const MarineEchoLine& line);

    /**
     * @brief 雷达状态更新(从回波帧状态字段)
     * @param status 最新状态
     */
    void radarStatusUpdated(const MarineRadarStatus& status);

    /**
     * @brief 日志消息
     */
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();
    void onAutoSend();

private:
    void parseEchoDatagram(const QByteArray& data);

    QUdpSocket* m_rxSocket = nullptr;   ///< 接收回波用
    QUdpSocket* m_txSocket = nullptr;   ///< 发送控制用
    QHostAddress m_servoAddr;
    uint16_t     m_servoPort = 0;

    MarineControlFrame m_ctrl;          ///< 当前控制帧
    MarineRadarStatus  m_status;        ///< 最新雷达状态

    QTimer* m_autoSendTimer = nullptr;
    uint32_t m_txCount = 0;             ///< 发送计数
    uint32_t m_rxDatagramCount = 0;     ///< 接收UDP包计数
    uint32_t m_rxEchoCount = 0;         ///< 有效回波帧计数
    uint32_t m_rxInvalidCount = 0;      ///< 无效/不完整帧计数
    bool m_rxDrainScheduled = false;    ///< 接收积压分片处理标记
};

#endif // MARINERADARMANAGER_H
