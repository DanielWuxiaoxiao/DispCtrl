/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-20 18:52:02
 * @Description: 
 */
#include "mapprox.h"
#include "../Basic/log.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QTimer>
#include <QDir>
#include <QUrlQuery>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QRegularExpression>
#include "../Basic/ConfigManager.h"
#include "Basic/uifontscale.h"
#include "Basic/diagnosticlogwindow.h"

namespace {
class MapPage final : public QWebEnginePage {
public:
    explicit MapPage(QObject* parent) : QWebEnginePage(parent) { m_clock.start(); }
protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message,
                                  int line, const QString& source) override {
        if (level == InfoMessageLevel) return;
        auto& window = level == ErrorMessageLevel ? m_errors : m_warnings;
        const auto decision = window.sample(m_clock.elapsed(), 5000);
        if (!decision.write) return;
        QString text = message.left(1024);
        text.replace(QRegularExpression(QStringLiteral("(?i)(key|token|password|scode|secret)=([^&\\s]+)")),
                     QStringLiteral("\\1=<redacted>"));
        text.replace(QLatin1Char('\n'), QLatin1Char(' '));
        text.replace(QStringLiteral("ANGLE"), QStringLiteral("Angle"));
        const QString location = QUrl(source).adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment).toString();
        LOG_WARNING(QStringLiteral("[MAP][JS] level=%1 source=%2:%3 message=%4 suppressed=%5")
            .arg(int(level)).arg(location.left(256)).arg(line).arg(text).arg(qulonglong(decision.suppressed)));
    }
private:
    QElapsedTimer m_clock;
    DiagnosticLogWindow m_errors, m_warnings;
};
}

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
    m_currentMapType = CF_INS.mapType("default_type", 0);

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
    mView->setPage(new MapPage(mView));
    mView->page()->setBackgroundColor(Qt::black);
    mView->setStyleSheet(UiFontScale::style("background-color: black;"));
    m_loadWatchdog = new QTimer(this);
    m_loadWatchdog->setSingleShot(true);
    connect(m_loadWatchdog, &QTimer::timeout, this, [this] {
        if (!mView) return;
        LOG_ERROR(QStringLiteral("[MAP][LOAD_TIMEOUT] type=%1 url=%2 viewport=%3x%4; native black fallback")
            .arg(m_currentMapType).arg(m_expectedMapUrl.toString()).arg(mView->width()).arg(mView->height()));
        m_mapPageSyncPending = false;
        mView->stop();
        mView->hide();
    });

    if (!tilesAvailable) {
        // Missing optional tiles must not skip HTML/WebChannel initialization.
        LOG_WARNING("Optional map tiles not found; missing offline basemaps will use black.html with WebChannel initialized.");
    }

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

    // URL 参数可完成首帧定位；页面加载完成后仍要直接刷新一次地图。
    // 这样不依赖 WebChannel 注册完成的先后，也避免卫星瓦片在首次布局尚未稳定时保持空白。
    connect(mView, &QWebEngineView::loadStarted, this, [this] {
        LOG_INFO(QStringLiteral("[MAP][LOAD_STARTED] type=%1 url=%2 viewport=%3x%4")
            .arg(m_currentMapType).arg(mView->url().toString()).arg(mView->width()).arg(mView->height()));
    });
    connect(mView, &QWebEngineView::loadFinished, this, [this](bool success) {
        LOG_INFO(QStringLiteral("[MAP][LOAD_FINISHED] ok=%1 type=%2 url=%3 expected=%4 viewport=%5x%6")
            .arg(success).arg(m_currentMapType).arg(mView->url().toString()).arg(m_expectedMapUrl.toString())
            .arg(mView->width()).arg(mView->height()));
        if (!m_mapPageSyncPending || mView->url() != m_expectedMapUrl) {
            return;
        }

        m_mapPageSyncPending = false;
        m_loadWatchdog->stop();
        if (!success) {
            LOG_ERROR(QString("[MAP][LOAD_FAILED] %1; native black fallback").arg(m_expectedMapUrl.toString()));
            mView->hide();
            return;
        }
        const QUrl expected = m_expectedMapUrl;
        QPointer<MapProxyWidget> self(this);
        mView->page()->runJavaScript(QStringLiteral(
            "JSON.stringify({ready:document.readyState,w:innerWidth,h:innerHeight,"
            "htmlBackground:getComputedStyle(document.documentElement).backgroundColor,"
            "bodyBackground:document.body?getComputedStyle(document.body).backgroundColor:'none',"
            "hasMap:typeof map!=='undefined'})"), [self, expected](const QVariant& value) {
            if (!self || self->m_expectedMapUrl != expected) return;
            LOG_INFO(QStringLiteral("[MAP][DOM] %1").arg(value.toString().left(1024)));
        });

        QTimer::singleShot(0, this, &MapProxyWidget::refreshLoadedMapState);
    });

    // 渲染进程崩溃监控（外场黑屏排查用）：WebEngine 渲染/ GPU 进程一旦终止，地图区域立即变黑。
    // 记录终止状态与退出码，便于判断是否需要切换 gl_backend(angle→desktop→software)。
    connect(mView->page(), &QWebEnginePage::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
                m_loadWatchdog->stop();
                m_initialLoadPending = false;
                m_mapPageSyncPending = false;
                if (mView) mView->hide();
                LOG_ERROR(QString("[WebEngine] 渲染进程终止! status=%1 exitCode=%2；请结合[WEBENGINE_CONFIG]排查沙箱/系统调用与图形后端兼容性")
                              .arg(static_cast<int>(status))
                              .arg(exitCode));
            });

    /* 加载网页，注意加载网页必须在通道注册之后，其有有一个注册完成的信号，
           可根据需要调用  "http://localhost:8080/index.html"*/

    // 构造函数中不加载地图HTML——此时QWebEngineView还未加入布局，容器尺寸为0，
    // AMap在零尺寸容器中初始化会导致瓦片不渲染。
    // 地图等待布局；无地图仍实际加载 black.html，不以 Qt 黑底代替网页。
    // 页面背景与中央容器黑底仅用于覆盖加载期间或明确失败后的空白。

    // 等待视图实际拥有可用尺寸后再加载；不能以固定 500ms 猜测 WebEngine 的布局状态。
    // 此期间的 chooseMap 调用只保存用户选择，首次加载时统一使用最终的类型。
    QTimer::singleShot(0, this, &MapProxyWidget::loadInitialMapWhenReady);
}

