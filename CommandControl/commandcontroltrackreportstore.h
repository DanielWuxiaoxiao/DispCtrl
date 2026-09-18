/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-18 23:22:44
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-18 23:42:18
 * @Description: 
 */
#ifndef COMMANDCONTROL_TRACKREPORTSTORE_H
#define COMMANDCONTROL_TRACKREPORTSTORE_H

#include <QFile>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

/*
 * 一行 JSONL 对应一次会话开始、一个源航迹点或一次会话结束。
 * 它记录的是显控收到的真实 TRAINFO 点，而不是只记录已经发出的 DDA4 点，
 * 因而可以保留“起批点到停止上报”的完整上下文。
 */
struct CommandControlTrackReportRecord {
    QString event;
    QString reportSessionId;
    QString reportMode;
    QString stopReason;
    quint32 sourceBatch = 0;
    quint64 pointIndex = 0;
    qint64 observedUtcMs = 0;
    qint64 sourceTrackStartedUtcMs = 0;
    qint64 reportingStartedUtcMs = 0;
    unsigned sourceType = 0;
    quint8 statMethod = 0;
    quint32 targetRecognition = 0;
    double targetConfidence = 0.0;
    quint8 radarId = 0;
    double rangeM = 0.0;
    double azimuthDeg = 0.0;
    double elevationDeg = 0.0;
    double snr = 0.0;
    double speedMps = 0.0;
    double relativeAltitudeM = 0.0;
    double amplitude = 0.0;
    bool radarOriginValid = false;
    double radarLongitudeDeg = 0.0;
    double radarLatitudeDeg = 0.0;
    double radarAltitudeM = 0.0;
    bool targetLlaValid = false;
    double targetLongitudeDeg = 0.0;
    double targetLatitudeDeg = 0.0;
    double targetAltitudeM = 0.0;
};

Q_DECLARE_METATYPE(CommandControlTrackReportRecord)

class CommandControlTrackReportStore final
{
public:
    ~CommandControlTrackReportStore();

    bool startSession(const QString& directoryPath, QString* errorMessage = nullptr);
    void close();
    bool append(const CommandControlTrackReportRecord& record, QString* errorMessage = nullptr);

    QString sessionFilePath() const { return m_sessionFile.fileName(); }
    bool isOpen() const { return m_sessionFile.isOpen(); }

private:
    QFile m_sessionFile;
};

#endif  // COMMANDCONTROL_TRACKREPORTSTORE_H
