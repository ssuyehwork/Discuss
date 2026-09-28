#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include "FolderSectionWidget.h"
#include "DropTreeView.h"
#include "models/FilterProxyModel.h"
#include "models/DiskItemModel.h"

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 列表模式双列表解耦容器：上方文件夹列表 + 下方文件列表
 */
class ListViewSectionContainer : public QWidget {
    Q_OBJECT

public:
    explicit ListViewSectionContainer(ContentPanel* panel, QWidget* parent = nullptr);
    ~ListViewSectionContainer() override = default;

    void setModel(QSortFilterProxyModel* mainProxyModel);
    void applyFilters(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);
    void toggleFolderSectionCollapse();

    DropTreeView* folderListView() const { return m_folderListView; }
    DropTreeView* fileListView() const { return m_fileListView; }

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceModel);

private:
    void setupUi();
    void syncHeaderColumnWidths();

    ContentPanel* m_panel = nullptr;
    QSortFilterProxyModel* m_mainProxyModel = nullptr;

    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QVBoxLayout* m_mainLayout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    DropTreeView* m_folderListView = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    DropTreeView* m_fileListView = nullptr;
};

} // namespace QuarkMeta
