#ifndef HEALTHSTATUSWIDGETS_H
#define HEALTHSTATUSWIDGETS_H

// 只在 GUI 线程由五秒健康快照更新，不持有或改写协议数据。
#pragma pack(push, 8)
#include "Basic/uitheme.h"
#include "Basic/uifontscale.h"
#include <QFrame>
#include <QLabel>
#include <QBoxLayout>

namespace HealthUi {
inline void applyStyle(QWidget* widget, const QString& designStyle)
{
    const QString scaled = UiFontScale::style(designStyle);
    if (widget->styleSheet() != scaled) widget->setStyleSheet(scaled);
}

// 标题和值独立排版，避免用禁用按钮承载正文后整体变色或被压成细条。
class Field final : public QFrame {
public:
    enum class Layout { Card, Row, Inline };
    Field(const QString& title, Layout presentation, QWidget* parent = nullptr, bool compact = false)
        : QFrame(parent), m_presentation(presentation)
    {
        setObjectName(QStringLiteral("HealthField"));
        setAttribute(Qt::WA_StyledBackground);
        auto* box = new QBoxLayout(presentation == Layout::Card
                                      ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, this);
        const int inset = presentation == Layout::Card ? ScaleHelper::uiScaled(compact ? 12 : 16) : 0;
        const int verticalInset = presentation == Layout::Card ? inset : ScaleHelper::uiScaled(compact ? 3 : 7);
        box->setContentsMargins(inset, verticalInset, inset, verticalInset);
        box->setSpacing(ScaleHelper::uiScaled(presentation == Layout::Card ? 10 : 12));
        m_title = new QLabel(title, this);
        m_title->setWordWrap(true);
        m_value = new QLabel(this);
        m_value->setWordWrap(true);
        box->addWidget(m_title, presentation == Layout::Row ? 1 : 0);
        box->addWidget(m_value);
        if (presentation == Layout::Inline) box->addStretch();
        if (presentation == Layout::Row) m_value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        if (presentation == Layout::Card) setMinimumHeight(ScaleHelper::uiScaled(compact ? 92 : 102));
        applyStyle(m_title, QStringLiteral("QLabel {color:%1;background:transparent;border:none;font-size:%2px;}")
            .arg(presentation == Layout::Inline ? UiTheme::Secondary : UiTheme::Text)
            .arg(presentation == Layout::Card ? 18 : 16));
        setStatus(UiTheme::Status::Unknown, QStringLiteral("未上报"));
    }

    void setStatus(UiTheme::Status state, const QString& text, bool marker = true)
    {
        const bool fault = state == UiTheme::Status::Fault;
        const char* color = fault ? UiTheme::Fault
            : state == UiTheme::Status::Normal && m_presentation != Layout::Row ? UiTheme::Normal : UiTheme::Secondary;
        QString prefix;
        if (marker && m_presentation != Layout::Row) {
            prefix = state == UiTheme::Status::Normal ? QStringLiteral("● ")
                   : fault ? QStringLiteral("! ") : QStringLiteral("— ");
        }
        const QString value = prefix + text;
        if (m_value->text() != value) m_value->setText(value);
        applyStyle(m_value, QStringLiteral("QLabel {color:%1;background:transparent;border:none;font-size:16px;}").arg(color));
        QString frame;
        if (m_presentation == Layout::Card) {
            frame = QStringLiteral("QFrame#HealthField {background:%1;border:1px solid %2;border-radius:6px;}")
                .arg(fault ? UiTheme::FaultSurface : UiTheme::Surface, fault ? UiTheme::Fault : UiTheme::Border);
        } else {
            frame = QStringLiteral("QFrame#HealthField {background:transparent;border:none;%1}")
                .arg(m_presentation == Layout::Row ? QStringLiteral("border-bottom:1px solid #303b44;") : QString());
        }
        applyStyle(this, frame);
        setAccessibleName(m_title->text() + QStringLiteral("：") + text);
    }

private:
    Layout m_presentation;
    QLabel* m_title = nullptr;
    QLabel* m_value = nullptr;
};

// 保留物理编号，仅异常/未知添加符号；悬停和无障碍名称包含完整状态。
class PowerCell final : public QLabel {
public:
    PowerCell(int number, QWidget* parent = nullptr) : QLabel(parent), m_number(number)
    {
        setAlignment(Qt::AlignCenter);
        setMinimumSize(ScaleHelper::uiScaled(54), ScaleHelper::uiScaled(46));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStatus(UiTheme::Status::Unknown);
    }
    void setStatus(UiTheme::Status state)
    {
        const bool normal = state == UiTheme::Status::Normal;
        const bool fault = state == UiTheme::Status::Fault;
        const QString number = QStringLiteral("%1").arg(m_number, 2, 10, QLatin1Char('0'));
        const QString value = number + (fault ? QStringLiteral(" !") : normal ? QString() : QStringLiteral(" —"));
        if (text() != value) setText(value);
        const QString status = normal ? QStringLiteral("正常") : fault ? QStringLiteral("故障") : QStringLiteral("未上报");
        const QString description = QStringLiteral("电源 %1 · %2").arg(number, status);
        setToolTip(description);
        setAccessibleName(description);
        applyStyle(this, QStringLiteral("QLabel {background:%1;color:%2;border:1px solid %3;border-radius:5px;font-size:16px;padding:0;}")
            .arg(normal ? UiTheme::NormalSurface : fault ? UiTheme::FaultSurface : UiTheme::Surface,
                 normal ? UiTheme::Normal : fault ? UiTheme::Fault : UiTheme::Secondary,
                 fault ? UiTheme::Fault : "transparent"));
    }
private:
    int m_number;
};
} // namespace HealthUi
#pragma pack(pop)
#endif
