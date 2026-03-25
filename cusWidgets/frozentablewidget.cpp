/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-02-28 16:46:29
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 16:20:18
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-02-05
 * @Description: 表格首列冻结辅助类实现
 */

#include "frozentablewidget.h"
#include <QScrollBar>
#include <QHeaderView>
#include <QEvent>

FrozenColumnHelper::FrozenColumnHelper(QTableWidget* table, int frozenCount, QObject* parent)
    : QObject(parent)
    , m_mainTable(table)
    , m_frozenTable(nullptr)
    , m_frozenCount(frozenCount)
    , m_syncing(false)
{
    if (m_mainTable) {
        init();
    }
}

FrozenColumnHelper::~FrozenColumnHelper()
{
    // m_frozenTable 的父对象是 m_mainTable，会自动删除
}

void FrozenColumnHelper::init()
{
    // 创建冻结列覆盖表格，父对象设为主表格
    m_frozenTable = new QTableWidget(m_mainTable);
    m_frozenTable->setFocusPolicy(Qt::NoFocus);

    // 设置与主表格相同的基本属性
    m_frozenTable->verticalHeader()->hide();
    m_frozenTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_frozenTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_frozenTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_frozenTable->setAlternatingRowColors(m_mainTable->alternatingRowColors());
    m_frozenTable->setShowGrid(m_mainTable->showGrid());
    m_frozenTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 显示表头
    m_frozenTable->horizontalHeader()->setVisible(true);
    m_frozenTable->horizontalHeader()->setStretchLastSection(false);

    // 设置冻结表格的样式，与主表格 darkstyle.qss 完全一致
    // 适用于 #tableWidget 和 #droneTableWidget
    m_frozenTable->setStyleSheet(
        "QTableWidget {"
        "    border: none;"
        "    background-color: rgba(16, 24, 24, 0.9);"
        "    color: #E0E0E0;"
        "    font-size: 14px;"
        "    gridline-color: #333333;"
        "}"
        "QTableWidget::item {"
        "    border-bottom: 1px solid #333333;"
        "    padding: 4px 8px;"
        "    background-color: rgba(16, 24, 24, 0.8);"
        "    color: #E0E0E0;"
        "}"
        "QTableWidget::item:alternate {"
        "    background-color: rgba(20, 30, 30, 0.8);"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: rgba(68, 136, 255, 0.3);"
        "    color: #FFFFFF;"
        "    border: 1px solid #4499ff;"
        "}"
        "QHeaderView::section {"
        "    background-color: rgba(16, 24, 44, 0.95);"
        "    color: #4499ff;"
        "    border: 1px solid #444444;"
        "    padding: 4px;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "    text-align: left;"
        "}"
    );

    // 同步垂直滚动
    connect(m_mainTable->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &FrozenColumnHelper::syncVerticalScroll);

    // 同步主表格选择到冻结表格
    connect(m_mainTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FrozenColumnHelper::syncSelection);

    // 点击冻结表格时同步选择到主表格
    connect(m_frozenTable, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (!m_syncing) {
            m_syncing = true;
            m_mainTable->selectRow(row);
            m_syncing = false;
        }
    });

    // 同步列宽变化
    connect(m_mainTable->horizontalHeader(), &QHeaderView::sectionResized,
            this, &FrozenColumnHelper::onSectionResized);

    // 监听主表格大小变化（使用事件过滤器方式更可靠）
    m_mainTable->viewport()->installEventFilter(this);

    // 初始化
    syncFrozenContent();
    updateGeometry();

    m_frozenTable->raise();
    m_frozenTable->show();
}

int FrozenColumnHelper::calculateFrozenWidth() const
{
    int width = 0;
    for (int col = 0; col < m_frozenCount && col < m_mainTable->columnCount(); ++col) {
        width += m_mainTable->columnWidth(col);
    }
    return width;
}

void FrozenColumnHelper::updateGeometry()
{
    if (!m_frozenTable || !m_mainTable) return;

    int frozenWidth = calculateFrozenWidth();

    // 冻结表格覆盖在主表格的左边部分，包括表头
    // 位置相对于主表格（不是viewport）
    m_frozenTable->setGeometry(
        m_mainTable->frameWidth(),                   // x：从主表格边框开始
        m_mainTable->frameWidth(),                   // y：从主表格边框开始
        frozenWidth + 2,                             // 宽度（加右边框线）
        m_mainTable->viewport()->height() + m_mainTable->horizontalHeader()->height()  // 高度：viewport + 表头
    );
}

