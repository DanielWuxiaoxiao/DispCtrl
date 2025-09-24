/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-24 11:22:47
 * @Description: 
 */
#include "azelrangewidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIntValidator>
#include <QtMath>
#include <QLabel>
#include <QPushButton>

namespace {
// 默认值，可通过config.toml中的polarDisp.elevationRange和polarDisp.azimuthRange配置
constexpr int EL_MIN = -45;  // 可用CF_INS.elevationRange("min", EL_MIN)
constexpr int EL_MAX =  45;  // 可用CF_INS.elevationRange("max", EL_MAX)
constexpr int AZ_MIN =   0;  // 可用CF_INS.azimuthRange("min", AZ_MIN)
constexpr int AZ_MAX = 360;  // 可用CF_INS.azimuthRange("max", AZ_MAX)
}

AzElRangeWidget::AzElRangeWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("AzElRangeWidget");
    // 移除可能导致背景问题的属性，让widget继承父widget的背景
    // setAttribute(Qt::WA_TranslucentBackground, true);
    // setAutoFillBackground(false);
    // setAttribute(Qt::WA_NoSystemBackground, true);

    // —— 下方四个数值编辑框 —— //
    edAzMin = new QLineEdit(QString::number(mAzMin), this);
    edAzMin->setToolTip("最小方位角(-180°~359°)");

    edAzMax = new QLineEdit(QString::number(mAzMax), this);
    edAzMax->setToolTip("最大方位角(-180°~359°)");    edElMin = new QLineEdit(QString::number(mElMin), this);
    edElMin->setToolTip("最小俯仰角(-90°~90°)");

    edElMax = new QLineEdit(QString::number(mElMax), this);
    edElMax->setToolTip("最大俯仰角(-90°~90°)");

    edAzMin->setValidator(new QIntValidator(-180, 359, edAzMin));  // 支持负数到359
    edAzMax->setValidator(new QIntValidator(-180, 359, edAzMax));  // 支持负数到359
    edElMin->setValidator(new QIntValidator(EL_MIN, EL_MAX, edElMin));
    edElMax->setValidator(new QIntValidator(EL_MIN, EL_MAX, edElMax));

    for (auto *e : {edAzMin, edAzMax, edElMin, edElMax}) {
        e->setFixedWidth(48);  // 减少宽度从64到48
        e->setAlignment(Qt::AlignCenter);
        e->setPlaceholderText("--");
        e->setObjectName("RangeLineEdit");
    }

    // 布局：上方绘图区域，下方编辑区
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(6);

    // 用一个空的 QWidget 承载绘制（本类自身绘制）
    auto *drawHolder = new QWidget(this);
    drawHolder->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    drawHolder->setAttribute(Qt::WA_TranslucentBackground, true);
    drawHolder->setStyleSheet("background-color: transparent;");
    drawHolder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//    drawHolder->setMinimumHeight(180);  // 设置绘图区域最小高度
    mainLay->addWidget(drawHolder, 1);  // 给绘图区域更多权重

    // 底部输入区
    auto *grid = new QHBoxLayout();
    grid->setSpacing(10);
    grid->setContentsMargins(20, 0, 20, 0);

    QLabel* azLabel = new QLabel("方位");
    azLabel->setToolTip("方位角范围");
    grid->addWidget(azLabel);

    grid->addWidget(edAzMin);

    QLabel* azSeparator = new QLabel("-");
    azSeparator->setToolTip("到");
    grid->addWidget(azSeparator);

    grid->addWidget(edAzMax);

    QLabel* elLabel = new QLabel("俯仰");
    elLabel->setToolTip("俯仰角范围");
    grid->addWidget(elLabel);

    grid->addWidget(edElMin);

    QLabel* elSeparator = new QLabel("-");
    elSeparator->setToolTip("到");
    grid->addWidget(elSeparator);

    grid->addWidget(edElMax);

    // 添加设置按钮
    QPushButton* settingsBtn = new QPushButton("设置", this);
    settingsBtn->setObjectName("SettingsButton");
    settingsBtn->setToolTip("打开详细角度范围设置");
    settingsBtn->setFixedSize(45, 24);
    grid->addWidget(settingsBtn);


    mainLay->addLayout(grid);

    // 统一样式，确保继承父widget背景
    setStyleSheet(R"(
                  QWidget#AzElRangeWidget {
                  background-color: transparent;
                  border: none;
                  }
                  QWidget {
                  background-color: transparent;
                  }
                  QLabel {
                  color: #a8d4c8;
                  font-family: "Microsoft YaHei";
                  font-size: 15px;  /* 增大字体从11px到13px */
                  background-color: transparent;
                  }
                  QLineEdit#RangeLineEdit {
                  color: #ffffff;
                  background-color: rgba(10,20,20,0.6);
                  border: 1px solid rgba(0,255,136,0.45);
                  border-radius: 6px;
                  padding: 4px 6px;
                  selection-background-color: rgba(0,255,136,0.35);
                  }
                  QLineEdit#RangeLineEdit:focus {
                  border: 1.5px solid #00ff88;
                  }
                  QPushButton#SettingsButton {
                  color: #ffffff;
                  background-color: rgba(0,100,70,0.7);
                  border: 1px solid rgba(0,255,136,0.6);
                  border-radius: 4px;
                  padding: 2px 6px;
                  font-family: "Microsoft YaHei";
                  font-size: 11px;
                  }
                  QPushButton#SettingsButton:hover {
                  background-color: rgba(0,255,136,0.2);
                  border: 1.5px solid #00ff88;
                  }
                  QPushButton#SettingsButton:pressed {
                  background-color: rgba(0,255,136,0.4);
                  }
                  )");

    connectEditors();
}

