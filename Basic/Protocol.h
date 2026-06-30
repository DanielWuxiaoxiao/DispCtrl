/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:17
 * @Description: 
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>
#include <string.h>
#include <QList>
#include <QMetaType>
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
constexpr unsigned short RADAR_CTRL_ID = 0xBB07;   // 外部雷控软件ID

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
constexpr unsigned short DATA_PRO_2_DISP3  = 6020;  //协同航迹端口号
constexpr unsigned short DISP_GET_DATA_PORT3 = 8020;

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

// =========================
// 外部雷控/调度链路协议（系统控制 + AD 数据）
// =========================

// 基本常量
constexpr unsigned EXTERNAL_SYSCTRL_HEAD = 0xFA55FA55;
constexpr unsigned EXTERNAL_SYSCTRL_TAIL = 0x55FA55FA;
constexpr unsigned EXTERNAL_SYSCTRL_CONTENT_LEN = 504;   // 4(头)+504(内容)+4(尾)=512B
constexpr unsigned EXTERNAL_SYSCTRL_FRAME_LEN = 512;

constexpr unsigned EXTERNAL_AD_HEAD = 0x7FFFBC1C;
constexpr unsigned EXTERNAL_AD_TAIL = 0x7FFF5A5A;
constexpr unsigned EXTERNAL_AD_HEADER_RESERVE_LEN = 19;
constexpr unsigned EXTERNAL_AD_TRAILER_RESERVE_LEN = 12;

// 系统控制帧：显控→外部雷控，长度固定 512B
struct ExternalSystemControl512 {
    unsigned head{EXTERNAL_SYSCTRL_HEAD};
    unsigned char content[EXTERNAL_SYSCTRL_CONTENT_LEN]{};
    unsigned tail{EXTERNAL_SYSCTRL_TAIL};
};

// 外部雷控回执 64B（仍按长度透传）
struct ExternalControlAck64 {
    unsigned char data[64] = {0};
};

// 伺服控制 32B（显控→外部雷控）
struct ExternalServoCmd32 {
    unsigned char data[32] = {0};
};

// 伺服回执 32B（外部雷控→显控）
struct ExternalServoAck32 {
    unsigned char data[32] = {0};
};

// AD 数据头（固定部分，不含复数采样和尾部预留）
struct ExternalAdHeader {
    unsigned head{EXTERNAL_AD_HEAD};          // 0x7FFFBC1C
    unsigned char arrayId{0};                 // 传输链路/阵面编号 0~3
    unsigned char dataWaveId{0};              // 数据波形 ID，数字阵场景使用
    unsigned short beamCount{0};              // 波束数量（相控阵 4，数字阵 3×波束指向）
    unsigned short pulseId{0};                // 脉冲 ID
    unsigned short samplesPerPulse{0};        // 单脉冲采样点数（距离单元个数）
    unsigned char waveformIndex{0};           // 控制表内的波形编号（1~3）
    unsigned char reserved[EXTERNAL_AD_HEADER_RESERVE_LEN]{}; // 19B 预留
};

// AD 数据尾部预留（12B）
struct ExternalAdTrailerReserve {
    unsigned char reserved[EXTERNAL_AD_TRAILER_RESERVE_LEN]{};
};

