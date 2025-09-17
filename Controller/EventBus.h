// 建议的架构改进方案2：事件总线模式
// EventBus.h

#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <QObject>
#include <QMap>
#include <QList>
#include <functional>
#include "Basic/Protocol.h"

// 事件类型
enum class EventType {
    DetectionReceived,
    TrackReceived,
    DataCleared,
    RangeChanged,
    AngleChanged
};

// 事件数据
struct EventData {
    EventType type;
    QVariant data;
    QString source;
    qint64 timestamp;
};

// 事件处理器接口
using EventHandler = std::function<void(const EventData&)>;

// 事件总线
class EventBus : public QObject
{
    Q_OBJECT
    
public:
    static EventBus& instance() {
        static EventBus instance;
        return instance;
    }
    
    // 订阅事件
    void subscribe(EventType type, const QString& subscriberId, EventHandler handler);
    void unsubscribe(EventType type, const QString& subscriberId);
    
    // 发布事件
    void publish(EventType type, const QVariant& data, const QString& source = "");
    
    // 便捷方法
    void publishDetection(const PointInfo& info, const QString& source = "UDP");
    void publishTrack(const PointInfo& info, const QString& source = "UDP");
    void publishRangeChange(float minRange, float maxRange, const QString& source = "UI");
    
private:
    EventBus() = default;
    QMap<EventType, QMap<QString, EventHandler>> m_subscribers;
};

// 使用宏简化订阅
#define SUBSCRIBE_EVENT(type, id, handler) \
    EventBus::instance().subscribe(type, id, handler)

#define PUBLISH_EVENT(type, data, source) \
    EventBus::instance().publish(type, data, source)

#endif // EVENTBUS_H