void AzElRangeWidget::connectEditors()
{
    auto onAz = [this]{
        int a1 = clamp(edAzMin->text().toInt(), -180, 359);  // 支持负数输入
        int a2 = clamp(edAzMax->text().toInt(), -180, 359);  // 支持负数输入
        setAzRange(a1, a2);
    };
    auto onEl = [this]{
        int e1 = clamp(edElMin->text().toInt(), EL_MIN, EL_MAX);
        int e2 = clamp(edElMax->text().toInt(), EL_MIN, EL_MAX);
        setElRange(e1, e2);
    };
    connect(edAzMin, &QLineEdit::editingFinished, this, onAz);
    connect(edAzMax, &QLineEdit::editingFinished, this, onAz);
    connect(edElMin, &QLineEdit::editingFinished, this, onEl);
    connect(edElMax, &QLineEdit::editingFinished, this, onEl);
}

int AzElRangeWidget::norm360(int d)
{
    int x = d % 360;
    if (x < 0) x += 360;
    return x;
}
int AzElRangeWidget::clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

void AzElRangeWidget::setAzRange(int minDeg, int maxDeg)
{
    mAzMin = norm360(minDeg);
    mAzMax = norm360(maxDeg);
    syncEditors();
    update();
    emit azRangeChanged(mAzMin, mAzMax);
}
void AzElRangeWidget::setElRange(int minDeg, int maxDeg)
{
    mElMin = clamp(minDeg, EL_MIN, EL_MAX);
    mElMax = clamp(maxDeg, EL_MIN, EL_MAX);
    if (mElMin > mElMax) std::swap(mElMin, mElMax);
    syncEditors();
    update();
    emit elRangeChanged(mElMin, mElMax);
}
void AzElRangeWidget::syncEditors()
{
    // 使用显示角度格式更新编辑框
    edAzMin->setText(QString::number(toDisplayAngle(mAzMin)));
    edAzMax->setText(QString::number(toDisplayAngle(mAzMax)));
    edElMin->setText(QString::number(mElMin));
    edElMax->setText(QString::number(mElMax));
}

int AzElRangeWidget::toDisplayAngle(int internalDeg)
{
    // 将0-359度的内部角度转换为显示角度
    // 如果角度大于180，显示为负数（如350°显示为-10°）
    if (internalDeg > 180) {
        return internalDeg - 360;
    }
    return internalDeg;
}

