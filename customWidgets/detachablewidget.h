#ifndef DETACHABLEWIDGET_H
#define DETACHABLEWIDGET_H

#include <QWidget>

class DetachableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetachableWidget(QWidget *parent = nullptr);

signals:

};

#endif // DETACHABLEWIDGET_H
