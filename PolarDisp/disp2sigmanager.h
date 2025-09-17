#ifndef DISP2SIGMANAGER_H
#define DISP2SIGMANAGER_H

#include <QObject>

class Disp2SigManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2SigManager(QObject *parent = nullptr);

signals:

};

#endif // DISP2SIGMANAGER_H
