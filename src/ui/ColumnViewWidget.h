#pragma once

#include <QWidget>
#include <QListView>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QList>
#include <QLabel>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "DropListView.h"
#include "FolderSectionWidget.h"

namespace QuarkMeta {

class ContentPanel;

class ColumnViewPane : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewPane() override = default;

    QString currentPath() const { return m_path; }
    void loadDirectory();

    void selectItemByPath(const QString& targetPath);
    void setPendingSelectPaths(const QSet<QString>& paths);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);

    DropListView* listView() const { return m_listView; }
    DropListView* folderListView() const { return m_folderListView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }
    DiskItemModel* model() const { return m_model; }

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
    QSet<QString> m_pendingSelectPaths;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    DropListView* m_folderListView = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    QScrollArea* m_paneScrollArea = nullptr;
    QWidget* m_canvasWidget = nullptr;
    DropListView* m_listView = nullptr;
    QLabel* m_emptyFilterHintLabel = nullptr;
};

class ColumnViewWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

    void setRootPath(const QString& path);
    void clearAllColumns();

    ColumnViewPane* activePane() const;
    ColumnViewPane* rightmostPane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void refreshAllColumns();
    void updateMetadataForPath(const QString& path);
    void applySort(int sortType, Qt::SortOrder sortOrder);
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);
    void goUpColumn();
    void goUpColumnFromIndex(int paneIndex);
    void clearAllSelections();

signals:
    void pathNavigated(const QString& path);
    void selectionChanged();
    void activeColumnRecordsChanged(const std::vector<ItemRecord>& records);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
    void updatePaneWidths();
    void updateParentHighlights();

    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    int m_activePaneIndex = -1;
    bool m_autoScrollToRight = false;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
    QWidget* m_blankCanvasWidget = nullptr;
};

} // namespace QuarkMeta
