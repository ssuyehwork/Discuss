# Implementation Plan - DuplicateStatusGroup-1.md

## 1. Overview
This implementation plan extracts the "重复状态" (Duplicate Status) filter group from `rebuildGroups()` in `FilterPanel.cpp` into an independent class **`DuplicateStatusGroup`** (`src/ui/DuplicateStatusGroup.h` / `.cpp`).

### Refactoring Rules & Constraints
- **Direct Refactoring Only**: Moving the exact code as-is with zero logic changes, zero UI changes, and zero optimization.
- **Allowed Files**:
  - New Files: `src/ui/DuplicateStatusGroup.h`, `src/ui/DuplicateStatusGroup.cpp`
  - Modified Files: `src/ui/FilterPanel.h`, `src/ui/FilterPanel.cpp`, `CMakeLists.txt`
- **Decoupled Design**:
  - `DuplicateStatusGroup` does NOT hold a `FilterPanel` pointer and does NOT `#include "FilterPanel.h"`.
  - Takes all dependencies via parameters: `QVBoxLayout* contentLayout`, `FilterStateModel* filterModel`, `const QuarkMeta::ScanStats& currentStats`, `const FilterState& currentState`, and `addFilterRow`.
- **Exact Behavioral & Structure Preservation**:
  - Group title: `"重复状态"`.
  - Preserves the `QList<std::tuple<...>>` loop structure for creating `"重复项"` and `"未重复"`.
  - Mutual exclusion using `QButtonGroup` with `exclusive=false` + manual unchecking.
  - Both unchecked state corresponds to `DupAll`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/DuplicateStatusGroup-1.md`.

## 2. Modified Files List
- `src/ui/DuplicateStatusGroup.h` (New File)
- `src/ui/DuplicateStatusGroup.cpp` (New File)
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/DuplicateStatusGroup.h` (New File)

```cpp
#pragma once

#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class DuplicateStatusGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count)>;

    static void populate(QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel,
                         const QuarkMeta::ScanStats& currentStats,
                         const FilterState& currentState,
                         AddFilterRowFunc addFilterRow);
};

} // namespace QuarkMeta
```

### 2. `src/ui/DuplicateStatusGroup.cpp` (New File)

```cpp
#include "DuplicateStatusGroup.h"
#include <QButtonGroup>
#include <QList>
#include <tuple>

namespace QuarkMeta {

void DuplicateStatusGroup::populate(QVBoxLayout* contentLayout,
                                    FilterStateModel* filterModel,
                                    const QuarkMeta::ScanStats& currentStats,
                                    const FilterState& currentState,
                                    AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* dupGroup = new QButtonGroup(contentLayout->parentWidget());
    dupGroup->setExclusive(false);

    const QList<std::tuple<FilterState::DuplicatePresence, QString, int>> dupItems = {
        {FilterState::DuplicateOnly, "重复项", currentStats.duplicateCount},
        {FilterState::UniqueOnly, "未重复", currentStats.uniqueCount}
    };
    for (const auto& [presence, label, count] : dupItems) {
        QCheckBox* cb = addFilterRow(contentLayout, label, count);
        if (currentState.duplicatePresence == presence) cb->setChecked(true);
        QObject::connect(cb, &QCheckBox::toggled, cb, [filterModel, presence, dupGroup, cb](bool on) {
            FilterState st = filterModel->state();
            if (on) {
                for (QAbstractButton* b : dupGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                st.duplicatePresence = presence;
            } else st.duplicatePresence = FilterState::DupAll;
            filterModel->setState(st);
        });
        dupGroup->addButton(cb);
    }
}

} // namespace QuarkMeta
```

### 3. `src/ui/FilterPanel.h`

```diff
<<<<<<< SEARCH
namespace QuarkMeta {

class ThumbnailStatusGroup;

class FilterPanel : public QFrame {
=======
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;

class FilterPanel : public QFrame {
>>>>>>> REPLACE
```

### 4. `src/ui/FilterPanel.cpp`

```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "../core/AppConfig.h"
=======
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 12. 重复状态 ───────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("重复状态", gl);

        QButtonGroup* dupGroup = new QButtonGroup(g);
        dupGroup->setExclusive(false);

        const QList<std::tuple<FilterState::DuplicatePresence, QString, int>> dupItems = {
            {FilterState::DuplicateOnly, "重复项", m_currentStats.duplicateCount},
            {FilterState::UniqueOnly, "未重复", m_currentStats.uniqueCount}
        };
        for (const auto& [presence, label, count] : dupItems) {
            QCheckBox* cb = addFilterRow(gl, label, count);
            if (currentSt.duplicatePresence == presence) cb->setChecked(true);
            connect(cb, &QCheckBox::toggled, this, [this, presence, dupGroup, cb](bool on) {
                FilterState st = m_filterModel->state();
                if (on) {
                    for (QAbstractButton* b : dupGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                    st.duplicatePresence = presence;
                } else st.duplicatePresence = FilterState::DupAll;
                m_filterModel->setState(st);
            });
            dupGroup->addButton(cb);
        }
        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 12. 重复状态 ───────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("重复状态", gl);

        DuplicateStatusGroup::populate(gl, m_filterModel, m_currentStats, currentSt,
            [this](QVBoxLayout* layout, const QString& label, int count) {
                return addFilterRow(layout, label, count);
            });

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
>>>>>>> REPLACE
```

### 5. `CMakeLists.txt`

```diff
<<<<<<< SEARCH
    src/ui/ThumbnailStatusGroup.cpp
    src/ui/ThumbnailStatusGroup.h
=======
    src/ui/ThumbnailStatusGroup.cpp
    src/ui/ThumbnailStatusGroup.h
    src/ui/DuplicateStatusGroup.cpp
    src/ui/DuplicateStatusGroup.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open `FilterPanel`:
   - Locate the `"重复状态"` group.
   - Verify `"重复项"` displays `m_currentStats.duplicateCount` and `"未重复"` displays `m_currentStats.uniqueCount`.
   - Check `"重复项"` -> verify duplicate filtering takes effect.
   - Check `"未重复"` -> verify `"重复项"` is automatically unchecked and filter updates.
   - Uncheck both -> verify filter state resets to `DupAll`.
   - Click "重置所有筛选条件" -> verify group resets properly.
   - Switch folders -> verify counts update dynamically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` without modifying their signatures or implementations.
- **Zero Redundancy**: Pure 1:1 code move preserving `QList<std::tuple<...>>` loop structure with zero extra logic or altered parameters.

## 6. Header API Signature Verification
- `DuplicateStatusGroup::populate(...)` in `src/ui/DuplicateStatusGroup.h`.
- `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` signatures remain untouched.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `DuplicateStatusGroup.h` / `.cpp` to `CMakeLists.txt`.
- Forward declared `class DuplicateStatusGroup;` in `FilterPanel.h`.
- Included `"DuplicateStatusGroup.h"` in `FilterPanel.cpp`.
