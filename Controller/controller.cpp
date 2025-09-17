#include "controller.h"
#include "disp2resmanager.h"
#include "disp2sigmanager.h"
#include "disp2photomanager.h"
#include "sig2dispmanager.h"
#include "data2dispmanager.h"
#include "disp2datamanager.h"
#include "targetdispmanager.h"
#include "disp2monmanager.h"
#include "mon2dispmanager.h"

Q_GLOBAL_STATIC(Controller, ControllerInstance)
Controller* Controller::getInstance() {
    return ControllerInstance();
}

Controller::Controller(QObject* parent)
    : QObject(parent) {
}

void Controller::init()
{
    resMgr = new Disp2ResManager(this);
    sigMgr = new Disp2SigManager(this);
    photoMgr = new Disp2PhotoManager(this);
    sigRecvMgr = new sig2dispmanager(this);
    sigRecvMgr2 = new sig2dispmanager2(this);
    dataRecvMgr = new Data2DispManager(this);
    dataMgr = new Disp2DataManager(this);
    tarMgr = new targetDispManager(this);
    monMgr = new Disp2MonManager(this);
    monRecvMgr  = new Mon2DispManager(this);

    // Optionally connect signals across managers here
    connect(this,&Controller::sendBCParam,resMgr,&Disp2ResManager::sendBCParam);
    connect(this,&Controller::sendTRParam,resMgr,&Disp2ResManager::sendTRParam);
    connect(this,&Controller::sendFCParam,resMgr,&Disp2ResManager::sendFCParam);
    connect(this,&Controller::sendSRParam,resMgr,&Disp2ResManager::sendSRParam);
    connect(this,&Controller::sendWCParam,resMgr,&Disp2ResManager::sendWCParam);
    connect(this,&Controller::sendSPParam,resMgr,&Disp2ResManager::sendSPParam);
    connect(this,&Controller::sendDPParam,resMgr,&Disp2ResManager::sendDPParam);
    connect(this,&Controller::sendPEParam,photoMgr,&Disp2PhotoManager::sendPEParam);
    connect(this,&Controller::sendPEParam2,photoMgr,&Disp2PhotoManager::sendPEParam2);
    connect(this,&Controller::sendDSParam,sigMgr,&Disp2SigManager::sendDSParam);
    connect(this,&Controller::setManual,dataMgr,&Disp2DataManager::setManual);
    connect(this,&Controller::sendSysStart,monMgr,&Disp2MonManager::sendSysStart);
}

Controller::~Controller() {}
