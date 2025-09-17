#include "mapprox.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>

MapProxyWidget::MapProxyWidget()
{
    // 设置工作目录为包含index.html的目录,即index.html的绝对目录
    QString htmlFile = QCoreApplication::applicationDirPath() + "/indexNoL.html"; // 替换为实际路径
    qDebug() << htmlFile;

    //qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");
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
    if(index == 0)
    {
        QString htmlFile = QCoreApplication::applicationDirPath() + "/black.html"; // 替换为实际路径
        QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
        mView->setUrl(QUrl(baseUrl));
    }
    else
    {
        mView->setVisible(true);

        if(index == 1)
        {
            QString htmlFile = QCoreApplication::applicationDirPath() + "/indexNoL.html"; // 替换为实际路径
            QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
            mView->setUrl(QUrl(baseUrl));
        }
        else if(index == 2)
        {
            QString htmlFile = QCoreApplication::applicationDirPath() + "/index.html"; // 替换为实际路径
            QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
            mView->setUrl(QUrl(baseUrl));
        }
        else if(index == 3)
        {
            QString htmlFile = QCoreApplication::applicationDirPath() + "/indexS.html"; // 替换为实际路径
            QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
            mView->setUrl(QUrl(baseUrl));
        }
        else if(index == 4)
        {
            QString htmlFile = QCoreApplication::applicationDirPath() + "/index3d.html"; // 替换为实际路径
            QUrl baseUrl = QUrl::fromLocalFile(htmlFile);
            mView->setUrl(QUrl(baseUrl));
        }
    }
}

void MapProxyWidget::setCenterOn(float lng, float lat,float range)
{
    emit centerOn(lng,lat,range);
}

void MapProxyWidget::setGray(int value)
{
    emit changeGrayScale(value);
}

