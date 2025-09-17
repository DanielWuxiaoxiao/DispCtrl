#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>
#include <string.h>
#pragma pack(1)

//优先是config.json中的默认值，其次是这里的constexpr值
constexpr char DISP_CTRL_IP[] = "192.168.64.4";
constexpr char DATA_PRO_IP[] = "192.168.64.3";
constexpr char SIG_PRO_IP[] = "192.168.64.3";
constexpr char RES_DIS_IP[] = "192.168.64.3";
constexpr char PHOTO_ELE_IP[] = "192.168.101.10";  //实际光电信息仍然下发给192.168.64.3服务器，该ubuntu服务器上部署了光电系统服务器软件
constexpr char MONITOR_IP[] = "192.168.64.3";  //实际光电信息仍然下发给192.168.64.3服务器，该ubuntu服务器上部署了光电系统服务器软件
constexpr char DISP_CTRL_IP_FOR_PHOTO[] = "192.168.101.14";

constexpr unsigned short RES_DIS_ID = 0xBB01;
constexpr unsigned short SIG_PRO_ID = 0xBB02;
constexpr unsigned short DATA_PRO_ID = 0xBB03;
constexpr unsigned short DISP_CTRL_ID = 0xBB04;
constexpr unsigned short TAR_CLA_ID = 0xBB05;
constexpr unsigned short MONITOR_ID = 0xBB06;

constexpr unsigned short DISP_2_RES_PORT = 6012;
constexpr unsigned short RES_GET_DISP_PORT = 8012;
constexpr unsigned short DISP_2_SIG_PORT = 6002;
constexpr unsigned short SIG_GET_DISP_PORT = 8002;
constexpr unsigned short SIG_2_DISP_PORT1 = 6003;
constexpr unsigned short SIG_2_DISP_PORT2 = 6004;
constexpr unsigned short DISP_GET_SIG_PORT1 = 8003;
constexpr unsigned short DISP_GET_SIG_PORT2 = 8004;

constexpr unsigned short DATA_PRO_2_DISP  = 6006;
constexpr unsigned short DISP_GET_DATA_PORT = 8006;

constexpr unsigned short DISP_2_DATA  = 6008;
constexpr unsigned short DATA_GET_DISP = 8008;

constexpr unsigned short DATA_PRO_2_DISP2  = 6010;  //TBD端口号
constexpr unsigned short DISP_GET_DATA_PORT2 = 8010;

constexpr unsigned short TARGET_2_DISP  = 6017;  //目标识别端口号
constexpr unsigned short DISP_GET_TARGET_PORT = 8017;

constexpr unsigned short DISP_2_MONITOR  = 6018;  //目标识别端口号
constexpr unsigned short MONITOR_GET_DISP_PORT = 8018;

constexpr unsigned short MONITOR_2_DISP  = 6019;  //目标识别端口号
constexpr unsigned short DISP_GET_MONITOR_PORT = 8019;

constexpr unsigned short DISP_2_PHOTO_PORT = 10100;
constexpr unsigned short PHOTO_GET_DISP_PORT = 21001;

constexpr unsigned HEADCODE = 0xFA55FA55;
constexpr unsigned ENDCODE = 0x55FA55FA;

typedef struct _ProtocolFrame
{
    unsigned head;
    unsigned short srcID;
    unsigned short destID;
    unsigned commCount;
    unsigned short dataLen;

    _ProtocolFrame()  //default value
    {
        head= HEADCODE;
    }

}ProtocolFrame;

typedef struct _ProtocolEnd
{
    unsigned char checkCode;
    unsigned end;

    _ProtocolEnd()  //default value
    {
        end= ENDCODE;
    }
}ProtocolEnd;

typedef struct _BatteryControlM
{
    unsigned short mesID;
    unsigned char quadrant1;  //0关 1开
    unsigned char quadrant2;
    unsigned char quadrant3;
    unsigned char quadrant4;

    _BatteryControlM()  //default value
    {
        mesID = 0xAA01;
        quadrant1 = 1;
        quadrant2 = 1;
        quadrant3 = 1;
        quadrant4 = 1;
    }
}BatteryControlM;

