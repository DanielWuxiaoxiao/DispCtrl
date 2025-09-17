#ifndef TARGETDISPMANAGER_H
#define TARGETDISPMANAGER_H

#include <QObject>
#include "Basic/Protocol.h"
#include <QHostAddress>

class ThreadedUdpSocket;
class QThread;

class targetDispManager : public QObject
{
    Q_OBJECT
public:
    explicit targetDispManager(QObject *parent = nullptr);
    ~targetDispManager();
private:
    ThreadedUdpSocket* socket;
    QThread* thread;
    unsigned commCount;
    QHostAddress host;
    quint16 port;
    quint16 src;
    quint16 dst;
};

#endif // TARGETDISPMANAGER_H
