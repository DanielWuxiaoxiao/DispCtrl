/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:52
 * @Description: 
 */
#include "pviewtopleft.h"
#include "ui_pviewtopleft.h"
#include <QStyleOption>
#include <QPainter>
#include "Basic/ConfigManager.h"
#include "Controller/controller.h"
#include "ElectromagneticDevice/devicemodule.h"
#include "cusWidgets/custommessagebox.h"
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QVBoxLayout>
#include <QFontMetrics>
#include "Basic/uifontscale.h"
#include "paramWidget/radarpanelpositiondialog.h"
#pragma pack(push, 8)
#include "cusWidgets/cuswindow.h"
#pragma pack(pop)

namespace {
int referencePrecision(int field) {
    return field < 2 ? 9 : field == RadarReference::Altitude || field == RadarReference::Roll ? 2 : 3;
}
}

mainviewTopLeft::mainviewTopLeft(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainviewTopLeft)
{
    ui->setupUi(this);
    setStyleSheet(UiFontScale::style(
        "QLabel {color:#66ffcc;font-size:13px;font-weight:bold;}"
        "QLineEdit {color:#66ffcc;background:#101c1c;border:1px solid #79a6a6;border-radius:4px;"
        "font-size:13px;padding:2px;}"));

    // 设置简化的tooltip
    ui->label->setToolTip("雷达经度");
    ui->lineEdit->setToolTip("经度(-180°~180°)");
    ui->label_2->setToolTip("雷达纬度");
    ui->lat->setToolTip("纬度(-90°~90°)");
    ui->label_3->setToolTip("海拔高度");
    ui->height->setToolTip("高度(米)");
    ui->label_4->setToolTip("阵面偏航");
    ui->dir->setToolTip("指北角(0°~360°)");
    ui->label_6->setToolTip("倾角");
    ui->yaw->setToolTip("俯仰倾角(度)");

    auto* rollLabel = new QLabel(QStringLiteral("横滚角"), this);
    rollLabel->setAlignment(Qt::AlignCenter);
    m_rollEdit = new QLineEdit(this);
    m_rollEdit->setObjectName(QStringLiteral("radarRoll"));
    m_rollEdit->setReadOnly(true);
    m_rollEdit->setAlignment(Qt::AlignCenter);
    ui->gridLayout->addWidget(rollLabel, 5, 0);
    ui->gridLayout->addWidget(m_rollEdit, 5, 1);
    m_originButton = new QPushButton(QStringLiteral("配置雷达经纬高/姿态"), this);
    m_originButton->setObjectName("radarReferenceButton");
    m_originButton->setStyleSheet(UiFontScale::style("QPushButton { background:#075a58; color:#d9ffff; font-size:15px; "
                                 "padding:6px 10px; border:1px solid #00bfa5; border-radius:5px; }"
                                 "QPushButton:hover { background:#087a75; }"));
    ui->gridLayout->addWidget(m_originButton, 6, 0, 1, 2);
    m_sendPositionButton = new QPushButton(QStringLiteral("下发雷达阵面经纬高姿态"), this);
    m_sendPositionButton->setObjectName(QStringLiteral("sendRadarPanelPositionButton"));
    m_sendPositionButton->setStyleSheet(m_originButton->styleSheet());
    ui->gridLayout->addWidget(m_sendPositionButton, 7, 0, 1, 2);
    const bool multiPanel = CF_INS.defaultPanelCount() > 1;
    m_originButton->setVisible(!multiPanel);
    m_sendPositionButton->setVisible(multiPanel);
    connect(m_sendPositionButton, &QPushButton::clicked, this, &mainviewTopLeft::showPanelPositionDialog);
    connect(m_originButton, &QPushButton::clicked, this, &mainviewTopLeft::showReferenceDialog);
    connect(CON_INS, &Controller::radarReferenceChanged, this, &mainviewTopLeft::refreshReference);
    refreshReference();
    fitReferenceLayout();
}

