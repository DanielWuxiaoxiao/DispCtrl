/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 15:27:09
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-19 10:10:45
 * @Description: 
 */
/**
 * @file echorenderer.cpp
 * @brief 船用雷达PPI回波渲染器实现
 */
#include "echorenderer.h"
#include <QtMath>
#include <QPainter>
#include <QDebug>
#include <QTransform>
#include <cstring>
#include "Basic/ConfigManager.h"
#include "Basic/log.h"
#include "polaraxis.h"

namespace {

bool shouldLogSample(uint32_t count, uint32_t firstCount, uint32_t interval)
{
    return count <= firstCount || (interval > 0 && count % interval == 0);
}

} // namespace

EchoRenderer::EchoRenderer(QGraphicsScene* scene, PolarAxis* axis, int imageSize, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_axis(axis)
    , m_imageSize(imageSize)
    , m_ppiImage(imageSize, imageSize, QImage::Format_ARGB32_Premultiplied)
{
    m_ppiImage.fill(Qt::transparent);

    // 创建 PixmapItem 并添加到场景
    m_pixmapItem = new QGraphicsPixmapItem();
    m_pixmapItem->setTransformationMode(Qt::FastTransformation);
    m_pixmapItem->setZValue(-1);  // 在网格之下，避免回波覆盖量程圆和刻度
    if (m_scene) {
        m_scene->addItem(m_pixmapItem);
    }

    // 初始化颜色表和三角函数表
    buildColorTable();
    buildTrigTables();

    // 渲染定时器 (默认30fps)
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &EchoRenderer::renderFrame);
    m_renderTimer->start(33);
}

EchoRenderer::~EchoRenderer()
{
    m_renderTimer->stop();
    // m_pixmapItem 由 scene 管理, 不需手动删除
}

// ============================================================================
// 颜色映射表
// ============================================================================

void EchoRenderer::buildColorTable()
{
    switch (m_colorMap) {
    case SIMRAD:  buildColorTableSimrad();  break;
    case Rainbow: buildColorTableRainbow(); break;
    case Green:   buildColorTableGreen();   break;
    }
}

void EchoRenderer::buildColorTableSimrad()
{
    // SIMRAD Halo 高保真色阶:
    // 参照实机截图，色彩从深蓝→青→绿→黄→橙→红，最高幅值保持红色
    // 0~15:   透明(噪底)
    // 16~50:  深蓝→蓝(弱回波)
    // 51~85:  蓝→青(海面杂波)
    // 86~120: 青→绿(中等回波)
    // 121~155: 绿→黄(较强回波)
    // 156~190: 黄→橙(强回波)
    // 191~225: 橙→红(很强回波)
    // 226~255: 亮红→深红(极强回波，如岸线/最强目标)

    for (int i = 0; i < 256; ++i) {
        if (i < 16) {
            m_colorLUT[i] = qRgba(0, 0, 0, 0);
        } else if (i < 51) {
            // 深蓝→蓝
            double t = (i - 16) / 34.0;
            int b = static_cast<int>(100 + t * 155);
            int g = static_cast<int>(t * 20);
            m_colorLUT[i] = qRgba(0, g, b, 220);
        } else if (i < 86) {
            // 蓝→青
            double t = (i - 51) / 34.0;
            int g = static_cast<int>(20 + t * 220);
            int b = static_cast<int>(255 - t * 60);
            m_colorLUT[i] = qRgba(0, g, b, 235);
        } else if (i < 121) {
            // 青→绿
            double t = (i - 86) / 34.0;
            int g = static_cast<int>(240 + t * 15);
            int b = static_cast<int>(195 * (1.0 - t));
            m_colorLUT[i] = qRgba(0, g, b, 245);
        } else if (i < 156) {
            // 绿→黄
            double t = (i - 121) / 34.0;
            int r = static_cast<int>(t * 255);
            int g = 255;
            m_colorLUT[i] = qRgba(r, g, 0, 250);
        } else if (i < 191) {
            // 黄→橙
            double t = (i - 156) / 34.0;
            int g = static_cast<int>(255 - t * 120);
            m_colorLUT[i] = qRgba(255, g, 0, 255);
        } else if (i < 226) {
            // 橙→红
            double t = (i - 191) / 34.0;
            int g = static_cast<int>(135 * (1.0 - t));
            m_colorLUT[i] = qRgba(255, g, 0, 255);
        } else if (i < 246) {
            // 亮红
            double t = (i - 226) / 19.0;
            int g = static_cast<int>(40 * (1.0 - t));
            m_colorLUT[i] = qRgba(255, g, 0, 255);
        } else {
            // 亮红→深红，最高幅值仍保持红色
            double t = (i - 246) / 9.0;
            int r = static_cast<int>(255 - t * 55);
            m_colorLUT[i] = qRgba(r, 0, 0, 255);
        }
    }
}

