#ifndef RAEDATASETREADER_H
#define RAEDATASETREADER_H

#include <QString>
#include <QVector>
#include "Basic/Protocol.h"

class RaeDatasetReader
{
public:
    struct Record {
        quint32 timestampMs = 0;
        quint8 pointType = static_cast<quint8>(PointType::Detection);
        PointInfo point{};
    };

    struct Result {
        bool ok = false;
        QString message;
        QVector<Record> records;
    };

    static Result loadFile(const QString& filePath, int maxRecords);
};

#endif // RAEDATASETREADER_H