double AzElRangeWidget::azToThetaRad(int azDeg)
{
    // 北=0°, 顺时针增加 -> 数学角(以x轴正向为0°, 逆时针为正，屏幕上y向上为正)
    // θ = 90° - az
    return qDegreesToRadians(90.0 - double(azDeg));
}

void AzElRangeWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 整体布局：左部分是圆盘，右部分是俯仰角条
    const int W = width();
    const int H = height();
    const int gap = 8;

    // 优化布局比例，给俯仰角条合适的空间
    const int barW = qMax(60, W / 5);  // 俯仰角条宽度
    const int dialSize = qMin(W - barW - gap*1, H - 40);  // 圆盘大小
    const QRect rcDial(gap, gap, dialSize, dialSize);

    // 俯仰角条高度与圆盘高度相近，只稍微长一点
    const int barTop = rcDial.top();
    const int barHeight = dialSize + 30;  // 比圆盘稍长30像素
    const QRect rcBar(W - barW - gap, barTop, barW, barHeight);

    drawAzDial(p, rcDial);
    drawElBar (p, rcBar);
}

void AzElRangeWidget::drawAzDial(QPainter &p, const QRect &rcDial)
{
    const QPointF C = rcDial.center();
    const int R_outer = qMin(rcDial.width(), rcDial.height())/2 - 2;
    const int R_ring2 = R_outer - 18;   // 绿色弧线所在近外圈
    const int R_ring3 = R_ring2 - 18;   // 方向字母圈
    const int R_ticks = R_outer - 4;

    // —— 多重圆环 —— //
    QPen ringPen(QColor(160, 190, 185, 160), 1.2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(C, R_outer, R_outer);
    p.drawEllipse(C, R_ring2, R_ring2);
    p.drawEllipse(C, R_ring3, R_ring3);

    // —— 刻度线（每5°，10°中等，30°长） —— //
    for (int deg = 0; deg < 360; deg += 5) {
        const double th = azToThetaRad(deg);
        const double cs = std::cos(th), sn = std::sin(th);

        int len = (deg % 30 == 0) ? 10 : ((deg % 10 == 0) ? 7 : 4);
        QPointF a(C.x() + (R_ticks - len) * cs,
                  C.y() - (R_ticks - len) * sn);
        QPointF b(C.x() + R_ticks * cs,
                  C.y() - R_ticks * sn);

        QPen tp(QColor(120,150,140,180), (deg%30==0) ? 1.6 : 1.0);
        p.setPen(tp);
        p.drawLine(a,b);
    }

    // —— 内圈“点刻”（每15°一个小点） —— //
    for (int deg = 0; deg < 360; deg += 15) {
        const double th = azToThetaRad(deg);
        QPointF pt(C.x() + (R_ring3 - 8) * std::cos(th),
                   C.y() - (R_ring3 - 8) * std::sin(th));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(200, 200, 200, 160));
        p.drawEllipse(pt, 1.8, 1.8);
    }

    // —— 方向字母 —— //
    auto drawTextAt = [&](int az, const QString &txt, int r){
        const double th = azToThetaRad(az);
        QPointF pt(C.x() + r * std::cos(th),
                   C.y() - r * std::sin(th));
        QFont f("Microsoft YaHei", 10, QFont::DemiBold);  // 减小字体从12到10
        p.setFont(f);
        p.setPen(QColor(QColorConstants::Green));
        QRectF tr(pt.x()-14, pt.y()-12, 28, 24);
        p.drawText(tr, Qt::AlignCenter, txt);
    };
    drawTextAt(0,   "N", R_ring3);
    drawTextAt(90,  "E", R_ring3);
    drawTextAt(180, "S", R_ring3);
    drawTextAt(270, "W", R_ring3);

    // —— 数字角度刻度（每30°：0,30,...,330） —— //
    auto drawDegLabel = [&](int az){
        const double th = azToThetaRad(az);
        const int r = R_outer - 22; // 在外圈内一点
        QPointF pt(C.x() + r * std::cos(th),
                   C.y() - r * std::sin(th));
        QFont f("Microsoft YaHei", 8);  // 减小字体从9到8
        p.setFont(f);
        p.setPen(QColor(150, 190, 180, 220));
        QRectF tr(pt.x()-14, pt.y()-9, 28, 18);
        p.drawText(tr, Qt::AlignCenter, QString::number(az));
    };
    for (int d=0; d<360; d+=30) drawDegLabel(d);

    // —— 绿色弧线（严格覆盖 AzMin→AzMax，顺时针） —— //
    // —— 绿色弧线（AzMin → AzMax，起点=AzMin） —— //
    int spanCWdeg = cwSpan(mAzMin, mAzMax);         // 0..360
    QRectF arcRect(C.x()-R_ring2+4, C.y()-R_ring2+4,
                   (R_ring2-4)*2, (R_ring2-4)*2);

    // 关键两行：把 start 从 AzMax 改为 AzMin；span 仍为负表示顺时针
    int startDeg16 = int((90 - mAzMin) * 16);  // 原来是 (90 - mAzMax)*16
    int spanDeg16  = - spanCWdeg * 16;         // 负值=顺时针

    QPen arcPen(QColor(0,255,136,200), 8, Qt::SolidLine, Qt::RoundCap);
    p.setPen(arcPen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect, startDeg16, spanDeg16);


    // —— 两条绿色虚线（范围边界）+ 黄色箭头 —— //
    auto drawRay = [&](int az, int lenR, bool dashed, bool withArrow){
        const double th = azToThetaRad(az);
        QPointF a = C;
        QPointF b(C.x() + lenR * std::cos(th),
                  C.y() - lenR * std::sin(th));
        QPen pen(QColor(0,255,136,220), 1.6);
        if (dashed) pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.drawLine(a,b);

        if (withArrow) {
            const double ah = 10.0, aw = 6.0;
            QPointF tip = b;
            QPointF left (tip.x() - ah*std::cos(th) - aw*std::cos(th+M_PI_2),
                          tip.y() + ah*std::sin(th) - aw*std::sin(th+M_PI_2));
            QPointF right(tip.x() - ah*std::cos(th) - aw*std::cos(th-M_PI_2),
                          tip.y() + ah*std::sin(th) - aw*std::sin(th-M_PI_2));
            QPolygonF tri; tri << tip << left << right;
            p.setBrush(QColor(255,215,0,230));  // 亮黄色
            p.setPen(Qt::NoPen);
            p.drawPolygon(tri);
        }
    };
    drawRay(mAzMin, R_ring2, true,  true);
    drawRay(mAzMax, R_ring2, true,  true);

    // —— 中心“亮黄三角”指向扫描中心 —— //
    {
        int mid = (mAzMin + cwSpan(mAzMin, mAzMax)/2) % 360;  // 顺时针中点
        const double th = azToThetaRad(mid);

        const double tipR = R_ring3 - 8;   // 尖端距离中心
        const double baseBack = 12.0;      // 底边后移
        const double halfW = 8.0;          // 底边半宽

        QPointF tip ( C.x() + tipR * std::cos(th),
                      C.y() - tipR * std::sin(th) );
        QPointF baseCenter( tip.x() - baseBack*std::cos(th),
                            tip.y() + baseBack*std::sin(th) );
        // 垂直向量
        QPointF n( -std::sin(th), -std::cos(th) );
        QPointF b1 = baseCenter + halfW * n;
        QPointF b2 = baseCenter - halfW * n;

        QPolygonF tri; tri << tip << b1 << b2;
        p.setBrush(QColor(255, 230, 40, 255));      // 亮黄
        p.setPen(QPen(QColor(255, 255, 160, 220), 1.0));
        p.drawPolygon(tri);
    }

    // —— 中间数值文本 —— //
    {
        QString txt = QString("%1° - %2°").arg(mAzMin).arg(mAzMax);
        QFont f("Microsoft YaHei", 12, QFont::DemiBold);  // 减小字体从14到12
        p.setFont(f);
        p.setPen(QColor("#e3fff6"));
        p.drawText(QRectF(C.x()-80, C.y()-14, 160, 28), Qt::AlignCenter, txt);
    }
}