void mainviewTopLeft::fitReferenceLayout()
{
    // Size for the complete signed longitude, not the shorter currently displayed value.
    // This keeps the overlay stable while GPS values change and respects the actual Qt font.
    ensurePolished();
    ui->horizontalLayout->setContentsMargins(6, 6, 6, 6);
    ui->gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->gridLayout->setHorizontalSpacing(5);
    ui->gridLayout->setVerticalSpacing(3);
    const std::array<QLineEdit*, RadarReference::Count> edits = {
        ui->lineEdit, ui->lat, ui->height, ui->dir, ui->yaw, m_rollEdit};
    for (auto* edit : edits) {
        edit->ensurePolished();
        const QFontMetrics metrics(edit->font());
        edit->setMinimumWidth(metrics.horizontalAdvance(QStringLiteral("-1000.00000")) + 12);
        edit->setMinimumHeight(metrics.height() + 6);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    for (auto* label : findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly)) {
        label->ensurePolished();
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setMinimumWidth(label->fontMetrics().horizontalAdvance(label->text()) + 4);
    }
    const QString buttonStyle = UiFontScale::style("QPushButton {background:#075a58;color:#d9ffff;"
        "font-size:12px;padding:4px;border:1px solid #00bfa5;border-radius:4px;}"
        "QPushButton:hover {background:#087a75;}");
    m_originButton->setStyleSheet(buttonStyle);
    m_sendPositionButton->setStyleSheet(buttonStyle);
    m_originButton->setMinimumHeight(26);
    m_sendPositionButton->setMinimumHeight(26);
    ui->gridLayout->setColumnStretch(0, 0);
    ui->gridLayout->setColumnStretch(1, 1);
    layout()->activate();
    adjustSize();
    emit overlaySizeChanged();
}

double mainviewTopLeft::getLatitude()  const { return CON_INS->radarReference().value(RadarReference::Latitude); }
double mainviewTopLeft::getLongitude() const { return CON_INS->radarReference().value(RadarReference::Longitude); }
double mainviewTopLeft::getAltitude()  const { return CON_INS->radarReference().value(RadarReference::Altitude); }
double mainviewTopLeft::getYaw()       const { return CON_INS->radarReference().value(RadarReference::Yaw); }

void mainviewTopLeft::setElectromagneticDeviceModule(ElectromagneticDeviceModule* module)
{
    m_electromagneticModule = module;
    m_originButton->setText(module && module->isEnabled() ? QStringLiteral("手动配置经纬高")
                                                         : QStringLiteral("配置雷达经纬高/姿态"));
    fitReferenceLayout();
}

void mainviewTopLeft::refreshReference()
{
    const auto& reference = CON_INS->radarReference();
    const std::array<QLineEdit*, RadarReference::Count> edits = {ui->lineEdit, ui->lat, ui->height, ui->dir, ui->yaw, m_rollEdit};
    for (int i = 0; i < RadarReference::Count; ++i) {
        const auto field = static_cast<RadarReference::Field>(i);
        const QString text = QString::number(reference.value(field), 'f', 5);
        if (edits[i]->text() != text) edits[i]->setText(text);
        const auto source = reference.source(field);
        edits[i]->setToolTip(source == RadarReference::Source::Locked ? QStringLiteral("来源：阵面0已锁定，请在阵面下发窗口解除锁定")
            : source == RadarReference::Source::Manual ? QStringLiteral("来源：UI配置")
            : source == RadarReference::Source::Report ? QStringLiteral("来源：设备上报") : QStringLiteral("来源：开机默认值"));
    }
}

void mainviewTopLeft::showPanelPositionDialog()
{
    if (m_panelPositionWindow) {
        if (m_panelPositionWindow->isMinimized()) m_panelPositionWindow->showNormal();
        else m_panelPositionWindow->show();
        m_panelPositionWindow->raise();
        m_panelPositionWindow->activateWindow();
        return;
    }
    std::array<bool, 4> locked{};
    for (int id = 0; id < 4; ++id) locked[id] = CON_INS->radarPanelReference(id).locked();
    RadarPanelPositionDialog::Actions actions;
    actions.submit = [](const RadarPanelValues& values) { return CON_INS->sendRadarPanelPositions(values); };
    actions.live = [] { return CON_INS->radarPanelSnapshot(); };
    actions.lock = [](int id, const RadarReference::Pose& values) { return CON_INS->lockRadarPanelReference(id, values); };
    actions.unlock = [](int id) -> std::optional<RadarReference::Pose> {
        if (!CON_INS->unlockRadarPanelReference(id)) return std::nullopt;
        return CON_INS->radarPanelReference(id).pose();
    };
    m_panelPositionWindow = new CusWindow(QStringLiteral("雷达阵面经纬高 / 姿态下发"),
        QIcon(":/resources/icon/radararray.png"), this);
    m_panelPositionDialog = new RadarPanelPositionDialog(
        CON_INS->radarPanelDialogValues(), locked, std::move(actions), m_panelPositionWindow);
    const QSize contentSize = m_panelPositionDialog->size();
    m_panelPositionDialog->attachToWindow(m_panelPositionWindow);
    m_panelPositionWindow->show();
    // CusWindow adjusts to its size hint on first show; retain the chart's screen-capped size.
    m_panelPositionWindow->resize(contentSize + QSize(22, 38));
}