typedef struct _PhotoElectricParamSet
{
    unsigned char versionNumber;  //版本号
    unsigned head;                 //帧头
    unsigned short dataLen;
    unsigned short functionNum;  //功能号
    unsigned deviceNum;     //设备编号
    unsigned endDeviceNum;  //终端编号
    unsigned long long timeStamp;
    unsigned long long targetNum;  //引导目标编号
    unsigned lon;
    unsigned lat;
    unsigned short alt;
    short speed;
    unsigned char targetType;
    char checkCode;

    _PhotoElectricParamSet()  //default value
    {
        versionNumber = 0x01;
        head = 0x7D6D5D4C;
        dataLen = 0;
        functionNum = 0x3400;
        deviceNum = 0;
        endDeviceNum = 0;

        timeStamp = 0;
        targetNum = 0;
        targetType = 0;
        alt = 0;
        speed = 0;
        lat = 0;
        lon = 0;
    }
}PhotoElectricParamSet;

typedef struct _HeartbeatPacket {
    unsigned char versionNumber;  //版本号
    unsigned head;                 //帧头
    unsigned short dataLen;
    unsigned short functionNum;  //功能号
    unsigned deviceNum;     //设备编号
    unsigned endDeviceNum;  //终端编号
    char checkCode;
    _HeartbeatPacket()  //default value
    {
        versionNumber = 0x01;
        head = 0x7D6D5D4C;
        dataLen = 0;
        functionNum = 0x0100;
        deviceNum = 0;
        endDeviceNum = 0;
    }
}HeartbeatPacket;

//俯仰角配置
typedef struct _PhotoElectricParamSet2
{
    unsigned char versionNumber;  //版本号
    unsigned head;                 //帧头
    unsigned short dataLen;
    unsigned short functionNum;  //功能号
    unsigned deviceNum;     //设备编号
    unsigned endDeviceNum;  //终端编号
    unsigned long long timeStamp;
    unsigned long long targetNum;  //引导目标编号
    float a;  //0~360
    float e;   //-90~+90
    unsigned short r;  //m
    short speed;
    unsigned char targetType;
    char checkCode;

    _PhotoElectricParamSet2()  //default value
    {
        versionNumber = 0x01;
        head = 0x7D6D5D4C;
        dataLen = 0;
        functionNum = 0x3401;
        deviceNum = 0;
        endDeviceNum = 0;

        timeStamp = 0;
        targetNum = 0;
        targetType = 0;
        a = 0;
        speed = 0;
        e = 0;
        r = 0;
    }
}PhotoElectricParamSet2;

typedef struct _PhotoElectricParamUp  //光电设备上报的设备状态
{
    unsigned char versionNumber;  //版本号
    unsigned head;                 //帧头
    unsigned short dataLen;
    unsigned short functionNum;  //功能号
    unsigned deviceNum;     //设备编号
    unsigned endDeviceNum;  //终端编号
    unsigned long long timeStamp;

    unsigned lon;  //转台经纬高和转台方位俯仰
    unsigned lat;
    unsigned short alt;
    short azi;
    short ele;
    unsigned char workMode;
    //0x01:搜索
//    0x02：自动跟踪目标
//    0x03:手动跟踪目标
//    0x04:记忆跟踪
//    0x05：目标丢失
//    0x06：激光测距
    unsigned char workState;
//    0x00：正常
//    0x01：转台故障；
//    0x02：激光测距故障；
//    0x04：红外故障；
//    0x08：可见光故障
    char checkCode;

    _PhotoElectricParamUp()  //default value
    {
        versionNumber = 0x01;
        head = 0x7D6D5D4C;
        dataLen = 0;
        functionNum = 0x3000;

        timeStamp = 0;
        alt = 0;
        lat = 0;
        lon = 0;
    }
}PhotoElectricParamUp;

typedef struct _TranRecvControl
{
    unsigned short mesID;
    unsigned char recv;  //0关 1开
    unsigned char tran;

    _TranRecvControl()  //default value
    {
        mesID = 0xAA02;
        tran = 0;
        recv = 1;
    }
}TranRecControl;

typedef struct _DirGramScan
{
    unsigned short mesID;
    unsigned char gramControl; //  0 方位方向图 1 俯仰方向图  0 默认值
    unsigned char waveID;  // 默认值：8(2us-15MHz-40us)
    short scanStart;
    short scanEnd;
    short scanStep;   //0.01°量化
    unsigned short tranStart;
    unsigned short sampleStart;
    unsigned short sampleLen;  //0.1us量化

    _DirGramScan()  //default value
    {
        memset(this,0,sizeof (_DirGramScan));
        mesID = 0xAA03;
        gramControl = 0;
        waveID = 8;
        tranStart = 10;
        scanStart = -4500;
        scanEnd = 4500;
        scanStep = 1;
    }
}DirGramScan;

