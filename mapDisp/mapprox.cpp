/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:24
 * @Description: 
 */
#include "mapprox.h"
#include "../Basic/log.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QTimer>
#include <QDir>
#include <QUrlQuery>
#include "../Basic/ConfigManager.h"

MapProxyWidget::MapProxyWidget()
{
    // 加载配置文件
    CF_INS.load("config.toml");

    // 初始化当前雷达状态（从配置文件读取默认值）
    m_currentLongitude = CF_INS.longitude();
    m_currentLatitude = CF_INS.latitude();
    m_currentRange = CF_INS.range("max", 5);  // 默认使用最大显示距离
    // 默认使用高德离线瓦片（AMap）——已移除OSM选项，强制为 AMap
    m_currentEngine = EngineAMap;
    m_currentMapType = CF_INS.mapType("default_type", 1);

    // 检查必需的瓦片文件夹是否存在
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList requiredTileDirs = {"mapNoL", "map16", "map16S"};
    bool tilesAvailable = false;

    for (const QString& tileDir : requiredTileDirs) {
        QDir dir(appDir + "/" + tileDir);
        if (dir.exists()) {
            tilesAvailable = true;
            LOG_DEBUG(QString("Found tile directory: %1").arg(tileDir));
            break;
        }
    }

    //创建地图view
    mView = new QWebEngineView();

    if (!tilesAvailable) {
        // 瓦片文件夹不存在，设置为透明黑色背景
        LOG_WARNING("Map tile directories not found. Setting transparent black background.");
        mView->setStyleSheet("background-color: rgba(16, 24, 24, 0.9);");
        mView->setHtml("<html><body style='background-color: rgba(16, 24, 24, 0.9); margin: 0; padding: 0;'></body></html>");
        return;
    }

    // 瓦片存在，继续正常加载地图
    // 根据初始引擎选择HTML文件
    QString htmlFile;
    // 仅使用高德离线 HTML 路径
    htmlFile = appDir + "/indexNoL.html";
    LOG_DEBUG(QString("Map HTML file: %1").arg(htmlFile));

    // 从配置文件读取WebEngine调试设置
    if (CF_INS.webEngineDebugEnabled()) {
        int debugPort = CF_INS.webEngineDebugPort();
        qputenv("QTWEBENGINE_REMOTE_DEBUGGING", QString::number(debugPort).toLocal8Bit());
        LOG_DEBUG(QString("WebEngine remote debugging enabled on port: %1").arg(debugPort));
        LOG_DEBUG(QString("Open Chrome and navigate to: http://localhost:%1").arg(debugPort));
    } else {
        LOG_DEBUG("WebEngine remote debugging disabled. Set webengine.enable_debug=true in config.toml to enable.");
    }

    //开启WebGL支持
    QWebEngineSettings *settings = mView->settings();
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);

    /* 创建一个与网页交互的通道 */
    QWebChannel *webChannel = new QWebChannel(this);
    //语句顺序很主要  注册qt信号
    webChannel->registerObject(QString("qtChannel"), this);
    mView->page()->setWebChannel(webChannel);

    /* 加载网页，注意加载网页必须在通道注册之后，其有有一个注册完成的信号，
           可根据需要调用  "http://localhost:8080/index.html"*/

    // 构造函数中不加载地图HTML——此时QWebEngineView还未加入布局，容器尺寸为0，
    // AMap在零尺寸容器中初始化会导致瓦片不渲染。
    // 先显示黑色占位页，延迟到布局完成后再加载实际地图。
    mView->setHtml("<html><body style='background-color: black; margin: 0; padding: 0;'></body></html>");

    // 延迟加载：等待主窗口布局完成后QWebEngineView拥有有效尺寸，
    // 然后触发真正的地图加载。在此期间PPIVisualSettings的chooseMap调用
    // 会被拦截（仅更新m_currentMapType）。
    QTimer::singleShot(500, this, [this]() {
        m_initialLoadPending = false;
        chooseMap(m_currentMapType);
    });
}

