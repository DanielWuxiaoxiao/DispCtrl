#ifndef RADAR_TRACK_API_H
#define RADAR_TRACK_API_H

// Protocol.h 遗留 pack(1) 不得影响 Qt/线程对象或本模块成员对齐。
#pragma pack(push, 8)
#include <QElapsedTimer>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <atomic>
#include <limits>
#include <memory>
#pragma pack(push, 1)
#include "Basic/Protocol.h"
#pragma pack(pop)
#include "Basic/wgs84coordinate.h"

class QNetworkAccessManager;
class QNetworkReply;
class QWidget;
class QLabel;
class QCheckBox;

namespace RadarTrackApi {

// 独立 HTTP 通路；令牌只在工作线程的请求头中使用，不进入诊断文本。
struct Settings {
    QUrl endpoint{QStringLiteral("http://api.csscuss3d.com/api/gateway/radar_track/receive")};
    QByteArray token;
    QString model = QStringLiteral("RADAR"), idPrefix = QStringLiteral("X576-");
    bool autoReport = false;
    bool logEnabled = true, logPayloadEnabled = false;
    int logIntervalMs = 5000;
    int reportIntervalMs = 1000, requestIntervalMs = 100, timeoutMs = 3000;
    int trackTimeoutMs = 10000, maxTracks = 1024;
};
Settings readSettings();

struct Motion {
    double speedMps = std::numeric_limits<double>::quiet_NaN(); // 原报文空间速度
    double headingDeg = std::numeric_limits<double>::quiet_NaN(); // 真北0°，顺时针
};

// 时间戳取显控接收该样本时的 UTC 毫秒；缺失/无效的运动量省略。
QByteArray payload(const QString& trackId, const PointInfo& point,
                   const Wgs84Coordinate::Lla& origin, qint64 receivedUtcMs, const QString& model,
                   const Motion& motion = {});

struct Request {
    QByteArray body;
    quint64 generation = 0;
    int batch = -1;
    qint64 expiresAt = 0; // QElapsedTimer 单调参考时刻，跨线程共享同一时钟。
};

class Transport final : public QObject {
    Q_OBJECT
public:
    Transport(const Settings& settings, std::shared_ptr<std::atomic<quint64>> generation);
    ~Transport() override;
public slots:
    void start();
    void send(const RadarTrackApi::Request& request);
signals:
    void completed(quint64 generation, int batch, bool ok, const QString& result);
private:
    void drain();
    void finish();
    Settings m_settings;
    std::shared_ptr<std::atomic<quint64>> m_generation;
    QNetworkAccessManager* m_manager = nullptr;
    QNetworkReply* m_reply = nullptr;
    QTimer* m_timeout = nullptr;
    Request m_request;
    QByteArray m_response;
    bool m_timedOut = false, m_tooLarge = false;
};

// 所有选批/坐标/窗口操作在 GUI 线程，网络请求仅由 Transport 工作线程执行。
class Module final : public QObject {
    Q_OBJECT
public:
    explicit Module(QObject* parent = nullptr);
    explicit Module(const Settings& settings, QObject* parent = nullptr);
    ~Module() override;
    void init();
    bool startReport(int batch);
    void stopReport(int batch);
    void stopAll();
    void setAutoReport(bool enabled);
    bool isAutoReport() const { return m_auto; }
    bool isManualReport(int batch) const { return m_manual.contains(batch); }
    bool isReporting(int batch) const;
    QString statusText() const;
    void showWindow(QWidget* parent);
public slots:
    // Controller 同一 GUI 回调先发布整帧，再按帧内顺序发布 PointInfo。
    void processTrackFrame(const QByteArray& frame);
    void processTrack(const PointInfo& point);
    void updateRadarPosition(double latitude, double longitude, double altitude);
signals:
    void logMessage(const QString& text);
    void reportingStateChanged();
    void submit(const RadarTrackApi::Request& request);
private:
    friend class ApiTest;
    struct Track {
        PointInfo point{};
        QString id;
        qint64 at = 0, utcMs = 0, lastSent = -1;
        Motion motion;
        double eastM = 0.0, northM = 0.0;
        quint64 sourceNs = 0;
        bool directionAnchor = false;
    };
    struct FrameSample {
        int batch = -1;
        float range = 0, azimuth = 0, elevation = 0, spaceSpeed = 0;
        uchar statMethod = 0;
        quint64 sourceNs = 0;
    };
    void tick();
    void invalidate();
    void refreshWindow();
    void logLimited(const QString& text);
    void writeLog(const QString& text);
    bool allowLog(const QString& category);
    Settings m_settings;
    QMap<int, Track> m_tracks;
    QVector<FrameSample> m_frameSamples; // 最多4096项，只保留当前帧，重复批号不互相覆盖。
    int m_frameCursor = 0;
    QSet<int> m_manual;
    QHash<QString, qint64> m_logs;
    Wgs84Coordinate::Lla m_origin;
    bool m_originValid = false, m_started = false, m_ready = false, m_auto = false, m_busy = false;
    bool m_lastOk = false;
    int m_cursor = -1;
    int m_inFlightBatch = -1;
    qint64 m_nextRequest = 0, m_lastUi = -1;
    quint64 m_sent = 0, m_success = 0, m_failure = 0;
    QString m_problem, m_lastResult = QStringLiteral("尚未请求");
    const std::shared_ptr<std::atomic<quint64>> m_generation = std::make_shared<std::atomic<quint64>>(1);
    QElapsedTimer m_clock;
    QTimer m_timer;
    QThread m_thread;
    QPointer<QWidget> m_window;
    QLabel* m_status = nullptr;
    QLabel* m_selection = nullptr;
    QCheckBox* m_autoBox = nullptr;
};
}
Q_DECLARE_METATYPE(RadarTrackApi::Request)
#pragma pack(pop)
#endif
