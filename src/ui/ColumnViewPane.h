#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QSet>
#include <QPointer>
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "DropListView.h"
#include "FolderSectionWidget.h"

namespace QuarkMeta {

class ContentPanel;
class DualSectionPanel;

class ColumnViewPane : public QWidget {
    Q_OBJECT
public:
    explicit ColumnViewPane(const QString& path, ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewPane() override = default;

    QString currentPath() const { return m_path; }
    void loadDirectory();

    bool isActive() const { return m_isActive; }
    void setActive(bool active);

    void selectItemByPath(const QString& targetPath);
    void setPendingSelectPaths(const QSet<QString>& paths);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);

    DropListView* listView() const;
    DropListView* folderListView() const;
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    FilterProxyModel* folderProxyModel() const { return m_folderProxyModel; }
    FilterProxyModel* fileProxyModel() const { return m_fileProxyModel; }
    DiskItemModel* model() const { return m_model; }
    FolderSectionHeaderBar* folderHeader() const;

    void refreshVisibleThumbnails();

signals:
    void folderClicked(const QString& folderPath, int paneIndex);
    void fileClicked(const QString& filePath, int paneIndex);
    void folderExpandRequested(const QString& folderPath, int paneIndex);
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

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
    bool m_isActive = false;
    QScrollArea* m_paneScrollArea = nullptr;
    DualSectionPanel* m_panel = nullptr;
    DropListView* m_folderListView = nullptr;
    DropListView* m_listView = nullptr;
};

} // namespace QuarkMeta