void AzElRangeWidget::drawElBar(QPainter &p, const QRect &rcBar)
{
    // 背景透明；画一个多彩条+刻度+两个游标
    const int x = rcBar.x();
    const int y = rcBar.y();
    const int w = rcBar.width();
    const int h = rcBar.height();

    // 渐变条区域
    QRect gradRect(x + w/2 - 6, y + 8, 12, h - 16);

    // 三段：蓝(低) - 绿(中) - 红(高)
    QLinearGradient g(gradRect.topLeft(), gradRect.bottomLeft());
    g.setColorAt(0.00, QColor(255, 60, 60, 220));  // 顶端（+45）红
    g.setColorAt(0.50, QColor(  0,255,136,220));
    g.setColorAt(1.00, QColor( 50, 90,255,220));    // 底端（-45）蓝
    p.fillRect(gradRect, g);

    // 外框
    p.setPen(QPen(QColor(0,255,136,180), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(gradRect.adjusted(-6, -6, +6, +6), 6, 6);

    // 刻度：-45..+45，每 5°
    auto mapEl = [&](int el){ // +45 在顶部
        double t = (double)(EL_MAX - el) / (EL_MAX - EL_MIN); // 0..1
        return gradRect.top() + t * gradRect.height();
    };
    for (int d = EL_MIN; d <= EL_MAX; d += 5) {
        int yy = (int)std::round(mapEl(d));
        int len = (d % 15 == 0) ? 10 : 6;
        QPen tp(QColor(120,150,140,200), (d%15==0) ? 1.6 : 1.0);
        p.setPen(tp);
        p.drawLine(gradRect.left() - len - 4, yy, gradRect.left() - 4, yy);

        if (d % 15 == 0) {
            p.setPen(QColor("#a8d4c8"));
            QFont f("Microsoft YaHei", 9);  // 减小字体从10到9
            p.setFont(f);
            p.drawText(QRect(gradRect.left() - 44, yy - 9, 38, 18),
                       Qt::AlignRight|Qt::AlignVCenter,
                       QString::number(d));
        }
    }

    // 两个标记（最小、最大）
    auto drawThumb = [&](int el, bool top) {
        int yy = (int)std::round(mapEl(el));
        QRect r(gradRect.right() + 8, yy - 10, 32, 20);
        QPainterPath path;
        path.addRoundedRect(r, 6, 6);
        p.fillPath(path, QColor(10,20,20,180));
        p.setPen(QPen(QColor(0,255,136,220), 1.4));
        p.drawPath(path);
        // 指向条
        p.setPen(QPen(QColor(0,255,136,220), 1.4));
        p.drawLine(gradRect.right()+2, yy, gradRect.right()+8, yy);
        // 文本
        p.setPen(QColor("#e3fff6"));
        QFont f("Microsoft YaHei", 9, QFont::DemiBold);  // 减小字体从10到9
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, QString::number(el));
    };
    drawThumb(mElMin, false);
    drawThumb(mElMax, true);

    // 标题
    //    {
    //        QFont f("Microsoft YaHei", 12, QFont::DemiBold);
    //        p.setFont(f);
    //        p.setPen(QColor("#a8d4c8"));
    //        p.drawText(QRect(x, y-2, w, 24), Qt::AlignHCenter|Qt::AlignTop, "仰角范围");
    //    }
}

/**
 * @brief 计算从a1到a2的顺时针角度跨度
 * @param a1 起始角度(0-359)
 * @param a2 结束角度(0-359)
 * @return 顺时针跨度角度(0-360)
 */
int AzElRangeWidget::cwSpan(int a1, int a2)
{
    // 确保角度在0-359范围内
    a1 = norm360(a1);
    a2 = norm360(a2);

    // 计算顺时针跨度
    if (a2 >= a1) {
        return a2 - a1;
    } else {
        return 360 - a1 + a2;
    }
}
