#ifndef THREADUDPSOCKET_H
#define THREADUDPSOCKET_H

#include <QObject>

class ThreadUdpSocket : public QObject
{
    Q_OBJECT
public:
    explicit ThreadUdpSocket(QObject *parent = nullptr);

signals:

};

#endif // THREADUDPSOCKET_H
