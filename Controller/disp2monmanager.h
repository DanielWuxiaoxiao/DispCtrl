#ifndef DISP2MONMANAGER_H
#define DISP2MONMANAGER_H

#include <QObject>

class Disp2MonManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2MonManager(QObject *parent = nullptr);

signals:

};

#endif // DISP2MONMANAGER_H
