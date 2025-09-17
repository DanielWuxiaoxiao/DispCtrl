#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialog>

class DetachableWidget : public QWidget {
    Q_OBJECT
public:
    explicit DetachableWidget(QWidget* childWidget, QWidget* parent = nullptr);

private slots:
    void detach();
    void reattach();

private:
    QWidget* m_child;         // 真正的内容
    QPushButton* m_btn;       // 控制按钮
    QVBoxLayout* m_layout;

    QDialog* m_floatWindow = nullptr;
};
