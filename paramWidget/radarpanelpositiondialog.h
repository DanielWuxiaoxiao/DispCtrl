#ifndef RADARPANELPOSITIONDIALOG_H
#define RADARPANELPOSITIONDIALOG_H
#pragma pack(push, 8)
#include <QDialog>
#include <functional>
#include <optional>
#include <utility>
#pragma pack(pop)
#include "Basic/radarpanelstate.h"
#pragma pack(push, 8)
class QLineEdit;
class QPushButton;
class QLabel;
class CusWindow;

class RadarPanelPositionDialog final : public QDialog {
public:
    struct Actions {
        std::function<bool(const RadarPanelValues&)> submit;
        std::function<bool(int, const RadarReference::Pose&)> lock;
        std::function<std::optional<RadarReference::Pose>(int)> unlock;
        std::function<RadarPanelState::Snapshot()> live;
    };
    RadarPanelPositionDialog(const RadarPanelValues& values, const std::array<bool, 4>& locked,
                             Actions actions, QWidget* parent = nullptr);
    void attachToWindow(CusWindow* host);
private:
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
    Actions m_actions;
    std::array<bool, 4> m_locked{};
};
#pragma pack(pop)
#endif
