#ifndef RADARPANELSTORE_H
#define RADARPANELSTORE_H
#pragma pack(push, 8)
#include <QFile>
#include <QSaveFile>
#include <QStringList>
#pragma pack(pop)
#include "Basic/radarpositioncontrol.h"
#pragma pack(push, 8)

// Dedicated, versioned TOML subset. Startup/manual actions only; no BIT-path disk I/O.
class RadarPanelStore {
public:
    static QStringList fields() { return {"longitude", "latitude", "altitude", "heading", "pitch", "roll"}; }
    static bool save(const QString& path, const RadarPanelValues& values, QString& error) {
        QString text = "# Radar panel locked references; degrees / metres. Do not store live BIT here.\nformat_version = 1\n";
        const auto names = fields();
        for (int id = 0; id < 4; ++id) {
            text += QString("\n[panel%1]\n").arg(id);
            for (int f = 0; f < RadarReference::Count; ++f) {
                if (!RadarReference::valid(static_cast<RadarReference::Field>(f), values[id][f])) {
                    error = QString("invalid panel%1.%2").arg(id).arg(names[f]); return false;
                }
                text += names[f] + " = " + QString::number(values[id][f], 'g', 17) + '\n';
            }
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
        const QByteArray bytes = text.toUtf8();
        if (file.write(bytes) != bytes.size()) { error = file.errorString(); file.cancelWriting(); return false; }
        if (!file.commit()) { error = file.errorString(); return false; }
        return true;
    }
    static bool load(const QString& path, RadarPanelValues& output, QString& error) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
        if (file.size() > 16384) { error = "file too large"; return false; }
        const auto bytes = file.readAll();
        if (file.error() != QFile::NoError) { error = file.errorString(); return false; }
        RadarPanelValues values{};
        std::array<unsigned, 4> seen{};
        int id = -1; bool version = false;
        const auto names = fields();
        for (QString line : QString::fromUtf8(bytes).split('\n')) {
            line = line.section('#', 0, 0).trimmed();
            if (line.isEmpty()) continue;
            if (line.startsWith('[')) {
                if (line.size() != 8 || !line.startsWith("[panel") || !line.endsWith(']')
                    || line[6] < QChar('0') || line[6] > QChar('3')) { error = "invalid panel section"; return false; }
                id = line[6].digitValue(); continue;
            }
            const auto parts = line.split('=');
            if (parts.size() != 2) { error = "invalid assignment"; return false; }
            const QString key = parts[0].trimmed(), value = parts[1].trimmed();
            if (id < 0 && key == "format_version" && value == "1" && !version) { version = true; continue; }
            const int f = names.indexOf(key);
            bool ok = false; const double number = value.toDouble(&ok);
            if (id < 0 || f < 0 || !ok || (seen[id] & (1u << f))
                || !RadarReference::valid(static_cast<RadarReference::Field>(f), number)) {
                error = "duplicate, unknown or invalid value"; return false;
            }
            values[id][f] = number; seen[id] |= 1u << f;
        }
        if (!version) { error = "unsupported/missing format_version"; return false; }
        for (auto mask : seen) if (mask != 63) { error = "incomplete panel values"; return false; }
        output = values; // All four panels must be valid; never partially restore.
        return true;
    }
};
#pragma pack(pop)
#endif
