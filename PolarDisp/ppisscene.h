#ifndef PPISCENE_H
#define PPISCENE_H

#include <QGraphicsScene>

class EchoRenderer;
class PolarAxis;
class PolarGrid;

class PPIScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit PPIScene(QObject* parent = nullptr);
    ~PPIScene() override = default;

    PolarAxis* axis() const { return m_axis; }
    PolarGrid* grid() const { return m_grid; }
    EchoRenderer* echoRenderer() const { return m_echo; }

public slots:
    void setRange(float minRangeMeters, float maxRangeMeters);
    void updateSceneSize(const QSize& newSize);

signals:
    void rangeChanged(float minRangeMeters, float maxRangeMeters);

private:
    void updateAxisScaleFromScene();

    PolarAxis* m_axis = nullptr;
    PolarGrid* m_grid = nullptr;
    EchoRenderer* m_echo = nullptr;
    int m_viewMargin = 30;
};

#endif // PPISCENE_H
