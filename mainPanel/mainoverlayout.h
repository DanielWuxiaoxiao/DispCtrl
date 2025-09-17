#ifndef MAINOVERLAYOUT_H
#define MAINOVERLAYOUT_H

#include <QWidget>

class PPIView;
class PPIScene;
class ZoomViewWidget;
class SectorWidget;

namespace Ui {
class MainOverLayOut;
}

class MainOverLayOut : public QWidget
{
    Q_OBJECT

public:
    explicit MainOverLayOut(QWidget *parent = nullptr);
    ~MainOverLayOut();
    void mainPView();
    void topRightSet();

private:
    Ui::MainOverLayOut *ui;
    PPIView* mView;
    PPIScene* mScene;
    ZoomViewWidget* m_zoomView;
    SectorWidget* m_sectorWidget;
};

#endif // MAINOVERLAYOUT_H
