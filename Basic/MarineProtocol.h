/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 15:27:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-21 17:53:15
 * @Description: 
 */
/**
 * @file MarineProtocol.h
 * @brief 船用导航雷达通信协议结构体定义
 * @details 定义显控与伺服之间的通信协议：
 *   - 接口1: 显控→伺服 控制帧 (16字节)
 *   - 接口4: 伺服→显控 回波数据帧 (变长)
 *
 * 字节序：控制帧按结构体布局；回波方位为小端0.01度量化，回波长度/包序号为大端。
 */
#ifndef MARINEPROTOCOL_H
#define MARINEPROTOCOL_H

#include <cstdint>
#include <cstring>
#include <QVector>
#include <QMetaType>
#include <QtGlobal>
#include <QString>

#pragma pack(1)

// ============================================================================
// 常量定义
// ============================================================================

/// 控制帧/回波帧头尾标志
constexpr uint8_t MARINE_HEAD_FLAG = 0xA5;
constexpr uint8_t MARINE_TAIL_FLAG = 0x5A;

enum MarineControlCmd : uint8_t {
    MarineCmdParamsOnly = 0x00, ///< Only update radar parameters
    MarineCmdPosition   = 0x01, ///< Radar parameters + turntable position mode
    MarineCmdSpeed      = 0x02, ///< Radar parameters + turntable speed mode
    MarineCmdStop       = 0x03, ///< Radar parameters + stop turntable
    MarineCmdStart      = 0x04  ///< Radar parameters + start turntable
};

constexpr double MARINE_SERVO_RPM_PER_GEAR = 6.0;
constexpr uint8_t MARINE_SERVO_MAX_GEAR = 8;

inline uint16_t marineEncodeAzimuth(double degrees) {
    double normalized = degrees;
    while (normalized < 0.0) normalized += 360.0;
    while (normalized >= 360.0) normalized -= 360.0;
    return static_cast<uint16_t>(normalized * 100.0 + 0.5);
}

inline double marineDecodeAzimuth(uint16_t raw) {
    return raw * 0.01;
}

inline double marineServoGearRpm(uint8_t gear) {
    return qMin<uint8_t>(gear, MARINE_SERVO_MAX_GEAR) * MARINE_SERVO_RPM_PER_GEAR;
}

inline QString marineControlCmdLabel(uint8_t cmdNum) {
    switch (cmdNum) {
    case MarineCmdParamsOnly: return QStringLiteral("0x00 Params");
    case MarineCmdPosition:   return QStringLiteral("0x01 Position");
    case MarineCmdSpeed:      return QStringLiteral("0x02 Speed");
    case MarineCmdStop:       return QStringLiteral("0x03 Stop");
    case MarineCmdStart:      return QStringLiteral("0x04 Start");
    default:                  return QStringLiteral("Unknown");
    }
}

/// 回波帧前导字节
constexpr uint8_t MARINE_ECHO_LEAD = 0x00;

/// 方位角分辨率: 4096 steps = 360°
/// Internal renderer azimuth bins. Protocol azimuth is 0.01-degree quantized.
constexpr uint16_t MARINE_AZI_STEPS = 4096;

/// 量程编码表 (RangeVal → 最大量程, 单位: 米)
/// 对应关系: index 0~14 → meters
constexpr int MARINE_RANGE_TABLE_SIZE = 15;
constexpr double MARINE_RANGE_TABLE[] = {
    300.0,    // 0:  300  m
    500.0,    // 1:  500  m
    750.0,    // 2:  750  m
    1000.0,   // 3:  1    km
    1500.0,   // 4:  1.5  km
    2000.0,   // 5:  2    km
    3000.0,   // 6:  3    km
    4000.0,   // 7:  4    km  (默认)
    6000.0,   // 8:  6    km
    10000.0,  // 9:  10   km
    15000.0,  // 10: 15   km
    30000.0,  // 11: 30   km
    40000.0,  // 12: 40   km
    60000.0,  // 13: 60   km
    75000.0   // 14: 75   km
};