void FrozenColumnHelper::syncVerticalScroll(int value)
{
    if (m_frozenTable) {
        m_frozenTable->verticalScrollBar()->setValue(value);
    }
}

void FrozenColumnHelper::syncSelection()
{
    if (!m_frozenTable || m_syncing) return;

    m_syncing = true;

    // 清除冻结表格的选择
    m_frozenTable->clearSelection();

    // 同步选择的行
    QModelIndexList selectedRows = m_mainTable->selectionModel()->selectedRows();
    for (const QModelIndex& index : selectedRows) {
        if (index.row() < m_frozenTable->rowCount()) {
            m_frozenTable->selectRow(index.row());
        }
    }

    m_syncing = false;
}

void FrozenColumnHelper::onSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
{
    // 只关心冻结列的宽度变化
    if (logicalIndex < m_frozenCount) {
        m_frozenTable->setColumnWidth(logicalIndex, newSize);
        updateGeometry();
    }
}

void FrozenColumnHelper::syncFrozenContent()
{
    if (!m_frozenTable || !m_mainTable) return;

    // 设置列数和行数
    m_frozenTable->setColumnCount(m_frozenCount);
    m_frozenTable->setRowCount(m_mainTable->rowCount());

    // 强制显示表头
    m_frozenTable->horizontalHeader()->setVisible(true);
    m_frozenTable->horizontalHeader()->setStretchLastSection(false);

    // 同步表头高度
    int headerHeight = m_mainTable->horizontalHeader()->height();
    m_frozenTable->horizontalHeader()->setMinimumHeight(headerHeight);
    m_frozenTable->horizontalHeader()->setMaximumHeight(headerHeight);

    // 同步表头标签和列宽
    QStringList headers;
    for (int col = 0; col < m_frozenCount && col < m_mainTable->columnCount(); ++col) {
        QTableWidgetItem* headerItem = m_mainTable->horizontalHeaderItem(col);
        QString headerText = headerItem ? headerItem->text() : QString::number(col);
        headers << headerText;
        m_frozenTable->setColumnWidth(col, m_mainTable->columnWidth(col));
    }
    m_frozenTable->setHorizontalHeaderLabels(headers);

    // 同步单元格内容
    for (int row = 0; row < m_mainTable->rowCount(); ++row) {
        // 同步行高
        m_frozenTable->setRowHeight(row, m_mainTable->rowHeight(row));

        for (int col = 0; col < m_frozenCount && col < m_mainTable->columnCount(); ++col) {
            QTableWidgetItem* srcItem = m_mainTable->item(row, col);
            if (srcItem) {
                QTableWidgetItem* newItem = new QTableWidgetItem(srcItem->text());
                newItem->setTextAlignment(srcItem->textAlignment());
                newItem->setForeground(srcItem->foreground());
                newItem->setBackground(srcItem->background());
                newItem->setFont(srcItem->font());
                newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
                m_frozenTable->setItem(row, col, newItem);
            } else {
                // 源单元格为空时，创建一个空白项而不是设置为nullptr
                QTableWidgetItem* emptyItem = new QTableWidgetItem("");
                emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEditable);
                m_frozenTable->setItem(row, col, emptyItem);
            }
        }
    }

    // 更新几何位置
    updateGeometry();

    // 同步选择状态
    syncSelection();

    // 同步垂直滚动位置
    m_frozenTable->verticalScrollBar()->setValue(m_mainTable->verticalScrollBar()->value());
}

void FrozenColumnHelper::setFrozenColumnCount(int count)
{
    if (count < 0) count = 0;
    if (m_mainTable && count > m_mainTable->columnCount()) {
        count = m_mainTable->columnCount();
    }
    m_frozenCount = count;
    syncFrozenContent();
}

bool FrozenColumnHelper::eventFilter(QObject* watched, QEvent* event)
{
    // 监听主表格viewport的大小变化
    if (watched == m_mainTable->viewport() && event->type() == QEvent::Resize) {
        updateGeometry();
    }
    return QObject::eventFilter(watched, event);
}
