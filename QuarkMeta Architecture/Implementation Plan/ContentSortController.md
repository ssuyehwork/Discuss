# ContentSortController Implementation Plan

## 1. Overview
This implementation plan addresses the responsibility overload in `ContentPanel.cpp` and fixes the "architectural inversion" in `FilterProxyModel.cpp` (where the model layer in Layer 5 was improperly calling `qobject_cast<ContentPanel*>(parent())` to read sorting types from the View layer in Layer 2).

### Objectives:
1. Extract sorting state, sorting types (`SortType`), sorting order (`Qt::SortOrder`), persistence with `AppConfig`, and sorting logic out of `ContentPanel` into a dedicated controller: `ContentSortController` (`src/ui/controllers/ContentSortController.h/cpp`).
2. Update `FilterProxyModel` to hold its own `m_sortType` and `m_sortOrder` properties, receiving changes via explicit setters/signals from `ContentSortController`, eliminating all `qobject_cast<ContentPanel*>(parent())` calls.
3. Decouple `ContentPanel` from managing sorting rules, making it a clean visual container.
4. Register new files in `CMakeLists.txt`.

---

## 2. Modified Files List
1. `CMakeLists.txt`
2. `src/ui/controllers/ContentSortController.h` (New File)
3. `src/ui/controllers/ContentSortController.cpp` (New File)
4. `src/ui/ContentPanel.h`
5. `src/ui/ContentPanel.cpp`
6. `src/ui/models/FilterProxyModel.h`
7. `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Register `src/ui/controllers/ContentSortController.h` and `src/ui/controllers/ContentSortController.cpp` in `CMakeLists.txt`.

<<<<<<< SEARCH
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ContentKeyHandler.cpp
=======
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ContentKeyHandler.cpp
    src/ui/controllers/ContentSortController.cpp
>>>>>>> REPLACE

<<<<<<< SEARCH
    src/ui/controllers/ContentContextMenu.h
    src/ui/controllers/ContentKeyHandler.h
=======
    src/ui/controllers/ContentContextMenu.h
    src/ui/controllers/ContentKeyHandler.h
    src/ui/controllers/ContentSortController.h
>>>>>>> REPLACE

---

### 3.2 `src/ui/controllers/ContentSortController.h` (New File)
Create a dedicated controller for managing sorting types, order, configuration persistence, and proxy model sorting updates.

```cpp
#pragma once

#include <QObject>
#include <QSortFilterProxyModel>

namespace QuarkMeta {

enum class SortType {
    SortByName,
    SortByCreateDate,
    SortByModifyDate,
    SortByExtension,
    SortBySize,
    SortByDimension,
    SortByRating,
    SortByAddedDate
};

class ContentSortController : public QObject {
    Q_OBJECT

public:
    explicit ContentSortController(QObject* parent = nullptr);
    ~ContentSortController() override = default;

