#include "raedatasetreader.h"

#include "Basic/offlinerae.h"
#include <QFile>
#include <QtMath>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr quint32 kRaeMagic = 0x31454152u; // "RAE1" little-endian
constexpr quint32 kRaeVersion = 1u;

#pragma pack(push, 1)
struct RaeFileHeader {
    quint32 magic;
    quint32 version;
    quint32 recordCount;
};

struct RaeRecordDisk {
    quint32 timestampMs;
    quint8 pointType;
    quint8 reserved0[3];
    quint32 batch;
    float rangeM;
    float azimuthDeg;
    float elevationDeg;
    float snr;
    float speed;
    float amp;
    quint8 targetRecResult;
    quint8 reserved1[3];
};
#pragma pack(pop)

static_assert(sizeof(RaeFileHeader) == 12, "Unexpected RAE header size");
static_assert(sizeof(RaeRecordDisk) == 40, "Unexpected RAE record size");

float normalizedAzimuth(float azimuth)
{
    float v = std::fmod(azimuth, 360.0f);
    if (v < 0.0f) {
        v += 360.0f;
    }
    return v;
}

bool validRecord(const RaeRecordDisk& rec)
{
    return std::isfinite(rec.rangeM) &&
           std::isfinite(rec.azimuthDeg) &&
           std::isfinite(rec.elevationDeg) &&
           rec.rangeM >= 0.0f &&
           rec.elevationDeg >= -90.0f &&
           rec.elevationDeg <= 90.0f;
}

quint8 sanitizeType(quint8 type)
{
    switch (type) {
        case PointType::Detection:
        case PointType::Track:
        case PointType::TBDPointType:
        case PointType::CooperativeTrackPointType:
            return type;
        default:
            return PointType::Detection;
    }
}

} // namespace

RaeDatasetReader::Result RaeDatasetReader::loadFile(const QString& filePath, int maxRecords)
{
    Result result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.message = QStringLiteral("无法打开RAE数据文件: %1 (%2)")
                             .arg(filePath, file.errorString());
        return result;
    }

    RaeFileHeader header{};
    if (file.read(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header)) {
        result.message = QStringLiteral("RAE数据文件头不完整: %1").arg(filePath);
        return result;
    }

    header.magic = qFromLittleEndian(header.magic);
    header.version = qFromLittleEndian(header.version);
    header.recordCount = qFromLittleEndian(header.recordCount);

    if (header.magic != kRaeMagic) {
        result.message = QStringLiteral("RAE数据文件magic无效: %1").arg(filePath);
        return result;
    }
    if (header.version != kRaeVersion) {
        result.message = QStringLiteral("RAE数据文件版本不支持: %1").arg(header.version);
        return result;
    }

    const qint64 payloadBytes = file.size() - static_cast<qint64>(sizeof(RaeFileHeader));
    const quint32 availableRecords = static_cast<quint32>(
        std::max<qint64>(0, payloadBytes) / static_cast<qint64>(sizeof(RaeRecordDisk)));
    const quint32 declaredRecords = std::min(header.recordCount, availableRecords);
    const quint32 cappedRecords = maxRecords > 0
        ? std::min<quint32>(declaredRecords, static_cast<quint32>(maxRecords))
        : declaredRecords;

    result.records.reserve(static_cast<int>(cappedRecords));

    int invalidCount = 0;
    for (quint32 i = 0; i < cappedRecords; ++i) {
        RaeRecordDisk disk{};
        if (file.read(reinterpret_cast<char*>(&disk), sizeof(disk)) != sizeof(disk)) {
            break;
        }

        disk.timestampMs = qFromLittleEndian(disk.timestampMs);
        disk.batch = qFromLittleEndian(disk.batch);
        // Float byte order is little-endian in the exported file. Windows/x86 is
        // little-endian, which is the supported deployment target for this tool.

        if (!validRecord(disk)) {
            ++invalidCount;
            continue;
        }

        PointInfo point{};
        point.type = sanitizeType(disk.pointType);
        point.range = disk.rangeM;
        point.azimuth = normalizedAzimuth(disk.azimuthDeg);
        point.elevation = disk.elevationDeg;
        point.SNR = std::isfinite(disk.snr) ? disk.snr : 0.0f;
        point.speed = std::isfinite(disk.speed) ? disk.speed : 0.0f;
        point.amp = std::isfinite(disk.amp) ? disk.amp : 0.0f;
        point.altitute = disk.rangeM * static_cast<float>(qSin(qDegreesToRadians(static_cast<double>(point.elevation))));
        point.batch = disk.batch;
        point.statMethod = OfflineRae::kStatMethod;
        point.targetRecResult = disk.targetRecResult;

        result.records.append({disk.timestampMs, static_cast<quint8>(point.type), point});
    }

    result.ok = !result.records.isEmpty();
    result.message = QStringLiteral("RAE数据加载完成: file=%1 records=%2 declared=%3 invalid=%4")
                         .arg(filePath)
                         .arg(result.records.size())
                         .arg(header.recordCount)
                         .arg(invalidCount);
    return result;
}
