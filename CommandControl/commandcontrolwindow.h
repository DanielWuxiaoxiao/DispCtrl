/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:22:04
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:55
 * @Description: 
 */
/* 指控通信独立控制窗口。 */
#ifndef COMMANDCONTROL_WINDOW_H
#define COMMANDCONTROL_WINDOW_H

#include <QDialog>
#include <QPoint>

class CommandControlModule;
class QLabel;
class QCheckBox;
class QTableWidget;
class QEvent;

class CommandControlWindow final : public QDialog
{
    Q_OBJECT
public:
    explicit CommandControlWindow(CommandControlModule* module);

private slots:
    void refreshStatus();
    void refreshPeers();
    void refreshRecords();
    void selectReplayFile();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    CommandControlModule* m_module = nullptr;
    QWidget* m_titleBar = nullptr;
    QPoint m_titleBarDragOffset;
    QLabel* m_statusLabel = nullptr;
    QCheckBox* m_autoReportCheckBox = nullptr;
    QTableWidget* m_peerTable = nullptr;
    QTableWidget* m_recordTable = nullptr;
};

#endif  // COMMANDCONTROL_WINDOW_H
