// ServiceContainer.h - 服务容器和依赖注入
#ifndef SERVICECONTAINER_H
#define SERVICECONTAINER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <functional>
#include <memory>

class IService {
public:
    virtual ~IService() = default;
};

class ServiceContainer : public QObject {
    Q_OBJECT
    
public:
    static ServiceContainer& instance();
    
    // 注册服务
    template<typename T>
    void registerService(const QString& name, std::function<T*()> factory) {
        m_factories[name] = [factory]() -> QObject* {
            return factory();
        };
    }
    
    // 注册单例服务
    template<typename T>
    void registerSingleton(const QString& name, T* instance) {
        m_singletons[name] = instance;
    }
    
    // 获取服务
    template<typename T>
    T* getService(const QString& name) {
        // 先检查单例
        if (m_singletons.contains(name)) {
            return qobject_cast<T*>(m_singletons[name]);
        }
        
        // 再检查工厂
        if (m_factories.contains(name)) {
            auto obj = m_factories[name]();
            return qobject_cast<T*>(obj);
        }
        
        return nullptr;
    }
    
    // 检查服务是否已注册
    bool hasService(const QString& name) const {
        return m_singletons.contains(name) || m_factories.contains(name);
    }
    
    // 清理
    void clear();
    
private:
    ServiceContainer(QObject* parent = nullptr);
    
    QMap<QString, QObject*> m_singletons;
    QMap<QString, std::function<QObject*()>> m_factories;
};

// 便捷宏
#define SERVICE_CONTAINER ServiceContainer::instance()

// 服务注册宏
#define REGISTER_SERVICE(Type, Name) \
    SERVICE_CONTAINER.registerSingleton<Type>(Name, this)

#define GET_SERVICE(Type, Name) \
    SERVICE_CONTAINER.getService<Type>(Name)

#endif // SERVICECONTAINER_H