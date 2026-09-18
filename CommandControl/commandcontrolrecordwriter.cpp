/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-18 23:42:18
 * @Description: 
 */
#include "commandcontrolrecordwriter.h"

CommandControlRecordWriter::CommandControlRecordWriter(QObject* parent)
    : QObject(parent)
{
}

void CommandControlRecordWriter::startSessions(const QString& dda4DirectoryPath, bool dda4RecordEnabled,
                                               const QString& trackReportDirectoryPath, bool trackReportEnabled)
{
    if (dda4RecordEnabled) {
        QString error;
        const bool opened = m_store.startSession(dda4DirectoryPath, &error);
        emit sessionOpened(opened, opened ? m_store.sessionFilePath() : QString(), error);
    }
    if (trackReportEnabled) {
        QString error;
        const bool opened = m_trackReportStore.startSession(trackReportDirectoryPath, &error);
        emit trackReportSessionOpened(opened,
                                      opened ? m_trackReportStore.sessionFilePath() : QString(), error);
    }
}

void CommandControlRecordWriter::appendRecord(const CommandControlRecord& record)
{
    QString error;
    if (!m_store.append(record, &error)) {
        emit writeFailed(error);
    }
}

void CommandControlRecordWriter::appendTrackReportRecord(const CommandControlTrackReportRecord& record)
{
    QString error;
    if (!m_trackReportStore.append(record, &error)) {
        emit writeFailed(error);
    }
}

void CommandControlRecordWriter::loadRecords(const QString& filePath)
{
    QString error;
    emit recordsLoaded(m_store.load(filePath, &error), error);
}

void CommandControlRecordWriter::stop()
{
    m_store.close();
    m_trackReportStore.close();
}
