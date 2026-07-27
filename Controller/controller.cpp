#include "controller.h"

#include "MarineRadarManager.h"
#include "Basic/log.h"

Q_GLOBAL_STATIC(Controller, ControllerInstance)

Controller* Controller::getInstance()
{
    return ControllerInstance();
}

Controller::Controller(QObject* parent)
    : QObject(parent)
{
}

Controller::~Controller() = default;

void Controller::init()
{
    if (m_marineMgr) {
        return;
    }

    LOG_INFO("[Controller] initializing ship-radar data path");

    m_marineMgr = new MarineRadarManager(this);
    const QString localIp = CF_INS.marineNetworkStr("local_ip", "192.168.1.100");
    const int echoPort = CF_INS.marineNetworkInt("echo_port", 9000);
    const QString servoIp = CF_INS.marineNetworkStr("servo_ip", "192.168.1.30");
    const int servoPort = CF_INS.marineNetworkInt("servo_port", 9000);
    const int autoSendMs = CF_INS.marineNetworkInt("auto_send_ms", 0);

    LOG_INFO(QString("[Controller] marine network local=%1:%2 servo=%3:%4 autoSendMs=%5")
                 .arg(localIp).arg(echoPort).arg(servoIp).arg(servoPort).arg(autoSendMs));

    m_marineMgr->init(localIp, static_cast<uint16_t>(echoPort),
                      servoIp, static_cast<uint16_t>(servoPort));

    MarineControlFrame control;
    control.cmdNum = static_cast<uint8_t>(qBound(0,
        CF_INS.marineControl("cmd_num", MarineCmdParamsOnly),
        static_cast<int>(MarineCmdStart)));
    control.azimuth = marineEncodeAzimuth(CF_INS.marineControlDouble("azimuth_deg", 0.0));
    control.rangeVal = static_cast<uint8_t>(CF_INS.marineControl("range", 7));
    control.gain = static_cast<uint8_t>(CF_INS.marineControl("gain", 0));
    control.ganRao = static_cast<uint8_t>(CF_INS.marineControl("interference", 0));
    control.level = static_cast<uint8_t>(CF_INS.marineControl("level", 0));
    control.seaVal = static_cast<uint8_t>(CF_INS.marineControl("sea_clutter", 0));
    control.rainVal = static_cast<uint8_t>(CF_INS.marineControl("rain_clutter", 0));
    control.txCtrl = CF_INS.marineControlBool("tx_on", false) ? 1 : 0;
    control.servo = static_cast<uint8_t>(qBound(0,
        CF_INS.marineControl("servo_gear", CF_INS.marineControl("servo_speed", 0)),
        static_cast<int>(MARINE_SERVO_MAX_GEAR)));
    m_marineMgr->setCurrentControl(control);
    m_marineMgr->setAutoSendInterval(autoSendMs);

    connect(m_marineMgr, &MarineRadarManager::echoLineReceived,
            this, &Controller::marineEchoLine);
    connect(m_marineMgr, &MarineRadarManager::radarStatusUpdated,
            this, &Controller::marineStatusUpdated);
}

void Controller::logMarineRxSnapshot(const QString& reason) const
{
    if (m_marineMgr) {
        m_marineMgr->logRxSnapshot(reason);
    }
}