typedef struct _TranRecvControl
{
    unsigned short mesID;
    unsigned char recv;  //0关 1开
    unsigned char tran;  //0关 1开

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

// 伺服控制（显控→资源调度→雷达控制），信息编号 0xAA03
// 字段：指令/转速/方位角(0.01°)，预留7字节
typedef struct _ServoControlParam
{
    unsigned short mesID;
    unsigned char cmd;    // 0停转 1转动 2寻位 3归北
    unsigned char speed;  // 秒/转
    unsigned short az;    // 0.01°量化 [0,36000]
    unsigned char reserve[7];

    _ServoControlParam()
    {
        memset(this, 0, sizeof(_ServoControlParam));
        mesID = 0xAA03;
    }
} ServoControlParam;

typedef struct _ScanRange
{
    unsigned short mesID;
    unsigned char workMode; // 0 TWS 1 TAS

    _ScanRange()  //default value
    {
        memset(this,0,sizeof (_ScanRange));
        mesID = 0xAA04;
        workMode = 0;
    }
}ScanRange;

typedef struct _BeamControl
{
    unsigned short mesID;
    unsigned char freqID;  //0-7: 9.0GHz~9.8GHz (默认5:9.4GHz)
    unsigned char type;  // 1 线性负调频，2 线性正调频（默认）
    int aziStart;      // 0.01°量化 (默认-4500)
    int aziEnd;        // 0.01°量化 (默认4500)
    short aziStep;       // 0.01°量化 (默认400)
    unsigned char scene;   //下发场景 1~5，给数字阵使用
    unsigned char flagNum;  // 波形起效数量
    unsigned short pulseNum;  // 积累脉冲数 32,64,128,256,512,1024 (默认128)

    unsigned char beam1Flag; //0不起效 1起效（默认1）
    unsigned char beam1Code; //波形码 0~11 (默认6)
    unsigned short sampleStart1;   // 采样起始 0.1us量化
    unsigned short sampleEnd1;     // 采样终止 0.1us量化
    short elestart1; // 俯仰起始 0.01°量化 (默认0)
    short eleend1;   // 俯仰终止 0.01°量化 (默认3000)
    short elestep1;  // 俯仰间隔 0.01°量化 (默认400)

    unsigned char beam2Flag; //0不起效 1起效（默认1）
    unsigned char beam2Code; //波形码 0~11 (默认9)
    unsigned short sampleStart2;   // 采样起始 0.1us量化
    unsigned short sampleEnd2;     // 采样终止 0.1us量化
    short elestart2; // 俯仰起始 0.01°量化 (默认0)
    short eleend2;   // 俯仰终止 0.01°量化 (默认1500)
    short elestep2;  // 俯仰间隔 0.01°量化 (默认400)

    unsigned char beam3Flag; //0不起效 1起效（默认0）
    unsigned char beam3Code; //波形码 0~11 (默认11)
    unsigned short sampleStart3;   // 采样起始 0.1us量化
    unsigned short sampleEnd3;     // 采样终止 0.1us量化
    short elestart3; // 俯仰起始 0.01°量化 (默认0)
    short eleend3;   // 俯仰终止 0.01°量化 (默认400)
    short elestep3;  // 俯仰间隔 0.01°量化 (默认400)

    _BeamControl()  //default value
    {
        memset(this,0,sizeof (_BeamControl));
        mesID = 0xAA05;
        freqID = 5;      // 默认5: 9.4GHz
        type = 2;        // 默认线性正调频
        aziStart = -4500;  // -45°
        aziEnd = 4500;     // 45°
        aziStep = 400;     // 4°
        flagNum = 2;
        pulseNum = 128;  // 统一的积累脉冲数，默认128

        beam1Flag = 1;
        beam1Code = 6;   // 1-15-15
        sampleStart1 = 30;
        sampleEnd1 = 130;
        elestart1 = 0;
        eleend1 = 3000;  // 30°
        elestep1 = 400;  // 4°

        beam2Flag = 1;
        beam2Code = 9;   // 10-15-50
        sampleStart2 = 120;
        sampleEnd2 = 490;
        elestart2 = 0;
        eleend2 = 1500;  // 15°
        elestep2 = 400;  // 4°

        beam3Flag = 0;   // 默认不起效
        beam3Code = 11;  // 35-15-175
        sampleStart3 = 270;
        sampleEnd3 = 1730;
        elestart3 = 0;
        eleend3 = 400;   // 4°
        elestep3 = 400;  // 4°
    }
}BeamControl;

// TWS模式参数结构体（整合三个子参数）
typedef struct _TWSModeParam
{
    ScanRange scanRange;           // 工作模式
    BeamControl beamControl;       // 波形及采样控制
    ServoControlParam servoControl; // 伺服控制

    _TWSModeParam()
    {
        // 使用各自结构体的默认值
    }
} TWSModeParam;

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

/**
 * @brief 经纬高信息上报（信号处理→显控）
 * @details 消息ID: 0xDD05
 *          纬度/经度单位: 0.000000001°（需除以1e9转为度）
 *          高程单位: 0.01m（需除以100转为米）
 */
typedef struct _GeoLocationReport
{
    unsigned short mesID;
    qint64 latitude;    // [-90, 90] 单位: 0.000000001°
    qint64 longitude;   // [-180, 180] 单位: 0.000000001°
    qint32 altitude;    // 单位: 0.01m

    _GeoLocationReport()
    {
        memset(this, 0, sizeof(_GeoLocationReport));
        mesID = 0xDD05;
    }
}GeoLocationReport;

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

// 三类航迹通用帧头
// 0xEE01: 常规航迹
// 0xEE02: TBD 航迹
// 0xEE03: 协同航迹
// 帧格式统一为：TrackResult + N * trackInfo
// 其中 mesID 用于区分航迹类别，trackNum 表示后续 trackInfo 数量
typedef struct _TrackResult
{
    unsigned short mesID;
    unsigned short trackNum;

    _TrackResult()  //default value
    {
        mesID = 0xEE01;
    }
}TrackResult;

constexpr unsigned short TRACK_INFO_MSG_ID = 0xEE01;
constexpr unsigned short TBD_TRACK_MSG_ID = 0xEE02;
constexpr unsigned short COOPERATIVE_TRACK_MSG_ID = 0xEE03;

// 航迹点通用结构
// 三个通道统一复用相同的 trackInfo 定义，仅通过 mesID 和端口号区分来源

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
    unsigned targetRecResult;  // 目标识别结果：0=其它，1=无人机
    unsigned reserve1;
    unsigned reserve2;
}trackInfo;

