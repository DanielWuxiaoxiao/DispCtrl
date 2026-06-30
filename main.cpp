/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-21 11:46:03
 * @Description: 
 */
/**
 * @file main.cpp
 * @brief DispCtrl雷达显示控制系统主程序入口
 * @details 程序初始化流程：
 *          1. Qt应用程序属性配置（高DPI、OpenGL等）
 *          2. 错误处理框架初始化
 *          3. 系统配置加载和验证
 *          4. 用户界面初始化（字体、样式、主窗口）
 *          5. 日志系统和控制器启动
 * @author DispCtrl Team
 * @date 2024
 */

#include <QApplication>
#include <QDir>
#include <QStringList>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QAbstractSocket>
#include "mainwindow.h"
#include "Basic/bindThread.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/ConfigManager.h"
#include "Basic/Protocol.h"
#include "Controller/ErrorHandler.h"
#include <QLoggingCategory>
#include "Controller/controller.h"
#include "Basic/authmanager.h"

// --- 登录对话框所需头文件 ---
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QIcon>

#ifdef Q_OS_WIN
// 双显卡（联想 Y9000P 等 NVIDIA Optimus / AMD 双显）笔记本：强制使用独立显卡。
// 避免 Intel 核显 ↔ 独显 之间切换导致 QtWebEngine 共享 GL 上下文失效，
// 在"切换软件 / 点击窗口 / 切换地图"时出现整屏黑屏闪烁。
// 这两个导出符号会被显卡驱动识别（必须从 exe 导出、全局可见）。
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

/**
 * @brief 设置OpenGL渲染格式
 * @details 配置OpenGL上下文：
 *          - 使用Core Profile（核心配置文件）
 *          - OpenGL 3.3版本
 *          - 确保图形渲染的兼容性和性能
 */
void setupOpenGL() {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
}

/**
 * @brief 设置应用程序字体
 * @param app Qt应用程序实例
 * @details 配置全局字体：
 *          - 使用微软雅黑字体，确保中文显示效果
 *          - 字体大小使用MAIN_FONT_SIZE常量
 *          - 提供一致的用户界面体验
 */
void setupFont(QApplication& app) {
    QFont font("Microsoft YaHei", MAIN_FONT_SIZE);
    app.setFont(font);
}

/**
 * @brief 设置应用程序样式
 * @param app Qt应用程序实例
 * @details 加载并应用深色主题样式：
 *          - 从资源文件加载QSS样式表
 *          - 提供专业的雷达显示界面外观
 *          - 深色主题有助于减少眼疲劳
 */
void setupStyle(QApplication& app) {
    QFile file(":/resources/style/darkstyle.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString style = QString::fromUtf8(file.readAll());

        // 统一右下角两个信息窗字体大小（鼠标信息窗 + 视觉设置窗）
        // 放入全局QSS，避免在各自widget中硬编码导致大小不一致
        // 注意：字体由 AA_EnableHighDpiScaling 自动处理，此处保持固定px即可
        style += R"(

/* --- Unified overlay panel fonts --- */
#MousePositionInfo QLabel,
#MousePositionInfo QCheckBox,
#MousePositionInfo QDoubleSpinBox {
    font-size: 12px;
}

#PPIVisualSettings QLabel,
#PPIVisualSettings QLineEdit,
#PPIVisualSettings QComboBox,
#PPIVisualSettings QPushButton {
    font-size: 12px;
}

)";

        style += R"(

/* --- Main layout text fit overrides --- */
#MainOverLayOut QPushButton#minButton,
#MainOverLayOut QPushButton#CloseButton {
    min-width: 78px;
    padding: 4px 8px;
}

#MainOverLayOut QToolButton[buttonGroup="A"] {
    font-size: 13px;
    padding: 6px 8px;
    min-width: 76px;
}

QSplitter#MainContentSplitter::handle {
    background-color: rgba(0, 255, 136, 0.012);
    border: none;
}

