/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-10 17:18:12
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 16:20:19
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-10 15:46:41
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-10 17:18:14
 * @Description:
 */
/**
 * @file screenrecorderwidget.cpp
 * @brief 屏幕录制与回放组件实现
 * @details 实现基于截图序列的屏幕录制和逐帧回放功能
 *
 * 技术方案：
 * - 录制：QTimer定时触发 → QScreen::grabWindow() 截取主窗口 → 保存为PNG序列
 * - 回放：加载PNG序列 → QTimer驱动逐帧显示 → QSlider可拖动定位
 * - 存储：运行目录下 recordings/ 子目录，每次录制一个子文件夹
 * - 跨平台：QScreen API 在 Windows/Linux/macOS 均可用
 *
 * @author DispCtrl Team
 * @date 2026
 */

#include "screenrecorderwidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QWindow>

#include "Basic/log.h"

// =============================================================================
// 构造与析构
// =============================================================================

ScreenRecorderWidget::ScreenRecorderWidget(QWidget* parent)
    : QWidget(parent)
    , m_isRecording(false)
    , m_captureTimer(new QTimer(this))
    , m_frameCount(0)
    , m_recordTarget(nullptr)
    , m_isPlaying(false)
    , m_playTimer(new QTimer(this))
    , m_playIndex(0)
    , m_playFromAvi(false)
{
    setupUI();
    connectSignals();
    refreshFileList();
    updateButtonStates();
}

ScreenRecorderWidget::~ScreenRecorderWidget()
{
    // 如果正在录制，强制停止
    if (m_isRecording) {
        m_captureTimer->stop();
        m_isRecording = false;
    }
    if (m_isPlaying) {
        m_playTimer->stop();
        m_isPlaying = false;
    }
}

// =============================================================================
// 公共接口
// =============================================================================

void ScreenRecorderWidget::setRecordTarget(QWidget* target)
{
    m_recordTarget = target;
}

// =============================================================================
// UI 构建
// =============================================================================

void ScreenRecorderWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // --- 状态标签 ---
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setObjectName("RecorderStatusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // --- 控制按钮行 ---
    QHBoxLayout* ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(8);

    m_recordBtn = new QPushButton("开始录屏", this);
    m_recordBtn->setObjectName("RecordBtn");

    m_playBtn = new QPushButton("回放", this);
    m_playBtn->setObjectName("PlayBtn");

    m_stopPlayBtn = new QPushButton("停止回放", this);
    m_stopPlayBtn->setObjectName("StopPlayBtn");

    m_deleteBtn = new QPushButton("删除", this);
    m_deleteBtn->setObjectName("DeleteBtn");

    m_refreshBtn = new QPushButton("刷新", this);
    m_refreshBtn->setObjectName("RefreshBtn");

    ctrlLayout->addWidget(m_recordBtn);
    ctrlLayout->addWidget(m_playBtn);
    ctrlLayout->addWidget(m_stopPlayBtn);
    ctrlLayout->addWidget(m_deleteBtn);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(ctrlLayout);

    // --- 录制质量选择行 ---
    QHBoxLayout* qualityLayout = new QHBoxLayout();
    qualityLayout->setSpacing(8);

    QLabel* qualityLabel = new QLabel("录制质量：", this);
    qualityLabel->setObjectName("RecorderQualityLabel");

    m_qualityCombo = new QComboBox(this);
    m_qualityCombo->setObjectName("RecorderQualityCombo");
    m_qualityCombo->addItem("低清 (25%)", 0.25);
    m_qualityCombo->addItem("中清 (50%)", 0.50);
    m_qualityCombo->addItem("高清 (100%)", 1.0);
    m_qualityCombo->setCurrentIndex(1); // 默认中清

    qualityLayout->addWidget(qualityLabel);
    qualityLayout->addWidget(m_qualityCombo);
    qualityLayout->addStretch();
    mainLayout->addLayout(qualityLayout);

    // --- 文件列表 ---
    QLabel* listLabel = new QLabel("历史录制：", this);
    listLabel->setObjectName("RecorderSectionLabel");
    mainLayout->addWidget(listLabel);

    m_fileList = new QListWidget(this);
    m_fileList->setObjectName("RecorderFileList");
    m_fileList->setAlternatingRowColors(true);
    m_fileList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_fileList, 1); // stretch=1

    // --- 预览区域 ---
    QLabel* previewTitle = new QLabel("预览/回放：", this);
    previewTitle->setObjectName("RecorderSectionLabel");
    mainLayout->addWidget(previewTitle);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setObjectName("RecorderPreview");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(200);
    m_previewLabel->setStyleSheet(
        "QLabel#RecorderPreview {"
        "  background-color: rgba(0,0,0,0.6);"
        "  border: 1px solid rgba(68,136,255,0.3);"
        "  border-radius: 4px;"
        "  color: #66aaff;"
        "}");
    m_previewLabel->setText("无预览");
    mainLayout->addWidget(m_previewLabel, 2); // stretch=2

    // --- 进度条行 ---
    QHBoxLayout* progressLayout = new QHBoxLayout();
    progressLayout->setSpacing(8);

    m_progressSlider = new QSlider(Qt::Horizontal, this);
    m_progressSlider->setObjectName("RecorderSlider");
    m_progressSlider->setRange(0, 0);
    m_progressSlider->setEnabled(false);

    m_frameInfoLabel = new QLabel("0/0", this);
    m_frameInfoLabel->setObjectName("FrameInfoLabel");
    m_frameInfoLabel->setFixedWidth(80);
    m_frameInfoLabel->setAlignment(Qt::AlignCenter);

    progressLayout->addWidget(m_progressSlider, 1);
    progressLayout->addWidget(m_frameInfoLabel);
    mainLayout->addLayout(progressLayout);
}

