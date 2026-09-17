/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-11 22:04:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-17 22:45:15
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
class QDoubleSpinBox;
class QPushButton;
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
    void applyNetworkStatus(QPushButton* button, int index);
    void applyLoginStatus();
    void applyAutoReportRange();
    void updateAutoReportRange();

    CommandControlModule* m_module = nullptr;
    QWidget* m_titleBar = nullptr;
    QPoint m_titleBarDragOffset;
    QLabel* m_statusLabel = nullptr;
    QCheckBox* m_autoReportCheckBox = nullptr;
    QDoubleSpinBox* m_autoReportHeightMaxSpin = nullptr;
    QDoubleSpinBox* m_autoReportRangeMinSpin = nullptr;
    QDoubleSpinBox* m_autoReportRangeMaxSpin = nullptr;
    QDoubleSpinBox* m_autoReportAzimuthStartSpin = nullptr;
    QDoubleSpinBox* m_autoReportAzimuthEndSpin = nullptr;
    QPushButton* m_applyAutoReportRangeButton = nullptr;
    QPushButton* m_loginStatusButton = nullptr;
    QPushButton* m_localNetworkButton = nullptr;
    QPushButton* m_controlNetworkButton = nullptr;
    QPushButton* m_ntpNetworkButton = nullptr;
    QPushButton* m_manualTimeSyncButton = nullptr;
    QPushButton* m_pauseReplayButton = nullptr;
    QPushButton* m_rewindReplayButton = nullptr;
    QPushButton* m_fastForwardReplayButton = nullptr;
    QPushButton* m_stopReplayButton = nullptr;
    QTableWidget* m_peerTable = nullptr;
    QTableWidget* m_recordTable = nullptr;
};

#endif  // COMMANDCONTROL_WINDOW_H