void mainviewTopLeft::showReferenceDialog()
{
    if (m_electromagneticModule && m_electromagneticModule->isEnabled()) {
        m_electromagneticModule->showOriginDialog(this);
        return;
    }
    QPointer<QDialog> dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("配置雷达经纬高/姿态"));
    dialog->setMinimumWidth(580);
    dialog->setStyleSheet(UiFontScale::style("QDialog { background:#101818; } QLabel { color:#d9ffff; font-size:18px; }"
        "QLineEdit { background:#142d28; color:white; border:1px solid #66ccaa; padding:6px; font-size:18px; }"
        "QPushButton { background:#123630; color:#66ffcc; border:1px solid #66ccaa; padding:7px; font-size:18px; }"));
    auto* layout = new QVBoxLayout(dialog);
    auto* form = new QFormLayout;
    layout->addLayout(form);
    const QStringList names = {QStringLiteral("经度(°)"), QStringLiteral("纬度(°)"), QStringLiteral("海拔高度(m)"),
                               QStringLiteral("阵面偏航(°)"), QStringLiteral("倾角(°)"), QStringLiteral("横滚角(°)")};
    std::array<QLineEdit*, RadarReference::Count> edits{};
    const auto& reference = CON_INS->radarReference();
    for (int i = 0; i < RadarReference::Count; ++i) {
        edits[i] = new QLineEdit(dialog);
        edits[i]->setObjectName(QStringLiteral("radarReferenceField%1").arg(i));
        edits[i]->setText(QString::number(reference.value(static_cast<RadarReference::Field>(i)), 'f', referencePrecision(i)));
        form->addRow(names[i], edits[i]);
    }
    auto* error = new QLabel(dialog);
    error->setStyleSheet(UiFontScale::style("color:#ff9b9b"));
    error->setWordWrap(true);
    error->hide();
    layout->addWidget(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    auto* save = buttons->addButton(QStringLiteral("保存本地"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    const auto apply = [dialog, edits, names, error](bool persist) {
        error->setStyleSheet(UiFontScale::style("color:#ff9b9b"));
        if (CON_INS->radarReference().locked()) {
            error->setText(QStringLiteral("阵面0已锁定，请先在阵面下发窗口解除锁定。")); error->show(); return;
        }
        RadarReference::Values values{};
        for (int i = 0; i < RadarReference::Count; ++i) {
            const QString text = edits[i]->text().trimmed();
            bool ok = false;
            const double value = text.toDouble(&ok);
            if (!ok || !RadarReference::validUi(static_cast<RadarReference::Field>(i), value)) {
                error->setText(names[i] + QStringLiteral("无效。经度±180、纬度/倾角/横滚±90、偏航0～360、高度-1000～5000m；各项不能为空。"));
                error->show();
                edits[i]->setFocus();
                return;
            }
            values[i] = value;
        }
        if (persist && !CF_INS.saveRadarReference(*values[0], *values[1], *values[2], *values[3], *values[4], *values[5])) {
            error->setText(QStringLiteral("保存失败，请检查 config.toml 的路径及写入权限。"));
            error->show();
            return;
        }
        if (!CON_INS->setManualRadarReference(values)) return;
        if (persist) {
            error->setStyleSheet(UiFontScale::style("color:#66ffcc"));
            error->setText(QStringLiteral("已保存到本地 config.toml。"));
            error->show();
        } else if (dialog) dialog->accept();
    };
    connect(buttons, &QDialogButtonBox::accepted, dialog, [apply] { apply(false); });
    connect(save, &QPushButton::clicked, dialog, [apply] { apply(true); });
    dialog->exec();
    if (dialog) dialog->deleteLater();
}

void mainviewTopLeft::paintEvent(QPaintEvent* event) {
    QStyleOption o;
    o.initFrom(this); // 初始化 QStyleOption
    o.rect = rect();  // 设置绘制区域
    // 绘制背景和边框
    // QWidget::paintEvent(event); // 如果要继承父类的绘制，可以调用，但对于纯背景绘制，可能不需要
    QPainter painter(this);
    // 绘制背景色和圆角
    // 注意：这里直接使用 QStyle 绘制，可以更好地处理样式表中的属性
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);
}

mainviewTopLeft::~mainviewTopLeft()
{
    delete ui;
}
