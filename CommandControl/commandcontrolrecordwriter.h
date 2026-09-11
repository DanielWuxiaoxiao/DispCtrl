/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:45:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
/*
 * DDA4 JSONL 记录专用工作对象。
 *
 * 该对象始终归属记录线程：追加记录按投递顺序写入并立即 flush；回放文件的读取也在
 * 同一线程完成。这样磁盘抖动不会占用 GUI/PPI 线程。
 */
#ifndef COMMANDCONTROL_RECORDWRITER_H
#define COMMANDCONTROL_RECORDWRITER_H

#include "commandcontrolrecordstore.h"

#include <QObject>

class CommandControlRecordWriter final : public QObject
{
    Q_OBJECT
public:
    explicit CommandControlRecordWriter(QObject* parent = nullptr);

public slots:
    void startSession(const QString& directoryPath);
    void appendRecord(const CommandControlRecord& record);
    void loadRecords(const QString& filePath);
    void stop();

signals:
    void sessionOpened(bool success, const QString& filePath, const QString& detail);
    void writeFailed(const QString& detail);
    void recordsLoaded(const QVector<CommandControlRecord>& records, const QString& detail);

private:
    CommandControlRecordStore m_store;
};

#endif  // COMMANDCONTROL_RECORDWRITER_H
