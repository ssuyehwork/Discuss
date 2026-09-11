#pragma once

#include <QScrollArea>
#include <QHBoxLayout>
#include <QListView>
#include <QLabel>
#include <QPointer>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"

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
    void clearSelection();
    void setFilterState(const FilterState& state);

    QListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);

private slots:
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
    ContentPanel* m_contentPanel = nullptr;
    DiskItemModel* m_model = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
    QListView* m_listView = nullptr;
};

class ColumnViewWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

    void setRootPath(const QString& path);
    void clearAllColumns();

    ColumnViewPane* activePane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);

signals:
    void pathNavigated(const QString& path);
    void selectionChanged();
    void activeColumnRecordsChanged(const std::vector<ItemRecord>& records);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
    void updatePaneWidths();

    ContentPanel* m_contentPanel = nullptr;
    FilterState m_currentFilter;
    int m_activePaneIndex = -1;
    bool m_autoScrollToRight = false;
    QWidget* m_container = nullptr;
    QHBoxLayout* m_layout = nullptr;
    QList<ColumnViewPane*> m_panes;
};

} // namespace QuarkMeta
