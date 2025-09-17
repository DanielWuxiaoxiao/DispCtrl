#ifndef DISP2PHOTOMANAGER_H
#define DISP2PHOTOMANAGER_H

#include <QObject>

class Disp2PhotoManager : public QObject
{
    Q_OBJECT
public:
    explicit Disp2PhotoManager(QObject *parent = nullptr);

signals:

};

#endif // DISP2PHOTOMANAGER_H