QSplitter#MainContentSplitter::handle:hover {
    background-color: rgba(0, 255, 136, 0.06);
    border-left: 1px solid rgba(0, 255, 136, 0.14);
}

)";

        if (ScaleHelper::compactLayout()) {
            style += R"(

/* --- 1920x1080/100% compact layout overrides --- */
#MainOverLayOut QTabBar::tab {
    font-size: 11px;
    padding: 3px 7px;
}

#MainOverLayOut QPushButton {
    font-size: 11px;
}

#MainOverLayOut QTableWidget,
#MainOverLayOut QHeaderView::section {
    font-size: 11px;
}

#MainOverLayOut #TitleLabel {
    font-size: 18px;
}

#MainOverLayOut #SubtitleLabel {
    font-size: 12px;
}

#mainviewTopLeft QLabel,
#PointInfoW QLabel {
    font-size: 12px;
}

#mainviewTopLeft QLineEdit,
#PointInfoW QLabel#batch,
#PointInfoW QLabel#range,
#PointInfoW QLabel#azi,
#PointInfoW QLabel#ele,
#PointInfoW QLabel#altitute,
#PointInfoW QLabel#speed,
#PointInfoW QLabel#SNR,
#PointInfoW QLabel#targetRec {
    font-size: 12px;
    padding: 1px 4px;
}

#mainviewTopLeft QLineEdit {
    min-height: 16px;
    max-height: 20px;
}

#MousePositionInfo QLabel,
#MousePositionInfo QCheckBox,
#MousePositionInfo QDoubleSpinBox,
#PPIVisualSettings QLabel,
#PPIVisualSettings QLineEdit,
#PPIVisualSettings QComboBox,
#PPIVisualSettings QPushButton {
    font-size: 10px;
}

#MainOverLayOut #FuncWidget {
    border-width: 6px;
    padding: 4px 8px;
}

#MainOverLayOut QToolButton[buttonGroup="A"] {
    font-size: 11px;
    padding: 4px 5px;
    min-width: 64px;
}

#MainOverLayOut QPushButton#minButton,
#MainOverLayOut QPushButton#CloseButton {
    min-width: 74px;
    padding: 3px 6px;
}

)";
        }

        app.setStyleSheet(style);
        file.close();
    }
}

/**
 * @brief 绑定主线程到指定CPU核心（Linux平台）
 * @details 性能优化功能：
 *          - 在Linux系统上将主线程绑定到CPU核心0
 *          - 提高实时性和性能稳定性
 *          - 适用于高性能雷达数据处理需求
 */
void bindMainThread() {
#ifdef Q_OS_LINUX
    if (bindThreadToCpu(0))
        LOG_DEBUG("Main thread bound to CPU 0.");
    else
        LOG_WARNING("Failed to bind main thread to CPU 0.");
#endif
}

/**
 * @brief 返回深色绿色对话框样式表
 */
static QString loginDialogStyleSheet()
{
    return QStringLiteral(
        "QDialog {"
        "  background-color: rgb(10, 16, 16);"
        "  border: 1px solid rgba(0, 255, 136, 0.4);"
        "}"
        "QLabel {"
        "  color: #00ff88;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  background-color: transparent;"
        "}"
        "QLineEdit {"
        "  background-color: rgba(0, 20, 10, 0.9);"
        "  color: #ffffff;"
        "  font-size: 14px;"
        "  border: 1px solid rgba(0, 255, 136, 0.5);"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  selection-background-color: rgba(0, 255, 136, 0.3);"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #00ff88;"
        "}"
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #00ff88;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid rgba(0, 255, 136, 0.45);"
        "  border-radius: 6px;"
        "  padding: 6px 20px;"
        "  min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(0, 255, 136, 0.15);"
        "  border: 1px solid #00ff88;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(0, 255, 136, 0.28);"
        "  border: 2px solid #00ffaa;"
        "  color: #ffffff;"
        "}");
}

/**
 * @brief 弹出管理者模式登录对话框
 * @details 先询问用户是否以管理者模式登录，
 *          若选择是则弹出用户名/密码输入框，
 *          验证通过后设置 AuthManager 为管理者模式。
 */
