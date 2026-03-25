/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-10 17:18:12
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 17:09:16
 * @Description: 
 */
/**
 * @file simpleavi.h
 * @brief 纯 Qt 无依赖 MJPEG AVI 视频写入器
 * @details 将 QImage 帧序列以 MJPEG 压缩写入 AVI 文件。
 *          不依赖 OpenCV / FFmpeg / 系统编解码器，完全跨平台。
 *          相比未压缩 RGB24，体积缩小 10~20 倍。
 *
 * 使用方法：
 *   SimpleAviWriter writer;
 *   writer.open("out.avi", width, height, fps, jpegQuality);
 *   writer.writeFrame(qimage);   // 循环调用
 *   writer.close();              // 自动回写帧数等头部字段
 */

#ifndef SIMPLEAVI_H
#define SIMPLEAVI_H

#include <QFile>
#include <QImage>
#include <QString>
#include <QVector>
#include <QBuffer>

/**
 * @class SimpleAviWriter
 * @brief 写入 MJPEG 压缩 AVI 的轻量工具类
 */
class SimpleAviWriter
{
public:
    SimpleAviWriter();
    ~SimpleAviWriter();

    /**
     * @brief 打开 AVI 文件并写入头部（MJPEG 编码）
     * @param filePath 输出文件路径
     * @param width    帧宽度（像素）
     * @param height   帧高度（像素）
     * @param fps      帧率
     * @param jpegQuality JPEG 压缩质量 (1-100, 默认70)
     * @return 成功返回 true
     */
    bool open(const QString& filePath, int width, int height, int fps,
              int jpegQuality = 70);

    /**
     * @brief 写入一帧图像（JPEG 压缩）
     * @param image 待写入帧
     */
    void writeFrame(const QImage& image);

    /**
     * @brief 关闭文件并回写帧数 / 文件大小等头部字段及 idx1 索引
     */
    void close();

    bool isOpen() const { return m_file.isOpen(); }
    int  frameCount() const { return m_frameCount; }

private:
    void writeFourCC(const char* cc);
    void writeU32(quint32 v);
    void writeU16(quint16 v);

    QFile   m_file;
    int     m_width        = 0;
    int     m_height       = 0;
    int     m_fps          = 5;
    int     m_jpegQuality  = 70;
    int     m_frameCount   = 0;
    qint64  m_moviStart    = 0;
    int     m_maxFrameSize = 0;

    struct IdxEntry {
        quint32 offset;
        quint32 size;
    };
    QVector<IdxEntry> m_idxEntries;
};

#endif // SIMPLEAVI_H
