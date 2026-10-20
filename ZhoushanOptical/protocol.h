#ifndef ZHOUSHAN_OPTICAL_PROTOCOL_H
#define ZHOUSHAN_OPTICAL_PROTOCOL_H
#include "Basic/wgs84coordinate.h"
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QtEndian>
#include <cstring>
#include <limits>

namespace ZhoushanOptical {
constexpr quint16 Indication = 0x0004; // document wire bytes: 04 00
constexpr quint16 ResponseType = 0x0104; // document wire bytes: 04 01
constexpr int HeaderSize = 18, PayloadSize = 26, FrameSize = 48;
static_assert(sizeof(float) == 4 && sizeof(double) == 8
    && std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::is_iec559,
    "Protocol requires IEEE754 binary32/binary64");

inline void append(QByteArray& b, quint64 value, int bytes) {
    for (int i = 0; i < bytes; ++i) b.append(char((value >> (8 * i)) & 255));
}
inline quint64 read(const QByteArray& b, int offset, int bytes) {
    quint64 value = 0;
    for (int i = 0; i < bytes; ++i) value |= quint64(quint8(b.at(offset + i))) << (8 * i);
    return value;
}
inline void appendDouble(QByteArray& b, double value) {
    quint64 bits; std::memcpy(&bits, &value, 8); append(b, bits, 8);
}
inline void appendFloat(QByteArray& b, float value) {
    quint32 bits; std::memcpy(&bits, &value, 4); append(b, bits, 4);
}
inline double readDouble(const QByteArray& b, int offset) {
    const quint64 bits = read(b, offset, 8); double value; std::memcpy(&value, &bits, 8); return value;
}
inline float readFloat(const QByteArray& b, int offset) {
    const quint32 bits = quint32(read(b, offset, 4)); float value; std::memcpy(&value, &bits, 4); return value;
}
inline quint16 checksum(const QByteArray& b) {
    // Appendix 4: sum unsigned 16-bit little-endian words, pad the odd byte with zero.
    quint32 sum = 0;
    for (int i = 0; i < b.size(); i += 2)
        sum += quint8(b[i]) | (i + 1 < b.size() ? quint32(quint8(b[i + 1])) << 8 : 0);
    return quint16(sum & 0xffff);
}
inline bool appendTime(QByteArray& b, const QDateTime& time) {
    const QDate d = time.date(); const QTime t = time.time();
    if (!time.isValid() || d.year() < 2000 || d.year() > 2099) return false;
    append(b, d.year() - 2000, 1); append(b, d.month(), 1); append(b, d.day(), 1);
    append(b, t.hour(), 1); append(b, t.minute(), 1); append(b, t.second(), 1); append(b, t.msec(), 2);
    return true;
}
inline bool validTime(const QByteArray& b, int offset) {
    return QDate(2000 + int(read(b, offset, 1)), int(read(b, offset + 1, 1)), int(read(b, offset + 2, 1))).isValid()
        && read(b, offset, 1) <= 99 && read(b, offset + 3, 1) < 24
        && read(b, offset + 4, 1) < 60 && read(b, offset + 5, 1) < 60 && read(b, offset + 6, 2) <= 999;
}
inline QByteArray indication(quint16 sender, quint16 receiver, quint8 kind, quint32 batch,
                              const Wgs84Coordinate::Lla& target, const QDateTime& time) {
    const float height = float(target.altitudeM);
    if ((kind != 1 && kind != 2) || !Wgs84Coordinate::isValid(target) || !std::isfinite(height)) return {};
    QByteArray b = QByteArray::fromHex("eb90");
    append(b, Indication, 2); append(b, sender, 2); append(b, receiver, 2);
    if (!appendTime(b, time)) return {};
    append(b, PayloadSize, 2);
    append(b, kind, 1); append(b, batch, 4); append(b, 1, 1); // source=radar, not drone classification
    appendDouble(b, target.longitudeDeg); appendDouble(b, target.latitudeDeg); appendFloat(b, height);
    append(b, checksum(b.mid(2)), 2); b += QByteArray::fromHex("4c5a");
    return b;
}
struct Response {
    quint8 kind = 0, state = 0;
    quint32 batch = 0;
    Wgs84Coordinate::Lla target;
    bool altitudeValid = true;
};
inline bool parseResponse(const QByteArray& b, quint16 sender, quint16 receiver, Response& result, QString& error) {
    error.clear();
    if (b.size() != FrameSize || b.left(2) != QByteArray::fromHex("eb90")
        || b.right(2) != QByteArray::fromHex("4c5a") || read(b, 2, 2) != ResponseType
        || read(b, 4, 2) != sender || read(b, 6, 2) != receiver || read(b, 16, 2) != PayloadSize
        || !validTime(b, 8) || read(b, 44, 2) != checksum(b.mid(2, 42))) {
        error = QStringLiteral("帧头尾/类型/ID/时标/长度/16位字校验错误"); return false;
    }
    result.kind = quint8(b[18]); result.batch = quint32(read(b, 19, 4)); result.state = quint8(b[23]);
    result.target = {readDouble(b, 24), readDouble(b, 32), readFloat(b, 40)};
    const quint32 altitudeBits = quint32(read(b, 40, 4));
    // Section 6 prints 0xFFFFFFF; other tables use 0xFFFFFFFF for an invalid float.
    result.altitudeValid = altitudeBits != 0x0fffffff && altitudeBits != 0xffffffff;
    Wgs84Coordinate::Lla checked = result.target;
    if (!result.altitudeValid) checked.altitudeM = 0;
    if (!Wgs84Coordinate::isValid(checked) || (result.kind != 1 && result.kind != 2)
        || result.state < 1 || result.state > (result.kind == 1 ? 2 : 3)) {
        error = QStringLiteral("响应类型/状态/经纬高非法"); return false;
    }
    return true;
}

// Only section-6 response frames are accepted. Unknown/control frames never reach the UI.
class StreamDecoder {
public:
    bool appendData(const QByteArray& bytes) {
        if (m_buffer.size() + bytes.size() > 8192) { m_buffer.clear(); return false; }
        m_buffer += bytes; return true;
    }
    QByteArray take(quint64& discarded) {
        while (!m_buffer.isEmpty()) {
            const int head = m_buffer.indexOf(QByteArray::fromHex("eb90"));
            if (head < 0) {
                const bool keep = m_buffer.endsWith(char(0xeb));
                discarded += quint64(m_buffer.size() - int(keep));
                m_buffer = keep ? QByteArray(1, char(0xeb)) : QByteArray(); return {};
            }
            if (head) { discarded += quint64(head); m_buffer.remove(0, head); }
            if (m_buffer.size() < HeaderSize) return {};
            if (read(m_buffer, 2, 2) != ResponseType || read(m_buffer, 16, 2) != PayloadSize) {
                ++discarded; m_buffer.remove(0, 1); continue;
            }
            if (m_buffer.size() < FrameSize) return {};
            const QByteArray frame = m_buffer.left(FrameSize);
            if (frame.right(2) != QByteArray::fromHex("4c5a") || read(frame, 44, 2) != checksum(frame.mid(2, 42))) {
                ++discarded; m_buffer.remove(0, 1); continue;
            }
            m_buffer.remove(0, FrameSize); return frame;
        }
        return {};
    }
    void clear() { m_buffer.clear(); }
    bool hasPartial() const { return !m_buffer.isEmpty(); }
private:
    QByteArray m_buffer;
};
} // namespace ZhoushanOptical
#endif