void ScreenRecorderWidget::connectSignals()
{
    connect(m_recordBtn, &QPushButton::clicked, this, &ScreenRecorderWidget::onRecordToggle);
    connect(m_playBtn, &QPushButton::clicked, this, &ScreenRecorderWidget::onPlayClicked);
    connect(m_stopPlayBtn, &QPushButton::clicked, this, &ScreenRecorderWidget::onStopPlayClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ScreenRecorderWidget::onDeleteClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ScreenRecorderWidget::refreshFileList);
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenRecorderWidget::captureFrame);
    connect(m_playTimer, &QTimer::timeout, this, &ScreenRecorderWidget::playNextFrame);
    connect(m_fileList, &QListWidget::itemSelectionChanged, this, &ScreenRecorderWidget::onSelectionChanged);
    connect(m_progressSlider, &QSlider::sliderMoved, this, &ScreenRecorderWidget::onSliderMoved);
}

void ScreenRecorderWidget::updateButtonStates()
{
    bool hasSelection = m_fileList->currentItem() != nullptr;

    m_recordBtn->setText(m_isRecording ? "停止录屏" : "开始录屏");
    m_playBtn->setEnabled(!m_isRecording && !m_isPlaying && hasSelection);
    m_stopPlayBtn->setEnabled(m_isPlaying);
    m_deleteBtn->setEnabled(!m_isRecording && !m_isPlaying && hasSelection);
    m_refreshBtn->setEnabled(!m_isRecording);
    m_fileList->setEnabled(!m_isRecording);
    m_qualityCombo->setEnabled(!m_isRecording);
    m_progressSlider->setEnabled(m_isPlaying);
}

// =============================================================================
// 录制目录管理
// =============================================================================

QString ScreenRecorderWidget::recordDir() const
{
    // 跨平台：使用应用程序运行目录下的 recordings 子文件夹
    QString dir = QCoreApplication::applicationDirPath() + "/recordings";
    QDir d(dir);
    if (!d.exists()) {
        d.mkpath(".");
    }
    return dir;
}

// =============================================================================
// 文件列表管理
// =============================================================================

