/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-06-17
 * @Description: Shared edge terminal optical recognition result data
 */
#ifndef EDGERECOGNITIONRESULT_H
#define EDGERECOGNITIONRESULT_H

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtGlobal>

struct EdgeDetection
{
    QString className;
    double confidence = -1.0;
    QVector<int> bbox;
};

struct EdgeRecognitionResult
{
    QString trackId;
    bool isDrone = false;
    int count = 0;
    QVector<EdgeDetection> detections;
    double timestampSec = 0.0;
    qint64 receivedTimestampMs = 0;
    QString senderAddress;
    quint16 senderPort = 0;
    QByteArray rawPayload;
};

Q_DECLARE_METATYPE(EdgeRecognitionResult)

#endif // EDGERECOGNITIONRESULT_H
