#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include <QObject>

class TrackManager : public QObject
{
    Q_OBJECT
public:
    explicit TrackManager(QObject *parent = nullptr);

signals:

};

#endif // TRACKMANAGER_H