void showAdminLoginDialog()
{
    // --- 第一步：询问是否使用管理者模式 ---
    QDialog askDlg;
    askDlg.setWindowTitle(QObject::tr("登录模式选择"));
    askDlg.setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    askDlg.setFixedSize(420, 160);
    askDlg.setStyleSheet(loginDialogStyleSheet());

    auto* askLayout = new QVBoxLayout(&askDlg);
    askLayout->setContentsMargins(24, 20, 24, 16);

    auto* askLabel = new QLabel(QObject::tr("是否使用管理者模式登录？"));
    askLabel->setAlignment(Qt::AlignCenter);
    askLayout->addWidget(askLabel);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(20);
    auto* yesBtn = new QPushButton(QObject::tr("是"));
    auto* noBtn  = new QPushButton(QObject::tr("否"));
    btnLayout->addStretch();
    btnLayout->addWidget(yesBtn);
    btnLayout->addWidget(noBtn);
    btnLayout->addStretch();
    askLayout->addLayout(btnLayout);

    bool wantAdmin = false;
    QObject::connect(yesBtn, &QPushButton::clicked, [&]() { wantAdmin = true;  askDlg.accept(); });
    QObject::connect(noBtn,  &QPushButton::clicked, [&]() { wantAdmin = false; askDlg.reject(); });

    askDlg.exec();

    if (!wantAdmin) {
        // 普通用户模式
        AuthManager::instance().setAdminMode(false);
        return;
    }

    // --- 第二步：输入管理者凭据 ---
    QDialog loginDlg;
    loginDlg.setWindowTitle(QObject::tr("管理者登录"));
    loginDlg.setWindowIcon(QIcon(":/resources/icon/radararray.png"));
    loginDlg.setFixedSize(420, 240);
    loginDlg.setStyleSheet(loginDialogStyleSheet());

    auto* loginLayout = new QVBoxLayout(&loginDlg);
    loginLayout->setContentsMargins(24, 20, 24, 16);
    loginLayout->setSpacing(12);

    // 用户名
    auto* userLabel = new QLabel(QObject::tr("用户名:"));
    auto* userEdit  = new QLineEdit;
    userEdit->setText("admin");
    loginLayout->addWidget(userLabel);
    loginLayout->addWidget(userEdit);

    // 密码
    auto* passLabel = new QLabel(QObject::tr("密码:"));
    auto* passEdit  = new QLineEdit;
    passEdit->setEchoMode(QLineEdit::Password);
    loginLayout->addWidget(passLabel);
    loginLayout->addWidget(passEdit);

    // 按钮
    auto* loginBtnLayout = new QHBoxLayout;
    loginBtnLayout->setSpacing(20);
    auto* loginBtn  = new QPushButton(QObject::tr("登录"));
    auto* cancelBtn = new QPushButton(QObject::tr("取消"));
    loginBtnLayout->addStretch();
    loginBtnLayout->addWidget(loginBtn);
    loginBtnLayout->addWidget(cancelBtn);
    loginBtnLayout->addStretch();
    loginLayout->addLayout(loginBtnLayout);

    QObject::connect(cancelBtn, &QPushButton::clicked, &loginDlg, &QDialog::reject);
    QObject::connect(loginBtn, &QPushButton::clicked, [&]() {
        if (userEdit->text().trimmed() == "admin" && passEdit->text() == "xidian") {
            loginDlg.accept();
        } else {
            // 凭据错误 — 简单提示
            passEdit->clear();
            passEdit->setPlaceholderText(QObject::tr("用户名或密码错误，请重试"));
        }
    });

    if (loginDlg.exec() == QDialog::Accepted) {
        AuthManager::instance().setAdminMode(true);
    } else {
        AuthManager::instance().setAdminMode(false);
    }
}

/**
 * @brief 程序主入口函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出代码（0表示正常退出）
 * @details 完整的应用程序初始化流程：
 *          1. Qt应用程序属性配置（必须在QApplication创建前）
 *          2. 创建QApplication实例
 *          3. 初始化错误处理框架
 *          4. 控制器系统初始化
 *          5. UI配置（字体、OpenGL、样式）
 *          6. 日志系统配置
 *          7. 配置文件加载和验证
 *          8. 主窗口创建和显示
 *          9. 进入事件循环
 */
