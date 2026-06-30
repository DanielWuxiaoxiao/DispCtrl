/*
 * @Description: 激光侦察上报（独立模块，仅“激光终端”功能）
 *  - 右键航迹“激光上报”→ 对该单一目标按周期(默认1s)持续发送侦察帧给激光控制终端；
 *  - 再次右键“关闭激光上报”或目标消批 → 停止（消批时补发一帧 cancelFlag=1）；
 *  - 每次“下发一个新目标”时把当时信息追加保存到本地 txt；
 *  - 由 config.toml [laser].enabled 控制；关闭时不创建、右键菜单也不出现该项；
 *  - 与其它上报/下发功能互不关联。协议见 docs/光电跟踪与激光上报协议.md 第4节。
 */
#ifndef LASERREPORTMANAGER_H
#define LASERREPORTMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <QHash>
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
    bool isReporting(int batch) const { return m_enabled && m_activeBatch == batch; }

public slots:
    void reportTrackPoint(const PointInfo& info);   ///< 缓存各批号最新航迹点（数据源）
    void removeTrackPoint(int batch);               ///< 消批：若为当前目标则补发消批帧并停止
    void startReport(int batch);                     ///< 右键开启：切到该目标持续上报，并存一条txt
    void stopReport();                              ///< 右键关闭：停止持续上报

signals:
    void logMessage(const QString& msg);

private slots:
    void onReportTick();                            ///< 周期发送当前目标侦察帧

private:
    QByteArray buildReconFrame(const PointInfo& info, unsigned char cancelFlag);
    QByteArray buildStatusFrame();                  ///< 状态帧(0x0200，隐含心跳)
    bool sendStatusFrame();
    bool sendFrame(const QByteArray& frame, const QString& tag, const PointInfo& info);
    void saveTxtRecord(const PointInfo& info);

    static LaserDataTime  nowLaserTime();
    static unsigned short mapTargetType(const PointInfo& info);
    static unsigned short mapTrackQuality(const PointInfo& info);
    static QString        targetTypeText(const PointInfo& info);
    static float          normalizedAzimuth(float azimuth);

    QUdpSocket*  m_socket = nullptr;
    QTimer*      m_timer  = nullptr;
    bool         m_enabled = false;
    QHostAddress m_radarHost;          ///< 雷达端本地绑定IP
    QHostAddress m_laserHost;          ///< 激光控制终端IP
    quint16      m_udpPort = 9009;     ///< 单端口双向
    int          m_intervalMs = 1000;
    bool         m_saveTxt = true;
    QString      m_saveDir = "LaserReportLog";

    int           m_activeBatch = -1;  ///< 当前持续上报的批号，-1=无
    unsigned int  m_dataSeq = 0;       ///< 侦察序号递增
    unsigned int  m_statusSeq = 0;     ///< 状态序号递增
    QHash<int, PointInfo> m_latest;    ///< 各批号最新航迹点缓存
};

#endif // LASERREPORTMANAGER_H
