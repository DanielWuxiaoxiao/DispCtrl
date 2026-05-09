/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-05-09 09:43:04
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-09 11:28:41
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-05-09 00:00:00
 * @LastEditors: GitHub Copilot
 * @LastEditTime: 2026-05-09 00:00:00
 * @Description:
 */
#ifndef COLLABTRACK2DISPMANAGER_H
#define COLLABTRACK2DISPMANAGER_H

#pragma once
#include <QObject>
#include <QHostAddress>
#include "Basic/Protocol.h"

class ThreadedUdpSocket;
class QThread;

class CollabTrack2DispManager : public QObject
{
    Q_OBJECT
public:
    explicit CollabTrack2DispManager(QObject *parent = nullptr);
    ~CollabTrack2DispManager();

public slots:
    void cooperativeTrackDecode(QByteArray data);

signals:
    void cooperativeTrackProcess(PointInfo info);

private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // COLLABTRACK2DISPMANAGER_H
