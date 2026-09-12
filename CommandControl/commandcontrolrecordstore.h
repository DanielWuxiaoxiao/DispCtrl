/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:45
 * @Description: 
 */
/* DDA4 会话记录与回放文件存储。 */
#ifndef COMMANDCONTROL_RECORDSTORE_H
#define COMMANDCONTROL_RECORDSTORE_H

#include "commandcontrolprotocol.h"

#include <QFile>
#include <QVector>

struct CommandControlRecord {
    bool outbound = false;
    qint64 observedUtcMs = 0;
    // 记录瞬间本机 DD05 阵面真值；DDA4 回放必须使用此原点，不能使用回放时的当前位置。
    bool hasReplayRadarOrigin = false;
    double replayRadarLongitudeDeg = 0.0;
    double replayRadarLatitudeDeg = 0.0;
    double replayRadarAltitudeM = 0.0;
    CommandControlProtocol::Dda4Track track;
    QByteArray packet;
};

Q_DECLARE_METATYPE(CommandControlRecord)
Q_DECLARE_METATYPE(QVector<CommandControlRecord>)

class CommandControlRecordStore final
{
public:
    ~CommandControlRecordStore();

    bool startSession(const QString& directoryPath, QString* errorMessage = nullptr);
    void close();
    bool append(const CommandControlRecord& record, QString* errorMessage = nullptr);
    QVector<CommandControlRecord> load(const QString& filePath, QString* errorMessage = nullptr) const;

    QString sessionFilePath() const { return m_sessionFile.fileName(); }
    bool isOpen() const { return m_sessionFile.isOpen(); }

private:
    QFile m_sessionFile;
};

#endif  // COMMANDCONTROL_RECORDSTORE_H
