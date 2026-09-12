/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-05-18 15:26:15
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:53
 * @Description: 
 */
#ifndef RANGEHEIGHTCHART_H
#define RANGEHEIGHTCHART_H

#include "../cusWidgets/customlinechart.h"
#include "../Basic/Protocol.h"
#include "../Basic/DispBasci.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMap>

class QGraphicsSimpleTextItem;
class RangeHeightBatchItem;

class RangeHeightChartToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit RangeHeightChartToolBar(QWidget* parent = nullptr);

    double getMinHeight() const;
    double getMaxHeight() const;
    void setHeightRange(double minHeight, double maxHeight);
    void updateStatus(int detCount, int trackCount);

signals:
    void clearRequested();
    void resetRequested();
    void heightRangeChanged(double minHeight, double maxHeight);

private slots:
    void onHeightRangeChanged();

private:
    QLineEdit* m_minHeightEdit = nullptr;
    QLineEdit* m_maxHeightEdit = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};

class RangeHeightChart : public CustomLineChart
{
    Q_OBJECT

public:
    explicit RangeHeightChart(QWidget* parent = nullptr);
    ~RangeHeightChart() override;

    void addDetectionPoint(const PointInfo& info);
    void addTrackPoint(const trackInfo& info);
    void addPointInfo(const PointInfo& info);
    void removeBatch(int batchID);
    void clearRadarData();

    void setRangeFromMain(double minRange, double maxRange);
    void setHeightRange(double minHeight, double maxHeight);
    double minHeight() const { return m_minHeight; }
    double maxHeight() const { return m_maxHeight; }

    void setDetectionVisible(bool visible);
    void setTrackVisible(bool visible);
    void setTbdTrackVisible(bool visible);
    void setCooperativeTrackVisible(bool visible);
    void setOnlyRecognizedDroneTracksVisible(bool enabled);

    void setDetectionSizeRatio(double ratio);
    void setTrackSizeRatio(double ratio);
    void setMaxDetectionPoints(int maxPoints);
    void setMaxTrackPoints(int maxTracks);

signals:
    void pointCountChanged(int detCount, int trackCount);
    void heightRangeChanged(double minHeight, double maxHeight);

private:
    void resizeEvent(QResizeEvent* event) override;

    struct DetectionItem {
        PointInfo info;
        qint64 timestamp = 0;
    };

    struct TrackItem {
        PointInfo info;
        qint64 timestamp = 0;
    };

    QList<DetectionItem> m_detections;
    QVector<TrackItem> m_tracks;
    QMap<quint64, QGraphicsSimpleTextItem*> m_trackLabels;
    QMap<quint64, int> m_latestTrackIndices;
    QMap<quint64, qint64> m_trackLabelRefreshMs;
    QMap<quint64, int> m_trackCountsByKey;
    RangeHeightBatchItem* m_batchItem = nullptr;

    bool m_detectionVisible = true;
    bool m_trackVisible = true;
    bool m_tbdTrackVisible = true;
    bool m_cooperativeTrackVisible = true;
    bool m_onlyRecognizedDroneTracksVisible = false;

    double m_detectionSizeRatio = 1.0;
    double m_trackSizeRatio = 1.0;

    int m_maxDetectionPoints = 10000;
    int m_maxTrackPoints = 200;
    bool m_trackLabelsEnabled = true;
    int m_trackLabelRefreshIntervalMs = 200;

    double m_minHeight = 0.0;
    double m_maxHeight = 500.0;

    QColor m_detectionColor = QColor(0, 255, 0);
    QColor m_trackColor = TRA_COLOR;
    QColor m_otherTrackColor = DRONE_COLOR;
    QColor m_tbdTrackColor = QColor(TBD_COLOR);
    QColor m_cooperativeTrackColor = QColor(CO_TRACK_COLOR);

    double m_baseDetectionSize = 3.0;
    double m_baseTrackSize = 5.0;

    void limitDetectionPoints();
    void limitTrackPoints();
    void updatePointCount();
    void refreshAllPoints();
    void refreshTrackLabels();
    void clearTrackLabels();
    void rebuildLatestTrackIndices();
    const TrackItem* latestTrackItem(unsigned type, int batch) const;
    void limitTrackPointsForBatch(unsigned type, int batch);
    void removeTrackAt(int index);

    QColor trackColor(const PointInfo& info) const;
    QString trackTooltipLabel(unsigned type) const;
    QString trackLabelText(const PointInfo& info) const;
    bool isTrackTypeVisible(unsigned type) const;
    bool isTrackBatchRecognitionVisible(unsigned type, int batch) const;
    bool isTrackRecognitionVisible(const PointInfo& info) const;
    void updateTrackVisibility();
    void updateTrackVisibilityForBatch(unsigned type, int batch);
    bool shouldShowDetection(const PointInfo& info) const;
    bool shouldShowTrack(const PointInfo& info) const;
    bool isHeightInRange(double height) const;

    friend class RangeHeightBatchItem;
};

class RangeHeightChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RangeHeightChartWidget(QWidget* parent = nullptr);
    ~RangeHeightChartWidget() override;

    RangeHeightChart* chart() const { return m_chart; }

public slots:
    void onClearRequested();
    void onResetRequested();

private:
    RangeHeightChartToolBar* m_toolbar = nullptr;
    RangeHeightChart* m_chart = nullptr;
};

#endif // RANGEHEIGHTCHART_H
