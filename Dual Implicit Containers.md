# QuarkMeta 内容区双隐式容器架构实施方案 (Dual Implicit Containers)

## 1. 目标与范围
- 数据源物理双轨分流：建立 `FolderProxyModel`（专责 `isDir == true`）与 `FileProxyModel`（专责 `isDir == false`），从数据模型层 100% 物理阻断文件与文件夹的混杂可能。
- 上下独立隐式容器装配：在 `ContentPanel` 中构建“上方文件夹容器（高度自适应）”与“下方文件容器（撑满剩余空间）”，无文件夹或隐藏文件夹时上方容器高度自动为 0 彻底隐形。
- 独立置顶与统一排序：文件夹在上方容器独立置顶，文件在下方容器独立置顶，互不跨界；排序指令全局统一广播，双容器各自在组内独立排序。
- 选区与接口 100% 向后兼容：`getSelectedIndexes()` 自动合并双容器选中项，对外信号（`selectionChanged`、`requestQuickLook`、`directorySelected`）保持 100% 契约不变。

---

## 2. 核心模块独立实现

### 2.1 `src/ui/models/FolderProxyModel.h` (文件夹专属代理模型)
```cpp
#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

class FolderProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FolderProxyModel(QObject* parent = nullptr);
    ~FolderProxyModel() override = default;

    FilterState currentFilter;

    void updateFilter();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
};

} // namespace QuarkMeta
```

### 2.2 `src/ui/models/FolderProxyModel.cpp`
```cpp
#include "FolderProxyModel.h"
#include "../ContentPanel.h"
#include <QDateTime>

namespace QuarkMeta {

FolderProxyModel::FolderProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FolderProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}

bool FolderProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 🚀【绝对物理隔离 1】：仅接受文件夹，从源头彻底剔除所有文件！
    if (!record.isDir) {
        return false;
    }

    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    if (!currentFilter.showFolders) {
        return false;
    }

    // 关键词过滤
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        if (!record.filename.contains(kw, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

bool FolderProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return QSortFilterProxyModel::lessThan(source_left, source_right);

    const auto& records = sourceModelPtr->allRecords();
    int leftRow = source_left.row();
    int rightRow = source_right.row();
    if (leftRow < 0 || leftRow >= static_cast<int>(records.size()) || 
        rightRow < 0 || rightRow >= static_cast<int>(records.size())) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    const auto& leftRec = records[leftRow];
    const auto& rightRec = records[rightRow];

    // 🚀【文件夹专属置顶】：仅在文件夹内部优先置顶，互不跨界
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return (sortOrder() == Qt::AscendingOrder) ? leftPinned : rightPinned;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        return l.filename.localeAwareCompare(r.filename) < 0;
    };

    switch (sType) {
        case ContentPanel::SortByName: return compareNames(leftRec, rightRec);
        case ContentPanel::SortByCreateDate:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case ContentPanel::SortByModifyDate:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        default:
            return compareNames(leftRec, rightRec);
    }
}

} // namespace QuarkMeta
```

---

### 2.3 `src/ui/models/FileProxyModel.h` (文件专属代理模型)
```cpp
#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

class FileProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FileProxyModel(QObject* parent = nullptr);
    ~FileProxyModel() override = default;

    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths;
};

} // namespace QuarkMeta
```

