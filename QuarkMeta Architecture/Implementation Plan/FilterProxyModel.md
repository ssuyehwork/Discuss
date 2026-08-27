# FilterProxyModel Implementation Plan

## 1. Overview
`FilterProxyModel` decouples the multi-dimensional filter and weighted sorting pipeline from `ContentPanel.h/cpp` into an independent model class in `src/ui/models/FilterProxyModel.h` and `FilterProxyModel.cpp`.
It standardizes `FilterState` as the Single Source of Truth (SSOT) for all filter criteria (including search `keyword`), removes inline proxy model declarations from `ContentPanel`, and purges intermediate glue/state-backup patches from `ContentPanel.cpp` and `PanelMediator.cpp`.

## 2. Modified Files List
- `CMakeLists.txt` (Register `FilterProxyModel.h` and `FilterProxyModel.cpp` under `SOURCES`)
- `src/ui/models/FilterProxyModel.h` (New header for FilterProxyModel class)
- `src/ui/models/FilterProxyModel.cpp` (New implementation of filterAcceptsRow and lessThan algorithms)
- `src/ui/ContentPanel.h` (Remove inline FilterProxyModel class declaration, `#include "models/FilterProxyModel.h"`)
- `src/ui/ContentPanel.cpp` (Remove inline FilterProxyModel implementation and simplify `applyFilters`)
- `src/ui/PanelMediator.cpp` (Connect `FilterPanel::filterChanged` directly to `ContentPanel::applyFilters`)

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/ui/FilterStateModel.cpp
=======
    src/ui/FilterStateModel.cpp
    src/ui/models/FilterProxyModel.h
    src/ui/models/FilterProxyModel.cpp
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/FilterProxyModel.h`
```cpp
#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>
#include "../FilterPanel.h"
#include "../../core/ItemRecord.h"

namespace QuarkMeta {

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

### 3.3 `src/ui/models/FilterProxyModel.cpp`
```cpp
#include "FilterProxyModel.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
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

    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

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

    if (!currentFilter.ratings.isEmpty()) {
        if (!currentFilter.ratings.contains(record.rating)) return false;
    }

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

    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) ? leftRec.isDir : !leftRec.isDir;
    }

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

### 3.4 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
class FilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit FilterProxyModel(QObject* parent = nullptr);

    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths; // 纯内存集合，主线程 0 磁盘 I/O
};
=======
>>>>>>> REPLACE
```

### 3.5 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
// --- FilterProxyModel 实现 ---
FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}
...
=======
>>>>>>> REPLACE
```

### 3.6 `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
        connect(filterPanel, &FilterPanel::filterChanged, contentPanel, [contentPanel, searchController](const FilterState& state) {
            FilterState mergedState = state;
            if (searchController && searchController->searchEdit()) {
                mergedState.keyword = searchController->searchEdit()->text().trimmed();
            }
            contentPanel->applyFilters(mergedState);
        });
=======
        connect(filterPanel, &FilterPanel::filterChanged, contentPanel, &ContentPanel::applyFilters);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build the CMake project to ensure `FilterProxyModel` compiles without MOC or link errors.
2. Verify multi-dimensional filtering, keyword search, and weighted sorting in ContentPanel.
