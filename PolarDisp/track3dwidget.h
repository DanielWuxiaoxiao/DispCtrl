#ifndef TRACK3DWIDGET_H
#define TRACK3DWIDGET_H

#include "../Basic/Protocol.h"

#include <QColor>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>
#include <QWidget>

class QHideEvent;
class QLabel;
class QPushButton;
class QShowEvent;
class QTimer;
class QWebChannel;
class QWebEngineView;

class Track3DBridge : public QObject
{
    Q_OBJECT

public:
    explicit Track3DBridge(QObject* parent = nullptr);

public slots:
    void pageReady();
    void reportError(const QString& message);

signals:
    void pageReadyReported();
    void errorReported(const QString& message);

    void snapshotChanged(const QVariantList& tracks, const QVariantMap& state);
    void trackDeltaChanged(const QVariantList& upserts, const QStringList& removals);
    void sceneStateChanged(const QVariantMap& state);
    void clearRequested();
    void resetViewRequested();
    void activeChanged(bool active);
};

class Track3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Track3DWidget(QWidget* parent = nullptr);
    ~Track3DWidget() override;

    int trackCount() const { return m_latestTracks.size(); }

public slots:
    void addPointInfo(const PointInfo& info);
    void clearRadarData();
    void setRangeFromMain(double minRange, double maxRange);
    void setHeightRange(double minHeight, double maxHeight);
    void setTrackVisible(bool visible);
    void setTbdTrackVisible(bool visible);
    void setCooperativeTrackVisible(bool visible);
    void setOnlyRecognizedDroneTracksVisible(bool enabled);
    void setTrackSizeRatio(double ratio);
    void setDisplayActive(bool active);
    void resetView();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void flushPendingUpdates();
    void onPageReady();
    void onPageError(const QString& message);
    void onLoadFinished(bool ok);

private:
    static quint64 makeTrackKey(unsigned type, unsigned batch);
    static QString makeTrackKeyString(unsigned type, unsigned batch);

    bool isTrackPoint(const PointInfo& info) const;
    bool isValidPoint(const PointInfo& info) const;
    QVariantMap pointPayload(const PointInfo& info) const;
    QVariantMap sceneState() const;
    QColor trackColor(const PointInfo& info) const;

    void scheduleFlush();
    void sendSnapshot();
    void sendSceneState();
    void setRendererActive(bool active);
    void updateTrackCountLabel();
    void logInvalidPoint(const PointInfo& info);
    void handleRendererFailure(const QString& reason);
    void showRendererFailurePage(const QString& reason);

    QLabel* m_trackCountLabel = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QWebEngineView* m_webView = nullptr;
    QWebChannel* m_webChannel = nullptr;
    Track3DBridge* m_bridge = nullptr;
    QTimer* m_flushTimer = nullptr;

    QHash<quint64, PointInfo> m_latestTracks;
    QHash<quint64, QVariantMap> m_pendingUpserts;
    QSet<quint64> m_pendingRemovals;

    double m_minRangeM = 0.0;
    double m_maxRangeM = 5000.0;
    double m_minHeightM = 0.0;
    double m_maxHeightM = 500.0;
    double m_trackSizeRatio = 1.0;

    bool m_trackVisible = true;
    bool m_tbdTrackVisible = true;
    bool m_cooperativeTrackVisible = true;
    bool m_onlyRecognizedDroneTracksVisible = false;
    bool m_pageReady = false;
    bool m_rendererActive = false;
    bool m_snapshotRequired = true;
    bool m_reloadAttempted = false;
    bool m_showingFailurePage = false;
    qint64 m_lastInvalidPointLogMs = 0;
};

#endif // TRACK3DWIDGET_H
