#ifndef DISP2DATAMANAGER_H
#define DISP2DATAMANAGER_H

#include <QObject>

class Disp2DataManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2DataManager(QObject *parent = nullptr);

signals:

};

#endif // DISP2DATAMANAGER_H
