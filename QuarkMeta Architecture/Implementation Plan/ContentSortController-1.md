# ContentSortController-1 Implementation Plan

## 1. Overview
This follow-up implementation plan fixes the MSVC C2039 compilation error in `ContentContextMenu.cpp` caused by the removal of `ContentPanel::SortType` and its constants (`ContentPanel::SortByName`, etc.) during the extraction of `ContentSortController`.

In accordance with the **Contract Lock (契约锁)** rule, `ContentPanel` must preserve backward compatibility for existing callers that reference `ContentPanel::SortType` and `ContentPanel::SortByName`.

### Objectives:
1. Provide alias `using SortType = QuarkMeta::SortType;` inside `ContentPanel`.
2. Add static constexpr aliases in `ContentPanel` (`ContentPanel::SortByName`, `ContentPanel::SortByCreateDate`, etc.) to map seamlessly to `SortType::SortByName`.
3. Update `ContentContextMenu.cpp` to include `ContentSortController.h`.

---

## 2. Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.h`
Add backward-compatible aliases for `SortType` and enum values inside `ContentPanel`.

<<<<<<< SEARCH
    ContentSortController* sortController() const { return m_sortController; }
    SortType currentSortType() const { return m_sortController ? m_sortController->sortType() : SortType::SortByName; }
    Qt::SortOrder currentSortOrder() const { return m_sortController ? m_sortController->sortOrder() : Qt::AscendingOrder; }
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
=======
    using SortType = QuarkMeta::SortType;
    static constexpr SortType SortByName = SortType::SortByName;
    static constexpr SortType SortByCreateDate = SortType::SortByCreateDate;
    static constexpr SortType SortByModifyDate = SortType::SortByModifyDate;
    static constexpr SortType SortByExtension = SortType::SortByExtension;
    static constexpr SortType SortBySize = SortType::SortBySize;
    static constexpr SortType SortByDimension = SortType::SortByDimension;
    static constexpr SortType SortByRating = SortType::SortByRating;
    static constexpr SortType SortByAddedDate = SortType::SortByAddedDate;

    ContentSortController* sortController() const { return m_sortController; }
    SortType currentSortType() const { return m_sortController ? m_sortController->sortType() : SortType::SortByName; }
    Qt::SortOrder currentSortOrder() const { return m_sortController ? m_sortController->sortOrder() : Qt::AscendingOrder; }
    void setSortType(SortType type) { if (m_sortController) m_sortController->setSortType(type); }
    void setSortOrder(Qt::SortOrder order) { if (m_sortController) m_sortController->setSortOrder(order); }
>>>>>>> REPLACE

---

### 3.2 `src/ui/controllers/ContentContextMenu.cpp`
Include `ContentSortController.h` in `ContentContextMenu.cpp`.

<<<<<<< SEARCH
#include "ContentContextMenu.h"
#include "../ContentPanel.h"
=======
#include "ContentContextMenu.h"
#include "../ContentPanel.h"
#include "ContentSortController.h"
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Verify build with MSVC:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Confirm C2039 error in `ContentContextMenu.cpp` is completely resolved.
