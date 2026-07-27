#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainOverLayOut;

class FramelessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FramelessMainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    MainOverLayOut* m_overlayWidget = nullptr;
};

#endif // MAINWINDOW_H
