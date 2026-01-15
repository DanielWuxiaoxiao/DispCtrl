/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-15 14:23:11
 * @Description: 
 */
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
    unsigned char tran;
    unsigned short tranStart;  //0.01°量化
    unsigned short tranEnd;    //0.01°量化

    _TranRecvControl()  //default value
    {
        mesID = 0xAA02;
        tran = 0;
        recv = 1;
        tranStart = 0;
        tranEnd = 36000; // 360.00°
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
    unsigned char freqID;  //0-80: 9.0GHz~9.8GHz (界面0-8映射到下发0-80)
    unsigned char type;  // 1 线性负调频，2 线性正调频
    short aziStart;      // 0.01°量化
    short aziEnd;
    short aziStep;
    unsigned char flagNum;  // 波形起效数量
    unsigned char beam1Flag; //0不起效 1起效
    unsigned char beam1Code; //波形码 0~11

    unsigned short pulseNum1;      // 积累脉冲数
    unsigned short sampleStart1;   // 采样起始 0.1us量化
    unsigned short sampleEnd1;     // 采样终止 0.1us量化
    short elestart1; // 俯仰起始 0.01°量化
    short eleend1;   // 俯仰终止
    short elestep1;  // 俯仰间隔

    unsigned char beam2Flag; //0不起效 1起效
    unsigned char beam2Code; //波形码 0~11
    unsigned short pulseNum2;      // 积累脉冲数
    unsigned short sampleStart2;   // 采样起始 0.1us量化
    unsigned short sampleEnd2;     // 采样终止 0.1us量化
    short elestart2; // 俯仰起始 0.01°量化
    short eleend2;   // 俯仰终止
    short elestep2;  // 俯仰间隔

    unsigned char beam3Flag; //0不起效 1起效
    unsigned char beam3Code; //波形码 0~11
    unsigned short pulseNum3;      // 积累脉冲数
    unsigned short sampleStart3;   // 采样起始 0.1us量化
    unsigned short sampleEnd3;     // 采样终止 0.1us量化
    short elestart3; // 俯仰起始 0.01°量化
    short eleend3;   // 俯仰终止
    short elestep3;  // 俯仰间隔

    _BeamControl()  //default value
    {
        memset(this,0,sizeof (_BeamControl));
        flagNum = 2;
        mesID = 0xAA05;
        freqID = 20;  // 界面默认选中第2项(9.2GHz)，下发值为2*10=20
        type = 2;
        pulseNum1 = 256;
        pulseNum2 = 256;
        pulseNum3 = 256;

        aziStart = 4500;
        aziEnd = 13500;
        aziStep = 300;

        beam1Flag = 1;
        beam1Code = 6;
        sampleStart1 = 30;
        sampleEnd1 = 130;  // 原来是 start=30, len=100, 所以 end=130
        elestart1 = 0;
        eleend1 = 6000;
        elestep1 = 600;

        beam2Flag = 1;
        beam2Code = 9;
        sampleStart2 = 120;
        sampleEnd2 = 490;  // 原来是 start=120, len=370, 所以 end=490
        elestart2 = 0;
        eleend2 = 2000;
        elestep2 = 600;

        beam3Flag = 0;
        beam3Code = 10;
        sampleStart3 = 270;
        sampleEnd3 = 620;  // 原来是 start=270, len=350, 所以 end=620
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

// TBD 航迹上报帧头
typedef struct _TBDTrackHead
{
    unsigned short mesID;
    unsigned short updateFlag;  // 显控更新标志位

    _TBDTrackHead()
    {
        mesID = 0xEE02;
        updateFlag = 0;
    }
}TBDTrackHead;

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

// TBD 航迹节点（批号+长度）
typedef struct _TBDTrackInfo
{
    unsigned batch;   // 航迹批号
    unsigned length;  // 点迹数量
}TBDTrackInfo;

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
    unsigned char powerState;   // 波控板电源状态：正常=1，不正常=0
    unsigned short fpgaTemp;    // 扩展：FPGA温度 0.1°
    unsigned short panelTemp;   // 扩展：阵面温度 0.1°
    unsigned short yaw;         // 扩展：方位角 0.01°
    unsigned char subArrayPower[5];  // 扩展：分阵供电状态
    unsigned char reserve[25];

    _BITReport()
    {
        mesID = 0xDE02;
        bitGroup = 0;
        powerState = 0;
        fpgaTemp = 0;
        panelTemp = 0;
        yaw = 0;
        memset(subArrayPower, 0, sizeof(subArrayPower));
        memset(reserve, 0, sizeof(reserve));
    }
}BITReport;

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
    unsigned int batch;
    unsigned char statMethod;
}PointInfo;

enum PointType
{
    Detection = 1,
    Track = 2,
    TBDPointType = 3
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

#pragma pack()

#endif // PROTOCOL_H