/// 量程对应的公里文本标签
inline QString marineRangeLabel(uint8_t rangeVal) {
    if (rangeVal < MARINE_RANGE_TABLE_SIZE) {
        double km = MARINE_RANGE_TABLE[rangeVal] / 1000.0;
        if (km >= 1.0)
            return QString("%1 km").arg(km, 0, 'f', 1);
        else
            return QString("%1 m").arg(static_cast<int>(MARINE_RANGE_TABLE[rangeVal]));
    }
    return QString("? km");
}

/// 获取量程对应的距离(米)
inline double marineRangeMeters(uint8_t rangeVal) {
    if (rangeVal < MARINE_RANGE_TABLE_SIZE)
        return MARINE_RANGE_TABLE[rangeVal];
    return 5000.0; // fallback
}

// ============================================================================
// 接口1: 显控 → 伺服 控制帧 (16字节)
// ============================================================================

/**
 * @struct MarineControlFrame
 * @brief 显控→伺服 控制帧 (16字节)
 * @details
 *  Byte 0:  HeadFlag  = 0xA5           帧头
 *  Byte 1:  CMDNum    预留
 *  Byte 2-3: Azimuth  预留 (方位信息)
 *  Byte 4:  RangeVal  量程编码 (0~14)，默认7
 *  Byte 5:  Gain      波束锐化 (关0/低1/中2/高3)，默认0
 *  Byte 6:  GanRao    同频干扰抑制 (关0/低1/中2/高3)，默认0
 *  Byte 7:  Level     数据传输截位选择 (自动0/手动1~255)，默认0
 *  Byte 8:  SeaVal    海浪抑制 (自动0/手动1~255)，默认0
 *  Byte 9:  RainVal   雨雪抑制 (自动0/手动1~255)，默认0
 *  Byte 10: CFAR      预留
 *  Byte 11: TXCtrl    发射控制 (关0/开1)，默认0
 *  Byte 12: Servo     天线转速控制 (0/8)，默认0
 *  Byte 13: MTD       预留
 *  Byte 14: CheckSum  前14字节的异或
 *  Byte 15: TailFlag  = 0x5A           帧尾
 */
struct MarineControlFrame {
    uint8_t  headFlag;    ///< Byte 0: 0xA5
    uint8_t  cmdNum;      ///< Byte 1: 预留
    uint16_t azimuth;     ///< Byte 2-3: 预留 (方位信息)
    uint8_t  rangeVal;    ///< Byte 4: 量程编码 0~14
    uint8_t  gain;        ///< Byte 5: 增益/波束锐化 0~3
    uint8_t  ganRao;      ///< Byte 6: 干扰抑制 0~3
    uint8_t  level;       ///< Byte 7: 灵敏度/数据传输截位 0=自动, 1~255手动
    uint8_t  seaVal;      ///< Byte 8: 海杂波 0=自动, 1~255手动
    uint8_t  rainVal;     ///< Byte 9: 雨杂波 0=自动, 1~255手动
    uint8_t  cfar;        ///< Byte 10: 预留 (CFAR)
    uint8_t  txCtrl;      ///< Byte 11: 发射开关 0/1
    uint8_t  servo;       ///< Byte 12: 转速控制 0/8
    uint8_t  mtd;         ///< Byte 13: 预留 (MTD)
    uint8_t  checkSum;    ///< Byte 14: XOR(byte[0]~byte[13])
    uint8_t  tailFlag;    ///< Byte 15: 0x5A

    MarineControlFrame() {
        memset(this, 0, sizeof(*this));
        headFlag = MARINE_HEAD_FLAG;
        tailFlag = MARINE_TAIL_FLAG;
        rangeVal = 7;  // 默认量程编号7
    }

    /// 计算并填入校验码
    double azimuthDegrees() const {
        return marineDecodeAzimuth(azimuth);
    }