// 伺服控制回送 0xDE01
typedef struct _ServoCtrlRet
{
    unsigned short mesID;
    unsigned char result;   // 0 失败 1 成功 2 执行中
    unsigned char cmd;      // 0 停转 1 转动 2 寻位 3 归北
    unsigned char speed;    // 秒/转
    unsigned short azCur;   // 0.01°
    unsigned char reserve[6];

    _ServoCtrlRet()
    {
        mesID = 0xDE01;
        result = 0;
        cmd = 0;
        speed = 0;
        azCur = 0;
        memset(reserve, 0, sizeof(reserve));
    }
}ServoCtrlRet;

// BIT 上报 0xDE02
typedef struct _BITReport
{
    unsigned short mesID;
    unsigned char bitGroup;     // BIT状态信息组（位标志）
    // [7]: 阵面发射开启=1，不开启=0
    // [6]: 发射占空比报警=1，正常=0
    // [5]: 发射脉宽报警=1，正常=0
    // [4]: 阵面接收开启=1，不开启=0
    // [3]: 频率源正常=1，不正常=0
    // [2]: 数字收发板建链=1，断链=0
    // [1]: 伺服正常工作=1，不正常=0
    // [0]: 北斗正常工作=1，不正常=0
    unsigned char powerState;   // BIT状态信息1：[1]=蓝牙正常，[0]=波控板电源正常
    unsigned short fpgaTemp;    // BIT状态信息2：数字收发板FPGA温度，0.1°量化
    unsigned short panelTemp;   // BIT状态信息3：阵面温度，0.1°量化
    unsigned short yaw;         // 阵面偏航角度 [0,360]，0.01°量化
    unsigned char subArrayPower[5];  // 子阵电源BIT信息（36bit，5字节）
    unsigned short scanAngle;   // 扫描角度 [0,360]，0.01°量化（用于显控扫描线绘制）
    unsigned char reserve[22];  // 预留22字节

    _BITReport()
    {
        mesID = 0xDE02;
        bitGroup = 0;
        powerState = 0;
        fpgaTemp = 0;
        panelTemp = 0;
        yaw = 0;
        memset(subArrayPower, 0, sizeof(subArrayPower));
        scanAngle = 0;
        memset(reserve, 0, sizeof(reserve));
    }
}BITReport;

typedef struct _PointInfo
{
    unsigned type; // det = 1, track = 2, TBD = 3, cooperative = 4
    float range;
    float azimuth;
    float elevation;
    float SNR;
    float speed;
    float altitute;
    float amp;
    unsigned int batch;
    unsigned char statMethod;
    unsigned targetRecResult;  // 目标识别结果：0=其它，1=无人机
}PointInfo;

