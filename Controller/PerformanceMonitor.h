// PerformanceMonitor.h - 性能监控系统
#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <QQueue>
#include <QMutex>

struct PerformanceMetric {
    QString name;
    double value;
    QString unit;
    QDateTime timestamp;
};

struct PerformanceStats {
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    double avg = 0.0;
    double current = 0.0;
    int count = 0;
};

class PerformanceMonitor : public QObject {
    Q_OBJECT
    
public:
    static PerformanceMonitor& instance();
    
    // 记录指标
    void recordMetric(const QString& name, double value, const QString& unit = "ms");
    
    // 计时器操作
    void startTimer(const QString& operationName);
    void endTimer(const QString& operationName);
    
    // 内存监控
    void recordMemoryUsage();
    
    // 数据处理监控
    void recordDataProcessingRate(int pointsPerSecond);
    void recordFrameRate(double fps);
    
    // 获取统计信息
    PerformanceStats getStats(const QString& metricName) const;
    QMap<QString, PerformanceStats> getAllStats() const;
    
    // 性能报告
    QString generateReport() const;
    
    // 配置
    void setReportInterval(int seconds);
    void setMaxHistorySize(int size);
    
signals:
    void performanceReport(const QString& report);
    void performanceAlarm(const QString& metricName, double value, double threshold);
    
private slots:
    void generatePeriodicReport();
    void checkPerformanceThresholds();
    
private:
    PerformanceMonitor(QObject* parent = nullptr);
    
    mutable QMutex m_mutex;
    QMap<QString, QQueue<PerformanceMetric>> m_metrics;
    QMap<QString, QElapsedTimer> m_activeTimers;
    QMap<QString, double> m_thresholds; // 性能阈值
    
    QTimer* m_reportTimer;
    int m_maxHistorySize;
    
    void updateStats(const QString& name, double value);
    void checkThreshold(const QString& name, double value);
};

// RAII计时器类
class ScopedTimer {
public:
    ScopedTimer(const QString& operationName) : m_name(operationName) {
        PerformanceMonitor::instance().startTimer(m_name);
    }
    
    ~ScopedTimer() {
        PerformanceMonitor::instance().endTimer(m_name);
    }
    
private:
    QString m_name;
};

// 便捷宏
#define PERF_MONITOR PerformanceMonitor::instance()
#define PERF_TIMER(name) ScopedTimer _timer(name)
#define PERF_RECORD(name, value, unit) PERF_MONITOR.recordMetric(name, value, unit)

#endif // PERFORMANCEMONITOR_H