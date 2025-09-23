/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 15:56:16
 * @Description: 
 */
#include "mapprox.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QTimer>
#include "../Basic/ConfigManager.h"

MapProxyWidget::MapProxyWidget()
{
    // 加载配置文件
    CF_INS.load("config.toml");

    // 初始化当前雷达状态（从配置文件读取默认值）
    m_currentLongitude = CF_INS.longitude();
    m_currentLatitude = CF_INS.latitude();
    m_currentRange = CF_INS.range("max", 5);  // 默认使用最大显示距离

    // 设置工作目录为包含index.html的目录,即index.html的绝对目录
    QString htmlFile = QCoreApplication::applicationDirPath() + "/indexNoL.html"; // 替换为实际路径
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

    //创建地图view
    mView = new QWebEngineView();

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

    QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
    mView->setUrl(QUrl(baseUrl));
    //直接在load后的语句进行javascript函数会导致无法运行，需要等待页面完全加载完毕才可使用函数。
}

void MapProxyWidget::chooseMap(int index)
{
    QString htmlFile;

    if(index == 0)
    {
        htmlFile = QCoreApplication::applicationDirPath() + "/black.html"; // 替换为实际路径
    }
    else
    {
        mView->setVisible(true);

        if(index == 1)
        {
            htmlFile = QCoreApplication::applicationDirPath() + "/indexNoL.html"; // 替换为实际路径
        }
        else if(index == 2)
        {
            htmlFile = QCoreApplication::applicationDirPath() + "/index.html"; // 替换为实际路径
        }
        else if(index == 3)
        {
            htmlFile = QCoreApplication::applicationDirPath() + "/indexS.html"; // 替换为实际路径
        }
        else if(index == 4)
        {
            htmlFile = QCoreApplication::applicationDirPath() + "/index3d.html"; // 替换为实际路径
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
