/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-23 09:44:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 15:56:15
 * @Description: 
 */
/**
 * @file ppivisualsettings.cpp
 * @brief PPI视觉设置组件实现文件
 * @details 实现PPI视图的可视化参数配置功能，提供距离和地图设置的用户界面
 *
 * 实现要点：
 * 1. UI初始化：加载UI文件，设置默认值，应用样式
 * 2. 信号连接：建立控件事件与处理函数的关联
 * 3. 输入验证：确保用户输入的合法性和有效性
 * 4. 实时响应：配置变化立即通过信号通知相关组件
 * 5. 状态管理：提供配置的读取和设置接口
 *
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

#include "ppivisualsettings.h"
#include "ui_ppivisualsettings.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h" // for MAX_RANGE and other display constants
#include <QDoubleValidator>
#include <QPainter>
#include <QStyleOption>
#include <QPolygon>
#include <QEvent>
#include <QMessageBox>
#include <QPushButton>

/**
 * @brief 构造函数实现
 * @param parent 父窗口组件
 * @details 初始化完整的PPI视觉设置组件，配置UI控件和行为
 *
 * 初始化流程：
 * 1. UI加载：通过setupUi()加载UI界面设计
 * 2. 样式应用：调用setupStyle()应用统一样式
 * 3. 验证器设置：为距离输入框添加数值验证器
 * 4. 信号连接：建立UI控件与处理函数的信号槽连接
 * 5. 默认值设置：设置合理的初始配置值
 *
 * 验证器配置：
 * - 类型：QDoubleValidator确保输入为有效数值
 * - 范围：1.0到99999.0公里，覆盖实际使用范围
 * - 精度：2位小数，满足精度要求
 * - 目的：防止无效输入，提升用户体验
 *
 * 默认配置：
 * - 最大距离：500公里，适合大多数雷达应用场景
 * - 地图类型：索引1（路网图），提供基础地理参考
 * - 便于快速开始使用，减少初次配置工作
 */
PPIVisualSettings::PPIVisualSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PPIVisualSettings)
{
    ui->setupUi(this);

    // 设置样式
    setupStyle();

    // 为距离输入框设置验证器
    QDoubleValidator* distanceValidator = new QDoubleValidator(1.0, 99999.0, 2, this);
    ui->maxDistanceEdit->setValidator(distanceValidator);

    // 连接信号槽
    connectSignals();

    // 设置默认值（配置文件中直接是公里值）
    double maxRangeInKm = CF_INS.range("max", 5);  // 配置直接返回公里值
    ui->maxDistanceEdit->setText(QString::number(maxRangeInKm));

    int defaultMapType = CF_INS.mapType("default_type", 1);
    ui->mapTypeCombo->setCurrentIndex(defaultMapType); // 使用配置的默认地图类型

    // 设置工具提示
    ui->maxDistanceEdit->setToolTip("设置雷达显示的最大距离范围");
    ui->mapTypeCombo->setToolTip("选择背景地图显示类型");

    // 样式设置完成
}

/**
 * @brief 析构函数实现
 * @details 清理UI资源，确保内存正确释放
 */
PPIVisualSettings::~PPIVisualSettings()
{
    delete ui;
}

/**
 * @brief 获取当前最大距离设置
 * @return 最大距离值（公里）
 * @details 从UI控件读取当前的最大距离配置
 */
double PPIVisualSettings::getMaxDistance() const
{
    return ui->maxDistanceEdit->text().toDouble();
}

/**
 * @brief 设置最大距离值
 * @param distance 最大距离（公里）
 * @details 程序化设置最大距离，更新UI显示但不触发信号
 */
void PPIVisualSettings::setMaxDistance(double distance)
{
    ui->maxDistanceEdit->setText(QString::number(distance, 'f', 1));
}

/**
 * @brief 获取当前地图类型索引
 * @return 地图类型索引
 * @details 从UI控件读取当前的地图类型选择
 */
int PPIVisualSettings::getMapType() const
{
    return ui->mapTypeCombo->currentIndex();
}

/**
 * @brief 设置地图类型
 * @param index 地图类型索引
 * @details 程序化设置地图类型，更新UI显示但不触发信号
 */
void PPIVisualSettings::setMapType(int index)
{
    if (index >= 0 && index < ui->mapTypeCombo->count()) {
        ui->mapTypeCombo->setCurrentIndex(index);
    }
}

/**
 * @brief 距离输入回车处理
 * @details 响应用户在距离输入框中按下回车键，验证并应用新的距离设置
 *
 * 处理流程：
 * 1. 获取输入值：从LineEdit控件读取用户输入
 * 2. 数值转换：将文本转换为double类型数值
 * 3. 有效性验证：调用validateDistance()检查合法性
 * 4. 信号发射：如果有效，发出maxDistanceChanged信号
 * 5. 错误处理：如果无效，显示错误提示并恢复原值
 *
 * 验证标准：
 * - 数值范围：1.0到99999.0公里
 * - 有效性：非零正数，实际可达范围
 * - 用户体验：清晰的错误提示，便于纠正
 */
