// 建议的架构改进方案3：分层数据流模式
// DataFlowManager.h

#ifndef DATAFLOWMANAGER_H
#define DATAFLOWMANAGER_H

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QTimer>
#include "Basic/Protocol.h"

// 数据过滤器接口
class IDataFilter {
public:
    virtual ~IDataFilter() = default;
    virtual bool shouldPass(const PointInfo& info) const = 0;
    virtual PointInfo transform(const PointInfo& info) const { return info; }
};

// 范围过滤器
class RangeFilter : public IDataFilter {
public:
    RangeFilter(float minRange, float maxRange, float minAngle, float maxAngle)
        : m_minRange(minRange), m_maxRange(maxRange), 
          m_minAngle(minAngle), m_maxAngle(maxAngle) {}
    
    bool shouldPass(const PointInfo& info) const override;
    
private:
    float m_minRange, m_maxRange, m_minAngle, m_maxAngle;
};

// 数据流节点
class DataFlowNode : public QObject {
    Q_OBJECT
    
public:
    DataFlowNode(const QString& name, QObject* parent = nullptr);
    
    // 添加过滤器
    void addFilter(std::unique_ptr<IDataFilter> filter);
    
    // 连接到下游节点
    void connectTo(DataFlowNode* downstream);
    
    // 数据处理
    virtual void processData(const PointInfo& info);
    
signals:
    void dataProcessed(const PointInfo& info);
    
protected:
    QString m_name;
    QList<std::unique_ptr<IDataFilter>> m_filters;
    QList<DataFlowNode*> m_downstream;
    
    virtual bool filterData(const PointInfo& info) const;
    virtual PointInfo transformData(const PointInfo& info) const;
};

// 显示节点
class DisplayNode : public DataFlowNode {
    Q_OBJECT
    
public:
    DisplayNode(const QString& name, QGraphicsScene* scene, 
                PolarAxis* axis, QObject* parent = nullptr);
    
    void processData(const PointInfo& info) override;
    
private:
    QGraphicsScene* m_scene;
    PolarAxis* m_axis;
    // 这里可以包含实际的显示逻辑
};

// 数据流管理器
class DataFlowManager : public QObject {
    Q_OBJECT
    
public:
    DataFlowManager(QObject* parent = nullptr);
    
    // 创建节点
    DataFlowNode* createNode(const QString& name);
    DisplayNode* createDisplayNode(const QString& name, QGraphicsScene* scene, PolarAxis* axis);
    
    // 数据入口
    void injectDetection(const PointInfo& info);
    void injectTrack(const PointInfo& info);
    
private:
    QMap<QString, std::unique_ptr<DataFlowNode>> m_nodes;
    DataFlowNode* m_rootNode;
};

#endif // DATAFLOWMANAGER_H