/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-10 16:31:49
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-10 17:18:14
 * @Description: 
 */
/**
 * @file simpleavi.cpp
 * @brief 纯 Qt 无依赖 AVI 视频写入器实现
 * @details
 *   AVI 文件结构 (RIFF AVI, 未压缩 RGB24):
 *
 *   RIFF 'AVI '
 *     LIST 'hdrl'
 *       'avih'  — 主 AVI 头
 *       LIST 'strl'
 *         'strh' — 流头（vids / DIB）
 *         'strf' — 流格式（BITMAPINFOHEADER）
 *     LIST 'movi'
 *       '00dc' chunk — 逐帧 DIB 数据（上下翻转 BGR24 行对齐到4字节）
 *
 *   close() 时回写: RIFF size / avih.dwTotalFrames / movi list size
 *
 * @author DispCtrl Team
 * @date 2026
 */

#include "simpleavi.h"
#include <QDebug>
#include <cstring>

// BMP 行字节 = ((width * 24 + 31) / 32) * 4
static int bmpRowBytes(int width)
{
    return ((width * 3 + 3) / 4) * 4;
}

SimpleAviWriter::SimpleAviWriter() = default;

SimpleAviWriter::~SimpleAviWriter()
{
    if (m_file.isOpen())
        close();
}

// ---------- 低级写入辅助 ----------
void SimpleAviWriter::writeFourCC(const char* cc)
{
    m_file.write(cc, 4);
}

void SimpleAviWriter::writeU32(quint32 v)
{
    char buf[4];
    buf[0] = static_cast<char>(v & 0xFF);
    buf[1] = static_cast<char>((v >> 8) & 0xFF);
    buf[2] = static_cast<char>((v >> 16) & 0xFF);
    buf[3] = static_cast<char>((v >> 24) & 0xFF);
    m_file.write(buf, 4);
}

void SimpleAviWriter::writeU16(quint16 v)
{
    char buf[2];
    buf[0] = static_cast<char>(v & 0xFF);
    buf[1] = static_cast<char>((v >> 8) & 0xFF);
    m_file.write(buf, 2);
}

// =============================================================================
// open — 写入完整 AVI 头部（占位帧数/文件大小，close() 时回写）
// =============================================================================
bool SimpleAviWriter::open(const QString& filePath, int width, int height, int fps)
{
    if (m_file.isOpen()) close();

    m_width  = width;
    m_height = height;
    m_fps    = (fps > 0) ? fps : 5;
    m_frameCount = 0;
    m_frameSize  = bmpRowBytes(m_width) * m_height;

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        qWarning() << "[SimpleAviWriter] Cannot open:" << filePath;
        return false;
    }

    quint32 usPerFrame = 1000000 / static_cast<quint32>(m_fps);

    // ---------- RIFF header ----------
    writeFourCC("RIFF");
    writeU32(0);            // placeholder: file size - 8, patched in close()
    writeFourCC("AVI ");

    // ---------- LIST hdrl ----------
    writeFourCC("LIST");
    // hdrl list size: avih chunk (8+56) + strl list (8+4 + strh(8+56) + strf(8+40)) = 192
    writeU32(192);
    writeFourCC("hdrl");

    // --- avih (MainAVIHeader) 56 bytes ---
    writeFourCC("avih");
    writeU32(56);                     // chunk size
    writeU32(usPerFrame);             // dwMicroSecPerFrame
    writeU32(static_cast<quint32>(m_frameSize) * static_cast<quint32>(m_fps)); // dwMaxBytesPerSec
    writeU32(0);                      // dwPaddingGranularity
    writeU32(0x10);                   // dwFlags: AVIF_HASINDEX (0x10) — 简化
    writeU32(0);                      // dwTotalFrames (placeholder → close)
    writeU32(0);                      // dwInitialFrames
    writeU32(1);                      // dwStreams
    writeU32(static_cast<quint32>(m_frameSize)); // dwSuggestedBufferSize
    writeU32(static_cast<quint32>(m_width));
    writeU32(static_cast<quint32>(m_height));
    writeU32(0); writeU32(0); writeU32(0); writeU32(0); // reserved[4]

    // ---------- LIST strl ----------
    writeFourCC("LIST");
    writeU32(116);          // strl list size = 4 + (8+56) + (8+40) = 116
    writeFourCC("strl");

    // --- strh (AVIStreamHeader) 56 bytes ---
    writeFourCC("strh");
    writeU32(56);
    writeFourCC("vids");              // fccType
    writeFourCC("DIB ");              // fccHandler (uncompressed)
    writeU32(0);                      // dwFlags
    writeU16(0);                      // wPriority
    writeU16(0);                      // wLanguage
    writeU32(0);                      // dwInitialFrames
    writeU32(1);                      // dwScale
    writeU32(static_cast<quint32>(m_fps));  // dwRate
    writeU32(0);                      // dwStart
    writeU32(0);                      // dwLength (placeholder → close)
    writeU32(static_cast<quint32>(m_frameSize)); // dwSuggestedBufferSize
    writeU32(0xFFFFFFFF);             // dwQuality (-1 = default)
    writeU32(0);                      // dwSampleSize
    writeU16(0); writeU16(0);         // rcFrame left, top
    writeU16(static_cast<quint16>(m_width));
    writeU16(static_cast<quint16>(m_height));

    // --- strf (BITMAPINFOHEADER) 40 bytes ---
    writeFourCC("strf");
    writeU32(40);                     // chunk size
    writeU32(40);                     // biSize
    writeU32(static_cast<quint32>(m_width));  // biWidth
    writeU32(static_cast<quint32>(m_height)); // biHeight（正值 = bottom-up）
    writeU16(1);                      // biPlanes
    writeU16(24);                     // biBitCount
    writeU32(0);                      // biCompression = BI_RGB
    writeU32(static_cast<quint32>(m_frameSize)); // biSizeImage
    writeU32(0);                      // biXPelsPerMeter
    writeU32(0);                      // biYPelsPerMeter
    writeU32(0);                      // biClrUsed
    writeU32(0);                      // biClrImportant

    // ---------- LIST movi ----------
    writeFourCC("LIST");
    writeU32(0);            // placeholder: movi data size → close()
    writeFourCC("movi");

    m_moviStart = m_file.pos();       // 记录 movi 数据区起始

    return true;
}

