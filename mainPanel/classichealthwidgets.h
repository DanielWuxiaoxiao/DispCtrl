#ifndef CLASSICHEALTHWIDGETS_H
#define CLASSICHEALTHWIDGETS_H

#include <array>
class QPushButton;
class QLabel;

// x576/f338142 的健康窗口控件。状态数据继续由 MainOverLayOut 的五秒快照提供。
struct ClassicHealthWidgets
{
    QPushButton* m_sigProBtn = nullptr;
    QPushButton* m_dataProBtn = nullptr;
    QPushButton* m_beamConBtn = nullptr;
    QPushButton* m_targetRecBtn = nullptr;
    QPushButton* m_radarLocalNetworkBtn = nullptr;
    QPushButton* m_radarPeerNetworkBtn = nullptr;
    QPushButton* m_laserLocalNetworkBtn = nullptr;
    QPushButton* m_laserPeerNetworkBtn = nullptr;
    QPushButton* m_opticalLocalNetworkBtn = nullptr;
    QPushButton* m_opticalPeerNetworkBtn = nullptr;
    QPushButton* m_zhoushanLocalNetworkBtn = nullptr;
    QPushButton* m_zhoushanPeerNetworkBtn = nullptr;
    QPushButton* m_btnTxOpen = nullptr;
    QPushButton* m_btnDutyCycle = nullptr;
    QPushButton* m_btnPulseWidth = nullptr;
    QPushButton* m_btnRxOpen = nullptr;
    QPushButton* m_btnFreqSrc = nullptr;
    QPushButton* m_btnDigBoard = nullptr;
    QPushButton* m_btnServo = nullptr;
    QPushButton* m_btnBeidou = nullptr;
    QPushButton* m_btnBluetooth = nullptr;
    QPushButton* m_btnPowerBoard = nullptr;
    QLabel* m_tempLabel = nullptr;

    struct Panel
    {
        QPushButton* txOpen = nullptr;
        QPushButton* dutyCycle = nullptr;
        QPushButton* pulseWidth = nullptr;
        QPushButton* rxOpen = nullptr;
        QPushButton* freqSrc = nullptr;
        QPushButton* digBoard = nullptr;
        QPushButton* servo = nullptr;
        QPushButton* beidou = nullptr;
        QPushButton* bluetooth = nullptr;
        QPushButton* powerBoard = nullptr;
        std::array<QPushButton*, 36> subArrayPowerCells{};
        QLabel* subArrayPowerSummaryLabel = nullptr;
        QLabel* tempLabel = nullptr;
        QLabel* lastUpdateLabel = nullptr;
    };
    std::array<Panel, 4> m_healthPanelWidgets{};
};
#endif
