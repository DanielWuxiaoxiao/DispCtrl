/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:14
 * @Description: 
 */
#include "mapprox.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QTimer>
#include <QDir>
#include "../Basic/ConfigManager.h"

MapProxyWidget::MapProxyWidget()
{
    // 加载配置文件
    // 初始化当前雷达状态（从配置文件读取默认值）
    m_currentLongitude = CF_INS.longitude();
    m_currentLatitude = CF_INS.latitude();
    m_currentRange = CF_INS.range("max", 5);  // 默认使用最大显示距离

    // 检查必需的瓦片文件夹是否存在
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList requiredTileDirs = {"mapNoL", "map16", "map16S"};
    bool tilesAvailable = false;

    for (const QString& tileDir : requiredTileDirs) {
        QDir dir(appDir + "/" + tileDir);
        if (dir.exists()) {
            tilesAvailable = true;
            qDebug() << "Found tile directory:" << tileDir;
            break;
        }
    }

    //创建地图view
    mView = new QWebEngineView();

    if (!tilesAvailable) {
        // 瓦片文件夹不存在，设置为透明黑色背景
        qWarning() << "Map tile directories not found. Setting transparent black background.";
        mView->setStyleSheet("background-color: rgba(16, 24, 24, 0.9);");
        mView->setHtml("<html><body style='background-color: rgba(16, 24, 24, 0.9); margin: 0; padding: 0;'></body></html>");
        return;
    }

    // 瓦片存在，继续正常加载地图
    // 设置工作目录为包含index.html的目录,即index.html的绝对目录
    QString htmlFile = appDir + "/indexNoL.html"; // 替换为实际路径
    qDebug() << htmlFile;

    // 从配置文件读取WebEngine调试设置
    if (CF_INS.webEngineDebugEnabled()) {
        int debugPort = CF_INS.webEngineDebugPort();
        qputenv("QTWEBENGINE_REMOTE_DEBUGGING", QString::number(debugPort).toLocal8Bit());
        qDebug() << "WebEngine remote debugging enabled on port:" << debugPort;
        qDebug() << "Open Chrome and navigate to: http://localhost:" << debugPort;
    } else {
        qDebug() << "WebEngine remote debugging disabled. Set webengine.enable_debug=true in config.toml to enable.";
    }

    //开启WebGL支持
    QWebEngineSettings *settings = mView->settings();
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);

    /* 创建一个与网页交互的通道 */
    QWebChannel *webChannel = new QWebChannel(this);
    //语句顺序很主要  注册qt信号
    webChannel->registerObject(QString("qtChannel"), this);
    mView->page()->setWebChannel(webChannel);

    connect(mView->page(), &QWebEnginePage::renderProcessTerminated, this,
            [](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
                qWarning() << "[WebEngine] render process terminated, status="
                           << static_cast<int>(status)
                           << "exitCode=" << exitCode
                           << "try config.toml [webengine] gl_backend=angle/desktop/software";
            });
    /* 加载网页，注意加载网页必须在通道注册之后，其有有一个注册完成的信号，
           可根据需要调用  "http://localhost:8080/index.html"*/

    QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
    mView->setUrl(QUrl(baseUrl));
    //直接在load后的语句进行javascript函数会导致无法运行，需要等待页面完全加载完毕才可使用函数。
}

void MapProxyWidget::chooseMap(int index)
{
    // 检查瓦片文件夹是否存在
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList requiredTileDirs = {"mapNoL", "map16", "map16S"};
    bool tilesAvailable = false;

    for (const QString& tileDir : requiredTileDirs) {
        QDir dir(appDir + "/" + tileDir);
        if (dir.exists()) {
            tilesAvailable = true;
            break;
        }
    }

    // 如果瓦片不存在，仅设置透明黑色背景
    if (!tilesAvailable) {
        qWarning() << "Map tiles not available. Cannot switch map.";
        mView->setHtml("<html><body style='background-color: rgba(16, 24, 24, 0.9); margin: 0; padding: 0;'></body></html>");
        return;
    }

    QString htmlFile;

    if(index == 0)
    {
        htmlFile = appDir + "/black.html"; // 替换为实际路径
    }
    else
    {
        mView->setVisible(true);

        if(index == 1)
        {
            htmlFile = appDir + "/indexNoL.html"; // 替换为实际路径
        }
        else if(index == 2)
        {
            htmlFile = appDir + "/index.html"; // 替换为实际路径
        }
        else if(index == 3)
        {
            htmlFile = appDir + "/indexS.html"; // 替换为实际路径
        }
        else if(index == 4)
        {
            htmlFile = appDir + "/index3d.html"; // 替换为实际路径
        }
    }

    if (!htmlFile.isEmpty()) {
        QUrl baseUrl = QUrl::fromLocalFile(htmlFile);

        // 连接页面加载完成信号，在地图加载后同步雷达状态
        // Qt 5.14不支持SingleShotConnection，使用手动断开连接的方式
        QMetaObject::Connection connection;
        connection = connect(mView, &QWebEngineView::loadFinished, this, [this, connection](bool success) mutable {
            if (success) {
                // 页面加载完成后，使用定时器延迟同步，确保JavaScript已完全初始化
                QTimer::singleShot(500, this, [this]() {
                    this->syncCurrentRadarState();
                });
            }
            // 手动断开连接，确保只执行一次
            QObject::disconnect(connection);
        });

        mView->setUrl(baseUrl);
        qDebug() << "Map switched to:" << htmlFile;
    }
}

void MapProxyWidget::setCenterOn(float lng, float lat,float range)
{
    emit centerOn(lng,lat,range);
}

void MapProxyWidget::syncRadarToMap(double longitude, double latitude, double range)
{
    // 更新当前雷达状态
    m_currentLongitude = longitude;
    m_currentLatitude = latitude;
    m_currentRange = range;

    // 调用现有的setCenterOn方法来同步地图显示
    setCenterOn(static_cast<float>(longitude), static_cast<float>(latitude), static_cast<float>(range));

    qDebug() << "Map sync radar position:" << longitude << "," << latitude << ", range:" << range << "km";
}

void MapProxyWidget::syncCurrentRadarState()
{
    // 使用当前存储的雷达状态同步地图
    setCenterOn(static_cast<float>(m_currentLongitude), static_cast<float>(m_currentLatitude), static_cast<float>(m_currentRange));
    qDebug() << "Synced current radar state to new map:" << m_currentLongitude << "," << m_currentLatitude << ", range:" << m_currentRange << "km";
}

void MapProxyWidget::setGray(int value)
{
    emit changeGrayScale(value);
}
