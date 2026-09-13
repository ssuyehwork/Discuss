#pragma once

#include <QWidget>
#include <QListView>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QList>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "DropListView.h"

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 单个列视图面板 (ColumnViewPane)
 */
class ColumnViewPane : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent = nullptr);

    QString path() const { return m_path; }
    DropListView* listView() const { return m_listView; }
    DiskItemModel* model() const { return m_model; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }

    void loadDirectory();
    void selectItemByPath(const QString& itemPath);
    void clearSelection();
    void setFilterState(const FilterState& state);

signals:
    void selectionChanged();
    void folderSelected(const QString& folderPath, ColumnViewPane* pane);
    void fileSelected(const QString& filePath, ColumnViewPane* pane);
    void recordsLoaded(const std::vector<QuarkMeta::ItemRecord>& records);

private slots:
    void onClicked(const QModelIndex& index);
    void onDoubleClicked(const QModelIndex& index);
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    DropListView* m_listView = nullptr;
};

/**
 * @brief 分栏视图（列视图）主容器 (ColumnViewWidget)
 */
class ColumnViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    void appendColumn(const QString& path);
    void dismissSubColumns(ColumnViewPane* targetPane);
    void dismissSubColumns(int fromIndex);
    
    ColumnViewPane* activePane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
    void clearOtherSelections(ColumnViewPane* currentPane);
    void clearAllColumns();
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);

    QList<ColumnViewPane*> panes() const { return m_panes; }

signals:
    void pathNavigated(const QString& path);
    void selectionChanged();
    void activeColumnRecordsChanged(const std::vector<QuarkMeta::ItemRecord>& records);

private slots:
    void onFolderSelected(const QString& folderPath, ColumnViewPane* pane);
    void onFileSelected(const QString& filePath, ColumnViewPane* pane);

private:
    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QString m_rootPath;
};

} // namespace QuarkMeta
