/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:45:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:54
 * @Description: 
 */
#include "commandcontrolrecordwriter.h"

CommandControlRecordWriter::CommandControlRecordWriter(QObject* parent)
    : QObject(parent)
{
}

void CommandControlRecordWriter::startSession(const QString& directoryPath)
{
    QString error;
    const bool opened = m_store.startSession(directoryPath, &error);
    emit sessionOpened(opened, opened ? m_store.sessionFilePath() : QString(), error);
}

void CommandControlRecordWriter::appendRecord(const CommandControlRecord& record)
{
    QString error;
    if (!m_store.append(record, &error)) {
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
}
