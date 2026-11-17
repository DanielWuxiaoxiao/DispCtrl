#include "Basic/uithemeruntime.h"
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:52
 * @Description: 
 */
#include "Basic/uitheme.h"
#include "pviewtopleft.h"
#include "ui_pviewtopleft.h"
#include <QStyleOption>
#include <QPainter>
#include "Basic/ConfigManager.h"
#include "Controller/controller.h"
#include "ElectromagneticDevice/devicemodule.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QFontMetrics>
#include "Basic/uifontscale.h"
#include "paramWidget/radarpanelpositiondialog.h"
#pragma pack(push, 8)
#include "cusWidgets/cuswindow.h"
#pragma pack(pop)

mainviewTopLeft::mainviewTopLeft(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainviewTopLeft)
{
    ui->setupUi(this);
    UiTheme::setStyle(this, [=] { return UiFontScale::style(
        (UiTheme::classicEnabled ? "QLabel {color:#66ffcc;font-size:13px;font-weight:bold;}"
        "QLineEdit {color:#66ffcc;background:#101c1c;border:1px solid #79a6a6;border-radius:4px;"
        "font-size:13px;padding:2px;}" : "QLabel {color:#e8edf1;font-size:13px;font-weight:normal;}"
        "QLineEdit {color:#e8edf1;background:rgba(18,23,28,85);border:1px solid #303b44;border-radius:4px;"
        "font-size:13px;padding:2px;}")); });

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
    m_originButton = new QPushButton(QStringLiteral("机扫 GPS 配置"), this);
    m_originButton->setObjectName("radarReferenceButton");
    UiTheme::setStyle(m_originButton, [=] { return UiFontScale::style((UiTheme::classicEnabled ? "QPushButton { background:#075a58; color:#d9ffff; font-size:15px; "
                                 "padding:6px 10px; border:1px solid #00bfa5; border-radius:5px; }"
                                 "QPushButton:hover { background:#087a75; }" : "QPushButton { background:rgba(27,34,40,100); color:#e8edf1; font-size:15px; "
                                 "padding:6px 10px; border:1px solid #303b44; border-radius:5px; }"
                                 "QPushButton:hover { background:rgba(41,59,67,205); }")); });
    ui->gridLayout->addWidget(m_originButton, 6, 0, 1, 2);
    m_sendPositionButton = new QPushButton(QStringLiteral("四面阵配置"), this);
    m_sendPositionButton->setObjectName(QStringLiteral("sendRadarPanelPositionButton"));
    UiTheme::setStyle(m_sendPositionButton, [=] { return m_originButton->styleSheet(); });
    ui->gridLayout->addWidget(m_sendPositionButton, 7, 0, 1, 2);
    const bool multiPanel = CF_INS.defaultPanelCount() > 1;
    m_originButton->setVisible(!multiPanel);
    m_sendPositionButton->setVisible(multiPanel);
    connect(m_sendPositionButton, &QPushButton::clicked, this, &mainviewTopLeft::showPanelPositionDialog);
    connect(m_originButton, &QPushButton::clicked, this, &mainviewTopLeft::showPanelPositionDialog);
    connect(CON_INS, &Controller::radarReferenceChanged, this, &mainviewTopLeft::refreshReference);
    refreshReference();
    fitReferenceLayout();
    UiTheme::onChanged(this, [this] { fitReferenceLayout(); });
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
    const QString buttonStyle = UiFontScale::style((UiTheme::classicEnabled ? "QPushButton {background:#075a58;color:#d9ffff;"
        "font-size:12px;padding:4px;border:1px solid #00bfa5;border-radius:4px;}"
        "QPushButton:hover {background:#087a75;}" : "QPushButton {background:rgba(27,34,40,100);color:#e8edf1;"
        "font-size:12px;padding:4px;border:1px solid #303b44;border-radius:4px;}"
        "QPushButton:hover {background:rgba(41,59,67,205);}"));
    UiTheme::setStyle(m_originButton, [=] { return buttonStyle; });
    UiTheme::setStyle(m_sendPositionButton, [=] { return buttonStyle; });
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
    m_originButton->setText(module && module->isEnabled() ? QStringLiteral("手动配置经纬高/偏航")
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
        edits[i]->setToolTip(source == RadarReference::Source::Locked ? QStringLiteral("来源：阵面0已锁定，请在四面阵配置中解除锁定")
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
    const bool multiPanel = CF_INS.defaultPanelCount() > 1;
    const QString title = multiPanel ? QStringLiteral("四面阵配置") : QStringLiteral("单面阵配置");
    m_panelPositionWindow = new CusWindow(title, QIcon(":/resources/icon/radararray.png"), this);
    if (!multiPanel) {
        m_panelPositionWindow->setWindowModality(Qt::ApplicationModal);
        m_panelPositionDialog = createSinglePanelDialog(m_panelPositionWindow);
    } else {
        std::array<bool, 4> locked{};
        for (int id = 0; id < 4; ++id) locked[id] = CON_INS->radarPanelReference(id).locked();
        RadarPanelPositionDialog::Actions actions;
        actions.submit = [](const RadarPanelValues& values) { return CON_INS->sendRadarPanelPositions(values); };
        actions.requestTimeSync = [](int radarId) { CON_INS->requestSystemTimeSync(radarId); };
        actions.live = [] { return CON_INS->radarPanelSnapshot(); };
        actions.lock = [](int id, const RadarReference::Pose& values) { return CON_INS->lockRadarPanelReference(id, values); };
        actions.unlock = [](int id) -> std::optional<RadarReference::Pose> {
            if (!CON_INS->unlockRadarPanelReference(id)) return std::nullopt;
            return CON_INS->radarPanelReference(id).pose();
        };
        m_panelPositionDialog = new RadarPanelPositionDialog(
            CON_INS->radarPanelDialogValues(), locked, std::move(actions), m_panelPositionWindow);
    }
    connect(CON_INS, &Controller::systemTimeSyncStateChanged, m_panelPositionDialog.data(),
            &RadarPanelPositionDialog::setSystemTimeSyncState);
    m_panelPositionDialog->setSystemTimeSyncState(CON_INS->systemTimeSyncState());
    const QSize contentSize = m_panelPositionDialog->size();
    m_panelPositionDialog->attachToWindow(m_panelPositionWindow);
    m_panelPositionWindow->show();
    // CusWindow adjusts to its size hint on first show; retain the chart's screen-capped size.
    m_panelPositionWindow->resize(contentSize + QSize(22, 38));
}

RadarPanelPositionDialog* mainviewTopLeft::createSinglePanelDialog(QWidget* parent)
{
    auto* controller = CON_INS;
    const auto electromagnetic = m_electromagneticModule;
    const bool withElectromagnetic = electromagnetic && electromagnetic->isEnabled();
    const int fieldCount = withElectromagnetic ? RadarReference::Yaw + 1 : RadarReference::Count;
    const auto checkReference = [controller, electromagnetic, withElectromagnetic]() -> QString {
        if (controller->radarReference().locked())
            return QStringLiteral("阵面0已锁定，请先在四面阵配置中解除锁定。");
        if (withElectromagnetic && (!electromagnetic || !electromagnetic->isEnabled()))
            return QStringLiteral("电磁模块不可用。");
        return {};
    };
    RadarPanelPositionDialog::SinglePanel actions;
    actions.fieldCount = fieldCount;
    actions.manualGps = controller->mechanicalGpsManual();
    actions.gps = &controller->mechanicalGps();
    actions.setManualGps = [controller, electromagnetic](bool manual) -> QString {
        if (!controller->setMechanicalGpsManual(manual)) return QStringLiteral("切换 GPS 模式失败。");
        if (!manual && electromagnetic && electromagnetic->isEnabled())
            electromagnetic->restoreAutomaticRadarPosition();
        return {};
    };
    actions.apply = [=](const RadarReference::Pose& edited, bool persist) -> QString {
        if (const QString error = checkReference(); !error.isEmpty()) return error;
        auto pose = controller->radarReference().pose();
        auto values = controller->radarReference().manual();
        for (int field = 0; field < fieldCount; ++field) {
            pose[field] = edited[field];
            values[field] = edited[field];
        }
        if (controller->mechanicalGpsManual() && !persist && !controller->sendMechanicalRadarPosition(pose))
            return QStringLiteral("机扫位置下发请求未提交，请检查参数和资源调度链路。");
        if (persist && !CF_INS.saveRadarReference(pose[0], pose[1], pose[2], pose[3], pose[4], pose[5]))
            return QStringLiteral("保存失败，请检查 config.toml 的路径及写入权限。");
        if (!controller->setManualRadarReference(values, withElectromagnetic))
            return QStringLiteral("应用本地参考值失败。");
        if (withElectromagnetic && electromagnetic)
            electromagnetic->setManualRadarPosition(pose[RadarReference::Latitude],
                pose[RadarReference::Longitude], pose[RadarReference::Altitude]);
        return {};
    };
    actions.submitGps = [controller] { return controller->sendMechanicalGpsPosition(); };
    actions.submitServoNorth = [controller] {
        return controller->requestMechanicalServoNorth(CF_INS.servoSpeed(3));
    };
    if (withElectromagnetic) {
        actions.restoreAutomatic = [=]() -> QString {
            if (const QString error = checkReference(); !error.isEmpty()) return error;
            auto values = controller->radarReference().manual();
            for (int field = 0; field < fieldCount; ++field) values[field].reset();
            if (!controller->setManualRadarReference(values)) return QStringLiteral("解除本地覆盖失败。");
            if (electromagnetic) electromagnetic->restoreAutomaticRadarPosition();
            return {};
        };
    }
    auto* dialog = new RadarPanelPositionDialog(controller->radarReference().pose(),
        std::move(actions), [controller](int id) { controller->requestSystemTimeSync(id); }, parent);
    connect(controller, &Controller::mechanicalGpsUpdated, dialog, &RadarPanelPositionDialog::refreshMechanicalGps);
    return dialog;
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
