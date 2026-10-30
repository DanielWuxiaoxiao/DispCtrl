#include "radarpanelpositiondialog.h"
#pragma pack(push, 8)
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QDateTime>
#include "Basic/uifontscale.h"
#include "cusWidgets/cuswindow.h"
#pragma pack(pop)

#pragma pack(push, 8)
namespace {
// Multi-panel diagnostics use the same high-resolution heading field as the table.
class BitYawPlot final : public QWidget {
public:
    explicit BitYawPlot(QWidget* parent) : QWidget(parent) {
        setMinimumHeight(ScaleHelper::uiScaled(260));
    }
    void setSnapshot(const RadarPanelState::Snapshot& s) { m_data = s; update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor("#111e1b"));
        p.setFont(UiFontScale::pointFont(p.font().family(), 10));
        const QRectF area = QRectF(rect()).adjusted(62, 28, -24, -32);
        if (area.width() <= 0 || area.height() <= 0) return;
        p.setPen(QColor("#92bdb4"));
        p.drawText(8, 18, QStringLiteral("解锁阵面偏航(°)  青色：原始值  黄色：最新100次滑动平均"));
        const auto series = RadarPanelState::yawPlotData(m_data);
        for (int tick = 0; tick <= 4; ++tick) {
            const double degree = series.lower + (series.upper - series.lower) * tick / 4.0;
            const double y = area.bottom() - area.height() * tick / 4.0;
            p.setPen(QColor("#2d4941")); p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
            p.setPen(QColor("#b9d8d0")); p.drawText(QRectF(0, y - 9, 55, 18), Qt::AlignRight, QString::number(degree, 'f', 2));
        }
        p.drawRect(area);
        if (!m_data.historyCount) { p.drawText(area, Qt::AlignCenter, QStringLiteral("等待有效BIT偏航")); return; }
        const auto first = m_data.history[0].frame;
        const auto last = m_data.history[m_data.historyCount - 1].frame;
        const auto& latest = m_data.history[m_data.historyCount - 1];
        p.drawText(QRectF(area.left(), 3, area.width(), 22), Qt::AlignRight,
            QStringLiteral("最新 ID%1：%2°").arg(latest.id).arg(latest.yaw, 0, 'f', 2));
        QPolygonF points, averages;
        for (std::size_t i = 0; i < m_data.historyCount; ++i) {
            const auto& s = m_data.history[i];
            points << QPointF(area.left() + area.width() * double(s.frame - first) / qMax<quint64>(1, last - first),
                              area.bottom() - area.height() * (series.raw[i] - series.lower) / (series.upper - series.lower));
            averages << QPointF(points.last().x(),
                area.bottom() - area.height() * (series.average[i] - series.lower) / (series.upper - series.lower));
        }
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#66ffcc"), 2));
        if (points.size() == 1) p.drawEllipse(points[0], 3, 3); else p.drawPolyline(points);
        p.setPen(QPen(QColor("#ffcc66"), 2));
        if (averages.size() == 1) p.drawEllipse(averages[0], 2, 2); else p.drawPolyline(averages);
        p.setPen(QColor("#b9d8d0"));
        p.drawText(QPointF(area.left(), area.bottom() + 20), QString::number(first));
        p.drawText(QRectF(area.right() - 150, area.bottom() + 3, 150, 22), Qt::AlignRight,
                   QStringLiteral("%1 帧").arg(last));
    }
private:
    RadarPanelState::Snapshot m_data;
};
const int precision[] = {9, 9, 2, 3, 3, 2};
const QStringList fields{QStringLiteral("经度(°)"), QStringLiteral("纬度(°)"), QStringLiteral("高程(m)"),
    QStringLiteral("航向角(°)"), QStringLiteral("倾角(°)"), QStringLiteral("横滚角(°)")};
}
#pragma pack(pop)