void MapProxyWidget::chooseMap(int index)
{
    m_currentMapType = index;

    // 启动期间widget还未布局完成，仅记录类型，等待延迟加载触发
    if (m_initialLoadPending) {
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();

    // 检查瓦片文件夹是否存在
    QStringList requiredTileDirs = {"mapNoL", "map16", "map16S"};
    bool tilesAvailable = false;
    for (const QString& tileDir : requiredTileDirs) {
        QDir dir(appDir + "/" + tileDir);
        if (dir.exists()) { tilesAvailable = true; break; }
    }
    if (!tilesAvailable) {
        LOG_WARNING("AMap tiles not available. Cannot switch map.");
        mView->setHtml("<html><body style='background-color: rgba(16, 24, 24, 0.9); margin: 0; padding: 0;'></body></html>");
        return;
    }

    QString htmlFile;
    switch (index) {
        case 0: htmlFile = appDir + "/black.html"; break;
        case 1: htmlFile = appDir + "/indexNoL.html"; break;
        case 2: htmlFile = appDir + "/index.html"; break;
        case 3: htmlFile = appDir + "/indexS.html"; break;
        case 4: htmlFile = appDir + "/index3d.html"; break;
        default: htmlFile = appDir + "/indexNoL.html"; break;
    }

    // 通过URL查询参数把当前中心坐标传给HTML，页面初始化时直接定位，避免闪回默认点
    QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
    QUrlQuery query;
    query.addQueryItem("lng", QString::number(m_currentLongitude, 'f', 9));
    query.addQueryItem("lat", QString::number(m_currentLatitude, 'f', 9));
    query.addQueryItem("range", QString::number(m_currentRange, 'f', 3));
    baseUrl.setQuery(query);
    mView->setUrl(baseUrl);

    LOG_DEBUG(QString("Map switched: AMap, typeIndex=%1").arg(index));
}

void MapProxyWidget::setCenterOn(float lng, float lat,float range)
{
    emit centerOn(lng,lat,range);
}

void MapProxyWidget::syncRadarToMap(double longitude, double latitude, double range)
{
    // 浮点容差判断：经纬度变化 < 0.000001°（约0.11m）且距离变化 < 0.001km（1m）时不触发更新
    const double LNG_LAT_EPS = 1e-6;
    const double RANGE_EPS   = 1e-3;
    if (qAbs(longitude - m_currentLongitude) < LNG_LAT_EPS &&
        qAbs(latitude  - m_currentLatitude)  < LNG_LAT_EPS &&
        qAbs(range     - m_currentRange)     < RANGE_EPS) {
        return;
    }

    // 更新当前雷达状态
    m_currentLongitude = longitude;
    m_currentLatitude = latitude;
    m_currentRange = range;

    // 调用现有的setCenterOn方法来同步地图显示
    setCenterOn(static_cast<float>(longitude), static_cast<float>(latitude), static_cast<float>(range));

    LOG_DEBUG(QString("Map sync radar position: %1, %2, range: %3km")
                  .arg(longitude)
                  .arg(latitude)
                  .arg(range));
}

void MapProxyWidget::syncCurrentRadarState()
{
    // 使用当前存储的雷达状态同步地图
    setCenterOn(static_cast<float>(m_currentLongitude), static_cast<float>(m_currentLatitude), static_cast<float>(m_currentRange));
    LOG_DEBUG(QString("Synced current radar state to new map: %1, %2, range: %3km")
                  .arg(m_currentLongitude)
                  .arg(m_currentLatitude)
                  .arg(m_currentRange));
}

void MapProxyWidget::setGray(int value)
{
    emit changeGrayScale(value);
}

void MapProxyWidget::switchEngine(int engineIndex, int mapTypeIndex)
{
    MapEngine newEngine = static_cast<MapEngine>(engineIndex);
    if (newEngine == m_currentEngine && mapTypeIndex == m_currentMapType) {
        return; // 引擎和类型都没变
    }

    m_currentEngine = newEngine;
    LOG_DEBUG(QString("Map engine switched to: %1")
                  .arg(newEngine == EngineOSM ? "OSM/MapLibre" : "AMap/高德"));

    // 用chooseMap加载对应HTML
    chooseMap(mapTypeIndex);
}