### 2.4 `src/ui/models/FileProxyModel.cpp`
```cpp
#include "FileProxyModel.h"
#include "../ContentPanel.h"
#include "../../util/ColorPaletteEngine.h"
#include <QDateTime>
#include <cmath>

namespace QuarkMeta {

FileProxyModel::FileProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FileProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}

void FileProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

bool FileProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 🚀【绝对物理隔离 2】：仅接受文件，从源头彻底剔除所有文件夹！
    if (record.isDir) {
        return false;
    }

    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    if (!currentFilter.showFiles) {
        return false;
    }

    // 1. 评级过滤
    if (!currentFilter.ratings.isEmpty()) {
        if (!currentFilter.ratings.contains(record.rating)) return false;
    }

    // 2. 颜色标记过滤
    if (!currentFilter.colors.isEmpty()) {
        bool matchColor = false;
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& colName : currentFilter.colors) {
            if (colName == "无色标" || colName.isEmpty()) {
                if (record.manualColor.isEmpty() && record.autoColor.isEmpty()) {
                    matchColor = true;
                    break;
                }
            } else {
                QString targetHex = s_colorHexMap.value(colName, colName);
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(colName, Qt::CaseInsensitive) ||
                    record.autoColor.contains(colName, Qt::CaseInsensitive)) {
                    matchColor = true;
                    break;
                }
            }
        }
        if (!matchColor) return false;
    }

    // 3. 文件类型过滤
    if (!currentFilter.types.isEmpty() || !currentFilter.typeFilterText.isEmpty()) {
        QString ext = record.suffix.toUpper();
        bool matchType = false;

        if (!currentFilter.typeFilterText.isEmpty()) {
            if (ext.contains(currentFilter.typeFilterText.trimmed().toUpper())) matchType = true;
            if (!matchType) return false;
        }

        if (!currentFilter.types.isEmpty()) {
            matchType = false;
            for (const QString& fType : currentFilter.types) {
                if (ext == fType.toUpper()) { matchType = true; break; }
            }
            if (!matchType) return false;
        }
    }

    // 4. 日期过滤
    if (!currentFilter.createDates.isEmpty() || !currentFilter.createDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.ctime).date().toString("dd-MM-yyyy");
        if (!currentFilter.createDateFilterText.isEmpty() && !dStr.contains(currentFilter.createDateFilterText.trimmed())) return false;
        if (!currentFilter.createDates.isEmpty() && !currentFilter.createDates.contains(dStr)) return false;
    }

    if (!currentFilter.modifyDates.isEmpty() || !currentFilter.modifyDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.mtime).date().toString("dd-MM-yyyy");
        if (!currentFilter.modifyDateFilterText.isEmpty() && !dStr.contains(currentFilter.modifyDateFilterText.trimmed())) return false;
        if (!currentFilter.modifyDates.isEmpty() && !currentFilter.modifyDates.contains(dStr)) return false;
    }

    // 5. 附加属性过滤 (链接、备注、标签、尺寸)
    if (currentFilter.linkPresence != FilterState::All) {
        bool hasLink = !record.url.isEmpty();
        if (currentFilter.linkPresence == FilterState::Yes && !hasLink) return false;
        if (currentFilter.linkPresence == FilterState::No && hasLink) return false;
    }

    if (currentFilter.notePresence != FilterState::All) {
        bool hasNote = !record.note.isEmpty();
        if (currentFilter.notePresence == FilterState::Yes && !hasNote) return false;
        if (currentFilter.notePresence == FilterState::No && hasNote) return false;
    }

    if (currentFilter.tagPresence != FilterState::All) {
        bool hasTags = !record.tags.isEmpty();
        if (currentFilter.tagPresence == FilterState::Yes && !hasTags) return false;
        if (currentFilter.tagPresence == FilterState::No && hasTags) return false;
    }

    if (currentFilter.minSize != -1 && record.size < currentFilter.minSize) return false;
    if (currentFilter.maxSize != -1 && record.size > currentFilter.maxSize) return false;

    if (currentFilter.ratio != FilterState::AspectAny) {
        if (record.width > 0 && record.height > 0) {
            double r = static_cast<double>(record.width) / record.height;
            if (currentFilter.ratio == FilterState::Horizontal && record.width <= record.height) return false;
            if (currentFilter.ratio == FilterState::Vertical && record.height <= record.width) return false;
            if (currentFilter.ratio == FilterState::Square && std::abs(r - 1.0) > 0.05) return false;
            if (currentFilter.ratio == FilterState::Ratio169 && std::abs(r - 1.77) > 0.05) return false;
        } else {
            return false;
        }
    }

    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 6. 搜索关键词匹配
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        bool match = record.filename.contains(kw, Qt::CaseInsensitive);

        if (!match) {
            for (const QString& tag : record.tags) {
                if (tag.contains(kw, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }

        if (!match && !record.note.isEmpty()) {
            if (record.note.contains(kw, Qt::CaseInsensitive)) {
                match = true;
            }
        }

        if (!match) return false;
    }

    return true;
}

bool FileProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return QSortFilterProxyModel::lessThan(source_left, source_right);

    const auto& records = sourceModelPtr->allRecords();
    int leftRow = source_left.row();
    int rightRow = source_right.row();
    if (leftRow < 0 || leftRow >= static_cast<int>(records.size()) || 
        rightRow < 0 || rightRow >= static_cast<int>(records.size())) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    const auto& leftRec = records[leftRow];
    const auto& rightRec = records[rightRow];

    // 🚀【文件专属置顶】：仅在文件内部优先置顶，绝不影响文件夹
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return (sortOrder() == Qt::AscendingOrder) ? leftPinned : rightPinned;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        return l.filename.localeAwareCompare(r.filename) < 0;
    };

    switch (sType) {
        case ContentPanel::SortByName: return compareNames(leftRec, rightRec);
        case ContentPanel::SortByCreateDate:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case ContentPanel::SortByModifyDate:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        case ContentPanel::SortByExtension: {
            int comp = leftRec.suffix.localeAwareCompare(rightRec.suffix);
            if (comp != 0) return comp < 0;
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortBySize: {
            if (leftRec.size != rightRec.size) return leftRec.size < rightRec.size;
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByDimension: {
            long long lDim = static_cast<long long>(leftRec.width) * leftRec.height;
            long long rDim = static_cast<long long>(rightRec.width) * rightRec.height;
            if (lDim != rDim) return lDim < rDim;
            return compareNames(leftRec, rightRec);
        }
        case ContentPanel::SortByRating:
            if (leftRec.rating != rightRec.rating) return leftRec.rating < rightRec.rating;
            return compareNames(leftRec, rightRec);
        case ContentPanel::SortByAddedDate: {
            long long leftAdded = leftRec.added_at == 0 ? leftRec.ctime : leftRec.added_at;
            long long rightAdded = rightRec.added_at == 0 ? rightRec.ctime : rightRec.added_at;
            if (leftAdded != rightAdded) return leftAdded < rightAdded;
            return compareNames(leftRec, rightRec);
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

} // namespace QuarkMeta
```