void MapProxyWidget::loadInitialMapWhenReady()
{
    if (!m_initialLoadPending || !mView) {
        return;
    }

    if (m_currentMapType == 0) {
        chooseMap(0);
        return;
    }

    if (!mView->isVisible() || mView->width() <= 1 || mView->height() <= 1) {
        QTimer::singleShot(50, this, &MapProxyWidget::loadInitialMapWhenReady);
        return;
    }

    m_initialLoadPending = false;
    chooseMap(m_currentMapType);
}

void MapProxyWidget::chooseMap(int index)
{
    if (!mView) return;
    if (index < 0 || index > 4) index = 0;
    const bool needsLayout = mView->isHidden();
    m_currentMapType = index;

    const QString appDir = QCoreApplication::applicationDirPath();
    m_effectiveMapType = index;
    if (index >= 1 && index <= 3) {
        const QStringList tileDirs{QStringLiteral("mapNoL"), QStringLiteral("map16"), QStringLiteral("map16S")};
        const QString tilePath = appDir + QLatin1Char('/') + tileDirs.at(index - 1);
        const QDir tiles(tilePath);
        if (!tiles.exists() || tiles.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot).isEmpty()) {
            m_effectiveMapType = 0;
            LOG_WARNING(QStringLiteral("[MAP][TILES_MISSING] requested=%1 path=%2 missing/empty; loading black.html")
                .arg(index).arg(tilePath));
        }
    }
    const QStringList pages{QStringLiteral("black.html"), QStringLiteral("indexNoL.html"),
                            QStringLiteral("index.html"), QStringLiteral("indexS.html"), QStringLiteral("index3d.html")};
    const QString htmlFile = appDir + QLatin1Char('/') + pages.at(m_effectiveMapType);
    const QFileInfo file(htmlFile);
    if (!file.isFile() || !file.isReadable() || file.size() == 0) {
        m_initialLoadPending = false;
        m_mapPageSyncPending = false;
        m_expectedMapUrl = QUrl();
        m_loadWatchdog->stop();
        mView->stop();
        mView->hide();
        LOG_ERROR(QStringLiteral("[MAP][FILE_MISSING] type=%1 path=%2; native black fallback").arg(index).arg(htmlFile));
        return;
    }
    // black.html has no map canvas: load it even before layout to exercise WebEngine.
    if (m_effectiveMapType == 0) m_initialLoadPending = false;
    else if (needsLayout) m_initialLoadPending = true;
    mView->show();

    // 启动期间widget还未布局完成，仅记录类型，等待延迟加载触发
    if (m_initialLoadPending) {
        QTimer::singleShot(0, this, &MapProxyWidget::loadInitialMapWhenReady);
        return;
    }

    // 通过URL查询参数把当前中心坐标传给HTML，页面初始化时直接定位，避免闪回默认点
    QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
    QUrlQuery query;
    query.addQueryItem("lng", QString::number(m_currentLongitude, 'f', 9));
    query.addQueryItem("lat", QString::number(m_currentLatitude, 'f', 9));
    query.addQueryItem("range", QString::number(m_currentRange, 'f', 3));
    // 把当前PPI偏移比例一并透传，保证地图首次加载即应用偏移补偿（避免切图瞬间无偏移）
    query.addQueryItem("offx", QString::number(m_offsetRatioX, 'f', 6));
    query.addQueryItem("offy", QString::number(m_offsetRatioY, 'f', 6));
    baseUrl.setQuery(query);
    m_expectedMapUrl = baseUrl;
    m_mapPageSyncPending = true;
    m_loadWatchdog->start(10000);
    mView->setUrl(baseUrl);

    LOG_DEBUG(QString("Map switched: AMap, typeIndex=%1").arg(index));
}

