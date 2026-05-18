/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-23 09:44:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
 * @Description: 
 */
#include "mousepositioninfo.h"
#include "ui_mousepositioninfo.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include <QStyleOption>
#include <QPainter>
#include <QLabel>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QVBoxLayout>

MousePositionInfo::MousePositionInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MousePositionInfo)
{
    ui->setupUi(this);

    // 设置简化的tooltip
    ui->label_distance->setToolTip("鼠标距离");
    ui->distanceValue->setToolTip("鼠标到雷达中心的距离(公里)");
    ui->label_azimuth->setToolTip("方位角");
    ui->azimuthValue->setToolTip("鼠标位置的方位角(度)");

    // 设置对象名称以便样式表应用
    setObjectName("MousePositionInfo");
    ui->distanceValue->setObjectName("MouseDistanceValue");
    ui->azimuthValue->setObjectName("MouseAzimuthValue");

    // 在checkbox旁边添加颜色圆圈指示
    // 检测点：绿色圆圈
    {
        QLabel* detColorLabel = new QLabel("●", this);
        detColorLabel->setStyleSheet("color: #00FF00; font-size: 18px;");
        detColorLabel->setFixedWidth(16);
        ui->checkboxLayout->insertWidget(1, detColorLabel);
    }
    // 跟踪点：红色圆圈
    {
        QLabel* trackColorLabel = new QLabel("●", this);
        trackColorLabel->setStyleSheet("color: #FF0000; font-size: 18px;");
        trackColorLabel->setFixedWidth(16);
        ui->checkboxLayout->addWidget(trackColorLabel);
    }

    auto addTrackToggle = [this](QHBoxLayout* layout,
                                 QCheckBox*& checkBox,
                                 const QString& text,
                                 const QString& toolTip,
                                 const QString& objectName,
                                 const QColor& color,
                                 void (MousePositionInfo::*signal)(bool)) {
        checkBox = new QCheckBox(text, this);
        checkBox->setToolTip(toolTip);
        checkBox->setChecked(true);
        checkBox->setObjectName(objectName);
        checkBox->setStyleSheet("QCheckBox::indicator { width: 12px; height: 12px; }");
        layout->addWidget(checkBox);

        QLabel* colorLabel = new QLabel("●", this);
        colorLabel->setStyleSheet(QString("color: %1; font-size: 18px;").arg(color.name()));
        colorLabel->setFixedWidth(16);
        layout->addWidget(colorLabel);

        connect(checkBox, &QCheckBox::toggled, this, signal);
    };

    QHBoxLayout* secondaryCheckboxLayout = nullptr;
    if (CF_INS.iftbd(false) || CF_INS.ifxietong(false)) {
        secondaryCheckboxLayout = new QHBoxLayout();
        secondaryCheckboxLayout->setSpacing(ui->checkboxLayout->spacing());
        secondaryCheckboxLayout->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout->insertLayout(2, secondaryCheckboxLayout);
    }

    if (CF_INS.iftbd(false)) {
        addTrackToggle(secondaryCheckboxLayout,
                       m_tbdTrackCheckBox,
                       tr("TBD航迹"),
                       tr("显示/隐藏TBD航迹"),
                       "MouseCheckBoxTBDTrack",
                       trackTypeColor(PointType::TBDPointType),
                       &MousePositionInfo::tbdTrackVisibilityChanged);
    }

    if (CF_INS.ifxietong(false)) {
        addTrackToggle(secondaryCheckboxLayout,
                       m_cooperativeTrackCheckBox,
                       tr("协同航迹"),
                       tr("显示/隐藏协同航迹"),
                       "MouseCheckBoxCooperativeTrack",
                       trackTypeColor(PointType::CooperativeTrackPointType),
                       &MousePositionInfo::cooperativeTrackVisibilityChanged);
    }

    // 连接checkbox信号
    // 标识 objectName 并调整指示器尺寸为稍小（更紧凑）
    ui->checkBoxDetection->setObjectName("MouseCheckBoxDetection");
    ui->checkBoxTrack->setObjectName("MouseCheckBoxTrack");
    ui->checkBoxDetection->setStyleSheet("QCheckBox::indicator { width: 12px; height: 12px; }");
    ui->checkBoxTrack->setStyleSheet("QCheckBox::indicator { width: 12px; height: 12px; }");

    connect(ui->checkBoxDetection, &QCheckBox::toggled, this, &MousePositionInfo::detectionVisibilityChanged);
    connect(ui->checkBoxTrack, &QCheckBox::toggled, this, &MousePositionInfo::trackVisibilityChanged);

    // 连接点迹大小spinbox信号
    connect(ui->spinBoxDetSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MousePositionInfo::detectionSizeChanged);
    connect(ui->spinBoxTrackSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MousePositionInfo::trackSizeChanged);
}

void MousePositionInfo::paintEvent(QPaintEvent* event) {
    QStyleOption o;
    o.initFrom(this);
    o.rect = rect();
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);
}

void MousePositionInfo::updatePosition(double distance, double azimuth)
{
    // 距离显示为公里，保留1位小数，添加单位
    ui->distanceValue->setText(QString::number(distance, 'f', 1) + " km");

    // 方位角显示为度，保留1位小数，添加单位
    ui->azimuthValue->setText(QString::number(azimuth, 'f', 1) + "°");
}

MousePositionInfo::~MousePositionInfo()
{
    delete ui;
}

// 获取当前状态
bool MousePositionInfo::isDetectionVisible() const
{
    return ui->checkBoxDetection->isChecked();
}

bool MousePositionInfo::isTrackVisible() const
{
    return ui->checkBoxTrack->isChecked();
}

bool MousePositionInfo::isTbdTrackVisible() const
{
    return m_tbdTrackCheckBox ? m_tbdTrackCheckBox->isChecked() : false;
}

bool MousePositionInfo::isCooperativeTrackVisible() const
{
    return m_cooperativeTrackCheckBox ? m_cooperativeTrackCheckBox->isChecked() : false;
}

double MousePositionInfo::getDetectionSize() const
{
    return ui->spinBoxDetSize->value();
}

double MousePositionInfo::getTrackSize() const
{
    return ui->spinBoxTrackSize->value();
}
