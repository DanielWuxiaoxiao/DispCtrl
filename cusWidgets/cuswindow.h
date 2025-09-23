/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:17
 * @Description: 
 */
/**
 * @file cuswindow.h
 * @brief 自定义窗口组件
 * @details 提供完全自定义的窗口实现，支持拖拽、缩放、最大化等窗口操作
 * 
 * 功能特性：
 * - 无边框窗口设计
 * - 自定义标题栏和控制按钮
 * - 支持窗口拖拽和边框缩放
 * - 窗口最大化/还原功能
 * - 自定义窗口样式和主题
 * - 内容区域自定义支持
 * 
 * 交互功能：
 * - 拖拽移动窗口
 * - 八个方向的边框缩放
 * - 双击标题栏最大化/还原
 * - 最小化/最大化/关闭按钮
 * - 动态鼠标指针变化
 * 
 * 使用场景：
 * - 替代系统原生窗口
 * - 统一应用程序窗口风格
 * - 特殊UI需求的窗口
 * - 嵌入式窗口管理
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QRubberBand>

/**
 * @class CusWindow
 * @brief 自定义窗口组件
 * @details 基于QWidget的完全自定义窗口实现，提供原生窗口的所有基本功能
 * 
 * 该类实现了一个功能完整的自定义窗口：
 * - 完全自绘的窗口边框和标题栏
 * - 支持拖拽移动和八方向缩放
 * - 标准窗口控制按钮（最小化、最大化、关闭）
 * - 双击标题栏切换最大化状态
 * - 自定义内容区域支持
 * - 橡皮筋缩放预览效果
 * 
 * 设计特点：
 * - 无系统边框，完全自定义外观
 * - 响应式鼠标指针变化
 * - 平滑的窗口操作体验
 * - 灵活的内容区域管理
 * 
 * @example
 * ```cpp
 * CusWindow* window = new CusWindow("自定义窗口", QIcon(":/icon.png"));
 * 
 * QWidget* content = new QWidget();
 * // 设置内容组件...
 * window->setContentWidget(content);
 * 
 * connect(window, &CusWindow::windowClosed, [&]() {
 *     qDebug() << "窗口已关闭";
 * });
 * 
 * window->show();
 * ```
 */
class CusWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param title 窗口标题
     * @param icon 窗口图标
     * @param parent 父窗口指针
     * @details 创建自定义窗口实例，初始化标题栏和控制按钮
     */
    explicit CusWindow(const QString& title, const QIcon& icon, QWidget* parent = nullptr);

    /**
     * @brief 设置内容组件
     * @param w 内容组件指针
     * @details 将指定的组件设置为窗口的内容区域
     */
    void setContentWidget(QWidget* w);

protected:
    // === 鼠标事件处理 ===
    
    /**
     * @brief 鼠标按下事件
     * @param e 鼠标事件对象
     * @details 处理窗口拖拽和缩放的开始操作
     */
    void mousePressEvent(QMouseEvent* e) override;
    
    /**
     * @brief 鼠标移动事件
     * @param e 鼠标事件对象
     * @details 处理窗口拖拽移动和缩放操作
     */
    void mouseMoveEvent(QMouseEvent* e) override;
    
    /**
     * @brief 鼠标释放事件
     * @param e 鼠标事件对象
     * @details 结束窗口拖拽和缩放操作
     */
    void mouseReleaseEvent(QMouseEvent* e) override;
    
    /**
     * @brief 鼠标双击事件
     * @param e 鼠标事件对象
     * @details 处理标题栏双击最大化/还原操作
     */
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    
    /**
     * @brief 绘制事件
     * @param e 绘制事件对象
     * @details 自定义绘制窗口边框和装饰
     */
    void paintEvent(QPaintEvent* e) override;
    
    /**
     * @brief 鼠标离开事件
     * @param e 事件对象
     * @details 恢复鼠标指针为默认样式
     */
    void leaveEvent(QEvent* e) override;

    /**
     * @brief 事件过滤器
     * @param obj 事件源对象
     * @param event 事件对象
     * @return 是否处理了事件
     * @details 监听并处理特定事件，如鼠标悬停
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    /**
     * @enum ResizeRegion
     * @brief 缩放区域枚举
     * @details 定义窗口边框的八个缩放区域和无缩放区域
     */
    enum ResizeRegion {
        None,         ///< 无缩放区域
        Left,         ///< 左边框
        Right,        ///< 右边框
        Top,          ///< 上边框
        Bottom,       ///< 下边框
        TopLeft,      ///< 左上角
        TopRight,     ///< 右上角
        BottomLeft,   ///< 左下角
        BottomRight   ///< 右下角
    };
    
    /**
     * @brief 命中测试
     * @param pos 鼠标位置
     * @return 缩放区域类型
     * @details 检测鼠标位置对应的窗口缩放区域
     */
    ResizeRegion hitTest(const QPoint& pos) const;
    
    /**
     * @brief 更新鼠标指针
     * @param pos 鼠标位置
     * @details 根据鼠标位置更新指针样式
     */
    void updateCursor(const QPoint& pos);

    // === 拖拽和缩放状态 ===
    QPoint m_dragPos;                   ///< 拖拽起始位置
    bool m_dragging = false;            ///< 是否正在拖拽
    bool m_resizing = false;            ///< 是否正在缩放
    ResizeRegion m_resizeRegion = None; ///< 当前缩放区域
    QRect m_resizeStartRect;            ///< 缩放开始时的窗口矩形
    QPoint m_resizeStartPos;            ///< 缩放开始时的鼠标位置

    // === UI组件 ===
    QWidget* m_content = nullptr;       ///< 内容组件
    QLabel* m_titleLabel;               ///< 标题标签
    QPushButton* m_closeBtn;            ///< 关闭按钮
    QPushButton* m_minBtn;              ///< 最小化按钮
    QPushButton* m_maxBtn;              ///< 最大化按钮
    
    // === 窗口状态 ===
    bool m_maximized = false;           ///< 是否已最大化
    QRect m_restoreRect;                ///< 还原时的窗口矩形

    // === 橡皮筋效果 ===
    QRubberBand* m_rubberBand = nullptr; ///< 橡皮筋组件
    QRect m_rubberBandRect;              ///< 橡皮筋矩形

signals:
    /**
     * @brief 窗口关闭信号
     * @details 当用户点击关闭按钮时发射此信号
     */
    void windowClosed();
};
