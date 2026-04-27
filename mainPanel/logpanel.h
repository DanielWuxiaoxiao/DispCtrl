/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:16:47
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 11:21:02
 * @Description: 
 */
/**
 * @file logpanel.h
 * @brief 操作日志时间轴面板
 * @details 以时间轴形式记录并展示所有雷达参数下发、系统告警、用户操作，
 *          支持按类型过滤和导出为文本文件。
 *          风格：SIMRAD 暗色主题，与整体界面保持一致。
 */
#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <QScrollBar>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>

/**
 * @class LogPanel
 * @brief 操作日志时间轴面板
 *
 * 用法：
 * @code
 * // 在 MainOverLayOut 初始化
 * m_logPanel = new LogPanel(this);
 *
 * // 外部追加日志
 * m_logPanel->addLog(LogPanel::ParamSend, "下发增益: 75");
 * m_logPanel->addLog(LogPanel::Alert,    "电子围栏: 目标#021进入A区");
 * @endcode
 */
class LogPanel : public QWidget
{
    Q_OBJECT
public:
    /// 日志条目类型
    enum LogType {
        Info     = 0,   ///< 普通信息（蓝色）
        ParamSend = 1,  ///< 参数下发（绿色）
        Alert    = 2,   ///< 告警事件（橙色）
        Error    = 3,   ///< 错误（红色）
        System   = 4,   ///< 系统状态变更（白色）
    };
    Q_ENUM(LogType)

    explicit LogPanel(QWidget* parent = nullptr);
    ~LogPanel() override = default;

    /**
     * @brief     追加一条日志
     * @param type 类型（Info/ParamSend/Alert/Error/System）
     * @param text 日志正文
     */
    void addLog(LogType type, const QString& text);

    /** @brief 清空所有日志 */
    void clearLogs();

    /** @brief 设置最大保留条数（默认5000） */
    void setMaxEntries(int n);

public slots:
    /** @brief 连接到 Controller 的参数发送信号，自动记录 */
    void onParamSent(const QString& paramDesc);

    /** @brief 连接到 GeoFenceManager 的告警信号 */
    void onFenceAlert(const QString& fenceAlertDesc);

    /** @brief 系统健康状态变化 */
    void onHealthChanged(const QString& desc);

private slots:
    void onExportClicked();
    void onFilterChanged(int index);

private:
    void applyStyle();
    void rebuildDisplay();  // 按过滤器重新填充列表

    QListWidget*  m_list        = nullptr;
    QComboBox*    m_filterCombo = nullptr;
    QPushButton*  m_exportBtn   = nullptr;
    QLabel*       m_countLabel  = nullptr;

    struct LogEntry {
        QDateTime  time;
        LogType    type;
        QString    text;
    };

    QVector<LogEntry> m_entries;
    LogType           m_currentFilter = static_cast<LogType>(-1); // -1 = 全部
    int               m_maxEntries    = 5000;

    static QString typeLabel(LogType t);
    static QColor  typeColor(LogType t);
};

#endif // LOGPANEL_H