    double servoRpm() const {
        return marineServoGearRpm(servo);
    }

    void updateChecksum() {
        uint8_t xorVal = 0;
        const auto* p = reinterpret_cast<const uint8_t*>(this);
        for (int i = 0; i < 14; ++i)
            xorVal ^= p[i];
        checkSum = xorVal;
    }

    /// 校验是否有效
    bool isValid() const {
        if (headFlag != MARINE_HEAD_FLAG || tailFlag != MARINE_TAIL_FLAG)
            return false;
        uint8_t xorVal = 0;
        const auto* p = reinterpret_cast<const uint8_t*>(this);
        for (int i = 0; i < 14; ++i)
            xorVal ^= p[i];
        return xorVal == checkSum;
    }
};

static_assert(sizeof(MarineControlFrame) == 16, "MarineControlFrame must be 16 bytes");

// ============================================================================
// 接口4: 伺服 → 显控 回波数据帧 (变长)
// ============================================================================

/**
 * @struct MarineEchoHeader
 * @brief 回波帧头部 (固定 24 字节)
 * @details
 *  Byte 0:    0x00 (前导)
 *  Byte 1:    0xA5 (帧头)
 *  Byte 2:    方位角低字节
 *  Byte 3:    方位角高字节 (小端，0.01度量化；19834表示198.34°)
 *  Byte 4:    style (0x01=普通 0x02=高分辨率)
 *  Byte 5:    header checksum (byte[0]~byte[4] XOR)
 *  Byte 6:    0x5A (头部尾标志)
 *  // Status block (byte 7~15)
 *  Byte 7:    rangeCode  当前量程
 *  Byte 8:    txState    发射状态
 *  Byte 9:    gain       当前增益
 *  Byte 10:   level      当前灵敏度
 *  Byte 11:   seaVal     海杂波
 *  Byte 12:   rainVal    雨杂波
 *  Byte 13:   ganRao     干扰抑制
 *  Byte 14:   freqStatus 频率状态
 *  Byte 15:   statusChecksum (byte[7]~byte[14] XOR)
 *  // Data info (byte 16~21)
 *  Byte 16-17: fftDataLen  FFT模值数据长度，单位为4字节word (大端)
 *  Byte 18-19: packetNum   包序号 (大端)
 *  Byte 20-21: reserved    保留
 *  // Byte 22~(22+fftDataLen*4-1): 回波幅值数据, 暂按2B/range cell解析
 */
struct MarineEchoHeader {
    uint8_t  leadByte;      ///< 0x00
    uint8_t  headFlag;      ///< 0xA5
    uint8_t  aziLow;        ///< 方位角低8位
    uint8_t  aziHigh;       ///< 方位角高8位
    uint8_t  style;         ///< 0x01/0x02
    uint8_t  hdrChecksum;   ///< XOR(byte[0]~byte[4])
    uint8_t  hdrTail;       ///< 0x5A
    // Status
    uint8_t  rangeCode;     ///< 量程编码
    uint8_t  txState;       ///< 发射状态 0/1
    uint8_t  gain;          ///< 增益
    uint8_t  level;         ///< 灵敏度
    uint8_t  seaVal;        ///< 海杂波
    uint8_t  rainVal;       ///< 雨杂波
    uint8_t  ganRao;        ///< 干扰抑制
    uint8_t  freqStatus;    ///< 频率状态
    uint8_t  statusChecksum;///< XOR(byte[7]~byte[14])
    // Data info
    uint8_t  fftDataLenHigh;///< FFT data length high byte, unit: 4-byte word
    uint8_t  fftDataLenLow; ///< FFT data length low byte, unit: 4-byte word
    uint8_t  packetNumHigh; ///< Packet number high byte
    uint8_t  packetNumLow;  ///< Packet number low byte
    uint8_t  reservedHigh;  ///< Reserved high byte
    uint8_t  reservedLow;   ///< Reserved low byte

