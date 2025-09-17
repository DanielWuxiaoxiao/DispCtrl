// sectorscene.h
#ifndef SECTORSCENE_H
#define SECTORSCENE_H

#include <QGraphicsScene>
#include <QObject>

class PolarAxis;
class SectorPolarGrid;
class SectorDetManager;
class SectorTrackManager;

class SectorScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit SectorScene(QObject* parent = nullptr);
    virtual ~SectorScene();

    // 设置扇形显示范围
    void setSectorRange(float minAngle, float maxAngle, float minRange, float maxRange);
    
    // 获取当前扇形参数
    float minAngle() const { return m_minAngle; }
    float maxAngle() const { return m_maxAngle; }
    float minRange() const { return m_minRange; }
    float maxRange() const { return m_maxRange; }
    
    // 更新场景大小
    void updateSceneSize(const QSize& newSize);
    
    // 获取管理器
    SectorDetManager* detManager() const { return m_det; }
    SectorTrackManager* trackManager() const { return m_track; }
    PolarAxis* axis() const { return m_axis; }

signals:
    void sectorChanged(float minAngle, float maxAngle, float minRange, float maxRange);
    void rangeChanged(float minRange, float maxRange);

private slots:
    void updateSectorDisplay();

private:
    void initLayerObjects();

private:
    PolarAxis* m_axis;
    SectorPolarGrid* m_grid;
    SectorDetManager* m_det;
    SectorTrackManager* m_track;
    
    float m_minAngle = -30.0f;
    float m_maxAngle = 30.0f;
    float m_minRange = 0.0f;
    float m_maxRange = 500.0f;
};

#endif // SECTORSCENE_H