void ScreenRecorderWidget::refreshFileList()
{
    m_fileList->clear();

    QDir dir(recordDir());

    // --- 扫描 AVI 文件 ---
    QStringList aviFiles = dir.entryList({"*.avi"}, QDir::Files, QDir::Time);
    for (const QString& entry : aviFiles) {
        QFileInfo fi(dir.absoluteFilePath(entry));
        double sizeMB = fi.size() / (1024.0 * 1024.0);
        QString displayText = QString("%1  (%2 MB)")
                                  .arg(fi.baseName())
                                  .arg(sizeMB, 0, 'f', 1);
        QListWidgetItem* item = new QListWidgetItem(displayText, m_fileList);
        item->setData(Qt::UserRole, entry);      // 存储文件名
        item->setData(Qt::UserRole + 1, true);   // 标记为 AVI
    }

    // --- 兼容旧 PNG 序列目录 ---
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const QString& entry : entries) {
        QDir subDir(dir.absoluteFilePath(entry));
        int frameCount = subDir.entryList({"frame_*.png"}, QDir::Files).count();
        if (frameCount > 0) {
            QString displayText = QString("%1  (%2帧, ~%3秒) [旧格式]")
                                      .arg(entry)
                                      .arg(frameCount)
                                      .arg(frameCount * CAPTURE_INTERVAL_MS / 1000.0, 0, 'f', 1);
            QListWidgetItem* item = new QListWidgetItem(displayText, m_fileList);
            item->setData(Qt::UserRole, entry);
            item->setData(Qt::UserRole + 1, false); // 非 AVI
        }
    }

    if (m_fileList->count() == 0) {
        m_statusLabel->setText(QString("就绪 (录制保存至: %1)").arg(recordDir()));
    } else {
        m_statusLabel->setText(QString("共 %1 个录制").arg(m_fileList->count()));
    }

    updateButtonStates();
}

