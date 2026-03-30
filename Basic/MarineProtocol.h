/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 11:44:45
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:09
 * @Description: 
 */
/**
 * @file MarineProtocol.h
 * @brief 船用导航雷达通信协议结构体定义
 * @details 定义显控与伺服之间的通信协议：
 *   - 接口1: 显控→伺服 控制帧 (16字节)
 *   - 接口4: 伺服→显控 回波数据帧 (变长)
 *
 * 字节序：小端，1字节对齐
 */
#ifndef MARINEPROTOCOL_H
#define MARINEPROTOCOL_H

#include <cstdint>
#include <cstring>
#include <QVector>
#include <QMetaType>

#pragma pack(1)

// ============================================================================
// 常量定义
// ============================================================================

/// 控制帧/回波帧头尾标志
constexpr uint8_t MARINE_HEAD_FLAG = 0xA5;
constexpr uint8_t MARINE_TAIL_FLAG = 0x5A;

/// 回波帧前导字节
constexpr uint8_t MARINE_ECHO_LEAD = 0x00;

/// 方位角分辨率: 4096 steps = 360°
constexpr uint16_t MARINE_AZI_STEPS = 4096;

/// 量程编码表 (RangeVal → 实际距离, 单位: 米)
/// 对应关系: index → meters
constexpr int MARINE_RANGE_TABLE_SIZE = 24;
constexpr double MARINE_RANGE_TABLE[] = {
    // 近量程
    69.4,    // 0:  1/16 nm ≈ 69.4m  (约 125 英尺)
    138.9,   // 1:  1/8  nm
    231.5,   // 2:  1/4  nm (约 750 英尺)  — 短程港内
    347.2,   // 3:  3/8  nm (约 0.19 km)
    462.9,   // 4:  1/2  nm
    694.4,   // 5:  3/4  nm
    926.0,   // 6:  1    nm
    1388.9,  // 7:  3/4  nm × 2 = 1.5 nm
    1852.0,  // 8:  2    nm
    2778.0,  // 9:  3    nm — 典型港湾
    3704.0,  // 10: 4    nm
    5556.0,  // 11: 6    nm
    7408.0,  // 12: 8    nm
    11112.0, // 13: 12   nm — 中程航行
    14816.0, // 14: 16   nm
    18520.0, // 15: 20   nm
    22224.0, // 16: 24   nm — 远程巡航
    27780.0, // 17: 30   nm
    37040.0, // 18: 36   nm
    46300.0, // 19: 48   nm
    55560.0, // 20: 60   nm
    74080.0, // 21: 72   nm — 超远程
    92600.0, // 22: 96   nm
    129640.0 // 23: 120  nm (约 70 海里)
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
 *  Byte 0:  HeadFlag  = 0xA5
 *  Byte 1:  RangeVal  量程编码 (0~23)
 *  Byte 2:  Gain      增益 (0=自动, 1=低, 2=中, 3=高)
 *  Byte 3:  GanRao    干扰抑制 (0=关, 1=低, 2=中, 3=高)
 *  Byte 4:  Level     灵敏度 (0=自动, 1~255=手动)
 *  Byte 5:  SeaVal    海杂波抑制 (0=自动, 1~255=手动)
 *  Byte 6:  RainVal   雨杂波抑制 (0=自动, 1~255=手动)
 *  Byte 7:  TXCtrl    发射控制 (0=关闭, 1=开启)
 *  Byte 8:  SpeedL    伺服转速低字节 (0=停, 8=正常)
 *  Byte 9:  SpeedH    伺服转速高字节
 *  Byte 10-13: Reserved
 *  Byte 14: CheckSum  校验 (Byte[0]~Byte[13] XOR)
 *  Byte 15: TailFlag  = 0x5A
 */
struct MarineControlFrame {
    uint8_t  headFlag;    ///< 0xA5
    uint8_t  rangeVal;    ///< 量程编码 0~23
    uint8_t  gain;        ///< 增益 0~3
    uint8_t  ganRao;      ///< 干扰抑制 0~3
    uint8_t  level;       ///< 灵敏度 0=自动, 1~255手动
    uint8_t  seaVal;      ///< 海杂波 0=自动, 1~255手动
    uint8_t  rainVal;     ///< 雨杂波 0=自动, 1~255手动
    uint8_t  txCtrl;      ///< 发射开关 0/1
    uint8_t  speedL;      ///< 转速低字节
    uint8_t  speedH;      ///< 转速高字节
    uint8_t  reserved[4]; ///< 保留
    uint8_t  checkSum;    ///< XOR(byte[0]~byte[13])
    uint8_t  tailFlag;    ///< 0x5A

    MarineControlFrame() {
        memset(this, 0, sizeof(*this));
        headFlag = MARINE_HEAD_FLAG;
        tailFlag = MARINE_TAIL_FLAG;
        rangeVal = 8;  // 默认 2nm
        speedL = 8;    // 默认正常转速
    }

    /// 计算并填入校验码
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
 *  Byte 3:    方位角高字节 (0~4095 → 0°~360°)
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
 *  Byte 16-17: fftDataLen  FFT数据长度 (小端)
 *  Byte 18-19: packetNum   包序号 (小端)
 *  Byte 20-21: reserved    保留
 *  // Byte 22~(22+fftDataLen-1): 回波幅值数据, 1B/range cell, 值 0~255
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
    uint16_t fftDataLen;    ///< 回波数据字节数 (小端)
    uint16_t packetNum;     ///< 包序号
    uint16_t reserved;      ///< 保留

    /// 获取方位角 (0~4095)
    uint16_t azimuthRaw() const {
        return static_cast<uint16_t>(aziLow) | (static_cast<uint16_t>(aziHigh) << 8);
    }

    /// 获取方位角 (度, 0~360)
    double azimuthDeg() const {
        return azimuthRaw() * 360.0 / MARINE_AZI_STEPS;
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
    uint8_t  rangeCode = 8;    ///< 当前量程编码
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
    uint16_t azimuthRaw = 0;   ///< 原始方位码 (0~4095)
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