typedef struct _ScanRange
{
    unsigned short mesID;
    unsigned char place;  //0水平放置  1竖直放置
    unsigned char method;  // 0先列后行 1先行后列
    unsigned char workMode; // 0 TWS 1 TAS
    short azi; //阵面的物理指向姿态设置 0.01°
    short ele;

    _ScanRange()  //default value
    {
        memset(this,0,sizeof (_ScanRange));
        mesID = 0xAA04;
        place = 0;
        method = 0;
        workMode = 0;
        azi = 2000;
        ele = 1500;
    }
}ScanRange;

typedef struct _BeamControl
{
    unsigned short mesID;
    //unsigned short pulseNum1;
    unsigned char freqID;  //0-9 15.6GHZ~16.5GHz
    unsigned char type;  // 1 线性负调频，2 线性正调频
    short aziStart;
    short aziEnd;
    short aziStep;
    unsigned char flagNum;
    unsigned char beam1Flag; //0不起效 1起效
    unsigned char beam1Code; //波形码 0~11

    unsigned short pulseNum1;
    unsigned short tranStart1; // 0.1us
    unsigned short sampleStart1;
    unsigned short sampleLen1;
    short elestart1; // 0.01°量化
    short eleend1;
    short elestep1;

    unsigned char beam2Flag; //0不起效 1起效
    unsigned char beam2Code; //波形码 0~11
    unsigned short pulseNum2;
    unsigned short tranStart2; // 0.1us
    unsigned short sampleStart2;
    unsigned short sampleLen2;
    short elestart2; // 0.01°量化
    short eleend2;
    short elestep2;

    unsigned char beam3Flag; //0不起效 1起效
    unsigned char beam3Code; //波形码 0~11
    unsigned short pulseNum3;
    unsigned short tranStart3; // 0.1us
    unsigned short sampleStart3;
    unsigned short sampleLen3;
    short elestart3; // 0.01°量化
    short eleend3;
    short elestep3;

    _BeamControl()  //default value
    {
        memset(this,0,sizeof (_BeamControl));
        flagNum = 2;
        mesID = 0xAA05;
        freqID = 2;
        type = 2;
        pulseNum1 = 256;
        pulseNum2 = 256;
        pulseNum3 = 256;

        aziStart = 4500;
        aziEnd = 13500;
        aziStep = 300;

        beam1Flag = 1;
        beam1Code = 6;
        tranStart1 = 10;
        sampleStart1 = 30;
        sampleLen1 = 100; //都需要配置，全部是界面的默认参数，需要进行计算
        elestart1 = 0;
        eleend1 = 6000;
        elestep1 = 600;

        beam2Flag = 1;
        beam2Code = 9;
        tranStart2 = 10;
        sampleStart2 = 120;
        sampleLen2 = 370;
        elestart2 = 0;
        eleend2 = 2000;
        elestep2 = 600;

        beam3Flag = 0;
        beam3Code = 10;
        tranStart3 = 10;
        sampleStart3 = 270;
        sampleLen3 = 350;
        elestart3 = -1400;
        eleend3 = -800;
        elestep3 = 600;
    }
}BeamControl;

typedef struct _SigProParam
{
    unsigned short mesID;
    unsigned short noise;  //0.1dB
    unsigned short thresh1;
    unsigned short thresh2;
    unsigned short clutterThresh; // 0.1us
    unsigned char clutterMapFlaseRate;  // 01e-2
    unsigned char CFARType;  // 0CA（默认） 1：GO 2 :SO 3:OS
    unsigned char disProWin; //
    unsigned char disRefWin; //
    unsigned char dopProWin; // 0.1us
    unsigned char dopRefWin;
    unsigned char MTDWinType;  //0 FFT 1 FIR
    unsigned char clutterMode; //0 静态幅度 1 动态幅度 2 静态幅相 3 动态幅相
    unsigned char clutterChannelWidth; //杂波通道宽度
    unsigned char clutterUnitWin; // 0.1us
    unsigned char clutterIter;
    unsigned char algorithmSwitch;  //0关 1开 D0杂波感知 D1副瓣匿影 D2 近区补盲
    unsigned char reserve[10];

    _SigProParam()  //default value
    {
        memset(this,0,sizeof (_SigProParam));
        mesID = 0xAA06;
        noise = 350;
        thresh1 = 90;
        thresh2 = 150;
        clutterThresh = 170;
        clutterMapFlaseRate = 4;
        CFARType = 0;
        disProWin = 2;
        disRefWin = 16;
        dopProWin = 2;
        dopRefWin = 16;
        MTDWinType = 1;
        clutterMode = 1;
        clutterChannelWidth = 5;
        clutterUnitWin = 1;
        clutterIter = 19;
        algorithmSwitch = 7;
    }
}SigProParam;