void EchoRenderer::buildColorTableRainbow()
{
    for (int i = 0; i < 256; ++i) {
        if (i < 16) {
            m_colorLUT[i] = qRgba(0, 0, 0, 0);
        } else {
            // HSV hue 从 240(蓝) → 0(红)
            double t = (i - 16) / 239.0;
            int hue = static_cast<int>(240 * (1.0 - t));
            QColor c = QColor::fromHsv(hue, 255, 255);
            m_colorLUT[i] = qRgba(c.red(), c.green(), c.blue(), static_cast<int>(180 + t * 75));
        }
    }
}

void EchoRenderer::buildColorTableGreen()
{
    // 经典绿色雷达
    for (int i = 0; i < 256; ++i) {
        if (i < 16) {
            m_colorLUT[i] = qRgba(0, 0, 0, 0);
        } else {
            double t = (i - 16) / 239.0;
            int g = static_cast<int>(60 + t * 195);
            int alpha = static_cast<int>(100 + t * 155);
            m_colorLUT[i] = qRgba(0, g, 0, alpha);
        }
    }
}

// ============================================================================
// 三角函数预计算
// ============================================================================

void EchoRenderer::buildTrigTables()
{
    m_sinTable.resize(AZI_STEPS);
    m_cosTable.resize(AZI_STEPS);

    for (int i = 0; i < AZI_STEPS; ++i) {
        double angle = i * 2.0 * M_PI / AZI_STEPS;
        // PPI坐标: 0°=北(上), 顺时针
        // 屏幕坐标: X右, Y下
        // x = sin(azi), y = -cos(azi)
        m_sinTable[i] = qSin(angle);
        m_cosTable[i] = -qCos(angle);
    }
}

// ============================================================================
// 公共接口
// ============================================================================

void EchoRenderer::setColorMap(ColorMap cm)
{
    m_colorMap = cm;
    buildColorTable();
}

void EchoRenderer::setDecayTime(int ms)
{
    m_decayMs = qMax(1000, ms);
}

void EchoRenderer::setRange(double rangeMeters)
{
    if (rangeMeters <= 0)
        return;

    if (!qFuzzyCompare(m_rangeMeter, rangeMeters)) {
        m_rangeMeter = rangeMeters;
        m_logNextLineInfo = true;
        LOG_DEBUG(QString("[EchoRenderer] display range set to %1m").arg(m_rangeMeter, 0, 'f', 1));
        clear();
    }
}

void EchoRenderer::setRenderInterval(int ms)
{
    m_renderTimer->setInterval(qMax(16, ms));  // 最高~60fps
}

void EchoRenderer::setSweepHistoryRounds(int rounds)
{
    const int bounded = qBound(1, rounds, MAX_SWEEP_HISTORY);
    if (m_sweepHistoryRounds == bounded) {
        if (CF_INS.marineDisplayBool("echo_debug", false)) {
            LOG_DEBUG(QString("[EchoRenderer][CONFIG-DEBUG] sweep history rounds already %1").arg(m_sweepHistoryRounds));
        }
        return;
    }

    const int oldRounds = m_sweepHistoryRounds;
    m_sweepHistoryRounds = bounded;
    LOG_INFO(QString("[EchoRenderer] sweep history rounds %1 -> %2").arg(oldRounds).arg(m_sweepHistoryRounds));
    clear();
}

void EchoRenderer::clear()
{
    for (auto& bucket : m_sweepBuf) {
        for (auto& line : bucket.lines) {
            line.amp.fill(0);
            line.cellCount = 0;
            line.sourceRangeMeters = 0.0;
            line.generation = 0;
        }
        bucket.count = 0;
    }
    m_lastSweepAziIdx = -1;
    m_currentSweepGeneration = 0;
    m_ppiImage.fill(Qt::transparent);
    m_imageDirty = true;
    updatePixmapItem();
}

