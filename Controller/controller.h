#ifndef CONTROLLER_H
#define CONTROLLER_H
// Controller.h
#pragma once
#include <QObject>
#include "Basic/Protocol.h"
#include "Basic/ConfigManager.h"
class Disp2ResManager;
class Disp2SigManager;
class sig2dispmanager;
class sig2dispmanager2;
class Data2DispManager;
class Disp2DataManager;
class Disp2PhotoManager;
class targetDispManager;
class Disp2MonManager;
class Mon2DispManager;

#define CON_INS Controller::getInstance()
#define CF_INS ConfigManager::instance()

class Controller : public QObject {
    Q_OBJECT
public:
    explicit Controller(QObject *parent = nullptr);
    static Controller* getInstance();
    ~Controller();
    void init();

signals:
    //to res
    void sendBCParam(BatteryControlM param);
    void sendTRParam(TranRecControl param);
    void sendFCParam(DirGramScan param);
    void sendSRParam(ScanRange param);
    void sendWCParam(BeamControl param);
    void sendSPParam(SigProParam param);
    void sendDPParam(DataProParam param);
    void sendPEParam(PhotoElectricParamSet param);
    void sendPEParam2(PhotoElectricParamSet2 param);
    //to sig
    void sendDSParam(DataSet param);
    //from sig
    void dataSaveOK(DataSaveOK datasaveok);
    void dataDelOK(DataDelOK datadelok);
    void offLineStat(OfflineStat offlinestat);
    //from sig
    void detInfoProcess(PointInfo info);
    //from track
    void traInfoProcess(PointInfo info);
    //to data
    void setManual(SetTrackManual data);
    //to mon
    void sendSysStart(StartSysParam data);
    //from tar
    void targetClaRes(TargetClaRes res);
    //from mon
    void monitorParamSend(MonitorParam res);
    void minimizeWindow(bool checked = false);

private:
    Disp2ResManager* resMgr;
    Disp2SigManager* sigMgr;
    Disp2PhotoManager* photoMgr;
    sig2dispmanager* sigRecvMgr;
    sig2dispmanager2* sigRecvMgr2;
    Data2DispManager* dataRecvMgr;
    Disp2DataManager* dataMgr;
    targetDispManager* tarMgr;
    Disp2MonManager* monMgr;
    Mon2DispManager* monRecvMgr;
};

#endif // CONTROLLER_H
