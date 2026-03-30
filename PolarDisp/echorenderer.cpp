/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 11:48:38
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:10
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
#include <QDateTime>
#include <QTransform>

EchoRenderer::EchoRenderer(QGraphicsScene* scene, int imageSize, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_imageSize(imageSize)
    , m_ppiImage(imageSize, imageSize, QImage::Format_ARGB32_Premultiplied)
{
    m_ppiImage.fill(Qt::transparent);

    // 创建 PixmapItem 并添加到场景
    m_pixmapItem = new QGraphicsPixmapItem();
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    m_pixmapItem->setZValue(1);  // 在网格之上, 标注之下
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

    m_elapsed.start();
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
    // 参照实机截图，色彩从深蓝→青→绿→黄→橙→红→亮红白，过渡丰富饱和
    // 0~15:   透明(噪底)
    // 16~50:  深蓝→蓝(弱回波)
    // 51~85:  蓝→青(海面杂波)
    // 86~120: 青→绿(中等回波)
    // 121~155: 绿→黄(较强回波)
    // 156~190: 黄→橙(强回波)
    // 191~225: 橙→红(很强回波)
    // 226~245: 亮红(极强回波，如岸线)
    // 246~255: 亮红→白(天气/最强)

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
            int g = static_cast<int>(t * 60);
            int b = static_cast<int>(t * 30);
            m_colorLUT[i] = qRgba(255, g, b, 255);
        } else {
            // 亮红→白
            double t = (i - 246) / 9.0;
            int g = static_cast<int>(60 + t * 195);
            int b = static_cast<int>(30 + t * 225);
            m_colorLUT[i] = qRgba(255, g, b, 255);
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
    if (rangeMeters > 0)
        m_rangeMeter = rangeMeters;
}

void EchoRenderer::setRenderInterval(int ms)
{
    m_renderTimer->setInterval(qMax(16, ms));  // 最高~60fps
}

void EchoRenderer::clear()
{
    for (auto& line : m_sweepBuf) {
        line.amp.fill(0);
        line.cellCount = 0;
        line.timestamp = 0;
    }
    m_ppiImage.fill(Qt::transparent);
    if (m_pixmapItem) {
        m_pixmapItem->setPixmap(QPixmap::fromImage(m_ppiImage));
    }
}

// ============================================================================
// 数据输入
// ============================================================================

void EchoRenderer::updateEchoLine(const MarineEchoLine& line)
{
    int aziIdx = line.azimuthRaw % AZI_STEPS;
    auto& sl = m_sweepBuf[aziIdx];

    int count = qMin(line.cellCount(), MAX_CELLS);
    sl.cellCount = count;
    sl.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 拷贝幅值
    if (count > 0) {
        memcpy(sl.amp.data(), line.amplitudes.constData(), count);
    }
    // 剩余部分清零
    if (count < MAX_CELLS) {
        memset(sl.amp.data() + count, 0, MAX_CELLS - count);
    }
}

// ============================================================================
// 渲染
// ============================================================================

void EchoRenderer::renderFrame()
{
    renderSweepToImage();

    if (m_pixmapItem) {
        m_pixmapItem->setPixmap(QPixmap::fromImage(m_ppiImage));

        // 将 m_imageSize×m_imageSize 的图像映射到场景坐标
        // 图像中心 = halfSize 像素 ↔ 场景中心 (0,0)
        // 图像边缘 = halfSize 像素 ↔ 场景中距离=range的位置
        // 场景中 range 距离 = sceneRect.width()/2 像素(近似)
        double halfSize = m_imageSize / 2.0;
        double sceneHalf = 0;
        if (m_scene && !m_scene->sceneRect().isEmpty()) {
            sceneHalf = qMin(m_scene->sceneRect().width(), m_scene->sceneRect().height()) / 2.0;
        }

        if (sceneHalf > 0 && halfSize > 0) {
            double scale = sceneHalf / halfSize;
            m_pixmapItem->setTransform(QTransform::fromScale(scale, scale));
            m_pixmapItem->setOffset(-halfSize, -halfSize);
        } else {
            m_pixmapItem->setOffset(-halfSize, -halfSize);
        }
    }
}

void EchoRenderer::renderSweepToImage()
{
    m_ppiImage.fill(Qt::transparent);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    double halfImg = m_imageSize / 2.0;
    double pixelsPerMeter = halfImg / m_rangeMeter;

    auto* bits = reinterpret_cast<QRgb*>(m_ppiImage.bits());
    int stride = m_ppiImage.width();

    // 遍历每条扫描线
    for (int azi = 0; azi < AZI_STEPS; ++azi) {
        const auto& sl = m_sweepBuf[azi];
        if (sl.cellCount <= 0 || sl.timestamp == 0)
            continue;

        // 余辉衰减
        qint64 age = now - sl.timestamp;
        if (age > m_decayMs)
            continue;  // 已过期

        double decayFactor = 1.0 - static_cast<double>(age) / m_decayMs;
        if (decayFactor <= 0.0)
            continue;

        double sinA = m_sinTable[azi];
        double cosA = m_cosTable[azi];

        // 距离单元间距(米)
        double cellSpacing = m_rangeMeter / sl.cellCount;

        for (int r = 0; r < sl.cellCount; ++r) {
            uint8_t amp = sl.amp[r];
            if (amp < 16) continue;  // 噪底跳过

            // 距离(米)
            double dist = (r + 0.5) * cellSpacing;

            // 像素坐标
            double px = halfImg + dist * pixelsPerMeter * sinA;
            double py = halfImg + dist * pixelsPerMeter * cosA;

            int ix = static_cast<int>(px);
            int iy = static_cast<int>(py);

            if (ix < 0 || ix >= m_imageSize || iy < 0 || iy >= m_imageSize)
                continue;

            QRgb color = m_colorLUT[amp];

            // 应用余辉衰减到alpha
            int alpha = static_cast<int>(qAlpha(color) * decayFactor);
            color = qRgba(qRed(color), qGreen(color), qBlue(color), alpha);

            // 写入像素 (简单覆盖, 亮者优先)
            int idx = iy * stride + ix;
            if (qAlpha(bits[idx]) < alpha) {
                bits[idx] = color;
            }

            // 为了视觉效果, 近距离单元画2×2像素
            if (pixelsPerMeter * cellSpacing > 1.5) {
                // 扩展到相邻像素
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = ix + dx;
                        int ny = iy + dy;
                        if (nx >= 0 && nx < m_imageSize && ny >= 0 && ny < m_imageSize) {
                            int ni = ny * stride + nx;
                            if (qAlpha(bits[ni]) < alpha) {
                                bits[ni] = color;
                            }
                        }
                    }
                }
            }
        }
    }
}
