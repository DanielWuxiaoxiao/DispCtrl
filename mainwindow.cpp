#include "mainwindow.h"

#include <QKeyEvent>

#include "Basic/DispBasci.h"
#include "Controller/controller.h"
#include "mainPanel/mainoverlayout.h"

FramelessMainWindow::FramelessMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(APP_NAME);
    setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    setCentralWidget(m_overlayWidget = new MainOverLayOut(this));
    showFullScreen();

    connect(CON_INS, &Controller::minimizeWindow,
            this, &QMainWindow::showMinimized);
}

void FramelessMainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        isFullScreen() ? showMaximized() : showFullScreen();
        return;
    }
    QMainWindow::keyPressEvent(event);
}