int EchoRenderer::advanceSweepWindow(int aziIdx)
{
    if (aziIdx < 0 || aziIdx >= AZI_STEPS)
        return 0;

    if (m_lastSweepAziIdx < 0) {
        m_lastSweepAziIdx = aziIdx;
        return 0;
    }

    if (aziIdx == m_lastSweepAziIdx)
        return 0;

    const int forward = (aziIdx - m_lastSweepAziIdx + AZI_STEPS) % AZI_STEPS;
    if (forward <= 0 || forward >= AZI_STEPS / 2) {
        m_lastSweepAziIdx = aziIdx;
        return 0;
    }

    if (aziIdx < m_lastSweepAziIdx)
        ++m_currentSweepGeneration;

    const uint32_t minGeneration =
        (m_currentSweepGeneration >= static_cast<uint32_t>(m_sweepHistoryRounds - 1))
            ? (m_currentSweepGeneration - static_cast<uint32_t>(m_sweepHistoryRounds - 1))
            : 0;

    int expiredPoints = 0;
    int idx = (m_lastSweepAziIdx + 1) % AZI_STEPS;
    while (idx != aziIdx) {
        expiredPoints += clearExpiredBucket(idx, minGeneration);
        idx = (idx + 1) % AZI_STEPS;
    }

    m_lastSweepAziIdx = aziIdx;
    return expiredPoints;
}

int EchoRenderer::clearExpiredBucket(int aziIdx, uint32_t minGeneration)
{
    auto& bucket = m_sweepBuf[aziIdx];
    if (bucket.count <= 0)
        return 0;

    int clearedPoints = 0;
    int writeIndex = 0;
    for (int i = 0; i < bucket.count; ++i) {
        const auto& oldLine = bucket.lines[i];
        if (oldLine.cellCount <= 0)
            continue;

        if (oldLine.generation < minGeneration) {
            clearedPoints += drawEchoLineToImage(aziIdx, oldLine.amp.data(), oldLine.cellCount,
                                                 oldLine.sourceRangeMeters, true);
            continue;
        }

        if (writeIndex != i)
            bucket.lines[writeIndex] = bucket.lines[i];
        ++writeIndex;
    }

    for (int i = writeIndex; i < bucket.count; ++i) {
        bucket.lines[i].amp.fill(0);
        bucket.lines[i].cellCount = 0;
        bucket.lines[i].sourceRangeMeters = 0.0;
        bucket.lines[i].generation = 0;
    }
    bucket.count = writeIndex;
    if (clearedPoints > 0)
        m_imageDirty = true;
    return clearedPoints;
}

// ============================================================================
// 数据输入
// ============================================================================

