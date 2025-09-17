// sectorwidget.h - 使用扇形场景的Widget示例
#ifndef SECTORWIDGET_H
#define SECTORWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>

class SectorScene;

class SectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SectorWidget(QWidget* parent = nullptr);

private slots:
    void updateSectorRange();
    void addRandomDetPoint();
    void addRandomTrackPoint();
    void clearAllPoints();
    void onSceneRangeChanged(float minRange, float maxRange);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void setupControls();

private:
    QGraphicsView* m_view;
    SectorScene* m_scene;
    
    // 控制面板
    QSpinBox* m_minAngleSpinBox;
    QSpinBox* m_maxAngleSpinBox;
    QDoubleSpinBox* m_minRangeSpinBox;
    QDoubleSpinBox* m_maxRangeSpinBox;
    QPushButton* m_updateButton;
    QPushButton* m_addDetButton;
    QPushButton* m_addTrackButton;
    QPushButton* m_clearButton;
    QLabel* m_statusLabel;
    
    // 用于生成随机航迹的计数器
    int m_trackCounter = 1;
    QTimer* m_autoAddTimer;
};

#endif // SECTORWIDGET_H