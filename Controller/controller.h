#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>

#include "Basic/ConfigManager.h"
#include "Basic/MarineProtocol.h"

class MarineRadarManager;

#define CON_INS Controller::getInstance()
#define CF_INS ConfigManager::instance()

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller() override;

    static Controller* getInstance();
    void init();
    void logMarineRxSnapshot(const QString& reason) const;

    MarineRadarManager* marineMgr() const { return m_marineMgr; }
    MarineRadarManager* marineRadarManager() const { return m_marineMgr; }

signals:
    void marineEchoLine(const MarineEchoLine& line);
    void marineStatusUpdated(const MarineRadarStatus& status);
    void minimizeWindow(bool checked = false);

private:
    MarineRadarManager* m_marineMgr = nullptr;
};

#endif // CONTROLLER_H