int main(int argc, char *argv[]) {
    // =============================================================================
    // 第零步：注册Qt元类型（必须在任何线程操作之前）
    // =============================================================================
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<MonitorParam>("MonitorParam");

    // 注册数据存储管理相关类型（用于跨线程信号传递）
    qRegisterMetaType<DataSaveOK>("DataSaveOK");
    qRegisterMetaType<DataDelOK>("DataDelOK");
    qRegisterMetaType<OfflineStat>("OfflineStat");

    // 注册BIT上报和伺服控制相关类型（用于跨线程信号传递）
    qRegisterMetaType<BITReport>("BITReport");
    qRegisterMetaType<ServoCtrlRet>("ServoCtrlRet");

    // =============================================================================
    // 第一步：Qt应用程序属性配置（必须在QApplication实例化之前）
    // =============================================================================

    // 提前加载配置，以便据此选择渲染后端（GPU相关设置必须在 QApplication 之前生效）。
    // 第四步会再次 load 做完整校验，此处重复加载无副作用。
    ConfigManager::instance().load("config.toml");

    // ---- WebEngine / Chromium GPU 渲染开关（解决外场两类显示故障）----
    // 故障1：打开某些录屏软件后显控黑屏（录屏钩取GPU/桌面合成，WebEngine的GPU表面被判为受保护/被抢占）
    // 故障2：部分电脑持续黑屏闪烁（显卡/驱动与desktop OpenGL不兼容，Chromium合成器反复丢失GL上下文）
    // 对策：可通过 config.toml [webengine] 关闭 WebEngine 的 GPU 加速、或切换 GL 后端，逐台调试无需重新编译。
    {
        QStringList chromiumFlags;
        if (ConfigManager::instance().webEngineDisableGpu(false)) {
            // 地图为2D瓦片，软件渲染足够；关闭GPU加速/合成可消除上述黑屏与闪烁
            chromiumFlags << "--disable-gpu" << "--disable-gpu-compositing";
        }
        const QString extraFlags = ConfigManager::instance().webEngineExtraChromiumFlags("").trimmed();
        if (!extraFlags.isEmpty()) {
            chromiumFlags << extraFlags;
        }
        if (!chromiumFlags.isEmpty()) {
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags.join(' ').toUtf8());
        }
    }

    // 启用高DPI缩放，确保在4K显示器上正常显示
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // GL 后端选择：angle/gles（默认，ANGLE→D3D，Windows/双显卡最稳）| desktop（原生OpenGL）| software（软件渲染）
    const QString glBackend = ConfigManager::instance().webEngineGlBackend("angle").toLower();
    if (glBackend == "software") {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    } else if (glBackend == "angle" || glBackend == "gles") {
        QApplication::setAttribute(Qt::AA_UseOpenGLES);
    } else {
        // 系统原生桌面OpenGL（性能最好），并设置 Core 3.3 默认格式（仅此后端适用）
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        setupOpenGL();
    }

    // 启用OpenGL上下文共享，提高多窗口渲染性能
    // QWebEngineView依赖独立进程，此设置增强稳定性
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // =============================================================================
    // 第二步：创建Qt应用程序实例
    // =============================================================================
    QApplication app(argc, argv);

    // =============================================================================
    // 第二.五步：初始化屏幕缩放因子（必须在 QApplication 之后、setupFont 之前）
    // =============================================================================
    ScaleHelper::init();
    LOG_INFO(QString("ScaleHelper initialized: logical=%1x%2 factor=%3 leftPanel=%4 rightPanel=%5")
                 .arg(ScaleHelper::logicalWidth())
                 .arg(ScaleHelper::logicalHeight())
                 .arg(ScaleHelper::factor())
                 .arg(ScaleHelper::leftPanelWidth())
                 .arg(ScaleHelper::rightPanelWidth()));

    // =============================================================================
    // 渲染后端诊断日志（外场黑屏/黑屏闪烁排查用）
    // 记录：实际生效的 GL 后端、Chromium flags、以及真正在用的 GPU/驱动。
    //   · GL_RENDERER 含 "Direct3D11" → ANGLE 已走 D3D（预期）
    //   · GL_VENDOR/GL_RENDERER 含 "NVIDIA" → 独显强制生效；含 "Intel" → 仍在核显（问题点）
    //   · 含 "SwiftShader"/"WARP"/"llvmpipe" → 落到软件渲染
    // =============================================================================
    {
        LOG_INFO(QString("[Render] gl_backend=%1 | disable_gpu=%2 | AA(GLES=%3,Desktop=%4,Software=%5) | platform=%6 | chromium_flags=%7")
                     .arg(glBackend)
                     .arg(ConfigManager::instance().webEngineDisableGpu(false))
                     .arg(QApplication::testAttribute(Qt::AA_UseOpenGLES))
                     .arg(QApplication::testAttribute(Qt::AA_UseDesktopOpenGL))
                     .arg(QApplication::testAttribute(Qt::AA_UseSoftwareOpenGL))
                     .arg(QApplication::platformName())
                     .arg(QString::fromUtf8(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"))));

        QOffscreenSurface diagSurface;
        diagSurface.create();
        QOpenGLContext diagCtx;
        if (diagSurface.isValid() && diagCtx.create() && diagCtx.makeCurrent(&diagSurface)) {
            QOpenGLFunctions* f = diagCtx.functions();
            auto glStr = [f](GLenum name) -> QString {
                const GLubyte* s = f->glGetString(name);
                return s ? QString::fromLatin1(reinterpret_cast<const char*>(s)) : QStringLiteral("?");
            };
            LOG_INFO(QString("[Render] GL_VENDOR=%1 | GL_RENDERER=%2 | GL_VERSION=%3")
                         .arg(glStr(GL_VENDOR))
                         .arg(glStr(GL_RENDERER))
                         .arg(glStr(GL_VERSION)));
            diagCtx.doneCurrent();
        } else {
            LOG_WARNING("[Render] 无法创建 OpenGL 上下文以查询 GPU 信息（可能 ANGLE/驱动初始化异常，请尝试切换 gl_backend）");
        }
    }

    // =============================================================================
    // 第三步：初始化错误处理框架
    // =============================================================================
    ErrorHandler& errorHandler = ErrorHandler::instance();
    Q_UNUSED(errorHandler); // 标记为已使用，避免编译器警告
    LOG_INFO("Error handler initialized");

    // =============================================================================
    // 第四步：配置文件加载和验证
    // =============================================================================
    if (!ConfigManager::instance().load("config.toml")) {
        LOG_ERROR("Failed to load config.toml, using default configuration");
        // 不返回错误，继续运行，使用默认配置
    } else {
        LOG_INFO("Configuration loaded successfully from config.toml");
    }

    refreshRuntimeLogLevelFromConfig();

    // =============================================================================
    // 第五步：控制器系统初始化
    // =============================================================================
    CON_INS->init();

    // =============================================================================
    // 第六步：用户界面配置
    // =============================================================================
    setupFont(app);      // 设置全局字体
    // OpenGL 默认格式已在 QApplication 之前按 GL 后端配置（仅 desktop 后端设置 Core 3.3），此处不再重复
    setupStyle(app);     // 应用深色主题样式

    // =============================================================================
    // 第六.五步：管理者模式登录
    // =============================================================================
    showAdminLoginDialog();

    // =============================================================================
    // 第七步：日志系统配置
    // =============================================================================
    qInstallMessageHandler(enhancedLog);

    // 输出日志文件位置信息（确保能看到日志文件路径）
    QString logPath = QDir::currentPath() + "/disp_ctrl_log.txt";
    LOG_INFO(QString("Log file path: %1").arg(logPath));
    LOG_INFO(QString("Application starting, log file: %1").arg(logPath));

    // =============================================================================
    // =============================================================================
    // 第八步：主窗口创建和显示
    // =============================================================================
    LOG_INFO("Application starting...");
    FramelessMainWindow window;
    LOG_INFO("Main window created successfully");

    // =============================================================================
    // 第九步：进入Qt事件循环
    // =============================================================================
    return app.exec();  // 程序主循环，直到用户退出
}