void ScreenRecorderWidget::onSelectionChanged()
{
    updateButtonStates();

    // 选中时加载首帧预览
    QListWidgetItem* item = m_fileList->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    bool isAvi = item->data(Qt::UserRole + 1).toBool();

    if (isAvi) {
        // AVI: 读取第一帧
        QString aviPath = recordDir() + "/" + name;
        QVector<QImage> frames = loadAviFrames(aviPath);
        if (!frames.isEmpty()) {
            QPixmap pix = QPixmap::fromImage(frames.first());
            m_previewLabel->setPixmap(
                pix.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        // 旧 PNG 目录
        QDir subDir(recordDir() + "/" + name);
        QStringList frames = subDir.entryList({"frame_*.png"}, QDir::Files, QDir::Name);
        if (!frames.isEmpty()) {
            QPixmap pix(subDir.absoluteFilePath(frames.first()));
            if (!pix.isNull()) {
                m_previewLabel->setPixmap(
                    pix.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
}

// =============================================================================
// 录制功能
// =============================================================================

void ScreenRecorderWidget::onRecordToggle()
{
    if (m_isRecording) {
        stopRecording();
    } else {
        // 确定录制目标尺寸
        QWidget* target = m_recordTarget ? m_recordTarget : window();
        if (!target) return;

        // 创建临时 AVI 文件名（时间戳）
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        m_currentRecordPath = recordDir() + "/" + timestamp + ".avi";

        double scale = currentScaleFactor();
        int w = static_cast<int>(target->width() * scale);
        int h = static_cast<int>(target->height() * scale);
        // 确保宽度为偶数（AVI行对齐）
        w = (w + 1) & ~1;
        h = (h + 1) & ~1;
        int fps = 1000 / CAPTURE_INTERVAL_MS; // 5 fps

        if (!m_aviWriter.open(m_currentRecordPath, w, h, fps, currentJpegQuality())) {
            m_statusLabel->setText("录制失败：无法创建AVI文件");
            return;
        }

        m_frameCount = 0;
        m_isRecording = true;
        m_captureTimer->start(CAPTURE_INTERVAL_MS);

        m_statusLabel->setText("● 正在录屏...");
        m_statusLabel->setStyleSheet("QLabel { color: #ff4444; font-weight: bold; }");

        LOG_INFO("Screen recording started: " + m_currentRecordPath);
        updateButtonStates();
    }
}

void ScreenRecorderWidget::captureFrame()
{
    if (!m_isRecording || !m_aviWriter.isOpen()) return;

    QWidget* target = m_recordTarget;
    if (!target) {
        target = window();
    }
    if (!target) return;

    // 跨平台截图：QScreen::grabWindow
    QScreen* screen = nullptr;
    QWindow* winHandle = target->windowHandle();
    if (winHandle) {
        screen = winHandle->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) return;

    QPixmap pixmap = screen->grabWindow(target->winId());

    if (!pixmap.isNull()) {
        m_aviWriter.writeFrame(pixmap.toImage());
        m_frameCount++;
        m_statusLabel->setText(QString("● 录屏中... 已捕获 %1 帧").arg(m_frameCount));
    }
}

void ScreenRecorderWidget::stopRecording()
{
    m_captureTimer->stop();
    m_isRecording = false;
    m_aviWriter.close();

    m_statusLabel->setStyleSheet(""); // 恢复默认样式

    if (m_frameCount == 0) {
        // 没有捕获任何帧，删除空文件
        QFile::remove(m_currentRecordPath);
        m_statusLabel->setText("录制取消（未捕获帧）");
        updateButtonStates();
        return;
    }

    // 弹出黑绿风格命名对话框
    QString defaultName = QFileInfo(m_currentRecordPath).baseName(); // 时间戳
    QString name = showStyledNameDialog(defaultName);

    if (!name.isEmpty()) {
        // 确保扩展名为 .avi
        if (!name.endsWith(".avi", Qt::CaseInsensitive))
            name += ".avi";

        QString newPath = recordDir() + "/" + name;

        // 如果已存在同名文件，添加时间戳后缀
        if (QFile::exists(newPath)) {
            QString base = QFileInfo(newPath).baseName();
            newPath = recordDir() + "/" + base + "_"
                      + QDateTime::currentDateTime().toString("HHmmss") + ".avi";
        }

        QFile::rename(m_currentRecordPath, newPath);
        m_statusLabel->setText(QString("录制完成：%1 (%2帧)")
                                   .arg(QFileInfo(newPath).baseName()).arg(m_frameCount));
        LOG_INFO(QString("Screen recording saved: %1, %2 frames").arg(newPath).arg(m_frameCount));
    } else {
        // 用户取消，保留原始时间戳名称
        m_statusLabel->setText(QString("录制已保存：%1 (%2帧)")
                                   .arg(QFileInfo(m_currentRecordPath).baseName())
                                   .arg(m_frameCount));
    }

    refreshFileList();
    updateButtonStates();
}

// =============================================================================
// 回放功能
// =============================================================================

void ScreenRecorderWidget::onPlayClicked()
{
    QListWidgetItem* item = m_fileList->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    bool isAvi = item->data(Qt::UserRole + 1).toBool();

    m_playImages.clear();
    m_playFrames.clear();
    m_playFromAvi = isAvi;

    if (isAvi) {
        // 从 AVI 加载帧到内存
        QString aviPath = recordDir() + "/" + name;
        m_playImages = loadAviFrames(aviPath);
        if (m_playImages.isEmpty()) {
            m_statusLabel->setText("没有可回放的帧");
            return;
        }
    } else {
        // 旧 PNG 序列
        QDir subDir(recordDir() + "/" + name);
        m_playFrames = subDir.entryList({"frame_*.png"}, QDir::Files, QDir::Name);
        if (m_playFrames.isEmpty()) {
            m_statusLabel->setText("没有可回放的帧");
            return;
        }
        for (int i = 0; i < m_playFrames.size(); ++i) {
            m_playFrames[i] = subDir.absoluteFilePath(m_playFrames[i]);
        }
    }

    int totalFrames = m_playFromAvi ? m_playImages.size() : m_playFrames.size();
    m_playIndex = 0;
    m_isPlaying = true;

    m_progressSlider->setRange(0, totalFrames - 1);
    m_progressSlider->setValue(0);

    m_playTimer->start(PLAY_INTERVAL_MS);
    m_statusLabel->setText(QString("▶ 回放中... %1").arg(QFileInfo(name).baseName()));

    updateButtonStates();
}

void ScreenRecorderWidget::onStopPlayClicked()
{
    m_playTimer->stop();
    m_isPlaying = false;
    m_statusLabel->setText("回放已停止");
    updateButtonStates();
}

void ScreenRecorderWidget::playNextFrame()
{
    int totalFrames = m_playFromAvi ? m_playImages.size() : m_playFrames.size();

    if (m_playIndex >= totalFrames) {
        onStopPlayClicked();
        m_statusLabel->setText("回放完成");
        return;
    }

    QPixmap pix;
    if (m_playFromAvi) {
        pix = QPixmap::fromImage(m_playImages[m_playIndex]);
    } else {
        pix.load(m_playFrames[m_playIndex]);
    }

    if (!pix.isNull()) {
        m_previewLabel->setPixmap(
            pix.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    m_progressSlider->setValue(m_playIndex);
    m_frameInfoLabel->setText(QString("%1/%2").arg(m_playIndex + 1).arg(totalFrames));

    m_playIndex++;
}

void ScreenRecorderWidget::onSliderMoved(int value)
{
    int totalFrames = m_playFromAvi ? m_playImages.size() : m_playFrames.size();
    if (value < 0 || value >= totalFrames) return;

    m_playIndex = value;

    QPixmap pix;
    if (m_playFromAvi) {
        pix = QPixmap::fromImage(m_playImages[value]);
    } else {
        pix.load(m_playFrames[value]);
    }

    if (!pix.isNull()) {
        m_previewLabel->setPixmap(
            pix.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_frameInfoLabel->setText(QString("%1/%2").arg(value + 1).arg(totalFrames));
}

// =============================================================================
// 删除功能
// =============================================================================

void ScreenRecorderWidget::onDeleteClicked()
{
    QListWidgetItem* item = m_fileList->currentItem();
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    bool isAvi = item->data(Qt::UserRole + 1).toBool();
    QString fullPath = recordDir() + "/" + name;
    QString displayName = isAvi ? QFileInfo(name).baseName() : name;

    // 黑绿风格确认对话框
    bool confirmed = showStyledConfirmDialog(
        "确认删除",
        QString("确定要删除录制 \"%1\" 吗？\n此操作不可恢复。").arg(displayName));

    if (confirmed) {
        if (isAvi) {
            QFile::remove(fullPath);
        } else {
            QDir(fullPath).removeRecursively();
        }
        LOG_INFO("Screen recording deleted: " + fullPath);
        refreshFileList();
        m_previewLabel->setText("无预览");
        m_previewLabel->setPixmap(QPixmap());
    }
}

// =============================================================================
// 通用暗黑绿色对话框样式
// =============================================================================

QString ScreenRecorderWidget::darkGreenDialogStyleSheet()
{
    return QStringLiteral(
        "QDialog {"
        "  background-color: rgb(8, 12, 24);"
        "  border: 1px solid rgba(68, 136, 255, 0.4);"
        "}"
        "QLabel {"
        "  color: #4488ff;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  background-color: transparent;"
        "}"
        "QLineEdit {"
        "  background-color: rgba(8, 14, 28, 0.9);"
        "  color: #ffffff;"
        "  font-size: 14px;"
        "  border: 1px solid rgba(68, 136, 255, 0.5);"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  selection-background-color: rgba(68, 136, 255, 0.3);"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #4488ff;"
        "}"
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #4488ff;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid rgba(68, 136, 255, 0.45);"
        "  border-radius: 6px;"
        "  padding: 6px 20px;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(68, 136, 255, 0.15);"
        "  border: 1px solid #4488ff;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(68, 136, 255, 0.28);"
        "  border: 2px solid #5599ff;"
        "  color: #ffffff;"
        "}");
}

// =============================================================================
// 录制质量
// =============================================================================

double ScreenRecorderWidget::currentScaleFactor() const
{
    return m_qualityCombo->currentData().toDouble();
}

int ScreenRecorderWidget::currentJpegQuality() const
{
    double scale = m_qualityCombo->currentData().toDouble();
    if (scale <= 0.3) return 50;
    if (scale <= 0.6) return 70;
    return 90;
}

// =============================================================================
// 黑绿风格命名对话框
// =============================================================================

QString ScreenRecorderWidget::showStyledNameDialog(const QString& defaultName)
{
    QDialog dlg(this);
    dlg.setWindowTitle("保存录制");
    dlg.setMinimumWidth(380);
    dlg.setStyleSheet(darkGreenDialogStyleSheet());

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    QLabel* label = new QLabel("请输入录制名称：", &dlg);
    layout->addWidget(label);

    QLineEdit* lineEdit = new QLineEdit(defaultName, &dlg);
    lineEdit->selectAll();
    layout->addWidget(lineEdit);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* okBtn = new QPushButton("确定", &dlg);
    QPushButton* cancelBtn = new QPushButton("取消", &dlg);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    lineEdit->setFocus();

    if (dlg.exec() == QDialog::Accepted) {
        QString text = lineEdit->text().trimmed();
        return text.isEmpty() ? defaultName : text;
    }
    return QString(); // 取消
}

// =============================================================================
// 黑绿风格确认对话框
// =============================================================================

bool ScreenRecorderWidget::showStyledConfirmDialog(const QString& title, const QString& message)
{
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(380);
    dlg.setStyleSheet(darkGreenDialogStyleSheet());

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 20, 24, 16);

    QLabel* msgLabel = new QLabel(message, &dlg);
    msgLabel->setWordWrap(true);
    layout->addWidget(msgLabel);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* yesBtn = new QPushButton("确定", &dlg);
    QPushButton* noBtn  = new QPushButton("取消", &dlg);
    btnLayout->addWidget(yesBtn);
    btnLayout->addWidget(noBtn);
    layout->addLayout(btnLayout);

    connect(yesBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(noBtn,  &QPushButton::clicked, &dlg, &QDialog::reject);

    noBtn->setFocus(); // 默认焦点在"取消"上，防止误删

    return dlg.exec() == QDialog::Accepted;
}

// =============================================================================
// AVI 帧读取（解析未压缩 AVI 的 00dc chunk）
// =============================================================================

QVector<QImage> ScreenRecorderWidget::loadAviFrames(const QString& aviPath)
{
    QVector<QImage> frames;
    QFile file(aviPath);
    if (!file.open(QIODevice::ReadOnly)) return frames;

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 12) return frames;

    auto readU32 = [&](int offset) -> quint32 {
        if (offset + 4 > data.size()) return 0;
        const uchar* p = reinterpret_cast<const uchar*>(data.constData() + offset);
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    };

    // 查找 "movi" list
    int moviPos = data.indexOf("movi");
    if (moviPos < 0) return frames;

    int pos = moviPos + 4;
    while (pos + 8 <= data.size()) {
        QByteArray chunkId = data.mid(pos, 4);
        quint32 chunkSize = readU32(pos + 4);

        if (chunkId == "00dc" && chunkSize > 0) {
            int frameStart = pos + 8;
            if (frameStart + static_cast<int>(chunkSize) > data.size()) break;

            QByteArray jpegData = data.mid(frameStart, static_cast<int>(chunkSize));
            QImage img;
            if (img.loadFromData(jpegData, "JPEG")) {
                frames.append(img);
            }
        } else if (chunkId == "idx1") {
            break;
        }

        pos += 8 + static_cast<int>(chunkSize);
        if (pos % 2 != 0) pos++;
    }

    return frames;
}
