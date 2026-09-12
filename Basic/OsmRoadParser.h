/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-03-30 22:07:37
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:43
 * @Description: 
 */
/**
 * @file OsmRoadParser.h
 * @brief OSM道路数据解析器
 * @details 解析OpenStreetMap .osm文件，提取道路节点经纬度坐标，
 *          并根据雷达位置计算各节点的方位角和距离。
 *          支持WGS84→GCJ-02坐标转换（匹配中国地图瓦片）和道路插值加密。
 */
#ifndef OSMROADPARSER_H
#define OSMROADPARSER_H

#include <QString>
#include <QVector>
#include <QPointF>

/**
 * @brief 单个道路节点（经纬度 + 计算后的方位距离）
 */
struct RoadNode {
    double lat;         ///< 纬度（度）
    double lon;         ///< 经度（度）
    double azimuthDeg;  ///< 相对雷达的方位角（度，北偏东）
    double distanceM;   ///< 相对雷达的距离（米）
};

/**
 * @class OsmRoadParser
 * @brief OSM文件解析器，提取道路节点坐标
 *
 * 解析流程：
 * 1. 读取 .osm XML 文件
 * 2. 收集所有 <node> 的 id→(lat,lon) 映射
 * 3. 可选：将WGS84坐标转换为GCJ-02（匹配中国高德/百度地图瓦片）
 * 4. 遍历 <way>，筛选含有 highway 标签的道路
 * 5. 对相邻节点间做线性插值加密（默认30m步长）
 * 6. 根据雷达经纬度计算每个节点的方位角和距离
 */
class OsmRoadParser
{
public:
    OsmRoadParser() = default;

    /**
     * @brief 启用/禁用WGS84→GCJ-02坐标转换
     * @param enabled true时将OSM的WGS84坐标转换为GCJ-02（匹配中国地图瓦片）
     * @details 中国境内的在线/离线地图瓦片（高德、百度、腾讯）均使用GCJ-02坐标系，
     *          而OSM数据为WGS84坐标系，两者存在约100~600m的偏移。
     *          启用此选项后，解析时自动将OSM节点坐标转换为GCJ-02，
     *          使道路点与地图瓦片对齐。
     */
    void setGcj02Enabled(bool enabled) { m_gcj02Enabled = enabled; }

    /**
     * @brief 设置道路插值步长
     * @param meters 相邻插值点之间的间距（米），0表示不插值
     * @details OSM原始道路节点可能很稀疏（间距数百米），开启插值后，
     *          在相邻节点间按指定步长生成加密点，使道路显示更连续。
     */
    void setInterpolationStep(double meters) { m_interpolationStep = meters; }

    /**
     * @brief 解析OSM文件并计算道路节点相对雷达的方位距离
     * @param filePath .osm文件路径
     * @param radarLat 雷达纬度（度，与config.toml中的坐标系一致）
     * @param radarLon 雷达经度（度，与config.toml中的坐标系一致）
     * @return true 解析成功
     */
    bool parse(const QString& filePath, double radarLat, double radarLon);

    const QVector<RoadNode>& roadNodes() const { return m_roadNodes; }
    QVector<RoadNode> nodesWithinRange(double maxDistanceM) const;
    int roadCount() const { return m_roadCount; }
    int nodeCount() const { return m_roadNodes.size(); }

    /**
     * @brief 雷达位置变化后重新计算所有节点的方位角和距离
     * @param radarLat 新雷达纬度（度）
     * @param radarLon 新雷达经度（度）
     * @details 不重新解析XML，仅根据新雷达位置重算已缓存节点的 azimuthDeg/distanceM
     */
    void recalculate(double radarLat, double radarLon);

    // ===== GCJ-02 坐标转换（静态工具方法）=====

    /** @brief 判断坐标是否在中国境外（境外不需要GCJ-02偏移）*/
    static bool outOfChina(double lon, double lat);
    /** @brief WGS84 → GCJ-02 坐标转换 */
    static void wgs84ToGcj02(double wgsLon, double wgsLat,
                              double& gcjLon, double& gcjLat);

private:
    static double transformLat(double x, double y);
    static double transformLon(double x, double y);

    /** @brief Haversine距离（米）*/
    static double haversineDistance(double lat1, double lon1,
                                    double lat2, double lon2);

    /** @brief 计算方位角（度，北偏东 0~360）和距离（米）*/
    static void geodesicCalc(double lat1, double lon1,
                             double lat2, double lon2,
                             double& azimuthDeg, double& distanceM);

    QVector<RoadNode> m_roadNodes;
    int m_roadCount = 0;
    bool m_gcj02Enabled = true;        ///< 默认启用GCJ-02转换（匹配中国地图）
    double m_interpolationStep = 30.0; ///< 默认30m插值步长
};

#endif // OSMROADPARSER_H