RadarPanelPositionDialog::RadarPanelPositionDialog(const RadarPanelValues& values,
    const std::array<bool, 4>& locked, Actions actions, QWidget* parent)
    : QDialog(parent), m_actions(std::move(actions))
{
    setWindowTitle(QStringLiteral("雷达阵面经纬高/姿态下发"));
    setWindowModality(Qt::NonModal);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
    setObjectName(QStringLiteral("panelPositionDialog"));
    setStyleSheet(UiFontScale::style(
        "QDialog#panelPositionDialog {background:#101818;}"
        "QScrollArea,QWidget#panelPositionViewport {background:#101818;border:0;}"
        "QWidget#panelPositionContent {background:#111e1b;border:1px solid #42665c;border-radius:6px;}"
        "QLabel {background:transparent;color:#d9ffff;font-size:16px;}"
        "QLabel#panelPositionTitle {color:#66ffcc;font-size:22px;font-weight:bold;}"
        "QLineEdit {background:#142d28;color:#66ffcc;border:1px solid #66aa99;padding:6px;font-size:16px;}"
        "QLineEdit[readOnly=\"true\"] {background:#253a35;color:#a4e4cc;}"
        "QPushButton {background:#123630;color:#66ffcc;border:1px solid #66aa99;padding:7px;font-size:16px;}"
        "QPushButton:hover {background:#205247;}"
        "QPushButton:disabled {background:#25332e;color:#a2b8b0;border-color:#52665f;}"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(16, 14, 16, 14);
    outer->setSpacing(12);
    auto* title = new QLabel(QStringLiteral("雷达阵面经纬高 / 姿态下发"), this);
    title->setObjectName(QStringLiteral("panelPositionTitle"));
    outer->addWidget(title);
    auto* scroll = new QScrollArea(this);
    scroll->viewport()->setObjectName(QStringLiteral("panelPositionViewport"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* contents = new QWidget(scroll);
    contents->setObjectName(QStringLiteral("panelPositionContent"));
    contents->setAttribute(Qt::WA_StyledBackground, true);
    auto* grid = new QGridLayout(contents);
    grid->setContentsMargins(12, 12, 12, 12);
    grid->setHorizontalSpacing(10); grid->setVerticalSpacing(12);
    grid->addWidget(new QLabel(QStringLiteral("阵面"), contents), 0, 0);
    for (int field = 0; field < RadarReference::Count; ++field) {
        auto* label = new QLabel(fields[field], contents);
        if (field == RadarReference::Yaw) label->setToolTip(QStringLiteral("多面阵表格与曲线均取扩展航向heading（0.001°）；表格按各阵面独立平均，曲线在解锁阵面切换时连续累计。"));
        grid->addWidget(label, 0, field + 1);
    }
    grid->addWidget(new QLabel(QStringLiteral("状态"), contents), 0, 7);
    for (int id = 0; id < 4; ++id) {
        grid->addWidget(new QLabel(QStringLiteral("阵面%1 / ID%2").arg(id + 1).arg(id), contents), id + 1, 0);
        for (int field = 0; field < RadarReference::Count; ++field) {
            auto* edit = new QLineEdit(contents);
            edit->setObjectName(QStringLiteral("panel%1Field%2").arg(id).arg(field));
            edit->setMinimumWidth(ScaleHelper::uiScaled(field < 2 ? 155 : 90));
            m_edits[id][field] = edit;
            grid->addWidget(edit, id + 1, field + 1);
        }
        m_status[id] = new QLabel(contents);
        m_lock[id] = new QPushButton(QStringLiteral("锁定"), contents);
        m_unlock[id] = new QPushButton(QStringLiteral("解除锁定"), contents);
        m_lock[id]->setObjectName(QStringLiteral("lockPanel%1").arg(id));
        m_unlock[id]->setObjectName(QStringLiteral("unlockPanel%1").arg(id));
        grid->addWidget(m_status[id], id + 1, 7);
        grid->addWidget(m_lock[id], id + 1, 8);
        grid->addWidget(m_unlock[id], id + 1, 9);
        fillRow(id, values[id]);
        setLocked(id, locked[id]);
        connect(m_lock[id], &QPushButton::clicked, this, [this, id] { lockRow(id); });
        connect(m_unlock[id], &QPushButton::clicked, this, [this, id] { unlockRow(id); });
    }
    scroll->setWidget(contents);
    // Only user edits propagate; BIT refreshes/unlock refills must not rewrite other rows.
    for (int id = 0; id < 4; ++id) {
        for (int field = RadarReference::Longitude; field <= RadarReference::Altitude; ++field) {
            connect(m_edits[id][field], &QLineEdit::textEdited, this,
                [this, id, field](const QString& text) {
                    if (m_edits[id][field]->isReadOnly()) return;
                    for (int other = 0; other < 4; ++other)
                        if (other != id && !m_edits[other][field]->isReadOnly())
                            m_edits[other][field]->setText(text);
                });
        }
    }
    scroll->setMinimumHeight(ScaleHelper::uiScaled(250));
    outer->addWidget(scroll);
    auto* note = new QLabel(QStringLiteral("默认全部锁定；每次仅解锁一个阵面接收BIT。切换阵面时曲线及100次滑动平均继续，不足100次按实际数量。锁定后可编辑，确定下发后应用。"), this);
    note->setWordWrap(true);
    outer->addWidget(note);
    auto* received = new QLabel(this);
    received->setObjectName(QStringLiteral("panelBitReceived"));
    received->setWordWrap(true);
    outer->addWidget(received);
    auto* plot = new BitYawPlot(this);
    outer->addWidget(plot, 1);
    if (m_actions.live) {
        const auto refresh = [this, received, plot] {
            const auto s = m_actions.live();
            QStringList times;
            for (int id = 0; id < 4; ++id) {
                if (m_locked[id] != s.locked[id]) {
                    fillRow(id, s.values[id]);
                    setLocked(id, s.locked[id]);
                }
                if (!m_locked[id]) fillRow(id, s.values[id]);
                times << QStringLiteral("阵面%1 / ID%2：%3").arg(id + 1).arg(id).arg(s.receivedMs[id]
                    ? QDateTime::fromMSecsSinceEpoch(s.receivedMs[id]).toString("yyyy-MM-dd HH:mm:ss.zzz")
                    : QStringLiteral("未收到BIT"));
            }
            received->setText(times.join(QStringLiteral("    ")));
            plot->setSnapshot(s);
        };
        auto* timer = new QTimer(this);
        timer->setInterval(200);
        connect(timer, &QTimer::timeout, this, refresh);
        refresh(); timer->start();
    }
    m_error = new QLabel(this);
    m_error->setStyleSheet(UiFontScale::style("color:#ff9999;"));
    m_error->setWordWrap(true); m_error->hide();
    outer->addWidget(m_error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定下发"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("submitPanels"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { submit(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
    auto* screen = parent ? QGuiApplication::screenAt(parent->mapToGlobal(parent->rect().center())) : nullptr;
    if (!screen) screen = QGuiApplication::primaryScreen();
    const int widthLimit = screen ? qMax(400, screen->availableGeometry().width() - 60) : 1280;
    if (screen && screen->availableGeometry().height() < ScaleHelper::uiScaled(800)) {
        scroll->setMinimumHeight(ScaleHelper::uiScaled(100));
        plot->setMinimumHeight(ScaleHelper::uiScaled(180));
        outer->setSpacing(8);
    }
    resize(qMin(ScaleHelper::uiScaled(1320), widthLimit), qMin(ScaleHelper::uiScaled(800),
        screen ? screen->availableGeometry().height() - 60 : 680));
}

void RadarPanelPositionDialog::attachToWindow(CusWindow* host)
{
    if (!host) return;
    setWindowFlags(Qt::Widget);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_DeleteOnClose, false); // The CusWindow owns and destroys the embedded content.
    if (auto* title = findChild<QLabel*>("panelPositionTitle")) title->hide();
    host->setContentWidget(this);
    // Defer host destruction until QDialog::done/accept/reject has returned.
    connect(this, &QDialog::finished, host, [host](int) { host->close(); }, Qt::QueuedConnection);
}

void RadarPanelPositionDialog::fillRow(int id, const RadarReference::Pose& values)
{
    for (int field = 0; field < RadarReference::Count; ++field)
        m_edits[id][field]->setText(QString::number(values[field], 'f', precision[field]));
}

bool RadarPanelPositionDialog::readRow(int id, RadarReference::Pose& values)
{
    for (int field = 0; field < RadarReference::Count; ++field) {
        bool ok = false;
        values[field] = m_edits[id][field]->text().trimmed().toDouble(&ok);
        if (!ok || !RadarReference::validUi(static_cast<RadarReference::Field>(field), values[field])) {
            m_error->setText(QStringLiteral("阵面%1（ID%2）的%3无效。经度±180，纬度/倾角/横滚±90，航向0～360，高程-1000～5000m；各项不能为空。")
                .arg(id + 1).arg(id).arg(fields[field]));
            m_error->show(); m_edits[id][field]->setFocus();
            return false;
        }
    }
    return true;
}

void RadarPanelPositionDialog::setLocked(int id, bool locked)
{
    m_locked[id] = locked;
    for (auto* edit : m_edits[id]) edit->setReadOnly(!locked);
    m_lock[id]->setEnabled(!locked); m_unlock[id]->setEnabled(locked);
    m_status[id]->setText(locked ? QStringLiteral("已锁定") : QStringLiteral("未锁定"));
    m_status[id]->setStyleSheet(UiFontScale::style(locked ? "color:#ffcc66;" : "color:#66ffcc;"));
}

void RadarPanelPositionDialog::lockRow(int id)
{
    RadarReference::Pose values{};
    if (!readRow(id, values)) return;
    if (!m_actions.lock || !m_actions.lock(id, values)) {
        m_error->setText(QStringLiteral("锁定失败，请检查输入值、锁定状态或本地文件写入权限；详情见文件日志。")); m_error->show(); return;
    }
    setLocked(id, true); m_error->hide();
}

void RadarPanelPositionDialog::unlockRow(int id)
{
    const auto values = m_actions.unlock ? m_actions.unlock(id) : std::nullopt;
    if (!values) { m_error->setText(QStringLiteral("解除锁定失败，请检查本地文件写入权限；详情见文件日志。")); m_error->show(); return; }
    fillRow(id, *values); setLocked(id, false); m_error->hide();
}

void RadarPanelPositionDialog::submit()
{
    RadarPanelValues values{};
    for (int id = 0; id < 4; ++id) if (!readRow(id, values[id])) return;
    if (!m_actions.submit || !m_actions.submit(values)) {
        m_error->setText(QStringLiteral("下发请求失败：请检查参数、资源调度初始化状态或本地文件写入权限。")); m_error->show(); return;
    }
    accept();
}
