/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-06-29 23:29:30 -0700
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-20 18:52:01
 * @Description: 
 */
/*
 * @Description: 激光侦察上报（独立模块，仅“激光终端”功能）
 *  - 右键航迹“引导光电跟踪(持续)”→ 对该单一目标的每个 TRAINFO 新点发送侦察帧给激光控制终端；
 *  - 再次右键关闭或目标消批 → 停止（消批时补发一帧 cancelFlag=1）；
 *  - 每次“下发一个新目标”时把当时信息追加保存到本地 txt；
 *  - 由 config.toml [laser].enabled 控制；关闭时不创建、右键菜单也不出现该项；
 *  - 仅执行雷达→激光端的9009出站上报，不接收激光端控制指令；
 *  - 与其它上报/下发功能互不关联。协议见 docs/光电跟踪与激光上报协议.md 第4节。
 */
#ifndef LASERREPORTMANAGER_H
#define LASERREPORTMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QHash>
#include <QVector>
#include <QUdpSocket>
#include <QTimer>

#include "Basic/Protocol.h"

class LaserReportManager : public QObject
{
    Q_OBJECT
public:
    explicit LaserReportManager(QObject* parent = nullptr);
    ~LaserReportManager() override;

    /// 读取配置并绑定UDP；enabled=false 时返回 false（不启用）
    bool init();
    bool isEnabled() const { return m_enabled; }
    int  activeBatch() const { return m_activeBatch; }
    /// 当前是否正在对该批号持续上报（供右键菜单决定显示“开启/关闭”）
    bool isReporting(int batch) const { return m_activeBatch == batch; }
    /// 自动上报（模式1：周期上报全部目标，最多10个）是否开启
    bool isAutoReport() const { return m_autoReportEnabled; }
    /// 指定航迹当前是否因自动模式处于上报状态；离线时仅本地测试航迹有效。
    bool isAutoReportingTrack(int batch) const;
    /// 指定批号是否已有可立即下发的普通航迹最新点。
    bool hasTrack(int batch) const { return m_latest.contains(batch); }

public slots:
    void reportTrackPoint(const PointInfo& info);   ///< 缓存各批号最新航迹点（数据源）
    void removeTrackPoint(int batch);               ///< 消批：若为当前目标则补发消批帧并停止
    void startReport(int batch);                     ///< 右键开启：引导光电持续跟踪，并存一条txt
    void stopReport();                              ///< 右键关闭：停止单目标持续上报
    void setAutoReport(bool on);                     ///< 开/关 自动上报（模式1，全部目标）

signals:
    void logMessage(const QString& msg);
    // 激光端控制指令（解析+已回响应）转发信号，默认未连接（保持模块独立）；
    // 如需让指令真正驱动雷达，可在外部连接到对应控制逻辑。
    void laserTimeSyncCommand(const LaserDataTime& syncTime);    ///< 收到授时(0x0101)
    void laserWorkStateCommand(unsigned char workState);        ///< 收到工作状态(0x0201)
    void laserSearchRangeCommand(const LaserSearchRange& range); ///< 收到搜索范围(0x0104)
    void reportingStateChanged();

private slots:
    void onReportTick();                            ///< 周期：仅发状态帧心跳
    void onIncomingDatagram();                       ///< 接收激光端控制帧

private:
    QByteArray buildReconFrame(const QVector<PointInfo>& targets, unsigned char cancelFlag);
    QByteArray buildStatusFrame();                  ///< 状态帧(0x0200，隐含心跳)
    bool sendStatusFrame();
    bool sendReconForTargets(const QVector<PointInfo>& targets, unsigned char cancelFlag, const QString& tag);
    /// 航迹新点或操作切换时发送当前模式所需的侦察快照，不由心跳定时器重复旧点。
    void sendReconForCurrentState();
    void parseControlFrame(const QByteArray& datagram);  ///< 解析激光端控制帧并回响应
    void sendControlResponse(unsigned int cmdSeq, unsigned short result);
    void fillTargetInfo(LaserTargetInfo& t, const PointInfo& info, unsigned char cancelFlag);

    void saveTxtRecord(const PointInfo& info);
    bool isConfiguredTestTrack(int batch) const;

    static LaserDataTime  nowLaserTime();
    static unsigned short mapTargetType(const PointInfo& info);
    static unsigned short mapTrackQuality(const PointInfo& info);
    static QString        targetTypeText(const PointInfo& info);
    static float          normalizedAzimuth(float azimuth);
    static float          bswapFloat(float v);

    QUdpSocket*  m_socket = nullptr;
    QTimer*      m_timer  = nullptr;
    bool         m_enabled = false;
    QHostAddress m_radarHost;          ///< 雷达端本地绑定IP
    QHostAddress m_laserHost;          ///< 激光控制终端IP
    quint16      m_udpPort = 9009;     ///< 单端口双向
    int          m_intervalMs = 1000; ///< 9009 状态心跳发送周期，默认 1Hz
    int          m_heartbeatLogIntervalMs = 0; ///< 正常状态心跳日志周期；0=抑制
    qint64       m_lastHeartbeatLogMs = -1;
    int          m_reconLogIntervalMs = 30000; ///< 正常侦察帧日志周期；0=抑制
    qint64       m_lastReconLogMs = -1;
    quint32      m_suppressedReconLogCount = 0; ///< 上次正常侦察帧摘要后未打印的帧数
    bool         m_saveTxt = true;
    QString      m_saveDir = "LaserReportLog";

    int           m_activeBatch = -1;  ///< 单目标持续上报的批号，-1=无
    bool          m_autoReportEnabled = false; ///< 自动上报（全部目标，最多10）开关
    unsigned int  m_dataSeq = 0;       ///< 侦察序号递增
    unsigned int  m_statusSeq = 0;     ///< 状态序号递增
    unsigned char m_workState = 0x0F;  ///< 工作状态（受激光端0x0201更新，反映到状态帧）
    unsigned char m_faultState = 0x0F; ///< 故障状态（全部正常）
    unsigned char m_workMode = 1;      ///< 与 RadarAPP 保持一致：跟踪模式
    QVector<LaserScanRangeInfo> m_scanRanges; ///< 当前状态帧的默认扫描范围
    QHash<int, PointInfo> m_latest;    ///< 各批号最新航迹点缓存
};

#endif // LASERREPORTMANAGER_H