void EchoRenderer::updateEchoLine(const MarineEchoLine& line)
{
    int aziIdx = line.azimuthRaw % AZI_STEPS;
    const bool azimuthChanged = aziIdx != m_lastDebugAziIdx;
    if (azimuthChanged) {
        m_lastDebugAziIdx = aziIdx;
        ++m_debugAziChangeCount;
    }
    const int expiredPoints = advanceSweepWindow(aziIdx);
    auto& bucket = m_sweepBuf[aziIdx];

    int count = qMin(line.cellCount(), MAX_CELLS);
    const int historyBefore = bucket.count;
    int clearedPoints = expiredPoints;
    for (int i = 0; i < bucket.count; ++i) {
        const auto& oldLine = bucket.lines[i];
        if (oldLine.cellCount > 0) {
            clearedPoints += drawEchoLineToImage(aziIdx, oldLine.amp.data(), oldLine.cellCount,
                                                 oldLine.sourceRangeMeters, true);
        }
    }

    SweepLine newLine;
    newLine.cellCount = count;
    newLine.sourceRangeMeters = line.sourceRangeMeters > 0.0 ? line.sourceRangeMeters : m_rangeMeter;
    newLine.generation = m_currentSweepGeneration;
    int drawnPoints = 0;
    bool acceptNewLine = true;
    bool droppedOldest = false;

    if (count > 0) {
        memcpy(newLine.amp.data(), line.amplitudes.constData(), count);
        int activeCount = 0;
        int saturatedCount = 0;
        for (int i = 0; i < count; ++i) {
            const uint8_t amp = newLine.amp[i];
            if (amp >= 16)
                ++activeCount;
            if (amp >= 250)
                ++saturatedCount;
        }

        // A whole radial line at max amplitude is not a physical echo. It is
        // usually a malformed/status/control datagram that passed weak framing
        // and would otherwise leave a white spoke at 0 degrees.
        if (activeCount > 64 && saturatedCount * 100 >= activeCount * 95) {
            newLine.amp.fill(0);
            newLine.cellCount = 0;
            acceptNewLine = false;
        }
    }

    if (acceptNewLine && newLine.cellCount > 0) {
        bool replacedCurrentGeneration = false;
        if (bucket.count > 0 && bucket.lines[bucket.count - 1].generation == m_currentSweepGeneration) {
            bucket.lines[bucket.count - 1] = newLine;
            replacedCurrentGeneration = true;
        }

        if (!replacedCurrentGeneration && bucket.count >= m_sweepHistoryRounds) {
            for (int i = 1; i < bucket.count; ++i) {
                bucket.lines[i - 1] = bucket.lines[i];
            }
            bucket.count = qMax(0, bucket.count - 1);
            droppedOldest = true;
        }
        if (!replacedCurrentGeneration) {
            bucket.lines[bucket.count++] = newLine;
        }
    }

    for (int i = 0; i < bucket.count; ++i) {
        const auto& visibleLine = bucket.lines[i];
        if (visibleLine.cellCount > 0) {
            drawnPoints += drawEchoLineToImage(aziIdx, visibleLine.amp.data(),
                                               visibleLine.cellCount,
                                               visibleLine.sourceRangeMeters, false);
        }
    }
    ++m_rxLineCount;
    const bool echoDebug = CF_INS.marineDisplayBool("echo_debug", false);
    const bool packetSample = shouldLogSample(m_rxLineCount, 20, 128);
    const bool forcedSample = (m_rxLineCount <= 20) || ((m_rxLineCount % 8192) == 0);
    const bool angleSample = azimuthChanged &&
                             (m_debugAziChangeCount <= 256 || (m_debugAziChangeCount % 64) == 0);
    if (echoDebug && (packetSample || forcedSample || angleSample)) {
        LOG_DEBUG(QString("[EchoRenderer][HISTORY-DEBUG] line=%1 aziIdx=%2 aziDeg=%3 angleChanged=%4 angleChanges=%5 sweepGen=%6 incomingCells=%7 historyBefore=%8 limit=%9 expiredPx=%10 clearedPx=%11 accept=%12 droppedOldest=%13 historyAfter=%14 redrawnPx=%15 sourceRange=%16m displayRange=%17m")
                  .arg(m_rxLineCount)
                  .arg(aziIdx)
                  .arg(line.azimuthDeg, 0, 'f', 3)
                  .arg(azimuthChanged ? 1 : 0)
                  .arg(m_debugAziChangeCount)
                  .arg(m_currentSweepGeneration)
                  .arg(count)
                  .arg(historyBefore)
                  .arg(m_sweepHistoryRounds)
                  .arg(expiredPoints)
                  .arg(clearedPoints)
                  .arg(acceptNewLine ? 1 : 0)
                  .arg(droppedOldest ? 1 : 0)
                  .arg(bucket.count)
                  .arg(drawnPoints)
                  .arg(newLine.sourceRangeMeters, 0, 'f', 1)
                  .arg(m_rangeMeter, 0, 'f', 1));
    } else if (m_logNextLineInfo || m_rxLineCount <= 5 || (m_rxLineCount % 512) == 0) {
        const QString msg = QString("[EchoRenderer] line #%1 aziIdx=%2 aziDeg=%3 cells=%4 cleared=%5 drawn=%6 sourceRange=%7m displayRange=%8m history=%9/%10")
                                .arg(m_rxLineCount)
                                .arg(aziIdx)
                                .arg(line.azimuthDeg, 0, 'f', 2)
                                .arg(count)
                                .arg(clearedPoints)
                                .arg(drawnPoints)
                                .arg(newLine.sourceRangeMeters, 0, 'f', 1)
                                .arg(m_rangeMeter, 0, 'f', 1)
                                .arg(bucket.count)
                                .arg(m_sweepHistoryRounds);
        if (m_logNextLineInfo) {
            LOG_DEBUG(msg);
            m_logNextLineInfo = false;
        } else {
            LOG_DEBUG(msg);
        }
    }
    m_imageDirty = true;
}