typedef struct _DataProParam
{
    unsigned short mesID;
    unsigned char startWinLen;
    unsigned char startPoint;
    unsigned char endWinLen;
    unsigned char endPoint;
    unsigned short noiseVar;  // 单位0.01
    unsigned short trackDisLower;  // 默认3 N倍sigma
    unsigned short trackDisUpper; //
    unsigned short trackAziThresh; //
    unsigned short trackEleThresh;
    unsigned short trackVelThresh;
    unsigned short trackStatThresh;  //默认 16
    unsigned char accuDisGate; //凝聚距离波们
    unsigned char accuAziGate; //杂波通道宽度
    unsigned char accuEleGate; // 0.1us
    unsigned char accuVelGate;
    unsigned char reserve[12];

    _DataProParam()  //default value
    {
        memset(this,0,sizeof (_DataProParam));
        mesID = 0xAA07;
        startWinLen = 4;
        startPoint = 3;
        endWinLen = 3;
        endPoint = 3;
        noiseVar = 1;
        trackDisLower = 10;
        trackDisUpper = 300;
        trackAziThresh = 15;
        trackEleThresh = 20;
        trackVelThresh = 100;
        trackStatThresh = 160;
        accuDisGate = 15;
        accuAziGate = 4;
        accuEleGate = 7;
        accuVelGate = 60;
    }
}DataProParam;

typedef struct _DataSave
{
    unsigned short mesID;
    unsigned char saveSwitch;
    unsigned char dataID;

    _DataSave()  //default value
    {
        memset(this,0,sizeof (_DataSave));
        mesID = 0xCC01;
    }
}DataSave;

typedef struct _DataDel
{
    unsigned short mesID;
    unsigned char dataID;

    _DataDel()  //default value
    {
        memset(this,0,sizeof (_DataDel));
        mesID = 0xCC02;
    }
}DataDel;

typedef struct _DataSaveOK
{
    unsigned short mesID;
    unsigned char dataID;
    unsigned short dataSize;

    _DataSaveOK()  //default value
    {
        memset(this,0,sizeof (_DataSaveOK));
        mesID = 0xDD02;
    }
}DataSaveOK;

typedef struct _DataDelOK
{
    unsigned short mesID;
    unsigned char dataID;

    _DataDelOK()  //default value
    {
        memset(this,0,sizeof (_DataDelOK));
        mesID = 0xDD03;
    }
}DataDelOK;

typedef struct _OfflineDel
{
    unsigned short mesID;
    unsigned char onSwitch;
    unsigned char dataID;

    _OfflineDel()  //default value
    {
        memset(this,0,sizeof (_OfflineDel));
        mesID = 0xCC03;
    }
}OfflineDel;

typedef struct _DataSet
{
    DataSave save;
    DataDel del;
    OfflineDel off;
    bool ifsave;
    bool ifoffline;
    bool ifdel;
    _DataSet()  //default value
    {
        ifdel = 0;
        ifoffline = 0;
        ifsave = 0;
    }
}DataSet;

typedef struct _OfflineStat
{
    unsigned short mesID;
    unsigned char delStat;  //0正常 1离线
    unsigned char dataID;

    _OfflineStat()  //default value
    {
        memset(this,0,sizeof (_OfflineStat));
        mesID = 0xDD04;
    }
}OfflineStat;

typedef struct _SysHead
{
    unsigned head;
    unsigned short srcID;
    unsigned short destID;
    unsigned char flag;
    unsigned char infoUnit;
    unsigned short dataLen;
    unsigned commCount;
    unsigned short year : 12;
    unsigned short month : 4;
    unsigned char day;
    unsigned char hour;
    unsigned minute : 6;
    unsigned second : 6;
    unsigned msecond : 10;
    unsigned usecond : 10;

    _SysHead()  //default value
    {
        head = 0xFA55FA55;
        srcID = 0xBB01;
        destID = 0xBB06;
    }
}SysHead;

