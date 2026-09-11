/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-09-11 19:22:04
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-11 22:04:55
 * @Description: 
 */
#include "commandcontrolwindow.h"

#include "commandcontrolmodule.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString deviceIdText(quint32 id)
{
    return QStringLiteral("0x") + QString::number(id, 16).rightJustified(8, QLatin1Char('0')).toUpper();
}

QString utcText(const QDateTime& time)
{
    return time.isValid() ? time.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                          : QStringLiteral("-");
}

QTableWidgetItem* item(const QString& text)
{
    auto* result = new QTableWidgetItem(text);
    result->setFlags(result->flags() & ~Qt::ItemIsEditable);
    return result;
}

}  // namespace

CommandControlWindow::CommandControlWindow(CommandControlModule* module)
    : QDialog(nullptr), m_module(module)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("总控通信"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    // 使用全局 darkstyle.qss 中的专属选择器，避免通用 QPushButton/QTabWidget 规则覆盖本窗口。
    setObjectName(QStringLiteral("CommandControlWindow"));
    resize(1000, 620);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(1, 1, 1, 1);
    root->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("CommandControlTitleBar"));
    m_titleBar->setFixedHeight(38);
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(14, 0, 6, 0);
    auto* titleLabel = new QLabel(QStringLiteral("总控通信"), m_titleBar);
    titleLabel->setObjectName(QStringLiteral("CommandControlTitleLabel"));
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* titleCloseButton = new QPushButton(QStringLiteral("×"), m_titleBar);
    titleCloseButton->setObjectName(QStringLiteral("CommandControlTitleCloseButton"));
    titleCloseButton->setFixedSize(28, 28);
    titleCloseButton->setToolTip(QStringLiteral("关闭"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(titleCloseButton);
    m_titleBar->installEventFilter(this);
    root->addWidget(m_titleBar);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("CommandControlContent"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(18, 16, 18, 16);
    contentLayout->setSpacing(12);
    auto* tabs = new QTabWidget(content);
    tabs->setObjectName(QStringLiteral("CommandControlTabs"));
    contentLayout->addWidget(tabs);
    root->addWidget(content, 1);

    auto* controlPage = new QWidget(tabs);
    controlPage->setObjectName(QStringLiteral("CommandControlControlPage"));
    auto* controlLayout = new QVBoxLayout(controlPage);
    controlLayout->setContentsMargins(16, 16, 16, 16);
    controlLayout->setSpacing(12);
    m_statusLabel = new QLabel(controlPage);
    m_statusLabel->setObjectName(QStringLiteral("CommandControlStatusLabel"));
    m_statusLabel->setWordWrap(true);
    controlLayout->addWidget(m_statusLabel);
    m_autoReportCheckBox = new QCheckBox(QStringLiteral("自动上报识别为无人机的正常航迹"), controlPage);
    m_autoReportCheckBox->setObjectName(QStringLiteral("CommandControlAutoReportCheck"));
    controlLayout->addWidget(m_autoReportCheckBox);
    auto* loginLayout = new QHBoxLayout();
    loginLayout->setSpacing(10);
    auto* loginButton = new QPushButton(QStringLiteral("登陆"), controlPage);
    loginButton->setObjectName(QStringLiteral("CommandControlLoginButton"));
    auto* logoutButton = new QPushButton(QStringLiteral("退出登陆"), controlPage);
    logoutButton->setObjectName(QStringLiteral("CommandControlLogoutButton"));
    loginLayout->addWidget(loginButton);
    loginLayout->addWidget(logoutButton);
    loginLayout->addStretch();
    controlLayout->addLayout(loginLayout);
    controlLayout->addStretch();
    tabs->addTab(controlPage, QStringLiteral("通信控制"));

    auto* peerPage = new QWidget(tabs);
    peerPage->setObjectName(QStringLiteral("CommandControlPeerPage"));
    auto* peerLayout = new QVBoxLayout(peerPage);
    peerLayout->setContentsMargins(16, 16, 16, 16);
    m_peerTable = new QTableWidget(peerPage);
    m_peerTable->setObjectName(QStringLiteral("CommandControlPeerTable"));
    m_peerTable->setColumnCount(8);
    m_peerTable->setHorizontalHeaderLabels({QStringLiteral("设备 ID"), QStringLiteral("角色"),
                                             QStringLiteral("IP"), QStringLiteral("端口"),
                                             QStringLiteral("最后心跳"), QStringLiteral("健康"),
                                             QStringLiteral("辐射"), QStringLiteral("链路状态")});
    m_peerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_peerTable->setAlternatingRowColors(true);
    m_peerTable->verticalHeader()->setVisible(false);
    m_peerTable->horizontalHeader()->setStretchLastSection(true);
    peerLayout->addWidget(m_peerTable);
    tabs->addTab(peerPage, QStringLiteral("组播设备状态"));

    auto* recordPage = new QWidget(tabs);
    recordPage->setObjectName(QStringLiteral("CommandControlRecordPage"));
    auto* recordLayout = new QVBoxLayout(recordPage);
    recordLayout->setContentsMargins(16, 16, 16, 16);
    recordLayout->setSpacing(12);
    auto* recordActions = new QHBoxLayout();
    recordActions->setSpacing(10);
    auto* replayButton = new QPushButton(QStringLiteral("选择记录并回放"), recordPage);
    replayButton->setObjectName(QStringLiteral("CommandControlReplayButton"));
    auto* stopReplayButton = new QPushButton(QStringLiteral("停止回放"), recordPage);
    stopReplayButton->setObjectName(QStringLiteral("CommandControlStopReplayButton"));
    recordActions->addWidget(replayButton);
    recordActions->addWidget(stopReplayButton);
    recordActions->addStretch();
    recordLayout->addLayout(recordActions);
    m_recordTable = new QTableWidget(recordPage);
    m_recordTable->setObjectName(QStringLiteral("CommandControlRecordTable"));
    m_recordTable->setColumnCount(8);
    m_recordTable->setHorizontalHeaderLabels({QStringLiteral("方向"), QStringLiteral("记录时间"),
                                               QStringLiteral("设备 ID"), QStringLiteral("本机批号"),
                                               QStringLiteral("经度"), QStringLiteral("纬度"),
                                               QStringLiteral("高度(m)"), QStringLiteral("更新方式")});
    m_recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordTable->setAlternatingRowColors(true);
    m_recordTable->verticalHeader()->setVisible(false);
    m_recordTable->horizontalHeader()->setStretchLastSection(true);
    recordLayout->addWidget(m_recordTable);
    tabs->addTab(recordPage, QStringLiteral("DDA4 记录与回放"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->setObjectName(QStringLiteral("CommandControlButtonBox"));
    buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("CommandControlCloseButton"));
    root->addWidget(buttons);

    connect(loginButton, &QPushButton::clicked, m_module, &CommandControlModule::requestLogin);
    connect(logoutButton, &QPushButton::clicked, m_module, &CommandControlModule::requestLogout);
    connect(m_autoReportCheckBox, &QCheckBox::toggled, m_module, &CommandControlModule::setAutoReportEnabled);
    connect(replayButton, &QPushButton::clicked, this, &CommandControlWindow::selectReplayFile);
    connect(stopReplayButton, &QPushButton::clicked, m_module, &CommandControlModule::stopReplay);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(titleCloseButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_module, &CommandControlModule::statusChanged, this, &CommandControlWindow::refreshStatus);
    connect(m_module, &CommandControlModule::autoReportChanged, this, &CommandControlWindow::refreshStatus);
    connect(m_module, &CommandControlModule::peersChanged, this, &CommandControlWindow::refreshPeers);
    connect(m_module, &CommandControlModule::recordsChanged, this, &CommandControlWindow::refreshRecords);

    refreshStatus();
    refreshPeers();
    refreshRecords();
}

bool CommandControlWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_titleBar) {
        return QDialog::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_titleBarDragOffset = mouseEvent->globalPos() - frameGeometry().topLeft();
            return true;
        }
    } else if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            move(mouseEvent->globalPos() - m_titleBarDragOffset);
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CommandControlWindow::refreshStatus()
{
    if (!m_module) {
        return;
    }
    m_statusLabel->setText(QStringLiteral("状态：%1\n会话记录：%2")
                           .arg(m_module->statusText())
                           .arg(m_module->sessionRecordPath()));
    m_autoReportCheckBox->blockSignals(true);
    m_autoReportCheckBox->setChecked(m_module->isAutoReportEnabled());
    m_autoReportCheckBox->blockSignals(false);
}

void CommandControlWindow::refreshPeers()
{
    if (!m_module) {
        return;
    }
    const QList<CommandControlPeer> peers = m_module->peers();
    m_peerTable->setRowCount(peers.size());
    int row = 0;
    for (const CommandControlPeer& peer : peers) {
        m_peerTable->setItem(row, 0, item(deviceIdText(peer.deviceId)));
        m_peerTable->setItem(row, 1, item(peer.role));
        m_peerTable->setItem(row, 2, item(peer.ip));
        m_peerTable->setItem(row, 3, item(QString::number(peer.port)));
        m_peerTable->setItem(row, 4, item(utcText(peer.lastLinkCheck)));
        m_peerTable->setItem(row, 5, item(peer.lastEquipmentStatus.isValid()
            ? QStringLiteral("0x%1").arg(peer.healthStatus, 2, 16, QChar('0')) : QStringLiteral("-")));
        m_peerTable->setItem(row, 6, item(peer.lastEquipmentStatus.isValid()
            ? QString::number(peer.radiationStatus) : QStringLiteral("-")));
        m_peerTable->setItem(row, 7, item(peer.online ? QStringLiteral("在线") : QStringLiteral("离线")));
        ++row;
    }
    m_peerTable->resizeColumnsToContents();
}

void CommandControlWindow::refreshRecords()
{
    if (!m_module) {
        return;
    }
    const QVector<CommandControlRecord>& records = m_module->recentRecords();
    m_recordTable->setRowCount(records.size());
    for (int row = 0; row < records.size(); ++row) {
        const CommandControlRecord& record = records.at(row);
        const auto& track = record.track;
        m_recordTable->setItem(row, 0, item(record.outbound ? QStringLiteral("上报") : QStringLiteral("接收")));
        m_recordTable->setItem(row, 1, item(QDateTime::fromMSecsSinceEpoch(record.observedUtcMs, Qt::UTC)
                                              .toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))));
        m_recordTable->setItem(row, 2, item(deviceIdText(track.deviceId)));
        m_recordTable->setItem(row, 3, item(QString::number(track.localBatch)));
        m_recordTable->setItem(row, 4, item(QString::number(track.longitudeE7 / 1e7, 'f', 7)));
        m_recordTable->setItem(row, 5, item(QString::number(track.latitudeE7 / 1e7, 'f', 7)));
        m_recordTable->setItem(row, 6, item(QString::number(track.altitudeM)));
        m_recordTable->setItem(row, 7, item(QStringLiteral("0x%1").arg(track.updateMethod, 2, 16, QChar('0'))));
    }
    m_recordTable->resizeColumnsToContents();
}

void CommandControlWindow::selectReplayFile()
{
    if (!m_module) {
        return;
    }
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择 DDA4 会话记录"), m_module->sessionRecordPath(),
        QStringLiteral("DDA4 记录 (*.jsonl);;所有文件 (*)"));
    if (!filePath.isEmpty()) {
        m_module->startReplay(filePath);
    }
}