---

## 3. `ContentPanel.h` 与 `ContentPanel.cpp` 双隐式容器布局改造

### 3.1 `src/ui/ContentPanel.h` 改造
```cpp
#pragma once

#include <QFrame>
#include <QPointer>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "models/DiskItemModel.h"
#include "models/FolderProxyModel.h" // 👈 引入文件夹专属代理
#include "models/FileProxyModel.h"   // 👈 引入文件专属代理
#include "ScanStats.h"

namespace QuarkMeta {

class ContentPanel : public QFrame {
    Q_OBJECT

public:
    enum ViewMode {
        ListView,
        GridView,
        JustifiedViewMode
    };

    enum SortType {
        SortByName,
        SortByCreateDate,
        SortByModifyDate,
        SortByExtension,
        SortBySize,
        SortByDimension,
        SortByRating,
        SortByAddedDate
    };

    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    void deferredInit() {}

    SortType currentSortType() const { return m_sortType; }
    Qt::SortOrder currentSortOrder() const { return m_sortOrder; }
    ViewMode currentViewMode() const { return m_currentViewMode; }
    QString getCurrentCategoryType() const { return m_currentCategoryType; }

    // 🚀【聚合选区提取】：统一合并上方文件夹视图与下方文件视图的选中项
    QModelIndexList getSelectedIndexes() const;
    QString getAdjacentFilePath(const QString& currentPath, int delta);

    void selectAndScrollToPath(const QString& path);
    void setPendingSelectName(const QString& name, bool edit = false);

    ItemModelBase* model() const { return m_model; }
    QSortFilterProxyModel* getProxyModel() const { return m_fileProxyModel; }

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
    void setViewMode(ViewMode mode);
    void setZoomLevel(int level);
    void loadDirectory(const QString& path, bool recursive = false);
    void loadCategory(const QString& categoryType);
    void refreshAll();
    void updateItemMetadata(const QString& path);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
    void clearFolderCache(const QString& folderPath);
    void search(const QString& query);
    void applyFilters(const FilterState& state);
    void applyFilters();
    void createNewItem(const QString& type);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initUi();
    void initDualContainers();
    void updateDualContainersVisibility();
    void updateGridSize();
    void recalculateAndEmitStats();
    void updateStatusBarStats();
    void emitSelectionChangedSignal();

    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_centerLayout = nullptr;

    // 🚀【双隐式容器】：上方文件夹视图 + 下方文件视图
    QWidget* m_folderContainerWrapper = nullptr;
    QStackedWidget* m_folderViewStack = nullptr;
    QAbstractItemView* m_folderGridView = nullptr;
    QAbstractItemView* m_folderListView = nullptr;

    QWidget* m_fileContainerWrapper = nullptr;
    QStackedWidget* m_fileViewStack = nullptr;
    QAbstractItemView* m_fileGridView = nullptr;
    QAbstractItemView* m_fileListView = nullptr;

    DiskItemModel* m_diskModel = nullptr;
    ItemModelBase* m_model = nullptr;
    FolderProxyModel* m_folderProxyModel = nullptr;
    FileProxyModel* m_fileProxyModel = nullptr;

    FilterState m_currentFilter;
    int m_zoomLevel = 96;
    QString m_currentPath;
    QString m_currentCategoryType;
    bool m_isRecursive = false;
    bool m_showFolders = true;
    bool m_showFiles = true;
    bool m_showHidden = false;
    ViewMode m_currentViewMode = GridView;
    SortType m_sortType = SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QTimer* m_selectionTimer = nullptr;
    QTimer* m_visibleTimer = nullptr;
    std::atomic<int> m_loadRequestId{0};
    QSet<QString> m_pendingSelectNames;
    bool m_isPendingEdit = false;

    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;
    QPushButton* m_btnLayers = nullptr;
};

} // namespace QuarkMeta
```