    // Azimuth is little-endian and quantized by 0.01 degree.
    uint16_t azimuthRaw() const {
        return static_cast<uint16_t>(aziLow) | (static_cast<uint16_t>(aziHigh) << 8);
    }

    double azimuthDeg() const {
        return azimuthRaw() * 0.01;
    }

    uint16_t azimuthRenderIndex() const {
        const uint32_t centiDeg = static_cast<uint32_t>(azimuthRaw()) % 36000U;
        return static_cast<uint16_t>(((centiDeg * MARINE_AZI_STEPS) + 18000U) / 36000U % MARINE_AZI_STEPS);
    }

    // FFT data length is big-endian/network order, unit: 4-byte word.
    uint16_t fftWordCount() const {
        return (static_cast<uint16_t>(fftDataLenHigh) << 8) | static_cast<uint16_t>(fftDataLenLow);
    }

    int echoByteCount() const {
        return static_cast<int>(fftWordCount()) * 4;
    }

    int rangeCellCount() const {
        return echoByteCount() / 2;
    }

    uint16_t packetNumber() const {
        return (static_cast<uint16_t>(packetNumHigh) << 8) | static_cast<uint16_t>(packetNumLow);
    }

    uint16_t reservedValue() const {
        return (static_cast<uint16_t>(reservedHigh) << 8) | static_cast<uint16_t>(reservedLow);
    }

    /// 校验帧头
    bool isHeaderValid() const {
        if (leadByte != MARINE_ECHO_LEAD || headFlag != MARINE_HEAD_FLAG || hdrTail != MARINE_TAIL_FLAG)
            return false;
        uint8_t xv = 0;
        const auto* p = reinterpret_cast<const uint8_t*>(this);
        for (int i = 0; i < 5; ++i) xv ^= p[i];
        return xv == hdrChecksum;
    }

    /// 校验状态块
    bool isStatusValid() const {
        uint8_t xv = 0;
        const auto* p = reinterpret_cast<const uint8_t*>(this);
        for (int i = 7; i < 15; ++i) xv ^= p[i];
        return xv == statusChecksum;
    }
};

static_assert(sizeof(MarineEchoHeader) == 22, "MarineEchoHeader must be 22 bytes");

// ============================================================================
// 运行时数据结构 (非协议, 不需要 pack)
// ============================================================================

#pragma pack()

/**
 * @struct MarineRadarStatus
 * @brief 从回波帧状态字段提取的雷达运行状态
 */
struct MarineRadarStatus {
    uint8_t  rangeCode = 7;    ///< 当前量程编码 (0~14)
    bool     txOn = false;     ///< 发射状态
    uint8_t  gain = 0;         ///< 增益
    uint8_t  level = 0;        ///< 灵敏度
    uint8_t  seaVal = 0;       ///< 海杂波
    uint8_t  rainVal = 0;      ///< 雨杂波
    uint8_t  ganRao = 0;       ///< 干扰抑制
    uint8_t  freqStatus = 0;   ///< 频率状态

    double rangeMeter() const { return marineRangeMeters(rangeCode); }
    QString rangeLabel() const { return marineRangeLabel(rangeCode); }
};

/**
 * @struct MarineEchoLine
 * @brief 一条扫描线的回波数据（解析后）
 */
struct MarineEchoLine {
    uint16_t azimuthRaw = 0;   ///< Internal render azimuth index (0~4095)
    double   azimuthDeg = 0.0; ///< 方位角(度)
    uint8_t  style = 0x01;     ///< 模式
    uint16_t packetNum = 0;    ///< 包序号
    QVector<uint8_t> amplitudes; ///< 各距离单元幅值 (0~255)

    /// 距离单元数
    int cellCount() const { return amplitudes.size(); }
};

Q_DECLARE_METATYPE(MarineControlFrame)
Q_DECLARE_METATYPE(MarineRadarStatus)
Q_DECLARE_METATYPE(MarineEchoLine)

#endif // MARINEPROTOCOL_H
