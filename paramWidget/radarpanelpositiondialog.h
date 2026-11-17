#ifndef RADARPANELPOSITIONDIALOG_H
#define RADARPANELPOSITIONDIALOG_H
#pragma pack(push, 8)
#include <QDialog>
#include <functional>
#include <optional>
#include <utility>
#pragma pack(pop)
#include "Basic/radarpanelstate.h"
#include "Basic/systemtimesync.h"
#pragma pack(push, 8)
class QLineEdit;
class QPushButton;
class QLabel;
class QComboBox;
class CusWindow;
class MechanicalGps;

class RadarPanelPositionDialog final : public QDialog {
public:
    struct Actions {
        std::function<bool(const RadarPanelValues&)> submit;
        std::function<bool(int, const RadarReference::Pose&)> lock;
        std::function<std::optional<RadarReference::Pose>(int)> unlock;
        std::function<RadarPanelState::Snapshot()> live;
        std::function<void(int)> requestTimeSync;
    };
    struct SinglePanel {
        int fieldCount = RadarReference::Count;
        bool manualGps = false;
        const MechanicalGps* gps = nullptr;
        std::function<QString(bool manual)> setManualGps;
        std::function<QString(const RadarReference::Pose&, bool saveLocal)> apply;
        std::function<QString()> restoreAutomatic;
        std::function<bool()> submitGps;
        std::function<std::optional<qint32>()> submitServoNorth;
    };
    RadarPanelPositionDialog(const RadarPanelValues& values, const std::array<bool, 4>& locked,
                             Actions actions, QWidget* parent = nullptr);
    RadarPanelPositionDialog(const RadarReference::Pose& values, SinglePanel single,
                             std::function<void(int)> requestTimeSync, QWidget* parent = nullptr);
    void attachToWindow(CusWindow* host);
    void setSystemTimeSyncState(SystemTimeSyncState state);
    void refreshMechanicalGps();
protected:
    void keyPressEvent(QKeyEvent* event) override;
private:
    RadarPanelPositionDialog(const RadarPanelValues& values, const std::array<bool, 4>& locked,
                             Actions actions, std::optional<SinglePanel> single, QWidget* parent);
    void applySinglePanel(bool saveLocal);
    void updateSinglePanelMode();
    void resizeSinglePanelWindow();
    int preferredContentHeight() const;
    void showFeedback(const QString& message, bool succeeded = false);
    bool readRow(int id, RadarReference::Pose& values);
    void fillRow(int id, const RadarReference::Pose& values);
    void setLocked(int id, bool locked);
    void submit();
    void lockRow(int id);
    void unlockRow(int id);
    std::array<std::array<QLineEdit*, RadarReference::Count>, 4> m_edits{};
    std::array<QPushButton*, 4> m_lock{}, m_unlock{};
    std::array<QLabel*, 4> m_status{};
    QLabel* m_error = nullptr;
    QPushButton* m_timeSyncButton = nullptr;
    QComboBox* m_timeSyncRadar = nullptr;
    QLabel* m_timeSyncStatus = nullptr;
    QLabel* m_received = nullptr;
    QWidget* m_deviationPlot = nullptr;
    QLabel* m_modeNote = nullptr;
    QPushButton* m_applyButton = nullptr;
    QPushButton* m_saveLocalButton = nullptr;
    QPushButton* m_restoreAutomaticButton = nullptr;
    QPushButton* m_submitGpsButton = nullptr;
    Actions m_actions;
    std::optional<SinglePanel> m_single;
    std::array<bool, 4> m_locked{};
};
#pragma pack(pop)
#endif