enum PointType
{
    Detection = 1,
    Track = 2,
    TBDPointType = 3,
    CooperativeTrackPointType = 4
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

// =========================
// 道路点经纬度下发（显控→数处），消息ID: 0xDF02
// 用于将量程内的OSM道路点坐标发送给数据处理模块
// =========================

constexpr int ROAD_POINTS_PER_FRAME = 150;  // 每帧最大道路点数（MTU安全）

// 单个道路点经纬度
typedef struct _RoadPointGeo {
    int latitude;    // 单位: 0.0000001°（1e-7度）
    int longitude;   // 单位: 0.0000001°（1e-7度）

    _RoadPointGeo() {
        latitude = 0;
        longitude = 0;
    }
}RoadPointGeo;

// 道路点数据帧头（显控→数处）
typedef struct _RoadPointFrame {
    unsigned short mesID;          // 0xDF02
    unsigned int totalPoints;      // 全部道路点总数
    unsigned short frameIndex;     // 当前帧序号（从0开始）
    unsigned short frameTotal;     // 总帧数
    unsigned short pointsInFrame;  // 本帧包含点数
    // 紧跟 pointsInFrame 个 RoadPointGeo

    _RoadPointFrame() {
        memset(this, 0, sizeof(_RoadPointFrame));
        mesID = 0xDF02;
    }
}RoadPointFrame;

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
    unsigned char targetRecSta; // 目标识别软件状态 0正常 1异常 2启动成功 3启动失败