// =============================================================================
// writeFrame — 将 QImage 转换为 BGR24 bottom-up 写入一个 "00dc" chunk
// =============================================================================
void SimpleAviWriter::writeFrame(const QImage& image)
{
    if (!m_file.isOpen()) return;

    // 转换为目标尺寸的 RGB888
    QImage frame = image.scaled(m_width, m_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_RGB888);

    int rowBytes = bmpRowBytes(m_width);
    QByteArray bmpData(m_frameSize, '\0');

    // BMP bottom-up: 文件中第一行对应图像最底行
    for (int y = 0; y < m_height; ++y) {
        const uchar* src = frame.constScanLine(m_height - 1 - y);
        char* dst = bmpData.data() + y * rowBytes;
        for (int x = 0; x < m_width; ++x) {
            // QImage RGB888: R G B → AVI DIB: B G R
            dst[x * 3 + 0] = static_cast<char>(src[x * 3 + 2]); // B
            dst[x * 3 + 1] = static_cast<char>(src[x * 3 + 1]); // G
            dst[x * 3 + 2] = static_cast<char>(src[x * 3 + 0]); // R
        }
        // 行尾 padding 保持为 0（QByteArray 已初始化）
    }

    // 写入 "00dc" chunk
    writeFourCC("00dc");
    writeU32(static_cast<quint32>(m_frameSize));
    m_file.write(bmpData);

    m_frameCount++;
}

// =============================================================================
// close — 回写帧数 / 文件大小 / movi list 大小
// =============================================================================
void SimpleAviWriter::close()
{
    if (!m_file.isOpen()) return;

    qint64 fileEnd = m_file.pos();

    // 1) 回写 RIFF size (offset 4): fileEnd - 8
    m_file.seek(4);
    writeU32(static_cast<quint32>(fileEnd - 8));

    // 2) 回写 avih.dwTotalFrames (offset = 12 + 8 + 4 + 8 + 16 = 48)
    //    RIFF(4+4) + AVI (4) + LIST(4+4) + hdrl(4) + avih(4+4) + usPerFrame(4) + maxBytes(4) + padding(4) + flags(4)
    //    = 48
    m_file.seek(48);
    writeU32(static_cast<quint32>(m_frameCount));

    // 3) 回写 strh.dwLength (avih 结束于 offset 12+8+56=76 之后)
    //    strh.dwLength 位于 strh chunk 内: strl LIST 开头 = 76
    //    76 = RIFF(8) + AVI(4) + LIST(8) + hdrl(4) + avih_chunk(8+56)
    //    strl: LIST(8) + strl(4) + strh(8) + fccType(4) + fccHandler(4) + flags(4) + priority(2) + lang(2) + initFrames(4) + scale(4) + rate(4) + start(4) = 76 + 8+4+8+4+4+4+2+2+4+4+4+4 = 128
    //    dwLength at offset 132
    m_file.seek(132);
    writeU32(static_cast<quint32>(m_frameCount));

    // 4) 回写 movi LIST size (offset = m_moviStart - 8, 即 LIST size 字段)
    //    movi data = m_moviStart ~ fileEnd 之间的数据 + "movi" 4CC (4 bytes)
    qint64 moviListSizeOffset = m_moviStart - 8;
    quint32 moviDataSize = static_cast<quint32>(fileEnd - m_moviStart + 4); // +4 for "movi" fourcc
    m_file.seek(moviListSizeOffset);
    writeU32(moviDataSize);

    m_file.close();
}