void MapProxyWidget::setCenterOn(float lng, float lat, float range, float offsetRatioX, float offsetRatioY)
{
    emit centerOn(lng, lat, range, offsetRatioX, offsetRatioY);
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

    // 仅更新位置/范围的场景（如经纬高实时上报），偏移比例复用上一次PPI计算结果
    setCenterOn(static_cast<float>(longitude), static_cast<float>(latitude), static_cast<float>(range),
                static_cast<float>(m_offsetRatioX), static_cast<float>(m_offsetRatioY));

    LOG_DEBUG(QString("Map sync radar position: %1, %2, range: %3km, offsetRatio(%4,%5)")
                  .arg(longitude)
                  .arg(latitude)
                  .arg(range)
                  .arg(m_offsetRatioX)
                  .arg(m_offsetRatioY));
}

void MapProxyWidget::syncRadarToMapWithOffset(double longitude, double latitude, double range,
                                              double offsetRatioX, double offsetRatioY)
{
    // 来自PPIView::radarCenterChanged：存储最新偏移比例并强制刷新（偏移可能变化但经纬度未变，
    // 故不走syncRadarToMap的容差跳过逻辑）
    m_currentLongitude = longitude;
    m_currentLatitude  = latitude;
    m_currentRange     = range;
    m_offsetRatioX     = offsetRatioX;
    m_offsetRatioY     = offsetRatioY;

    setCenterOn(static_cast<float>(longitude), static_cast<float>(latitude), static_cast<float>(range),
                static_cast<float>(offsetRatioX), static_cast<float>(offsetRatioY));

    LOG_DEBUG(QString("Map sync radar with offset: %1, %2, range: %3km, offsetRatio(%4,%5)")
                  .arg(longitude)
                  .arg(latitude)
                  .arg(range)
                  .arg(offsetRatioX)
                  .arg(offsetRatioY));
}

void MapProxyWidget::syncCurrentRadarState()
{
    // 使用当前存储的雷达状态与偏移比例同步地图
    setCenterOn(static_cast<float>(m_currentLongitude), static_cast<float>(m_currentLatitude),
                static_cast<float>(m_currentRange),
                static_cast<float>(m_offsetRatioX), static_cast<float>(m_offsetRatioY));
    LOG_DEBUG(QString("Synced current radar state to new map: %1, %2, range: %3km, offsetRatio(%4,%5)")
                  .arg(m_currentLongitude)
                  .arg(m_currentLatitude)
                  .arg(m_currentRange)
                  .arg(m_offsetRatioX)
                  .arg(m_offsetRatioY));
}

void MapProxyWidget::refreshLoadedMapState()
{
    if (!mView || !mView->page() || m_effectiveMapType == 0) {
        return;
    }

    const QString javascript = QStringLiteral(
        "(function() {"
        "if (typeof map === 'undefined' || typeof setCenterOn !== 'function') return;"
        "map.resize();"
        "setCenterOn(%1, %2, %3, %4, %5);"
        "})();")
        .arg(QString::number(m_currentLongitude, 'f', 9),
             QString::number(m_currentLatitude, 'f', 9),
             QString::number(m_currentRange, 'f', 3),
             QString::number(m_offsetRatioX, 'f', 6),
             QString::number(m_offsetRatioY, 'f', 6));
    mView->page()->runJavaScript(javascript);

    LOG_DEBUG(QString("Map initial page ready: refreshed typeIndex=%1, center=(%2,%3), range=%4km")
                  .arg(m_currentMapType)
                  .arg(m_currentLongitude, 0, 'f', 9)
                  .arg(m_currentLatitude, 0, 'f', 9)
                  .arg(m_currentRange, 0, 'f', 3));
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
