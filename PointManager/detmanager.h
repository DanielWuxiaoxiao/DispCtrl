#ifndef DETMANAGER_H
#define DETMANAGER_H

#include <QObject>

class DetManager : public QObject
{
    Q_OBJECT
public:
    explicit DetManager(QObject *parent = nullptr);

signals:

};

#endif // DETMANAGER_H