typedef struct _SigData
{
    SysHead head;
    unsigned char reserve[8];
    unsigned num;
    unsigned char CPIType;
    unsigned char CPIGroupID;
    unsigned char CPInum;
    unsigned char CPIID;
    unsigned short ADMMPulseNum;
    unsigned short pulseID;
    unsigned freq;  //1MH步进
    unsigned char tranCode : 6;
    unsigned char tranType : 2; // 0 单载频 1 线性正调频 2 线性负调频
    unsigned char reserve1[3];
    unsigned pulseSampleNum;
    unsigned pulseWidth;  //精度 0.0125us
    unsigned echoPeriod;
    unsigned sampleRate; // 0.1MhZ步进
    unsigned bandWidth; //
    unsigned char channelCode; // 回波光纤编号 0-3
    unsigned char timeCode;
    unsigned char workMode : 4;
    unsigned char workMethod : 4;
    unsigned char reserve2[9];
    short ele; // 0.01°量化
    unsigned char reserve3[6];
    short azi;
    unsigned char reserve4[10];
    unsigned short freqCode;
    unsigned char reserve5[4];
    unsigned short trackTarNum;
    unsigned short trackDisUnit;
    unsigned trackID;
    short trackTarVel; //0.1m/s精度
    unsigned char reserve6[40];
    unsigned char reserve7[64];

    _SigData()  //default value
    {
    }
}SigData;

typedef struct _detResult
{
    unsigned short mesID;
    SigData sigData;
    unsigned char RadarID;
    unsigned short detNum;

    _detResult()  //default value
    {
        mesID = 0xDD01;
    }
}detResult;

typedef struct _detInfo
{
    float dis;
    float vel;
    float azi;
    float ele;
    float altitute;
    float amp;
    float CFARSNR;
    float statSNR;
    float aziBeam;
    float eleBeam;
    unsigned disChannel;
    unsigned dopChannel;
    unsigned reserve;
    unsigned reserve1;
}detInfo;

typedef struct _TrackResult
{
    unsigned short mesID;
    unsigned short trackNum;

    _TrackResult()  //default value
    {
        mesID = 0xEE01;
    }
}TrackResult;

typedef struct _trackInfo
{
    unsigned short batch;
    unsigned short CPIID;
    unsigned UTCtime;
    unsigned nsecond;
    unsigned char statMethod;  // 0滤波  1 外推/预测  ==2 时 直接消掉
    float amp;
    float SNR;
    float dis;
    float azi;
    float ele;
    float altitute;
    float vel;
    float spaceVel;
    float accelerate;
    unsigned reserve;
    unsigned reserve1;
    unsigned reserve2;
}trackInfo;

typedef struct _TBDInfo
{
    unsigned short batch;
    unsigned length;
}TBDInfo;

typedef struct _TBDPoint
{
    unsigned short CPIID;
    float UTCtime;
    float amp;
    float SNR;
    float dis;
    float azi;
    float ele;
    float altitute;
    float vel;
    unsigned reserve;
    unsigned reserve1;
    unsigned reserve2;
}TBDPoint;

typedef struct _PointInfo
{
    unsigned type; //    det = 1,    trak = 2 , TBD = 3
    float range;
    float azimuth;
    float elevation;
    float SNR;
    float speed;
    float altitute;
    float amp;
    unsigned short batch;
    unsigned char statMethod;
}PointInfo;

enum PointType
{
    Detection = 1,
    Track = 2
};

typedef struct _SetTrackManual
{
    unsigned short mesID;
    unsigned short batchID;

    _SetTrackManual()  //default value
    {
        mesID = 0xDF01;
    }
}SetTrackManual;

typedef struct _TargetClaRes
{
    unsigned short mesID;
    unsigned short batchID;
    unsigned char claRes; //0 未确认 1 无人机 2 行人 3 车辆 4 鸟 5 其它

    _TargetClaRes()  //default value
    {
        mesID = 0xDB01;
        claRes = 0;
    };
}TargetClaRes;

typedef struct _MonitorParam
{
    unsigned short mesID;
    unsigned char dataProSta;  //0运行状态正常 1运行状态异常
    unsigned char beamConSta;  //2 数据处理软件启动成功 3 数据处理软件启动失败
    unsigned char sigProSta;

    _MonitorParam()  //default value
    {
        mesID = 0xCF01;
        dataProSta = 0;
        beamConSta = 0;
        sigProSta = 0;
    };
}MonitorParam;

typedef struct _StartSysParam
{
    unsigned short mesID;
    unsigned char sta;  //0关闭 1启动

    _StartSysParam()  //default value
    {
        mesID = 0xDA01;
        sta = 1;
    };
}StartSysParam;

unsigned char calculateXOR(const char* data, unsigned len);
char checkAccusation(char* data, unsigned len);

char* packData(char* data, unsigned dataLen, unsigned short srcID, unsigned short destID, unsigned commCount);

#pragma pack()

#endif // PROTOCOL_H