    SortType sortType() const { return m_sortType; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    void setSortType(SortType type);
    void setSortOrder(Qt::SortOrder order);
    void setSortCriteria(SortType type, Qt::SortOrder order);

    void applySortToModel(QSortFilterProxyModel* proxyModel);
    void loadFromConfig();
    void saveToConfig();

signals:
    void sortCriteriaChanged(SortType type, Qt::SortOrder order);

private:
    SortType m_sortType = SortType::SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/controllers/ContentSortController.cpp` (New File)
Implement `ContentSortController`.

```cpp
#include "ContentSortController.h"
#include "../../core/AppConfig.h"

namespace QuarkMeta {

ContentSortController::ContentSortController(QObject* parent)
    : QObject(parent) {
    loadFromConfig();
}

void ContentSortController::loadFromConfig() {
    int savedType = AppConfig::instance().getValue("ContentPanel/RightClickSortType", static_cast<int>(SortType::SortByName)).toInt();
    int savedOrder = AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", static_cast<int>(Qt::AscendingOrder)).toInt();

    m_sortType = static_cast<SortType>(savedType);
    m_sortOrder = static_cast<Qt::SortOrder>(savedOrder);
}

void ContentSortController::saveToConfig() {
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(m_sortType));
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(m_sortOrder));
    AppConfig::instance().sync();
}

void ContentSortController::setSortType(SortType type) {
    if (m_sortType != type) {
        m_sortType = type;
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortCriteria(SortType type, Qt::SortOrder order) {
    bool changed = (m_sortType != type || m_sortOrder != order);
    m_sortType = type;
    m_sortOrder = order;
    if (changed) {
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::applySortToModel(QSortFilterProxyModel* proxyModel) {
    if (proxyModel) {
        proxyModel->sort(0, m_sortOrder);
    }
}

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/models/FilterProxyModel.h`
Add `SortType` and `SortOrder` storage and setter methods to `FilterProxyModel`.

<<<<<<< SEARCH
    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);
=======
    FilterState currentFilter;

    void updateFilter();
    void setCachedDuplicatePaths(const QSet<QString>& paths);

    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }
>>>>>>> REPLACE

<<<<<<< SEARCH
private:
    QSet<QString> m_cachedDuplicatePaths;
=======
private:
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0; // SortByName
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
>>>>>>> REPLACE

---

### 3.5 `src/ui/models/FilterProxyModel.cpp`
Remove `qobject_cast<ContentPanel*>(parent())` from `FilterProxyModel::lessThan`.

<<<<<<< SEARCH
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
=======
    auto compareNames = [](const ItemRecord& l, const ItemRecord& r) {
        return l.filename.localeAwareCompare(r.filename) < 0;
    };

    // 0: SortByName, 1: SortByCreateDate, 2: SortByModifyDate, 3: SortByExtension, 4: SortBySize, 5: SortByDimension, 6: SortByRating, 7: SortByAddedDate
    switch (m_sortType) {
        case 0: return compareNames(leftRec, rightRec);
        case 1:
            if (leftRec.ctime != rightRec.ctime) return leftRec.ctime < rightRec.ctime;
            return compareNames(leftRec, rightRec);
        case 2:
            if (leftRec.mtime != rightRec.mtime) return leftRec.mtime < rightRec.mtime;
            return compareNames(leftRec, rightRec);
        case 3: {
            int comp = leftRec.suffix.localeAwareCompare(rightRec.suffix);
            if (comp != 0) return comp < 0;
            return compareNames(leftRec, rightRec);
        }
        case 4: {
            long long lSize = leftRec.isDir ? -1 : leftRec.size;
            long long rSize = rightRec.isDir ? -1 : rightRec.size;
            if (lSize != rSize) return lSize < rSize;
            return compareNames(leftRec, rightRec);
        }
        case 5: {
            long long lDim = static_cast<long long>(leftRec.width) * leftRec.height;
            long long rDim = static_cast<long long>(rightRec.width) * rightRec.height;
            if (lDim != rDim) return lDim < rDim;
            return compareNames(leftRec, rightRec);
        }
        case 6:
            if (leftRec.rating != rightRec.rating) return leftRec.rating < rightRec.rating;
            return compareNames(leftRec, rightRec);
        case 7: {
            long long leftAdded = leftRec.added_at == 0 ? leftRec.ctime : leftRec.added_at;
            long long rightAdded = rightRec.added_at == 0 ? rightRec.ctime : rightRec.added_at;
            if (leftAdded != rightAdded) return leftAdded < rightAdded;
            return compareNames(leftRec, rightRec);
        }
    }
>>>>>>> REPLACE

---

### 3.6 `src/ui/ContentPanel.h`
Delegate sort operations in `ContentPanel` to `ContentSortController`.

<<<<<<< SEARCH
    SortType currentSortType() const { return m_sortType; }
    Qt::SortOrder currentSortOrder() const { return m_sortOrder; }
    void setSortType(SortType type) { m_sortType = type; }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; }
=======
    ContentSortController* sortController() const { return m_sortController; }
>>>>>>> REPLACE

<<<<<<< SEARCH
    SortType m_sortType = SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
=======
    ContentSortController* m_sortController = nullptr;
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Verify CMake configuration:
   ```bash
   cmake -B build
   ```
2. Build the target:
   ```bash
   cmake --build build --config Release
   ```
3. Run tests to confirm sorting functionality and proxy model filtering pass without regressions.
