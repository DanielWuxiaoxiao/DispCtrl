#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>

#define CF_INS ConfigManager::instance()

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    bool load(const QString& path = "config.json") {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Config file not found:" << path << ", using defaults.";
            return false;
        }

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Invalid config.json format, using defaults.";
            return false;
        }

        root = doc.object();
        return true;
    }

    QString ip(const QString& key, const QString& def = "127.0.0.1") const {
        auto val = root["network"].toObject()["ips"].toObject().value(key);
        return val.isString() ? val.toString() : def;
    }

    int id(const QString& key, int def = -1) const {
        auto val = root["network"].toObject()["ids"].toObject().value(key);
        return val.isDouble() ? val.toInt() : def;
    }

    int port(const QString& key, int def = 0) const {
        auto val = root["network"].toObject()["ports"].toObject().value(key);
        return val.isDouble() ? val.toInt() : def;
    }

    int range(const QString& key, int def = 0) const {
        auto val = root["polarDisp"].toObject()["range"].toObject().value(key);
        return val.isDouble() ? val.toInt() : def;
    }

private:
    QJsonObject root;
    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

#endif // CONFIGMANAGER_H