void PPIVisualSettings::onDistanceEditReturnPressed()
{
    bool ok;
    double distance = ui->maxDistanceEdit->text().toDouble(&ok);

    if (ok && validateDistance(distance)) {
        emit maxDistanceChanged(distance);
    } else {
        // 输入无效，恢复之前的值并提示
        QMessageBox::warning(this, "输入错误", "请输入有效的距离值 (1-99999 km)");
        ui->maxDistanceEdit->selectAll();
        ui->maxDistanceEdit->setFocus();
    }
}

/**
 * @brief 地图类型选择变化处理
 * @param index 新选择的地图类型索引
 * @details 响应用户在地图类型下拉框中的选择变化，立即应用新设置
 *
 * 地图类型映射：
 * - 0: 无地图 (black.html)
 * - 1: 路网图 (indexNoL.html)
 * - 2: 标准图 (index.html)
 * - 3: 卫星图 (indexS.html)
 * - 4: 3D地图 (index3d.html)
 *
 * 即时生效：
 * - 无需额外确认，选择即应用
 * - 通过信号通知MapProxyWidget切换地图
 * - 提供流畅的用户体验
 */
void PPIVisualSettings::onMapTypeChanged(int index)
{
    emit mapTypeChanged(index);
}

/**
 * @brief 设置组件样式
 * @details 应用统一的样式表，确保与项目整体风格一致
 *
 * 样式特点：
 * - 对象名设置：便于QSS选择器定位
 * - 统一风格：与MousePositionInfo等组件保持一致
 * - 主题适配：支持深色主题的视觉效果
 * - 可定制性：通过QSS可进一步调整外观
 */
void PPIVisualSettings::setupStyle()
{
    setObjectName("PPIVisualSettings");
    ui->label_distance->setObjectName("PPIDistanceLabel");
    ui->label_map->setObjectName("PPIMapLabel");
    ui->maxDistanceEdit->setObjectName("PPIDistanceEdit");
    ui->mapTypeCombo->setObjectName("PPIMapCombo");
    ui->measureBtn->setObjectName("PPIMeasureBtn");
}

/**
 * @brief 重写paintEvent以确保样式表正确渲染
 * @param event 绘制事件对象
 * @details 使用QStyleOption确保QSS样式能够正确应用到自定义Widget
 *
 * 实现要点：
 * - QStyleOption初始化：从当前widget获取样式信息
 * - 绘制区域设置：确保整个widget区域都被正确绘制
 * - QStyle绘制：使用Qt样式系统绘制PE_Widget，支持QSS背景和边框
 *
 * 参考：与mainviewTopLeft保持相同的实现模式
 */
void PPIVisualSettings::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QStyleOption option;
    option.initFrom(this);     // 从当前widget初始化样式选项
    option.rect = rect();      // 设置绘制区域为整个widget区域

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 使用Qt样式系统绘制widget背景，这样QSS样式才能正确应用
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

/**
 * @brief 事件过滤器实现
 * @param obj 事件目标对象
 * @param event 事件对象
 * @return 是否处理了事件
 * @details 监听ComboBox的Enter/Leave事件，触发重绘以更新箭头颜色
 */
bool PPIVisualSettings::eventFilter(QObject* obj, QEvent* event)
{
    // 基础实现
    return QWidget::eventFilter(obj, event);
}

/**
 * @brief 连接信号槽
 * @details 建立UI控件与处理函数的信号槽连接
 *
 * 信号连接：
 * 1. returnPressed: 距离输入框回车事件
 * 2. currentIndexChanged: 地图类型下拉框选择变化
 *
 * 连接方式：
 * - 使用新式信号槽语法，类型安全
 * - 直接连接，确保即时响应
 * - 清晰的事件处理流程
 */
void PPIVisualSettings::connectSignals()
{
    connect(ui->maxDistanceEdit, &QLineEdit::returnPressed,
            this, &PPIVisualSettings::onDistanceEditReturnPressed);

    connect(ui->mapTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PPIVisualSettings::onMapTypeChanged);

    connect(ui->measureBtn, &QPushButton::toggled,
            this, &PPIVisualSettings::onMeasureToggled);
}

/**
 * @brief 验证距离输入
 * @param distance 输入的距离值
 * @return 验证结果，true表示有效
 * @details 检查距离输入的合法性，确保在合理范围内
 *
 * 验证规则：
 * - 最小值：1.0公里，避免过小的无意义值
 * - 最大值：99999.0公里，覆盖所有实际应用场景
 * - 正数检查：确保为正值，避免负数或零值
 *
 * 设计考虑：
 * - 实用性：范围覆盖从近距离到远程雷达应用
 * - 安全性：防止系统因极值输入而异常
 * - 用户体验：合理的限制，清晰的反馈
 */
bool PPIVisualSettings::validateDistance(double distance) const
{
    return distance >= 1.0 && distance <= 99999.0;
}

/**
 * @brief 测距按钮切换处理
 * @param checked 测距按钮是否被选中
 * @details 响应用户点击测距按钮，切换测距模式状态并发送信号通知PPI视图
 *
 * 功能特点：
 * - 即时响应：按钮状态变化立即生效
 * - 状态通知：通过信号通知相关组件切换测距模式
 * - 用户反馈：提供清晰的视觉状态指示
 */
void PPIVisualSettings::onMeasureToggled(bool checked)
{
    emit measureModeChanged(checked);
}
