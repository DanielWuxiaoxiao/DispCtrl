#ifndef MAPPROXYWIDGET_H
#define MAPPROXYWIDGET_H

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QtWebChannel>
#include <QWebEngineView>

class MapProxyWidget : public QObject
{
    Q_OBJECT // <<-- 在这里添加 Q_OBJECT 宏
public:
    MapProxyWidget();
    QWebEngineView * getView()
    {
        return mView;
    };

private:
    QWebEngineView *mView;

public slots:
    //html--> myChannel
    //html--> myChannel
    void setCenterOn(float lng, float lat,float range);
    void setGray(int value);
    void chooseMap(int index);

signals:
    //myChannel --> html
    void centerOn(float lng, float lat,float range);//myChannel --> html
    void changeGrayScale(int value);

    //myChannel --> mainwindow
    //myChannel --> mainwindow
};


#endif // MAPPROXYWIDGET_H

