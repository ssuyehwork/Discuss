#pragma once

#include <QFrame>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTreeView>
#include <QListView>
#include <QTimer>
#include <QSet>
#include <QModelIndexList>
#include <atomic>

#include "ScanStats.h"
#include "FilterPanel.h"
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "controllers/ContentSortController.h"
#include "../core/ModelContract.h"
#include "../core/ItemRecord.h"

namespace QuarkMeta {

class ContentKeyHandler;

/**
 * @brief 内容面板（面板四）：核心业务展示区
 */
class ContentPanel : public QFrame {
    Q_OBJECT

public:
    enum class DataSourceType {
        DiskNav,
        PathList
    };

    using SortType = QuarkMeta::SortType;
    static constexpr SortType SortByName = SortType::SortByName;
    static constexpr SortType SortByCreateDate = SortType::SortByCreateDate;
    static constexpr SortType SortByModifyDate = SortType::SortByModifyDate;
    static constexpr SortType SortByExtension = SortType::SortByExtension;
    static constexpr SortType SortBySize = SortType::SortBySize;
    static constexpr SortType SortByDimension = SortType::SortByDimension;
    static constexpr SortType SortByRating = SortType::SortByRating;
    static constexpr SortType SortByAddedDate = SortType::SortByAddedDate;

    enum ViewMode {
        ListView,
        GridView,
        JustifiedViewMode
    };

    enum ContextAction {
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionNewFolder, ActionNewMd, ActionNewTxt,
        ActionPin, ActionUnpin, ActionColorTag, ActionEncrypt, ActionDecrypt, ActionChangePwd,
        ActionBatchRename, ActionRename, ActionCopy, ActionCut, ActionPaste, ActionCopyTags, ActionPasteTags, ActionDelete,
        ActionPermanentDelete, ActionSecureDelete, ActionRestore, ActionRestoreAll, ActionEmptyTrash,
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate
    };

    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    void deferredInit() {}

    bool canPaste(const QString& targetOverride = QString()) const;
    DataSourceType dataSourceType() const;
    bool isContextMenuActive() const { return m_isContextMenuActive; }

    ContentSortController* sortController() const { return m_sortController; }
    SortType currentSortType() const { return m_sortController ? m_sortController->sortType() : SortType::SortByName; }
    Qt::SortOrder currentSortOrder() const { return m_sortController ? m_sortController->sortOrder() : Qt::AscendingOrder; }
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
    void setSortCriteria(SortType type, Qt::SortOrder order) { if (m_sortController) m_sortController->setSortCriteria(type, order); }
    void setContextMenuActive(bool active) { m_isContextMenuActive = active; }

    QString currentPath() const { return m_currentPath; }
    bool isRecursive() const { return m_isRecursive; }
    int zoomLevel() const { return m_zoomLevel; }
    DiskItemModel* diskModel() const { return m_diskModel; }
    QPushButton* btnLayers() const { return m_btnLayers; }
    QPushButton* btnToggleFolders() const { return m_btnToggleFolders; }
    QPushButton* btnToggleFiles() const { return m_btnToggleFiles; }
    QPushButton* btnToggleHidden() const { return m_btnToggleHidden; }
    QStackedWidget* viewStack() const { return m_viewStack; }
    QAbstractItemView* gridView() const { return m_gridView; }
    QTreeView* treeView() const { return m_treeView; }
    ContentKeyHandler* keyHandler() const { return m_keyHandler; }

    void performCopy(bool cutMode);
    void performPaste();
    void performBatchRename();

    ViewMode currentViewMode() const { return m_currentViewMode; }
    void setViewMode(ViewMode mode);
    void selectAndScrollToPath(const QString& path);
    void selectAndScrollToItem(const QString& path);
    QString getAdjacentFilePath(const QString& currentPath, int delta);

    bool eventFilter(QObject* obj, QEvent* event) override;

    QAbstractItemModel* model() const { return m_model; }
    QSortFilterProxyModel* getProxyModel() const { return m_proxyModel; }
    QStringList getSelectedPaths() const;
    QList<int> getSelectedTrashIds() const;
    QModelIndexList getSelectedIndexes() const;

signals:
    void zoomLevelChanged(int level);
    void viewModeChanged(ViewMode mode);
    void requestQuickLook(const QString& path);
    void selectionChanged(const QStringList& paths);
    void directorySelected(const QString& path);
    void requestAddFavorite(const QStringList& paths);
    void dataSourceChanged(const QString& source);
    void directoryStatsReady(const QuarkMeta::ScanStats& stats);
    void statusBarStatsUpdated(int fileCount, int folderCount, int totalCount);

public slots:
    void setZoomLevel(int level);
    void onSelectionChanged();
    void onCustomContextMenuRequested(const QPoint& pos);
    void onDoubleClicked(const QModelIndex& index);
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
    void loadDirectory(const QString& path, bool recursive = false);
    void setPendingSelectName(const QString& name, bool edit = false);
    void refreshAll();
    void updateItemMetadata(const QString& path);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
    void clearFolderCache(const QString& folderPath);
    void search(const QString& query);
    void applyFilters(const FilterState& state);
    void applyFilters();
    void createNewItem(const QString& type);
    void loadPaths(const QStringList& paths, int reqId = 0);
    void appendPaths(const QStringList& paths, int reqId = 0);
    int currentLoadRequestId() const { return m_loadRequestId.load(); }
    void loadCategory(const QString& categoryType);
    QString getCurrentCategoryType() const { return m_currentCategoryType; }
    void setCurrentCategoryType(const QString& type) { m_currentCategoryType = type; }
    void refreshVisibleThumbnails();

public:
    FilterState m_currentFilter;
    int m_zoomLevel = 96;
    QString m_currentPath;
    QSet<QString> m_pendingSelectNames;
    bool m_isPendingEdit = false;
    QString m_currentCategoryType;
    bool m_isRecursive = false;
    bool m_showFolders = true;
    bool m_showFiles = true;
    bool m_showHidden = false;
    ViewMode m_currentViewMode = GridView;
    ContentSortController* m_sortController = nullptr;
    std::atomic<bool> m_isLoading{false};
    bool m_isContextMenuActive = false;
    std::atomic<int> m_loadRequestId{0};

    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;

    QAbstractItemView* m_gridView = nullptr;
    QTreeView* m_treeView = nullptr;
    DiskItemModel* m_diskModel = nullptr;

private:
    void initUi();
    void initGridView();
    void initListView();
    void updateLayersButtonState();
    void updateGridSize();
    void updateStatusBarStats();
    void recalculateAndEmitStats();
    bool resolvePasteDestination();
    void restoreActiveView();
    void restoreSelections();
    void emitSelectionChangedSignal();

    QVBoxLayout* m_mainLayout = nullptr;
    QStackedWidget* m_viewStack = nullptr;
    ItemModelBase* m_model = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    QTimer* m_visibleTimer = nullptr;
    QTimer* m_selectionTimer = nullptr;
    ContentKeyHandler* m_keyHandler = nullptr;

protected:
    void wheelEvent(QWheelEvent* event) override;
};

} // namespace QuarkMeta