/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-26 11:27:33
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-30 11:45:45
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-26
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-26
 * @Description: 资源管理到显控管理器头文件
 */
#ifndef RES2DISPMANAGER_H
#define RES2DISPMANAGER_H

#include <QObject>
#include <QHostAddress>
#include "Basic/ConfigManager.h"

class ThreadedUdpSocket;
class QThread;

/**
 * @class Res2DispManager
 * @brief 资源管理到显控管理器
 * @details 接收来自资源管理系统的消息，包括BIT上报、伺服控制回送等
 */
class Res2DispManager : public QObject
{
    Q_OBJECT
public:
    explicit Res2DispManager(QObject *parent = nullptr);
    ~Res2DispManager();

private:
    ThreadedUdpSocket* socket;  ///< UDP套接字
    QThread* thread;            ///< 网络通信线程
    quint16 src;                ///< 源系统ID（资源管理）
    quint16 dst;                ///< 目标系统ID（显控）
    QHostAddress host;          ///< 目标主机地址
    quint16 port;               ///< 目标端口号
};

#endif // RES2DISPMANAGER_H
