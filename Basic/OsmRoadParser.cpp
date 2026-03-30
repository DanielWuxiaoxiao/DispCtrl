/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 16:40:21
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 22:07:37
 * @Description: 
 */
/**
 * @file OsmRoadParser.cpp
 * @brief OSM道路数据解析器实现
 * @details 支持WGS84→GCJ-02坐标转换和道路插值加密
 */
#include "OsmRoadParser.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QHash>
#include <QDebug>
#include <QtMath>

// ===== GCJ-02 坐标转换常量（克拉索夫斯基椭球参数）=====
static const double kGcjA  = 6378245.0;              // 长半轴
static const double kGcjEE = 0.00669342162296594323;  // 偏心率平方

bool OsmRoadParser::outOfChina(double lon, double lat)
{
    return !(lon > 73.66 && lon < 135.05 && lat > 3.86 && lat < 53.55);
}

double OsmRoadParser::transformLat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y
                 + 0.1 * x * y + 0.2 * qSqrt(qAbs(x));
    ret += (20.0 * qSin(6.0 * x * M_PI) + 20.0 * qSin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * qSin(y * M_PI) + 40.0 * qSin(y / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (160.0 * qSin(y / 12.0 * M_PI) + 320.0 * qSin(y * M_PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

double OsmRoadParser::transformLon(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y
                 + 0.1 * qSqrt(qAbs(x));
    ret += (20.0 * qSin(6.0 * x * M_PI) + 20.0 * qSin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * qSin(x * M_PI) + 40.0 * qSin(x / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (150.0 * qSin(x / 12.0 * M_PI) + 300.0 * qSin(x / 30.0 * M_PI)) * 2.0 / 3.0;
    return ret;
}

void OsmRoadParser::wgs84ToGcj02(double wgsLon, double wgsLat,
                                  double& gcjLon, double& gcjLat)
{
    if (outOfChina(wgsLon, wgsLat)) {
        gcjLon = wgsLon;
        gcjLat = wgsLat;
        return;
    }
    double dLat = transformLat(wgsLon - 105.0, wgsLat - 35.0);
    double dLon = transformLon(wgsLon - 105.0, wgsLat - 35.0);
    double radLat = wgsLat / 180.0 * M_PI;
    double magic  = qSin(radLat);
    magic = 1.0 - kGcjEE * magic * magic;
    double sqrtM = qSqrt(magic);
    dLat = (dLat * 180.0) / ((kGcjA * (1.0 - kGcjEE)) / (magic * sqrtM) * M_PI);
    dLon = (dLon * 180.0) / (kGcjA / sqrtM * qCos(radLat) * M_PI);
    gcjLat = wgsLat + dLat;
    gcjLon = wgsLon + dLon;
}

// ===== 大地测量计算 =====

double OsmRoadParser::haversineDistance(double lat1, double lon1,
                                        double lat2, double lon2)
{
    constexpr double R = 6378137.0;
    constexpr double D2R = M_PI / 180.0;
    double dLat = (lat2 - lat1) * D2R;
    double dLon = (lon2 - lon1) * D2R;
    double a = qSin(dLat / 2.0) * qSin(dLat / 2.0)
             + qCos(lat1 * D2R) * qCos(lat2 * D2R)
               * qSin(dLon / 2.0) * qSin(dLon / 2.0);
    return R * 2.0 * qAtan2(qSqrt(a), qSqrt(1.0 - a));
}

void OsmRoadParser::geodesicCalc(double lat1, double lon1,
                                  double lat2, double lon2,
                                  double& azimuthDeg, double& distanceM)
{
    constexpr double EARTH_R = 6378137.0;
    constexpr double DEG2RAD = M_PI / 180.0;

    double dLat = (lat2 - lat1) * DEG2RAD;
    double dLon = (lon2 - lon1) * DEG2RAD;
    double lat1r = lat1 * DEG2RAD;
    double lat2r = lat2 * DEG2RAD;

    // Haversine 距离
    double a = qSin(dLat / 2.0) * qSin(dLat / 2.0)
             + qCos(lat1r) * qCos(lat2r) * qSin(dLon / 2.0) * qSin(dLon / 2.0);
    double c = 2.0 * qAtan2(qSqrt(a), qSqrt(1.0 - a));
    distanceM = EARTH_R * c;

    // 方位角（北偏东，0~360）
    double y = qSin(dLon) * qCos(lat2r);
    double x = qCos(lat1r) * qSin(lat2r) - qSin(lat1r) * qCos(lat2r) * qCos(dLon);
    double bearing = qAtan2(y, x) * (180.0 / M_PI);
    azimuthDeg = fmod(bearing + 360.0, 360.0);
}

// ===== 解析 =====

bool OsmRoadParser::parse(const QString& filePath, double radarLat, double radarLon)
{
    m_roadNodes.clear();
    m_roadCount = 0;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[OsmRoadParser] Cannot open file:" << filePath;
        return false;
    }

    // ===== 第一遍：收集所有 node 的 id → (lat, lon) =====
    // 如果启用 GCJ-02 转换，在读取时即完成 WGS84 → GCJ-02
    QHash<qint64, QPointF> nodeMap;  // id -> (lon, lat)
    nodeMap.reserve(100000);

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("node")) {
            const auto attrs = xml.attributes();
            qint64 id  = attrs.value("id").toLongLong();
            double lat = attrs.value("lat").toDouble();
            double lon = attrs.value("lon").toDouble();

            if (m_gcj02Enabled) {
                double gcjLon, gcjLat;
                wgs84ToGcj02(lon, lat, gcjLon, gcjLat);
                nodeMap.insert(id, QPointF(gcjLon, gcjLat));
            } else {
                nodeMap.insert(id, QPointF(lon, lat));
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "[OsmRoadParser] XML parse error (pass 1):" << xml.errorString();
        file.close();
        return false;
    }

    qDebug() << "[OsmRoadParser] Loaded" << nodeMap.size() << "nodes"
             << (m_gcj02Enabled ? "(WGS84->GCJ-02)" : "(WGS84 raw)");

    // ===== 第二遍：遍历 way，筛选道路，按线段插值加密 =====
    file.seek(0);
    xml.setDevice(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QLatin1String("way")) {
            QVector<qint64> ndRefs;
            bool isHighway = false;

            while (!(xml.isEndElement() && xml.name() == QLatin1String("way"))) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("nd")) {
                        ndRefs.append(xml.attributes().value("ref").toLongLong());
                    } else if (xml.name() == QLatin1String("tag")) {
                        if (xml.attributes().value("k") == QLatin1String("highway")) {
                            isHighway = true;
                        }
                    }
                }
            }

            if (!isHighway || ndRefs.size() < 2)
                continue;

            m_roadCount++;

            // 遍历相邻节点对，在每段之间做线性插值
            for (int i = 0; i < ndRefs.size() - 1; ++i) {
                auto it1 = nodeMap.constFind(ndRefs[i]);
                auto it2 = nodeMap.constFind(ndRefs[i + 1]);
                if (it1 == nodeMap.constEnd() || it2 == nodeMap.constEnd())
                    continue;

                double lon1 = it1->x(), lat1 = it1->y();
                double lon2 = it2->x(), lat2 = it2->y();

                // 第一段的起点
                if (i == 0) {
                    RoadNode rn;
                    rn.lat = lat1;
                    rn.lon = lon1;
                    geodesicCalc(radarLat, radarLon, lat1, lon1,
                                 rn.azimuthDeg, rn.distanceM);
                    m_roadNodes.append(rn);
                }

                // 插值加密：当两节点间距 > 步长时，均匀插入中间点
                if (m_interpolationStep > 0) {
                    double segDist = haversineDistance(lat1, lon1, lat2, lon2);
                    if (segDist > m_interpolationStep) {
                        int numSteps = static_cast<int>(segDist / m_interpolationStep);
                        for (int s = 1; s < numSteps; ++s) {
                            double f = static_cast<double>(s) / numSteps;
                            double iLat = lat1 + f * (lat2 - lat1);
                            double iLon = lon1 + f * (lon2 - lon1);
                            RoadNode rn;
                            rn.lat = iLat;
                            rn.lon = iLon;
                            geodesicCalc(radarLat, radarLon, iLat, iLon,
                                         rn.azimuthDeg, rn.distanceM);
                            m_roadNodes.append(rn);
                        }
                    }
                }

                // 当前段的终点（也是下一段的起点，不会重复添加）
                RoadNode rn;
                rn.lat = lat2;
                rn.lon = lon2;
                geodesicCalc(radarLat, radarLon, lat2, lon2,
                             rn.azimuthDeg, rn.distanceM);
                m_roadNodes.append(rn);
            }
        }
    }

    file.close();

    qDebug() << "[OsmRoadParser] Parsed" << m_roadCount << "roads,"
             << m_roadNodes.size() << "nodes (interpolation step:"
             << m_interpolationStep << "m)";
    return true;
}

QVector<RoadNode> OsmRoadParser::nodesWithinRange(double maxDistanceM) const
{
    QVector<RoadNode> result;
    result.reserve(m_roadNodes.size() / 2);
    for (const auto& rn : m_roadNodes) {
        if (rn.distanceM <= maxDistanceM)
            result.append(rn);
    }
    return result;
}

void OsmRoadParser::recalculate(double radarLat, double radarLon)
{
    for (auto& rn : m_roadNodes) {
        geodesicCalc(radarLat, radarLon, rn.lat, rn.lon,
                     rn.azimuthDeg, rn.distanceM);
    }
    qDebug() << "[OsmRoadParser] Recalculated" << m_roadNodes.size()
             << "nodes for radar position" << radarLat << radarLon;
}
