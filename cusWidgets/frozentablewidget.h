/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-02-05 16:52:55
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:33
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-02-05
 * @Description: 表格首列冻结辅助类
 */
/**
 * @file frozentablewidget.h
 * @brief 表格首列冻结辅助类头文件
 * @details 为现有QTableWidget添加类似Excel的冻结首列功能
 */

#ifndef FROZENTABLEWIDGET_H
#define FROZENTABLEWIDGET_H

#include <QObject>
#include <QTableWidget>

/**
 * @class FrozenColumnHelper
 * @brief 表格首列冻结辅助类
 * @details 附加到现有QTableWidget，提供冻结首列功能：
 *          - 创建覆盖在主表格上的冻结列表格
 *          - 自动同步行数、行高、单元格内容
 *          - 同步垂直滚动和选择状态
 *
 * 使用方法：
 * @code
 * QTableWidget* table = ui->tableWidget;
 * FrozenColumnHelper* helper = new FrozenColumnHelper(table, 1);  // 冻结1列
 * // 之后正常使用 table，helper 会自动同步冻结列
 * @endcode
 */
class FrozenColumnHelper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param table 要添加冻结列功能的表格
     * @param frozenCount 要冻结的列数（从左边开始），默认为1
     * @param parent 父对象
     */
    explicit FrozenColumnHelper(QTableWidget* table, int frozenCount = 1, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FrozenColumnHelper() override;

    /**
     * @brief 同步冻结列内容
     * @details 在主表格数据更新后调用，同步冻结列的内容
     *          注意：添加/删除行后必须调用此方法
     */
    void syncFrozenContent();

    /**
     * @brief 设置冻结列数
     * @param count 要冻结的列数
     */
    void setFrozenColumnCount(int count);

    /**
     * @brief 获取冻结列数
     * @return 当前冻结的列数
     */
    int frozenColumnCount() const { return m_frozenCount; }

private slots:
    /**
     * @brief 更新冻结表格的几何位置
     */
    void updateGeometry();

    /**
     * @brief 同步垂直滚动
     */
    void syncVerticalScroll(int value);

    /**
     * @brief 同步选择状态
     */
    void syncSelection();

    /**
     * @brief 处理表头大小变化
     */
    void onSectionResized(int logicalIndex, int oldSize, int newSize);

protected:
    /**
     * @brief 事件过滤器
     * @param watched 被监视的对象
     * @param event 事件
     * @return 是否处理了事件
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /**
     * @brief 初始化冻结表格
     */
    void init();

    /**
     * @brief 计算冻结列总宽度
     * @return 冻结列的总宽度（像素）
     */
    int calculateFrozenWidth() const;

    QTableWidget* m_mainTable;    ///< 主表格指针
    QTableWidget* m_frozenTable;  ///< 冻结列覆盖表格
    int m_frozenCount;            ///< 冻结的列数
    bool m_syncing;               ///< 同步标志，防止循环调用
};

#endif // FROZENTABLEWIDGET_H
