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
 * @brief 内容面板（面板四）：核心业务展示区（纯视图与交互路由层）
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
        ActionBatchRename, ActionRename, ActionCopy, ActionCut, ActionPaste, ActionCopyTags, ActionPasteTags, ActionRepeatLastOp, ActionDelete,
        ActionPermanentDelete, ActionSecureDelete, ActionRestore, ActionRestoreAll, ActionEmptyTrash,
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate
    };

    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    // 🚀【物理沙盒契约】：硬性向外报告 230px 下限，切断内部组件尺寸反向渗透
    QSize minimumSizeHint() const override { return QSize(230, 100); }
    void deferredInit() {}

    // 只读状态查询接口
    QString currentPath() const { return m_currentPath; }
    bool isRecursive() const { return m_isRecursive; }
    int zoomLevel() const { return m_zoomLevel; }
    ViewMode currentViewMode() const { return m_currentViewMode; }
    bool canPaste(const QString& targetOverride = QString()) const;
    DataSourceType dataSourceType() const;
    bool isContextMenuActive() const { return m_isContextMenuActive; }
    QString getCurrentCategoryType() const { return m_currentCategoryType; }
    int currentLoadRequestId() const { return m_loadRequestId.load(); }
    const FilterState& currentFilter() const { return m_currentFilter; }

    // 排序与控制访问器
    ContentSortController* sortController() const { return m_sortController; }
    SortType currentSortType() const { return m_sortController ? m_sortController->sortType() : SortType::SortByName; }
    Qt::SortOrder currentSortOrder() const { return m_sortController ? m_sortController->sortOrder() : Qt::AscendingOrder; }
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
    void setSortCriteria(SortType type, Qt::SortOrder order) { if (m_sortController) m_sortController->setSortCriteria(type, order); }
    void setContextMenuActive(bool active) { m_isContextMenuActive = active; }
    void setCurrentCategoryType(const QString& type) { m_currentCategoryType = type; }

    // 内部只读组件引用（保持向后兼容性）
    DiskItemModel* diskModel() const { return m_diskModel; }
    QPushButton* btnLayers() const { return m_btnLayers; }
    QPushButton* btnToggleFolders() const { return m_btnToggleFolders; }
    QPushButton* btnToggleFiles() const { return m_btnToggleFiles; }
    QPushButton* btnToggleHidden() const { return m_btnToggleHidden; }
    QStackedWidget* viewStack() const { return m_viewStack; }
    QAbstractItemView* gridView() const { return m_gridView; }
    QTreeView* treeView() const { return m_treeView; }
    ContentKeyHandler* keyHandler() const { return m_keyHandler; }

    // 业务操作转发
    void performCopy(bool cutMode);
    void performPaste();
    void performBatchRename();
    void setViewMode(ViewMode mode);
    void selectAndScrollToPath(const QString& path);
    void selectAndScrollToItem(const QString& path);
    QString getAdjacentFilePath(const QString& currentPath, int delta);

    bool eventFilter(QObject* obj, QEvent* event) override;

    // 模型数据访问
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
    void loadCategory(const QString& categoryType);
    void refreshVisibleThumbnails();

protected:
    void wheelEvent(QWheelEvent* event) override;

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

    // 🚀【状态 100% 私有化安全封装】
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
    std::atomic<bool> m_isLoading{false};
    bool m_isContextMenuActive = false;
    std::atomic<int> m_loadRequestId{0};

    // UI 组件私有指针
    QVBoxLayout* m_mainLayout = nullptr;
    QPushButton* m_btnLayers = nullptr;
    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    QTreeView* m_treeView = nullptr;
    DiskItemModel* m_diskModel = nullptr;
    ItemModelBase* m_model = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    
    QTimer* m_visibleTimer = nullptr;
    QTimer* m_selectionTimer = nullptr;
    ContentSortController* m_sortController = nullptr;
    ContentKeyHandler* m_keyHandler = nullptr;
};

} // namespace QuarkMeta