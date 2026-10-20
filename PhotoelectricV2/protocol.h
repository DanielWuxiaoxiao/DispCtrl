#ifndef PHOTOELECTRIC_V2_PROTOCOL_H
#define PHOTOELECTRIC_V2_PROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QVariantMap>
#include <QtEndian>
#include <cmath>

// V2.0 uses unsigned, little-endian integers. No native struct layout on wire.
namespace PhotoelectricV2 {
constexpr quint32 Head = 0x7D6D5D4C;
constexpr quint16 Target = 0x3200, Status = 0x3000, Heartbeat = 0x3100;
constexpr quint16 Guide = 0x3400, Control = 0x3500;

inline void append(QByteArray& b, quint64 v, int n) {
    for (int i = 0; i < n; ++i) b.append(char((v >> (8 * i)) & 255));
}
inline quint64 read(const QByteArray& b, int offset, int n) {
    quint64 v = 0;
    for (int i = 0; i < n; ++i) v |= quint64(quint8(b.at(offset + i))) << (8 * i);
    return v;
}
inline quint8 checksum(const QByteArray& b) {
    quint8 sum = 0;
    for (char c : b) sum = quint8(sum + quint8(c));
    return sum;
}
inline QByteArray header(quint16 function) {
    QByteArray b;
    b.reserve(64);
    append(b, 1, 1); append(b, Head, 4); append(b, 0, 2); append(b, function, 2);
    return b;
}
inline QByteArray finish(QByteArray b) {
    const quint16 size = quint16(b.size() + 1);
    b[5] = char(size & 255); b[6] = char(size >> 8);
    b.append(char(checksum(b)));
    return b;
}
inline bool unsignedValue(double value, double scale, quint64 max, quint64& encoded) {
    if (!std::isfinite(value) || value < 0 || value * scale > double(max)) return false;
    encoded = quint64(std::llround(value * scale));
    return encoded <= max;
}
inline QByteArray guidance(quint32 device, quint32 terminal, quint64 timestamp,
                           quint64 batch, double lon, double lat, double altitude,
                           double speed, quint8 source = 0) {
    quint64 x, y, h, s;
    if (lon > 180 || lat > 90 || source > 3
        || !unsignedValue(lon, 100000, 0xffffffffULL, x)
        || !unsignedValue(lat, 100000, 0xffffffffULL, y)
        || !unsignedValue(altitude + 500, 1, 65535, h)
        || !unsignedValue(speed, 1, 65535, s)) return {};
    QByteArray b = header(Guide);
    append(b, device, 4); append(b, terminal, 4); append(b, timestamp, 8);
    append(b, batch, 8); append(b, x, 4); append(b, y, 4);
    append(b, h, 2); append(b, s, 2); append(b, source, 1);
    return finish(b); // 47 bytes, length includes version/header/checksum.
}
inline QByteArray control(quint32 device, quint32 terminal, quint64 timestamp,
                          const QByteArray& commands) {
    if (commands.size() != 8) return {};
    const int limits[] = {1, 1, 1, 1, 4, 1, 7, 6};
    for (int i = 0; i < 8; ++i) if (quint8(commands[i]) > limits[i]) return {};
    QByteArray b = header(Control);
    append(b, device, 4); append(b, terminal, 4); append(b, timestamp, 8);
    b += commands;
    return finish(b); // 34 bytes
}
inline QByteArray heartbeat(quint32 terminal, quint64 timestamp) {
    QByteArray b = header(Heartbeat);
    append(b, terminal, 4); append(b, timestamp, 8);
    return finish(b); // 22 bytes; not the legacy 0x0100 heartbeat.
}
inline bool parse(const QByteArray& b, QVariantMap& fields, QString& error) {
    fields.clear();
    error.clear();
    if (b.size() < 10 || quint8(b[0]) != 1 || read(b, 1, 4) != Head) {
        error = QStringLiteral("版本/帧头错误"); return false;
    }
    if (read(b, 5, 2) != quint64(b.size())
        || checksum(b.left(b.size() - 1)) != quint8(b.back())) {
        error = QStringLiteral("总长度/累加校验错误"); return false;
    }
    const quint16 f = quint16(read(b, 7, 2));
    const int expected = f == Target ? 60 : f == Status ? 43 : f == Heartbeat ? 22 : 0;
    if (!expected || b.size() != expected) {
        error = QStringLiteral("未知功能号/固定帧长错误"); return false;
    }
    fields["function"] = f;
    if (f == Heartbeat) {
        fields["terminal"] = quint32(read(b, 9, 4));
        fields["timestamp"] = QVariant::fromValue<qulonglong>(read(b, 13, 8));
        return true;
    }
    if (quint8(b[9]) > 1) { error = QStringLiteral("组网模式非法"); return false; }
    fields["networkMode"] = quint8(b[9]);
    fields["device"] = quint32(read(b, 10, 4));
    fields["terminal"] = quint32(read(b, 14, 4));
    fields["timestamp"] = QVariant::fromValue<qulonglong>(read(b, 18, 8));
    if (f == Target) {
        fields["batch"] = quint32(read(b, 26, 4));
        fields["guideTarget"] = QVariant::fromValue<qulonglong>(read(b, 30, 8));
        fields["azimuth"] = read(b, 38, 2) / 100.0;
        fields["elevation"] = (double(read(b, 40, 2)) - 10000) / 100.0;
        fields["range"] = quint32(read(b, 42, 4));
        fields["attribute"] = quint8(b[46]);
        fields["altitude"] = int(read(b, 47, 2)) - 500;
        fields["relativeHeight"] = quint16(read(b, 49, 2));
        fields["longitude"] = read(b, 51, 4) / 100000.0;
        fields["latitude"] = read(b, 55, 4) / 100000.0;
    } else {
        fields["longitude"] = read(b, 26, 4) / 100000.0;
        fields["latitude"] = read(b, 30, 4) / 100000.0;
        fields["altitude"] = int(read(b, 34, 2)) - 500;
        fields["azimuth"] = read(b, 36, 2) / 100.0;
        fields["elevation"] = (double(read(b, 38, 2)) - 10000) / 100.0;
        fields["mode"] = quint8(b[40]); fields["fault"] = quint8(b[41]);
    }
    if (fields["longitude"].toDouble() > 180 || fields["latitude"].toDouble() > 90
        || fields["azimuth"].toDouble() > 360 || std::abs(fields["elevation"].toDouble()) > 90
        || (f == Target && fields["attribute"].toUInt() > 8)
        || (f == Status && (fields["mode"].toUInt() < 1 || fields["mode"].toUInt() > 6
                          || fields["fault"].toUInt() > 15))) {
        error = QStringLiteral("字段超出V2.0范围"); return false;
    }
    return true;
}
} // namespace PhotoelectricV2
#endif
