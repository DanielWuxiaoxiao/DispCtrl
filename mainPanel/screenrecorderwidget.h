/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-03-10 17:18:12
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:57
 * @Description: 
 */
/**
 * @file screenrecorderwidget.h
 * @brief 屏幕录制与回放组件
 * @details 提供屏幕录制（截图序列）和回放功能
 *
 * 功能特性：
 * - 录屏：定时截取主窗口画面，保存为PNG序列
 * - 回放：加载历史录制，逐帧播放
 * - 文件管理：列出历史录制文件，支持删除
 * - 跨平台：使用QScreen::grabWindow()，支持Windows/Linux
 *
 * @author DispCtrl Team
 * @date 2026
 */

#ifndef SCREENRECORDERWIDGET_H
#define SCREENRECORDERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QDir>
#include <QPixmap>
#include <QVector>

#include "simpleavi.h"

class QListWidget;
class QComboBox;
class QPushButton;
class QLabel;
class QSlider;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;

/**
 * @class ScreenRecorderWidget
 * @brief 屏幕录制与回放组件
 * @details 录屏功能使用定时截图，回放功能加载截图序列逐帧显示
 */
class ScreenRecorderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenRecorderWidget(QWidget* parent = nullptr);
    ~ScreenRecorderWidget();

    /**
     * @brief 设置录制目标窗口
     * @param target 需要录制的窗口
     */
    void setRecordTarget(QWidget* target);

private slots:
    /// 开始/停止录制
    void onRecordToggle();

    /// 录制定时器触发，截取一帧
    void captureFrame();

    /// 停止录制，弹出命名对话框
    void stopRecording();

    /// 回放选中的录制
    void onPlayClicked();

    /// 停止回放
    void onStopPlayClicked();

    /// 回放定时器触发，显示下一帧
    void playNextFrame();

    /// 删除选中的录制
    void onDeleteClicked();

    /// 刷新录制列表
    void refreshFileList();

    /// 列表选择变化
    void onSelectionChanged();

    /// 进度滑块拖动
    void onSliderMoved(int value);

private:
    /// 获取录制存储根目录
    QString recordDir() const;

    /// 设置UI布局
    void setupUI();

    /// 连接信号槽
    void connectSignals();

    /// 更新按钮状态
    void updateButtonStates();

    /// 弹出黑绿风格命名对话框，返回用户输入名称（空串表示取消）
    QString showStyledNameDialog(const QString& defaultName);

    /// 弹出黑绿风格确认对话框，返回用户是否确认
    bool showStyledConfirmDialog(const QString& title, const QString& message);

    /// 获取通用暗黑绿色对话框样式表
    static QString darkGreenDialogStyleSheet();

    /// 从 AVI 文件加载所有帧
    QVector<QImage> loadAviFrames(const QString& aviPath);

    /// 获取当前选中的录制缩放比例
    double currentScaleFactor() const;

    /// 获取当前选中的 JPEG 压缩质量 (1-100)
    int currentJpegQuality() const;

    // === UI 组件 ===
    QListWidget* m_fileList;             ///< 历史录制列表
    QComboBox*   m_qualityCombo;         ///< 录制质量选择（低清/中清/高清）
    QPushButton* m_recordBtn;            ///< 开始/停止录制按钮
    QPushButton* m_playBtn;              ///< 回放按钮
    QPushButton* m_stopPlayBtn;          ///< 停止回放按钮
    QPushButton* m_deleteBtn;            ///< 删除按钮
    QPushButton* m_refreshBtn;           ///< 刷新列表按钮
    QLabel*      m_statusLabel;          ///< 状态标签
    QLabel*      m_previewLabel;         ///< 预览/回放显示区域
    QSlider*     m_progressSlider;       ///< 回放进度条
    QLabel*      m_frameInfoLabel;       ///< 帧信息标签

    // === 录制状态 ===
    bool         m_isRecording;          ///< 是否正在录制
    QTimer*      m_captureTimer;         ///< 截图定时器
    QString      m_currentRecordPath;    ///< 当前录制 AVI 路径
    int          m_frameCount;           ///< 已录制帧数
    QWidget*     m_recordTarget;         ///< 录制目标窗口
    SimpleAviWriter m_aviWriter;         ///< AVI 写入器

    // === 回放状态 ===
    bool         m_isPlaying;            ///< 是否正在回放
    QTimer*      m_playTimer;            ///< 回放定时器
    QStringList  m_playFrames;           ///< 回放帧文件列表（兼容旧PNG录制）
    int          m_playIndex;            ///< 当前回放帧索引
    QVector<QImage> m_playImages;        ///< 从AVI加载的帧缓存
    bool         m_playFromAvi;          ///< 当前回放是否来自AVI

    // === 常量 ===
    static constexpr int CAPTURE_INTERVAL_MS = 200;  ///< 截图间隔（ms），约5fps
    static constexpr int PLAY_INTERVAL_MS    = 200;  ///< 回放间隔（ms）
};

#endif // SCREENRECORDERWIDGET_H
