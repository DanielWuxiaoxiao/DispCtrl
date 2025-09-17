#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>

class MapProxyWidget;
class MainOverLayOut;

class FramelessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FramelessMainWindow(QWidget *parent = nullptr);

protected:
//    void keyPressEvent(QKeyEvent *event) override {
//        if (event->key() == Qt::Key_Escape) {
//            if (isFullScreen()) {
//                showMaximized();
//            } else {
//                showFullScreen();
//            }
//        }
//        else {
//            QMainWindow::keyPressEvent(event);
//        }
//    }
private:
    void setupUI();
    void setupCentralView();
    void setupOverlayUI();

    MainOverLayOut *m_overlayWidget;      //上层控件
    MapProxyWidget* m_map;         //地图
};

#endif // MAINWINDOW_H