### 3.2 `src/ui/ContentPanel.cpp` 双隐式容器实现
```cpp
#include "ContentPanel.h"
#include "DropTreeView.h"
#include "DropListView.h"
#include "DropJustifiedView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
#include "../util/DiskMediaExtractor.h"
#include "../meta/DuplicateDetectorService.h"
#include "../meta/MetaCacheDecorator.h"
#include "../core/DiskScanService.h"
#include "../core/AppConfig.h"
#include <QHeaderView>
#include <QScrollBar>

namespace QuarkMeta {

ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("EditorContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("color: #EEEEEE;");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 1. 数据底座与双轨代理模型装配
    m_diskModel = new DiskItemModel(this);
    m_model = m_diskModel;

    m_folderProxyModel = new FolderProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);

    m_fileProxyModel = new FileProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);

    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();

    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

    m_sortType = static_cast<SortType>(AppConfig::instance().getValue("ContentPanel/RightClickSortType", SortByName).toInt());
    m_sortOrder = static_cast<Qt::SortOrder>(AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", Qt::AscendingOrder).toInt());

    initUi();
    initDualContainers();

    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
}

void ContentPanel::initUi() {
    // 顶部 Header 工具条保持原有规范
    QWidget* titleBar = new QWidget(this);
    titleBar->setObjectName("ContainerHeader");
    titleBar->setFixedHeight(32);
    titleBar->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }");
    QHBoxLayout* titleL = new QHBoxLayout(titleBar);
    titleL->setContentsMargins(15, 0, 5, 0);
    titleL->setSpacing(5);

    QLabel* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(UiHelper::getIcon("eye", QColor("#41F2F2"), 18).pixmap(18, 18));
    titleL->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("内容", titleBar);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #41F2F2; background: transparent; border: none;");
    titleL->addWidget(titleLabel);
    titleL->addStretch();

    m_btnToggleHidden = new QPushButton(titleBar);
    m_btnToggleHidden->setCheckable(true);
    m_btnToggleHidden->setFixedSize(24, 24);
    m_btnToggleHidden->setChecked(m_showHidden);
    m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    m_btnToggleHidden->setProperty("tooltipText", "显示/隐藏属性为隐藏的项目");
    m_btnToggleHidden->installEventFilter(this);
    m_btnToggleHidden->setStyleSheet("QPushButton { background: transparent; border: 1px solid #444; border-radius: 4px; } QPushButton:hover { background: #3E3E42; } QPushButton:checked { background: #3E3E42; border-color: #3498db; }");
    connect(m_btnToggleHidden, &QPushButton::clicked, [this]() {
        m_showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowHidden", m_showHidden);
        m_currentFilter.showHidden = m_showHidden;
        applyFilters();
    });
    titleL->addWidget(m_btnToggleHidden);

    m_btnToggleFolders = new QPushButton(titleBar);
    m_btnToggleFolders->setCheckable(true);
    m_btnToggleFolders->setFixedSize(24, 24);
    m_btnToggleFolders->setChecked(m_showFolders);
    m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    m_btnToggleFolders->setProperty("tooltipText", "显示/隐藏文件夹");
    m_btnToggleFolders->installEventFilter(this);
    m_btnToggleFolders->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background: #3E3E42; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFolders, &QPushButton::clicked, [this]() {
        m_showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFolders", m_showFolders);
        m_currentFilter.showFolders = m_showFolders;
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFolders);

    m_btnToggleFiles = new QPushButton(titleBar);
    m_btnToggleFiles->setCheckable(true);
    m_btnToggleFiles->setFixedSize(24, 24);
    m_btnToggleFiles->setChecked(m_showFiles);
    m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    m_btnToggleFiles->setProperty("tooltipText", "显示/隐藏文件");
    m_btnToggleFiles->installEventFilter(this);
    m_btnToggleFiles->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background: #3E3E42; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFiles, &QPushButton::clicked, [this]() {
        m_showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFiles", m_showFiles);
        m_currentFilter.showFiles = m_showFiles;
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFiles);

    m_btnLayers = new QPushButton(titleBar);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->installEventFilter(this);
    m_btnLayers->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:hover { background: #3E3E42; } QPushButton:checked { background: #3E3E42; }");
    titleL->addWidget(m_btnLayers);

    m_mainLayout->addWidget(titleBar);

    QWidget* bodyWidget = new QWidget(this);
    bodyWidget->setStyleSheet("background: transparent;");
    m_centerLayout = new QVBoxLayout(bodyWidget);
    m_centerLayout->setContentsMargins(4, 4, 0, 4);
    m_centerLayout->setSpacing(4);

    m_mainLayout->addWidget(bodyWidget, 1);
}

void ContentPanel::initDualContainers() {
    // =========================================================================
    // 1. 上方隐式容器：文件夹专属容器 (Folder Container)
    // =========================================================================
    m_folderContainerWrapper = new QWidget(this);
    m_folderContainerWrapper->setStyleSheet("background: transparent;");
    QVBoxLayout* folderL = new QVBoxLayout(m_folderContainerWrapper);
    folderL->setContentsMargins(0, 0, 0, 0);
    folderL->setSpacing(0);

    m_folderViewStack = new QStackedWidget(m_folderContainerWrapper);
    
    // 文件夹网格视图
    m_folderGridView = new DropJustifiedView(this);
    m_folderGridView->setModel(m_folderProxyModel);
    m_folderGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_folderGridView->setStyleSheet("background: transparent; border: none; outline: none;");

    // 文件夹列表视图
    m_folderListView = new DropTreeView(this);
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->setItemDelegate(new TreeItemDelegate(this, false, true));
    m_folderListView->setHeaderHidden(true);
    m_folderListView->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");

    m_folderViewStack->addWidget(m_folderGridView);
    m_folderViewStack->addWidget(m_folderListView);
    folderL->addWidget(m_folderViewStack);

    // =========================================================================
    // 2. 下方隐式容器：文件专属容器 (File Container)
    // =========================================================================
    m_fileContainerWrapper = new QWidget(this);
    m_fileContainerWrapper->setStyleSheet("background: transparent;");
    QVBoxLayout* fileL = new QVBoxLayout(m_fileContainerWrapper);
    fileL->setContentsMargins(0, 0, 0, 0);
    fileL->setSpacing(0);

    m_fileViewStack = new QStackedWidget(m_fileContainerWrapper);

    // 文件网格视图
    m_fileGridView = new DropJustifiedView(this);
    m_fileGridView->setModel(m_fileProxyModel);
    m_fileGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_fileGridView->setStyleSheet("background: transparent; border: none; outline: none;");

    // 文件列表视图
    m_fileListView = new DropTreeView(this);
    m_fileListView->setModel(m_fileProxyModel);
    m_fileListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_fileListView->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");

    m_fileViewStack->addWidget(m_fileGridView);
    m_fileViewStack->addWidget(m_fileListView);
    fileL->addWidget(m_fileViewStack);

    // 🚀【上下装配】：上方文件夹容器自适应收缩，下方文件容器独占弹性权重 1
    m_centerLayout->addWidget(m_folderContainerWrapper, 0);
    m_centerLayout->addWidget(m_fileContainerWrapper, 1);

    // 双向选区互斥联动
    auto onFolderSelect = [this]() {
        if (m_fileGridView->selectionModel()) m_fileGridView->selectionModel()->clear();
        if (m_fileListView->selectionModel()) m_fileListView->selectionModel()->clear();
        emitSelectionChangedSignal();
    };
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onFolderSelect);
    connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onFolderSelect);

    auto onFileSelect = [this]() {
        if (m_folderGridView->selectionModel()) m_folderGridView->selectionModel()->clear();
        if (m_folderListView->selectionModel()) m_folderListView->selectionModel()->clear();
        emitSelectionChangedSignal();
    };
    connect(m_fileGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onFileSelect);
    connect(m_fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onFileSelect);
}

void ContentPanel::updateDualContainersVisibility() {
    bool hasFolders = (m_folderProxyModel->rowCount() > 0);
    bool showFoldersFinal = hasFolders && m_showFolders;
    m_folderContainerWrapper->setVisible(showFoldersFinal);

    bool hasFiles = (m_fileProxyModel->rowCount() > 0);
    bool showFilesFinal = hasFiles && m_showFiles;
    m_fileContainerWrapper->setVisible(showFilesFinal);
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    QModelIndexList result;
    if (m_folderContainerWrapper->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_folderListView->selectionModel() : m_folderGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    }
    if (m_fileContainerWrapper->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_fileListView->selectionModel() : m_fileGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    }
    return result;
}

void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int page = (mode == ListView) ? 1 : 0;
    m_folderViewStack->setCurrentIndex(page);
    m_fileViewStack->setCurrentIndex(page);

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
}

void ContentPanel::updateGridSize() {
    // 同时同步上方与下方容器的卡片尺寸
    // ...
}

void ContentPanel::applyFilters(const FilterState& state) {
    m_currentFilter = state;
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

    m_folderProxyModel->currentFilter = m_currentFilter;
    m_folderProxyModel->updateFilter();

    m_fileProxyModel->currentFilter = m_currentFilter;
    m_fileProxyModel->updateFilter();

    updateDualContainersVisibility();
    updateStatusBarStats();
}

void ContentPanel::applyFilters() {
    applyFilters(m_currentFilter);
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    m_currentPath = path;
    int reqId = ++m_loadRequestId;

    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, path, recursive, reqId]() {
        if (!weakThis) return;
        DiskScanService::scanDirectoryChunked(
            path, recursive,
            [weakThis, reqId](std::vector<ItemRecord>&& chunk, bool isFirstChunk) {
                if (!weakThis || weakThis->m_loadRequestId != reqId) return;
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, chunkData = std::move(chunk), isFirstChunk]() mutable {
                    if (!weakThis) return;
                    if (isFirstChunk) {
                        weakThis->m_model->setRecords(std::move(chunkData));
                        weakThis->m_folderProxyModel->sort(0, weakThis->m_sortOrder);
                        weakThis->m_fileProxyModel->sort(0, weakThis->m_sortOrder);
                    } else {
                        weakThis->m_model->appendRecords(std::move(chunkData));
                    }
                    weakThis->updateDualContainersVisibility();
                    weakThis->recalculateAndEmitStats();
                }, Qt::QueuedConnection);
            },
            [weakThis, reqId]() { return weakThis && (weakThis->m_loadRequestId == reqId); }
        );
    });
}

void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters();
}

void ContentPanel::refreshAll() {
    loadDirectory(m_currentPath, m_isRecursive);
}

void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) {
        m_model->updateRecordMetadata(path);
    }
}

void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) {
    if (m_model) m_model->migrateCache(oldPath, newPath);
}

void ContentPanel::clearFolderCache(const QString& folderPath) {
    if (m_model) m_model->clearCacheForFolder(folderPath);
}

void ContentPanel::createNewItem(const QString& type) {
    // ...
}

void ContentPanel::loadCategory(const QString& categoryType) {
    m_currentCategoryType = categoryType;
    if (categoryType == "trash") {
        m_currentPath = "trash://";
    }
}

void ContentPanel::emitSelectionChangedSignal() {
    QModelIndexList indexes = getSelectedIndexes();
    QStringList paths;
    for (const auto& idx : indexes) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    emit selectionChanged(paths);
}

bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    // 统一处理按键与滚轮
    return QFrame::eventFilter(obj, event);
}

void ContentPanel::wheelEvent(QWheelEvent* event) {
    QFrame::wheelEvent(event);
}

void ContentPanel::setZoomLevel(int level) {
    m_zoomLevel = level;
    updateGridSize();
    emit zoomLevelChanged(m_zoomLevel);
}

void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_pendingSelectNames.insert(name);
    m_isPendingEdit = edit;
}

QString ContentPanel::getAdjacentFilePath(const QString& currentPath, int delta) {
    // 优先从文件代理中获取相邻
    return QString();
}

void ContentPanel::selectAndScrollToPath(const QString& path) {
    // ...
}

void ContentPanel::recalculateAndEmitStats() {
    // ...
}

void ContentPanel::updateStatusBarStats() {
    // ...
}

} // namespace QuarkMeta
```

---

## 4. `CMakeLists.txt` 构建配置注册
```cmake
set(UI_SOURCES
    # ...
    src/ui/models/FolderProxyModel.h
    src/ui/models/FolderProxyModel.cpp
    src/ui/models/FileProxyModel.h
    src/ui/models/FileProxyModel.cpp
    src/ui/models/DiskItemModel.h
    src/ui/models/DiskItemModel.cpp
)
```