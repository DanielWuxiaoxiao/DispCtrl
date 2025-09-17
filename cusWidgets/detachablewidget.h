#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

class CusWindow;

class DetachableWidget : public QWidget {
    Q_OBJECT
public:
    explicit DetachableWidget(QString name, QWidget* childWidget,QIcon icon, QWidget* parent = nullptr);

private slots:
    void detach();
    void reattach();

private:
    QWidget* m_child;         // 真正的内容
    QPushButton* m_btn;       // 控制按钮
    QVBoxLayout* m_layout;
    QString m_name;
    CusWindow* m_floatWindow = nullptr;
    QIcon m_icon;
};
