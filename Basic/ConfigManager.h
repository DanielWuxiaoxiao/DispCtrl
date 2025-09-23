/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:53
 * @Description: 
 */
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QTextStream>

#define CF_INS ConfigManager::instance()

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    bool load(const QString& path = "config.toml") {
        // 首先尝试加载TOML文件
        if (path.endsWith(".toml")) {
            return loadToml(path);
        } 
        // 向后兼容，支持JSON文件
        else if (path.endsWith(".json")) {
            return loadJson(path);
        }
        // 默认尝试TOML
        else {
            QString tomlPath = path + ".toml";
            if (QFile::exists(tomlPath)) {
                return loadToml(tomlPath);
            } else {
                QString jsonPath = path + ".json";
                return loadJson(jsonPath);
            }
        }
    }

    QString ip(const QString& key, const QString& def = "127.0.0.1") const {
        return getValue("network.ips." + key, def).toString();
    }

    int id(const QString& key, int def = -1) const {
        return getValue("network.ids." + key, def).toInt();
    }

    int port(const QString& key, int def = 0) const {
        return getValue("network.ports." + key, def).toInt();
    }

    int range(const QString& key, int def = 0) const {
        return getValue("polarDisp.range." + key, def).toInt();
    }

    int mapType(const QString& key, int def = 1) const {
        return getValue("map." + key, def).toInt();
    }

    // 扇形显示相关配置
    double sectorAngle(const QString& key, double def = 0.0) const {
        return getValue("sectorDisp.angle." + key, def).toDouble();
    }

    double sectorRange(const QString& key, double def = 0.0) const {
        return getValue("sectorDisp.range." + key, def).toDouble();
    }

    // 雷达位置相关配置
    double latitude(const QString& key = "latitude", double def = 34.2311) const {
        return getValue("radar." + key, def).toDouble();
    }

    double longitude(const QString& key = "longitude", double def = 108.9138) const {
        return getValue("radar." + key, def).toDouble();
    }

    double altitude(const QString& key = "altitude", double def = 400.0) const {
        return getValue("radar." + key, def).toDouble();
    }

    // 新增的配置访问方法
    int azimuthRange(const QString& key, int def = 0) const {
        return getValue("polarDisp.azimuthRange." + key, def).toInt();
    }

    int elevationRange(const QString& key, int def = 0) const {
        return getValue("polarDisp.elevationRange." + key, def).toInt();
    }

    int pointSize(const QString& key, int def = 1) const {
        return getValue("targetDisplay.pointSizes." + key, def).toInt();
    }

    QString targetLabel(const QString& key, const QString& def = "") const {
        return getValue("targetDisplay.labels." + key, def).toString();
    }

    int zValue(const QString& key, int def = 0) const {
        return getValue("targetDisplay.zValues." + key, def).toInt();
    }

    int fontSize(const QString& key, int def = 9) const {
        return getValue("ui.fonts." + key, def).toInt();
    }

    int windowProperty(const QString& key, int def = 0) const {
        return getValue("ui.window." + key, def).toInt();
    }

    double mapCenter(const QString& key, double def = 0.0) const {
        return getValue("mapDisplay.center_" + key, def).toDouble();
    }

    QString mapProperty(const QString& key, const QString& def = "") const {
        return getValue("mapDisplay." + key, def).toString();
    }

    int systemProperty(const QString& key, int def = 0) const {
        return getValue("system." + key, def).toInt();
    }

    QString systemString(const QString& key, const QString& def = "") const {
        return getValue("system." + key, def).toString();
    }

    unsigned int protocolCode(const QString& key, unsigned int def = 0) const {
        return getValue("network.protocol." + key, def).toUInt();
    }

private:
    QMap<QString, QVariant> configData;
    QJsonObject root; // 保持向后兼容
    
    // TOML文件加载
    bool loadToml(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "TOML config file not found:" << path << ", using defaults.";
            return false;
        }
        
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        
        return parseToml(content);
    }
    
    // JSON文件加载（向后兼容）
    bool loadJson(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "JSON config file not found:" << path << ", using defaults.";
            return false;
        }

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Invalid config.json format, using defaults.";
            return false;
        }

        root = doc.object();
        convertJsonToMap(root, "");
        return true;
    }
    
    // 简单的TOML解析器（基础实现）
    bool parseToml(const QString& content) {
        QStringList lines = content.split('\n');
        QString currentSection = "";
        
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            
            // 跳过注释和空行
            if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                continue;
            }
            
            // 处理节（section）
            if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
                currentSection = trimmed.mid(1, trimmed.length() - 2);
                continue;
            }
            
            // 处理键值对
            int equalPos = trimmed.indexOf('=');
            if (equalPos > 0) {
                QString key = trimmed.left(equalPos).trimmed();
                QString value = trimmed.mid(equalPos + 1).trimmed();
                
                // 移除引号
                if ((value.startsWith('"') && value.endsWith('"')) ||
                    (value.startsWith('\'') && value.endsWith('\''))) {
                    value = value.mid(1, value.length() - 2);
                }
                
                // 移除行内注释
                int commentPos = value.indexOf('#');
                if (commentPos >= 0) {
                    value = value.left(commentPos).trimmed();
                }
                
                // 构建完整的键路径
                QString fullKey = currentSection.isEmpty() ? key : currentSection + "." + key;
                
                // 尝试转换为适当的类型
                QVariant varValue;
                bool ok;
                int intVal = value.toInt(&ok);
                if (ok) {
                    varValue = intVal;
                } else {
                    double doubleVal = value.toDouble(&ok);
                    if (ok) {
                        varValue = doubleVal;
                    } else if (value.toLower() == "true") {
                        varValue = true;
                    } else if (value.toLower() == "false") {
                        varValue = false;
                    } else {
                        varValue = value;
                    }
                }
                
                configData[fullKey] = varValue;
            }
        }
        
        return true;
    }
    
    // 将JSON对象转换为扁平化的Map
    void convertJsonToMap(const QJsonObject& obj, const QString& prefix) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
            
            if (it.value().isObject()) {
                convertJsonToMap(it.value().toObject(), key);
            } else {
                configData[key] = it.value().toVariant();
            }
        }
    }
    
    // 统一的值获取方法
    QVariant getValue(const QString& key, const QVariant& defaultValue) const {
        return configData.value(key, defaultValue);
    }

    
    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

#endif // CONFIGMANAGER_H
