/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-03-10 17:18:12
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:57
 * @Description: 
 */
#include "simpleavi.h"
#include "Basic/log.h"
#include <QDebug>
#include <QBuffer>
#include <cstring>

SimpleAviWriter::SimpleAviWriter() = default;
SimpleAviWriter::~SimpleAviWriter() { if (m_file.isOpen()) close(); }

void SimpleAviWriter::writeFourCC(const char* cc) { m_file.write(cc, 4); }
void SimpleAviWriter::writeU32(quint32 v) {
    char buf[4];
    buf[0] = static_cast<char>(v & 0xFF);
    buf[1] = static_cast<char>((v >> 8) & 0xFF);
    buf[2] = static_cast<char>((v >> 16) & 0xFF);
    buf[3] = static_cast<char>((v >> 24) & 0xFF);
    m_file.write(buf, 4);
}
void SimpleAviWriter::writeU16(quint16 v) {
    char buf[2];
    buf[0] = static_cast<char>(v & 0xFF);
    buf[1] = static_cast<char>((v >> 8) & 0xFF);
    m_file.write(buf, 2);
}

bool SimpleAviWriter::open(const QString& filePath, int width, int height, int fps,
                           int jpegQuality)
{
    if (m_file.isOpen()) close();
    m_width = width; m_height = height;
    m_fps = (fps > 0) ? fps : 5;
    m_jpegQuality = qBound(1, jpegQuality, 100);
    m_frameCount = 0; m_maxFrameSize = 0;
    m_idxEntries.clear();
    int est = m_width * m_height * 3;

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        LOG_WARNING(QString("[SimpleAviWriter] Cannot open: %1").arg(filePath));
        return false;
    }
    quint32 usPerFrame = 1000000 / static_cast<quint32>(m_fps);

    // RIFF header
    writeFourCC("RIFF"); writeU32(0); writeFourCC("AVI ");
    // LIST hdrl
    writeFourCC("LIST"); writeU32(192); writeFourCC("hdrl");
    // avih 56 bytes
    writeFourCC("avih"); writeU32(56);
    writeU32(usPerFrame);
    writeU32(static_cast<quint32>(est) * static_cast<quint32>(m_fps));
    writeU32(0); writeU32(0x10); writeU32(0); writeU32(0);
    writeU32(1); writeU32(static_cast<quint32>(est));
    writeU32(static_cast<quint32>(m_width));
    writeU32(static_cast<quint32>(m_height));
    writeU32(0); writeU32(0); writeU32(0); writeU32(0);
    // LIST strl
    writeFourCC("LIST"); writeU32(116); writeFourCC("strl");
    // strh 56 bytes
    writeFourCC("strh"); writeU32(56);
    writeFourCC("vids"); writeFourCC("MJPG");
    writeU32(0); writeU16(0); writeU16(0); writeU32(0);
    writeU32(1); writeU32(static_cast<quint32>(m_fps));
    writeU32(0); writeU32(0);
    writeU32(static_cast<quint32>(est));
    writeU32(0xFFFFFFFF); writeU32(0);
    writeU16(0); writeU16(0);
    writeU16(static_cast<quint16>(m_width));
    writeU16(static_cast<quint16>(m_height));
    // strf 40 bytes
    writeFourCC("strf"); writeU32(40);
    writeU32(40);
    writeU32(static_cast<quint32>(m_width));
    writeU32(static_cast<quint32>(m_height));
    writeU16(1); writeU16(24);
    writeFourCC("MJPG");
    writeU32(static_cast<quint32>(m_width * m_height * 3));
    writeU32(0); writeU32(0); writeU32(0); writeU32(0);
    // LIST movi
    writeFourCC("LIST"); writeU32(0); writeFourCC("movi");
    m_moviStart = m_file.pos();
    return true;
}

void SimpleAviWriter::writeFrame(const QImage& image) {
    if (!m_file.isOpen()) return;
    QImage frame = image.scaled(m_width, m_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QByteArray jpegData;
    QBuffer buffer(&jpegData);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG", m_jpegQuality);
    buffer.close();
    if (jpegData.isEmpty()) return;

    IdxEntry entry;
    entry.offset = static_cast<quint32>(m_file.pos() - m_moviStart + 4);
    entry.size = static_cast<quint32>(jpegData.size());
    m_idxEntries.append(entry);

    bool needPad = (jpegData.size() % 2) != 0;
    writeFourCC("00dc");
    writeU32(static_cast<quint32>(jpegData.size()));
    m_file.write(jpegData);
    if (needPad) { char z = 0; m_file.write(&z, 1); }
    if (jpegData.size() > m_maxFrameSize) m_maxFrameSize = jpegData.size();
    m_frameCount++;
}

void SimpleAviWriter::close() {
    if (!m_file.isOpen()) return;
    qint64 moviEnd = m_file.pos();

    // idx1
    writeFourCC("idx1");
    writeU32(static_cast<quint32>(m_idxEntries.size() * 16));
    for (const auto& e : m_idxEntries) {
        writeFourCC("00dc"); writeU32(0x10);
        writeU32(e.offset); writeU32(e.size);
    }
    qint64 fileEnd = m_file.pos();

    // patch RIFF size
    m_file.seek(4); writeU32(static_cast<quint32>(fileEnd - 8));
    // patch avih.dwTotalFrames
    m_file.seek(48); writeU32(static_cast<quint32>(m_frameCount));
    // patch avih.dwSuggestedBufferSize
    m_file.seek(60); writeU32(static_cast<quint32>(m_maxFrameSize));
    // patch strh.dwLength
    m_file.seek(132); writeU32(static_cast<quint32>(m_frameCount));
    // patch strh.dwSuggestedBufferSize
    m_file.seek(136); writeU32(static_cast<quint32>(m_maxFrameSize));
    // patch movi LIST size
    m_file.seek(m_moviStart - 8);
    writeU32(static_cast<quint32>(moviEnd - m_moviStart + 4));

    m_file.close();
    m_idxEntries.clear();
}
