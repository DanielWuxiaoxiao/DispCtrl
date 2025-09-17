#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class CustomMessageBox : public QDialog
{
    Q_OBJECT

public:
    enum MessageType {
        Info,
        Warning,
        Confirm
    };

    enum ButtonType {
        Ok,
        OkCancel,
        YesNo
    };

    explicit CustomMessageBox(QWidget *parent = nullptr);

    static bool showConfirm(QWidget *parent, const QString &title, const QString &text);
    static void showInfo(QWidget *parent, const QString &title, const QString &text);
    static void showWarning(QWidget *parent, const QString &title, const QString &text);

private:
    QLabel *m_titleLabel;
    QLabel *m_textLabel;
    QHBoxLayout *m_buttonLayout;

    void setupUI();
    void applyStyle();
    int showDialog(const QString &title, const QString &text, MessageType type, ButtonType buttons);
};

#endif // CUSTOMMESSAGEBOX_H