    _MonitorParam()  //default value
    {
        mesID = 0xCF01;
        dataProSta = 0;
        beamConSta = 0;
        sigProSta = 0;
        targetRecSta = 0;
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

// =========================
// 扩展功能新协议（A显 / 波束调度）
// =========================

// 新增端口常量（避免与现有端口冲突）
constexpr unsigned short SIG_2_DISP_PORT3   = 6005;   // 信处 → 显控：A显数据
constexpr unsigned short DISP_GET_SIG_PORT3 = 8005;   // 本机监听端口
constexpr unsigned short RES_2_DISP_PORT    = 6013;   // 资源调度 → 显控：波束调度报告
constexpr unsigned short DISP_GET_RES_PORT  = 8013;   // 本机监听端口

// A显数据帧 (信处 → 显控)，消息ID: 0xEE10
// 每帧包含一条方位线的原始回波幅度序列 + CFAR门限
// 结构：AScanFrame头 + pointNum 个 float (PC后幅度dB)
//       + pointNum 个 float (MTD后幅度dB)
typedef struct _AScanFrame
{
    unsigned short mesID;        // 0xEE10
    unsigned short pointNum;     // 采样点数（距离单元个数）
    short  azimuth;              // 0.01° 量化，当前方位角
    short  elevation;            // 0.01° 量化，当前俯仰角
    float  rangeResM;            // 距离分辨率（米/单元）
    float  cfar;                 // CFAR统一门限 (dB)
    unsigned char waveID;        // 波形编号
    unsigned char reserved[5];

    _AScanFrame()
    {
        memset(this, 0, sizeof(_AScanFrame));
        mesID = 0xEE10;
    }
} AScanFrame;
// 帧头之后紧跟:
//   float pcAmps[pointNum];   PC后幅度 (dB)
//   float mtdAmps[pointNum];  MTD后幅度 (dB)

// 波束时间槽（单个时间槽描述）
typedef struct _BeamSlot
{
    unsigned char  taskType;     // 0=搜索 1=跟踪 2=空闲
    unsigned char  beamID;       // 波束编号
    unsigned short azimuth;      // 波束方位 0.01°
    short          elevation;    // 波束俯仰 0.01°
    unsigned int   startUs;      // 槽起始时间（相对帧起始，µs）
    unsigned int   durationUs;   // 槽持续时间（µs）
    unsigned int   batchID;      // 关联航迹批号（taskType==1时有效）
    unsigned char  reserved[4];
} BeamSlot;

// 波束调度报告帧头 (资源调度 → 显控)，消息ID: 0xEE11
// 帧头之后紧跟 slotNum 个 BeamSlot
typedef struct _BeamScheduleReport
{
    unsigned short mesID;        // 0xEE11
    unsigned short slotNum;      // 本帧槽数量
    unsigned int   frameTimeUs;  // 帧总时长（µs）
    unsigned int   frameSeq;     // 帧序号（循环计数）
    unsigned char  reserved[4];

    _BeamScheduleReport()
    {
        memset(this, 0, sizeof(_BeamScheduleReport));
        mesID = 0xEE11;
    }
} BeamScheduleReport;

#pragma pack()

// Qt 元类型注册（必须在 #pragma pack() 之后）
Q_DECLARE_METATYPE(PointInfo)
Q_DECLARE_METATYPE(QList<PointInfo>)

// 便于跨线程传递的 Q_DECLARE_METATYPE
Q_DECLARE_METATYPE(BeamScheduleReport)
Q_DECLARE_METATYPE(AScanFrame)

// =========================
// GCS 链路协议（雷达平台 ↔ GCS 地面站）
// 帧格式：[0xF6 0x6F | src | dst | cmd | lenL lenH | params... | checksum]
// 校验和：累加 src+dst+cmd+lenL+lenH+params，取低8位
// =========================

constexpr unsigned char GCS_FRAME_HEAD0   = 0xF6;
constexpr unsigned char GCS_FRAME_HEAD1   = 0x6F;
constexpr unsigned char GCS_ADDR_RADAR    = 0x20;  // 雷达平台地址
constexpr unsigned char GCS_ADDR_GCS      = 0xC0;  // GCS地址

constexpr unsigned char GCS_CMD_HEARTBEAT = 0xA4;  // 心跳帧
constexpr unsigned char GCS_CMD_STATUS    = 0x50;  // 基本工作状态查询
constexpr unsigned char GCS_CMD_POSITION  = 0x51;  // 设备位置查询
constexpr unsigned char GCS_CMD_TARGET    = 0x52;  // 目标下发

// GCS帧头（8字节固定头，不含参数和校验）
#pragma pack(1)
struct GcsFrameHeader {
    unsigned char head0   = GCS_FRAME_HEAD0;
    unsigned char head1   = GCS_FRAME_HEAD1;
    unsigned char srcAddr = GCS_ADDR_RADAR;
    unsigned char dstAddr = GCS_ADDR_GCS;
    unsigned char cmd     = 0;
    unsigned char lenL    = 0;  // 参数长度低字节
    unsigned char lenH    = 0;  // 参数长度高字节
};

/**
 * @brief GCS目标下发参数块（0x52命令，41字节参数）
 * @details 字节7-10: Param1 目标编号(uint32)
 *          字节11-18: Param2 目标经度(double, 度)
 *          字节19-26: Param3 目标纬度(double, 度)
 *          字节27-30: Param4 目标高度(float, 米)
 *          字节31-34: Param5 航速(float, m/s)
 *          字节35-38: Param6 航向(float, 0-360度)
 *          字节39:    Param7 目标类型(0x00未知, 0x40无人机)
 *          字节40-43: Param8 UTC时间戳(uint32, s)
 *          字节44-47: Param9 频点(float, 10kHz, 预留填0)
 *          字节48:    校验和
 */
struct GcsTargetParams {
    quint32 targetId    = 0;       // Param1
    double  longitude   = 0.0;     // Param2 目标经度
    double  latitude    = 0.0;     // Param3 目标纬度
    float   altitude    = 0.0f;    // Param4 目标高度(m)
    float   speed       = 0.0f;    // Param5 航速(m/s)
    float   heading     = 0.0f;    // Param6 航向(度)
    quint8  targetType  = 0x00;    // Param7
    quint32 utcTime     = 0;       // Param8
    float   freqReserve = 0.0f;    // Param9 预留
};

// 心跳回执参数：无参数（lenL=0x00, lenH=0x00）
struct GcsHeartbeatAck {
    unsigned char dstAddr = GCS_ADDR_GCS;  // 发给GCS
};
#pragma pack()

// =============================================================================
// 激光侦察上报协议（显控 → 激光控制终端）—— 详见 docs/光电跟踪与激光上报协议.md 第4节
// 仅实现“激光终端”相关功能（不含光电转台/手动跟踪）。全部小端、1字节对齐。
// 完整侦察帧 = LaserFrameHeader + LaserReconData(12) + N×LaserTargetInfo(64) + LaserFrameTail(4)
// 注意：LaserFrameHeader 字段逐项相加为 18 字节（文档/RadarAPP 注释里的“20”为笔误，以本结构体为准，已与设备端字节兼容）
// =============================================================================
#pragma pack(push, 1)

// 激光协议常量
constexpr unsigned char LASER_FRAME_HEAD0 = 0xEB;
constexpr unsigned char LASER_FRAME_HEAD1 = 0x90;
constexpr unsigned char LASER_FRAME_TAIL0 = 0x4C;
constexpr unsigned char LASER_FRAME_TAIL1 = 0x5A;
constexpr unsigned short LASER_FT_RECON   = 0x0300;  // 侦察帧类型
constexpr unsigned short LASER_FT_STATUS  = 0x0200;  // 状态帧类型（隐含心跳）
constexpr unsigned short LASER_SENDER_ID  = 2100;    // 雷达端设备ID(210X)
constexpr unsigned short LASER_RECEIVER_ID = 5100;   // 激光端设备ID(510X)
constexpr unsigned short LASER_RECON_TYPE_ID = 1;    // 1=雷达侦察结果
constexpr int LASER_MAX_TARGETS = 10;                // 单帧最多10目标

// 8字节时标
typedef struct _LaserDataTime {
    unsigned char  year;     // 年份后两位 [0,99]
    unsigned char  month;    // [1,12]
    unsigned char  day;      // [1,31]
    unsigned char  hour;     // [0,24)
    unsigned char  minute;   // [0,59]
    unsigned char  second;   // [0,59]
    unsigned short msecond;  // [0,999]
} LaserDataTime;

// 帧头(20字节)
typedef struct _LaserFrameHeader {
    unsigned char  frameHead[2]; // 0xEB 0x90
    unsigned short frameType;    // 0x0300 侦察帧
    unsigned short senderID;     // 2100
    unsigned short receiverID;   // 5100
    LaserDataTime  timeStamp;    // 发送时刻
    unsigned short dataLen;      // 数据内容域字节数 n
} LaserFrameHeader;

// 帧尾(4字节)
typedef struct _LaserFrameTail {
    unsigned short checkSum;     // 从frameType起到数据内容止，按字节累加取低16位
    unsigned char  frameTail[2]; // 0x4C 0x5A
} LaserFrameTail;

// 侦察数据固定部分(12字节)
typedef struct _LaserReconData {
    unsigned short typeID;       // 1=雷达侦察结果
    unsigned int   contentLen;   // 侦察数据域字节数
    unsigned int   dataSeq;      // 侦察序号，递增
    unsigned short targetCount;  // 目标数N，最大10
} LaserReconData;

// 状态数据-扫描范围信息(18字节)
typedef struct _LaserScanRangeInfo {
    unsigned short rangeID;
    float startAzimuth;
    float endAzimuth;
    float startElevation;
    float endElevation;
} LaserScanRangeInfo;

// 状态数据固定部分(13字节)，1s周期上报，隐含心跳
// 完整状态帧数据内容 = LaserStatusData(13) + scanAreaCount × LaserScanRangeInfo(18)
typedef struct _LaserStatusData {
    unsigned short typeID;        // 1=雷达设备状态
    unsigned short contentLen;    // 状态数据域字节数（= 本结构+扫描区 − typeID − contentLen）
    unsigned int   statusSeq;     // 状态序号，递增
    unsigned char  workState;     // 工作状态 按位 Bit0~3 对应阵面1~4
    unsigned char  faultState;    // 故障状态 按位 0故障/1正常
    unsigned char  workMode;      // 工作模式 0搜索/1跟踪
    unsigned short scanAreaCount; // 扫描区域总数
} LaserStatusData;

// 单目标信息(64字节)
typedef struct _LaserTargetInfo {
    unsigned int   batchID;        // 目标批号
    LaserDataTime  targetTime;     // 该航迹点时间
    float          distance;       // 距离 m
    float          azimuth;        // 方位角 [0,360)
    float          elevation;      // 俯仰角 [-90,90]
    float          speed;          // 速度 m/s
    float          azimuthSpeed;   // 方位角速度 °/s（填0）
    float          elevationSpeed; // 俯仰角速度 °/s（填0）
    float          radialSpeed;    // 径向速度 m/s（=speed）
    unsigned char  cancelFlag;     // 0正常 / 1消批
    unsigned short targetType;     // 0普通/1无人机/2假目标/3鸟/4其他
    unsigned short trackQuality;   // 0~7
    unsigned char  reserved[19];   // 保留
} LaserTargetInfo;

#pragma pack(pop)

#endif // PROTOCOL_H
