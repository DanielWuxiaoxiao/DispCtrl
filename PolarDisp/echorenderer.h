/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 15:27:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-19 10:10:46
 * @Description: 
 */
/**
 * @file echorenderer.h
 * @brief 船用雷达PPI回波渲染器
 * @details 将接收到的扫描线回波数据(amplitude数组)渲染为PPI极坐标图像：
 *   - SweepBuffer: 4096方位 × N距离单元 环形缓冲
 *   - ColorLookupTable: 256级 amplitude → QRgb 颜色映射
 *   - 渲染为 QImage，以 QGraphicsPixmapItem 嵌入 PPIScene
 *   - 支持余辉衰减效果
 */
#ifndef ECHORENDERER_H
#define ECHORENDERER_H

#include <QObject>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QTimer>
#include <QVector>
#include <QRgb>
#include <array>
#include "Basic/MarineProtocol.h"

class PolarAxis;

/**
 * @class EchoRenderer
 * @brief PPI回波渲染引擎
 *
 * 工作流程:
 *   MarineRadarManager::echoLineReceived
 *     → updateEchoLine(azimuthRaw, amplitudes)
 *     → 写入 SweepBuffer
 *     → 定时渲染到 QImage
 *     → 更新 QGraphicsPixmapItem
 */
class EchoRenderer : public QObject
{
    Q_OBJECT

public:
    /// 色彩方案
    enum ColorMap {
        SIMRAD,     ///< SIMRAD Halo风格 (黑→蓝→绿→黄→红→白)
        Rainbow,    ///< 彩虹
        Green       ///< 绿色经典
    };

    /**
     * @brief 构造
     * @param scene PPIScene指针, 渲染结果添加到此场景
     * @param imageSize PPI渲染图像边长(像素), 默认2048
     */
    explicit EchoRenderer(QGraphicsScene* scene, PolarAxis* axis = nullptr,
                          int imageSize = 2048, QObject* parent = nullptr);
    ~EchoRenderer() override;

    /// 获取渲染的 PixmapItem (用于 PPIScene 层级管理)
    QGraphicsPixmapItem* pixmapItem() const { return m_pixmapItem; }

    /// 设置色彩方案
    void setColorMap(ColorMap cm);
    ColorMap colorMap() const { return m_colorMap; }

    /// 设置余辉衰减时间(ms)
    void setDecayTime(int ms);
    int decayTime() const { return m_decayMs; }

    /// 设置当前量程(米) — 控制距离单元到像素的映射
    void setRange(double rangeMeters);
    double range() const { return m_rangeMeter; }

    /// 设置渲染刷新率(ms)
    void setRenderInterval(int ms);

    /// 设置同一方位保留的扫描圈数，默认1圈。
    void setSweepHistoryRounds(int rounds);
    int sweepHistoryRounds() const { return m_sweepHistoryRounds; }

    /// 获取PPI图像尺寸
    int imageSize() const { return m_imageSize; }

    /// 清除所有回波数据
    void clear();

public slots:
    /**
     * @brief 更新一条扫描线的回波数据
     * @param line 回波线数据(来自协议或模拟器)
     */
    void updateEchoLine(const MarineEchoLine& line);

private slots:
    void renderFrame();

private:
    void buildColorTable();
    void buildColorTableSimrad();
    void buildColorTableRainbow();
    void buildColorTableGreen();
    int drawEchoLineToImage(int aziIdx, const uint8_t* amplitudes, int count,
                            double sourceRangeMeters, bool clearLine);
    int advanceSweepWindow(int aziIdx);
    int clearExpiredBucket(int aziIdx, uint32_t minGeneration);
    void updatePixmapItem();

    // ---- 扫描缓冲 ----
    static constexpr int AZI_STEPS = 4096;       ///< 方位分辨率
    static constexpr int MAX_CELLS = 2048;       ///< 最大距离单元数
    static constexpr int MAX_SWEEP_HISTORY = 8;  ///< 同一方位最多保留圈数

    /// 每条扫描线的缓冲
    struct SweepLine {
        std::array<uint8_t, MAX_CELLS> amp{};    ///< 幅值 0~255
        int cellCount = 0;                        ///< 有效单元数
        double sourceRangeMeters = 0.0;           ///< Physical range represented by amp[]
        uint32_t generation = 0;                  ///< Sweep generation that produced this line
    };

    struct SweepBucket {
        std::array<SweepLine, MAX_SWEEP_HISTORY> lines{};
        int count = 0;
    };

    std::array<SweepBucket, AZI_STEPS> m_sweepBuf; ///< 4096方位环形缓冲

    // ---- 渲染 ----
    int m_imageSize;                              ///< 图像边长
    QImage m_ppiImage;                            ///< PPI渲染画布
    QGraphicsPixmapItem* m_pixmapItem = nullptr;  ///< 场景中的显示项
    QGraphicsScene* m_scene = nullptr;
    PolarAxis* m_axis = nullptr;
    bool m_imageDirty = false;
    uint32_t m_rxLineCount = 0;
    bool m_logNextLineInfo = true;
    int m_lastDebugAziIdx = -1;
    uint32_t m_debugAziChangeCount = 0;
    int m_lastSweepAziIdx = -1;
    uint32_t m_currentSweepGeneration = 0;

    QTimer* m_renderTimer = nullptr;
    // ---- 颜色映射 ----
    ColorMap m_colorMap = SIMRAD;
    std::array<QRgb, 256> m_colorLUT{};           ///< amplitude → QRgb

    // ---- 参数 ----
    double m_rangeMeter = 1852.0;                 ///< 当前量程(米), 默认1nm
    int m_decayMs = 10000;                        ///< 余辉衰减时间(ms)
    int m_sweepHistoryRounds = 1;                 ///< 同一方位保留的扫描圈数

    // 预计算: 极坐标→像素映射辅助
    QVector<double> m_sinTable;                   ///< sin(azi) for 0~4095
    QVector<double> m_cosTable;                   ///< cos(azi) for 0~4095
    void buildTrigTables();
};

#endif // ECHORENDERER_H
