# QuarkMeta 搜索中枢与多维筛选流体系实施方案

## 1. 目标与范围
- 独立抽离 `FilterProxyModel`：将内嵌在 `ContentPanel` 中的过滤与排序算法彻底剥离，独立建立 `src/ui/models/FilterProxyModel.h` 与 `FilterProxyModel.cpp`。
- 筛选状态唯一真理源归一化：将搜索关键词（`keyword`）、11 维属性条件及显隐状态统一收敛，彻底消灭 `ContentPanel` 与 `PanelMediator` 中的胶水拼接与状态备份补丁。
- 净化 `ContentPanel.h/cpp`：移除 300 行代理模型代码，使内容面板回归纯粹的 View 容器。

---

## 2. 核心模块独立实现

### 2.1 `src/ui/models/FilterProxyModel.h`
```cpp
#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

/**
 * @brief 独立的高级多维条件过滤与加权排序代理模型
 */
class FilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FilterProxyModel(QObject* parent = nullptr);
    ~FilterProxyModel() override = default;

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

### 2.2 `src/ui/models/FilterProxyModel.cpp`
```cpp
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../../ui/UiHelper.h"
#include "../../util/DiskMediaExtractor.h"
#include <QDateTime>
#include <cmath>

namespace QuarkMeta {

FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}

void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与文件显隐控制
    if (!isTrashView) {
        if (record.isDir) {
            bool isEmptyFolder = record.isEmpty;
            bool isFolderExplicitlySelected = currentFilter.types.contains("folder") ||
                                              (isEmptyFolder && currentFilter.types.contains("空文件夹"));
            if (!currentFilter.showFolders && !isFolderExplicitlySelected) {
                return false;
            }
        } else {
            if (!currentFilter.showFiles) return false;
        }
    }

    // 2. 评级过滤
    if (!currentFilter.ratings.isEmpty()) {
        if (!currentFilter.ratings.contains(record.rating)) return false;
    }

    // 3. 颜色标记过滤
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

    // 4. 类型过滤
    if (!currentFilter.types.isEmpty() || !currentFilter.typeFilterText.isEmpty()) {
        QString type = record.isDir ? "folder" : "file";
        QString ext = record.suffix.toUpper();
        bool matchType = false;

        if (!currentFilter.typeFilterText.isEmpty()) {
            QString searchText = currentFilter.typeFilterText.trimmed();
            if (searchText == "文件夹" || searchText.toLower() == "folder") {
                if (type == "folder") matchType = true;
            } else if (searchText == "空文件夹") {
                if (type == "folder" && record.isEmpty) matchType = true;
            } else {
                if (ext.contains(searchText.toUpper())) matchType = true;
            }
            if (!matchType) return false;
        }

        if (!currentFilter.types.isEmpty()) {
            matchType = false;
            for (const QString& fType : currentFilter.types) {
                if (fType == "folder") {
                    if (type == "folder") { matchType = true; break; }
                } else if (fType == "file") {
                    if (type != "folder") { matchType = true; break; }
                } else if (fType == "空文件夹") {
                    if (type == "folder" && record.isEmpty) { matchType = true; break; }
                } else {
                    if (ext == fType.toUpper()) { matchType = true; break; }
                }
            }
            if (!matchType) return false;
        }
    }

    // 5. 日期过滤
    if (!currentFilter.createDates.isEmpty() || !currentFilter.createDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.ctime).date().toString("dd-MM-yyyy");
        if (!currentFilter.createDateFilterText.isEmpty() && !dStr.contains(currentFilter.createDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.createDates.isEmpty() && !currentFilter.createDates.contains(dStr)) {
            return false;
        }
    }

    if (!currentFilter.modifyDates.isEmpty() || !currentFilter.modifyDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.mtime).date().toString("dd-MM-yyyy");
        if (!currentFilter.modifyDateFilterText.isEmpty() && !dStr.contains(currentFilter.modifyDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.modifyDates.isEmpty() && !currentFilter.modifyDates.contains(dStr)) {
            return false;
        }
    }

    // 6. 附加属性过滤 (链接、备注、标签、尺寸)
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
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 7. 搜索关键词匹配
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

bool FilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
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

    // 绝对权重 1：文件夹排在文件前面
    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) ? leftRec.isDir : !leftRec.isDir;
    }

    // 绝对权重 2：置顶/加密优先
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return leftPinned;
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
            long long lSize = leftRec.isDir ? -1 : leftRec.size;
            long long rSize = rightRec.isDir ? -1 : rightRec.size;
            if (lSize != rSize) return lSize < rSize;
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

## 3. `ContentPanel.h` 与 `ContentPanel.cpp` 瘦身与净化

### 3.1 `ContentPanel.h` 瘦身
- 移除顶部内嵌的 `class FilterProxyModel` 声明。
- 引入 `#include "models/FilterProxyModel.h"`。

```cpp
#pragma once

#include <QFrame>
#include <QPointer>
#include "models/FilterProxyModel.h" // 👈 引入独立头文件
// ...
```

### 3.2 `ContentPanel.cpp` 净化
- 删除顶部内嵌的 250 行 `FilterProxyModel` 实现代码。
- 净化 `applyFilters`，彻底删除备份/还原补丁：

```cpp
#include "ContentPanel.h"
#include "models/FilterProxyModel.h"

// 🚨 彻底删除 ContentPanel.cpp 顶部的 FilterProxyModel 全部实现代码！

void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters();
}

void ContentPanel::applyFilters(const FilterState& state) {
    // 🚀【纯净赋值】：由 FilterState 完整承载，彻底删除 preservedShowFolders 补丁！
    m_currentFilter = state;
    applyFilters();
}

void ContentPanel::applyFilters() {
    auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel);
    if (proxy) {
        proxy->currentFilter = m_currentFilter;
        proxy->updateFilter();
    }
    updateStatusBarStats();
    m_visibleTimer->start();
}
```

---

## 4. `PanelMediator.cpp` 胶水代码消灭

在 `PanelMediator::setupConnections()` 中，彻底删除搜刮文本框拼接 `mergedState` 的补丁代码：

```cpp
// PanelMediator.cpp 改造点：
if (filterPanel && contentPanel) {
    connect(contentPanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
        filterPanel->populateStats(stats);
        AppEvent ev;
        ev.type = AppEventType::FilterStateChanged;
        CentralEventHub::instance().publishEvent(ev);
    });

    // 🚀【单向直连】：直接连接，彻底消灭手动拼接 mergedState 的补丁！
    connect(filterPanel, &FilterPanel::filterChanged, contentPanel, &ContentPanel::applyFilters);
}
```

---

## 5. `CMakeLists.txt` 构建配置注册
```cmake
set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
)
```