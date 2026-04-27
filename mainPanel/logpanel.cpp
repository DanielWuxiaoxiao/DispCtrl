/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:16:47
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 11:21:02
 * @Description: 
 */
/**
 * @file logpanel.cpp
 * @brief 操作日志时间轴面板实现
 */
#include "logpanel.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

LogPanel::LogPanel(QWidget* parent)
    : QWidget(parent)
{
    // --- 顶部工具栏 ---
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(QStringLiteral("全部类型"), -1);
    m_filterCombo->addItem(QStringLiteral("普通信息"), Info);
    m_filterCombo->addItem(QStringLiteral("参数下发"), ParamSend);
    m_filterCombo->addItem(QStringLiteral("告警事件"), Alert);
    m_filterCombo->addItem(QStringLiteral("错误"),     Error);
    m_filterCombo->addItem(QStringLiteral("系统状态"), System);

    m_exportBtn = new QPushButton(QStringLiteral("导出..."), this);
    m_countLabel= new QLabel(QStringLiteral("0 条"), this);

    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(4, 2, 4, 2);
    toolbar->setSpacing(6);
    auto* filterLbl = new QLabel(QStringLiteral("过滤:"), this);
    filterLbl->setStyleSheet("color:#aaa; font-size:12px;");
    toolbar->addWidget(filterLbl);
    toolbar->addWidget(m_filterCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_countLabel);
    toolbar->addWidget(m_exportBtn);

    // --- 日志列表 ---
    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setWordWrap(false);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // --- 整体布局 ---
    auto* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(4, 4, 4, 4);
    vlay->setSpacing(4);
    vlay->addLayout(toolbar);
    vlay->addWidget(m_list);
    setLayout(vlay);

    applyStyle();

    connect(m_exportBtn,   &QPushButton::clicked,
            this, &LogPanel::onExportClicked);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogPanel::onFilterChanged);
}

void LogPanel::addLog(LogType type, const QString& text)
{
    // 超限时移除最旧条目
    if (m_entries.size() >= m_maxEntries) {
        m_entries.removeFirst();
    }

    LogEntry entry{QDateTime::currentDateTime(), type, text};
    m_entries.append(entry);

    // 若当前过滤器允许该类型，则直接追加到列表（比整体重建快）
    int filterVal = m_filterCombo->currentData().toInt();
    if (filterVal == -1 || filterVal == static_cast<int>(type)) {
        QString ts  = entry.time.toString(QStringLiteral("HH:mm:ss.zzz"));
        QString lbl = typeLabel(type);
        QString line = QStringLiteral("[%1] [%2] %3").arg(ts, lbl, text);

        auto* item = new QListWidgetItem(line);
        item->setForeground(typeColor(type));
        item->setFont(QFont(QStringLiteral("Consolas"), 10));
        m_list->addItem(item);
        m_list->scrollToBottom();
    }

    m_countLabel->setText(QString::number(m_entries.size()) + QStringLiteral(" 条"));
}

void LogPanel::clearLogs()
{
    m_entries.clear();
    m_list->clear();
    m_countLabel->setText(QStringLiteral("0 条"));
}

void LogPanel::setMaxEntries(int n)
{
    m_maxEntries = qMax(100, n);
}

void LogPanel::onParamSent(const QString& paramDesc)
{
    addLog(ParamSend, paramDesc);
}

void LogPanel::onFenceAlert(const QString& fenceAlertDesc)
{
    addLog(Alert, fenceAlertDesc);
}

void LogPanel::onHealthChanged(const QString& desc)
{
    addLog(System, desc);
}

void LogPanel::onExportClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出操作日志"),
        QStringLiteral("log_%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))),
        QStringLiteral("文本文件 (*.txt);;全部文件 (*.*)")
    );
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             QStringLiteral("无法写入文件: ") + path);
        return;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QStringLiteral("# DispCtrl 操作日志导出  %1\n")
           .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    out << QStringLiteral("# 共 %1 条记录\n\n").arg(m_entries.size());
    for (const auto& e : m_entries) {
        out << QStringLiteral("[%1] [%2] %3\n")
               .arg(e.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                    typeLabel(e.type),
                    e.text);
    }
    file.close();
    addLog(System, QStringLiteral("日志已导出: ") + path);
}

void LogPanel::onFilterChanged(int /*index*/)
{
    rebuildDisplay();
}

void LogPanel::rebuildDisplay()
{
    m_list->clear();
    int filterVal = m_filterCombo->currentData().toInt();
    for (const auto& e : m_entries) {
        if (filterVal != -1 && filterVal != static_cast<int>(e.type))
            continue;
        QString ts  = e.time.toString(QStringLiteral("HH:mm:ss.zzz"));
        QString lbl = typeLabel(e.type);
        QString line = QStringLiteral("[%1] [%2] %3").arg(ts, lbl, e.text);
        auto* item = new QListWidgetItem(line);
        item->setForeground(typeColor(e.type));
        item->setFont(QFont(QStringLiteral("Consolas"), 10));
        m_list->addItem(item);
    }
    m_list->scrollToBottom();
}

void LogPanel::applyStyle()
{
    setStyleSheet(
        "LogPanel { background: #0a0a0a; }"
        "QListWidget { background: #0d0d0d; border: 1px solid #333; color: #ccc; }"
        "QListWidget::item:selected { background: #1a2a3a; }"
        "QComboBox { background: #1a1a1a; color: #ccc; border: 1px solid #444; "
        "           padding: 2px 6px; min-width: 90px; }"
        "QComboBox QAbstractItemView { background: #1a1a1a; color: #ccc; "
        "                              selection-background-color: #2a3a4a; }"
        "QPushButton { background: #1c2c1c; color: #44ff44; border: 1px solid #44ff44; "
        "              padding: 2px 10px; border-radius: 3px; font-size:12px; }"
        "QPushButton:hover { background: #2c3c2c; }"
        "QLabel { color: #ccc; font-size: 12px; }"
    );
}

QString LogPanel::typeLabel(LogType t)
{
    switch (t) {
    case Info:      return QStringLiteral("INFO ");
    case ParamSend: return QStringLiteral("PARAM");
    case Alert:     return QStringLiteral("ALERT");
    case Error:     return QStringLiteral("ERROR");
    case System:    return QStringLiteral("SYS  ");
    }
    return QStringLiteral("?????");
}

QColor LogPanel::typeColor(LogType t)
{
    switch (t) {
    case Info:      return QColor(0x66, 0xaa, 0xff);
    case ParamSend: return QColor(0x44, 0xff, 0x44);
    case Alert:     return QColor(0xff, 0x88, 0x00);
    case Error:     return QColor(0xff, 0x44, 0x44);
    case System:    return QColor(0xcc, 0xcc, 0xcc);
    }
    return QColor(Qt::white);
}
