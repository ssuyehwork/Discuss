#pragma once

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QListView>
#include <QStyledItemDelegate>
#include <QList>

namespace QuarkMeta {

class DiskItemModel;
class FilterProxyModel;

/**
 * @brief 列视图专用项委托：负责图标、文字排版及文件夹右侧的展开箭头 (>)
 */
class MillerColumnDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit MillerColumnDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

/**
 * @brief 单个独立纵列面板 (Column Pane)
 */
class MillerColumnPane : public QWidget {
    Q_OBJECT
public:
    explicit MillerColumnPane(const QString& path, QWidget* parent = nullptr);
    ~MillerColumnPane() override = default;

    QString path() const { return m_path; }
    QListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* diskModel() const { return m_diskModel; }

signals:
    void itemSelected(const QModelIndex& proxyIndex, const QString& itemPath, bool isDir);
    void itemDoubleClicked(const QString& itemPath);

private:
    void initUi();
    void loadDataAsync();

    QString m_path;
    QListView* m_listView = nullptr;
    DiskItemModel* m_diskModel = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
};

/**
 * @brief 级联分栏列视图核心控制器 (Miller Columns View)
 */
class MillerColumnsView : public QAbstractItemView {
    Q_OBJECT
public:
    explicit MillerColumnsView(QWidget* parent = nullptr);
    ~MillerColumnsView() override = default;

    void setRootPath(const QString& rootPath);
    QString rootPath() const { return m_rootPath; }

    // 真实的选区与模型穿透接口
    QModelIndexList selectedIndexes() const override;

    // QAbstractItemView 契约实现
    QRect visualRect(const QModelIndex& index) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint& point) const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex& index) const override;
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

signals:
    void fileActivated(const QString& path);
    void fileSelected(const QString& path);
    void directoryNavigated(const QString& path);

private:
    void appendColumn(const QString& folderPath, int parentPaneIndex);
    void truncateColumnsAfter(int paneIndex);
    void scrollToRightmost();

    QString m_rootPath;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_containerWidget = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<MillerColumnPane*> m_panes;

    // 保持与当前活跃分栏 selectionModel 的桥接
    QModelIndex m_currentActiveIndex;
};

} // namespace QuarkMeta