/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-10 16:30:42
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-10 17:18:14
 * @Description: 
 */
/**
 * @file simpleavi.h
 * @brief 纯 Qt 无依赖 AVI 视频写入器
 * @details 将 QImage 帧序列写入未压缩 AVI 文件（RIFF AVI, BI_RGB）。
 *          不依赖 OpenCV / FFmpeg / 系统编解码器，完全跨平台。
 *
 * 使用方法：
 *   SimpleAviWriter writer;
 *   writer.open("out.avi", width, height, fps);
 *   writer.writeFrame(qimage);   // 循环调用
 *   writer.close();              // 自动回写帧数等头部字段
 *
 * @author DispCtrl Team
 * @date 2026
 */

#ifndef SIMPLEAVI_H
#define SIMPLEAVI_H

#include <QFile>
#include <QImage>
#include <QString>

/**
 * @class SimpleAviWriter
 * @brief 写入未压缩 AVI (RGB24) 的轻量工具类
 */
class SimpleAviWriter
{
public:
    SimpleAviWriter();
    ~SimpleAviWriter();

    /**
     * @brief 打开 AVI 文件并写入头部
     * @param filePath 输出文件路径
     * @param width    帧宽度（像素）
     * @param height   帧高度（像素）
     * @param fps      帧率
     * @return 成功返回 true
     */
    bool open(const QString& filePath, int width, int height, int fps);

    /**
     * @brief 写入一帧图像
     * @param image 待写入帧（会自动缩放/转换为目标尺寸 RGB888）
     */
    void writeFrame(const QImage& image);

    /**
     * @brief 关闭文件并回写帧数 / 文件大小等头部字段
     */
    void close();

    bool isOpen() const { return m_file.isOpen(); }
    int  frameCount() const { return m_frameCount; }

private:
    void writeFourCC(const char* cc);
    void writeU32(quint32 v);
    void writeU16(quint16 v);

    QFile   m_file;
    int     m_width      = 0;
    int     m_height     = 0;
    int     m_fps        = 5;
    int     m_frameCount = 0;
    qint64  m_moviStart  = 0;   ///< "movi" list 数据区起始偏移
    int     m_frameSize  = 0;   ///< 每帧 BMP 行对齐后的字节数
};

#endif // SIMPLEAVI_H