// ============================================================================
// 渲染
// ============================================================================

void EchoRenderer::renderFrame()
{
    updatePixmapItem();
}

void EchoRenderer::updatePixmapItem()
{
    if (!m_pixmapItem)
        return;

    if (m_imageDirty) {
        m_pixmapItem->setPixmap(QPixmap::fromImage(m_ppiImage));
        m_imageDirty = false;
    }

    const double halfSize = m_imageSize / 2.0;
    double displayRadius = 0.0;
    if (m_axis && m_axis->maxRange() > 0.0 && m_axis->pixelsPerMeter() > 0.0) {
        displayRadius = m_axis->rangeToPixel(m_axis->maxRange());
    } else if (m_scene && !m_scene->sceneRect().isEmpty()) {
        displayRadius = qMin(m_scene->sceneRect().width(), m_scene->sceneRect().height()) / 2.0;
    }

    if (displayRadius > 0.0 && halfSize > 0.0) {
        const double scale = displayRadius / halfSize;
        m_pixmapItem->setTransform(QTransform::fromScale(scale, scale));
        m_pixmapItem->setOffset(-halfSize, -halfSize);
    } else {
        m_pixmapItem->setOffset(-halfSize, -halfSize);
    }
}

int EchoRenderer::drawEchoLineToImage(int aziIdx, const uint8_t* amplitudes, int count,
                                      double sourceRangeMeters, bool clearLine)
{
    if (!amplitudes || count <= 0 || m_rangeMeter <= 0 || sourceRangeMeters <= 0.0)
        return 0;

    const double halfImg = m_imageSize / 2.0;
    const double maxImageRadius = qMax(1.0, halfImg - 2.0);
    const double maxImageRadiusSq = maxImageRadius * maxImageRadius;
    const double pixelsPerMeter = maxImageRadius / m_rangeMeter;
    const double cellSpacing = sourceRangeMeters / count;
    const bool expandPoint = pixelsPerMeter * cellSpacing > 1.5;

    auto* bits = reinterpret_cast<QRgb*>(m_ppiImage.bits());
    const int stride = m_ppiImage.width();
    const double sinA = m_sinTable[aziIdx];
    const double cosA = m_cosTable[aziIdx];
    int drawnPoints = 0;

    for (int r = 0; r < count; ++r) {
        const uint8_t amp = amplitudes[r];
        if (amp < 16)
            continue;

        const double dist = (r + 0.5) * cellSpacing;
        if (dist >= m_rangeMeter)
            continue;

        const int ix = static_cast<int>(halfImg + dist * pixelsPerMeter * sinA);
        const int iy = static_cast<int>(halfImg + dist * pixelsPerMeter * cosA);

        if (ix < 0 || ix >= m_imageSize || iy < 0 || iy >= m_imageSize)
            continue;

        const double ixCenter = (ix + 0.5) - halfImg;
        const double iyCenter = (iy + 0.5) - halfImg;
        if ((ixCenter * ixCenter + iyCenter * iyCenter) >= maxImageRadiusSq)
            continue;

        const QRgb color = clearLine ? qRgba(0, 0, 0, 0) : m_colorLUT[amp];
        const int alpha = qAlpha(color);
        const int idx = iy * stride + ix;
        if (clearLine || qAlpha(bits[idx]) < alpha) {
            bits[idx] = color;
            ++drawnPoints;
        }

        if (expandPoint) {
            for (int dy = -1; dy <= 1; ++dy) {
                const int ny = iy + dy;
                if (ny < 0 || ny >= m_imageSize)
                    continue;

                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = ix + dx;
                    if (nx < 0 || nx >= m_imageSize)
                        continue;

                    const double rx = (nx + 0.5) - halfImg;
                    const double ry = (ny + 0.5) - halfImg;
                    if ((rx * rx + ry * ry) >= maxImageRadiusSq)
                        continue;

                    const int ni = ny * stride + nx;
                    if (clearLine || qAlpha(bits[ni]) < alpha) {
                        bits[ni] = color;
                        ++drawnPoints;
                    }
                }
            }
        }
    }
    return drawnPoints;
}
