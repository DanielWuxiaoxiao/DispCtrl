#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QRubberBand>

class CusWindow : public QWidget {
    Q_OBJECT
public:
    explicit CusWindow(const QString& title, const QIcon& icon, QWidget* parent = nullptr);

    void setContentWidget(QWidget* w);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void leaveEvent(QEvent* e) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    enum ResizeRegion {
        None, Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };
    ResizeRegion hitTest(const QPoint& pos) const;
    void updateCursor(const QPoint& pos);

    QPoint m_dragPos;
    bool m_dragging = false;
    bool m_resizing = false;
    ResizeRegion m_resizeRegion = None;
    QRect m_resizeStartRect;
    QPoint m_resizeStartPos;

    QWidget* m_content = nullptr;
    QLabel* m_titleLabel;
    QPushButton* m_closeBtn;
    QPushButton* m_minBtn;
    QPushButton* m_maxBtn;
    bool m_maximized = false;
    QRect m_restoreRect;

    QRubberBand* m_rubberBand = nullptr;
    QRect m_rubberBandRect;

signals:
    void windowClosed();
};
