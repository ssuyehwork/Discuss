# Implementation Plan - ThumbnailStatusGroup-1.md

## 1. Overview
This implementation plan refactors the "缩略图状态" (Thumbnail Status) group from `rebuildGroups()` in `FilterPanel.cpp` into an independent class **`ThumbnailStatusGroup`** (`src/ui/ThumbnailStatusGroup.h` / `.cpp`).

### Refactoring Rules & Constraints
- **Direct Refactoring Only**: Moving the exact code as-is with zero logic changes, zero UI changes, and zero optimization.
- **Allowed Files**:
  - New Files: `src/ui/ThumbnailStatusGroup.h`, `src/ui/ThumbnailStatusGroup.cpp`
  - Modified Files: `src/ui/FilterPanel.h`, `src/ui/FilterPanel.cpp`, `CMakeLists.txt`
- **Decoupled Design**:
  - `ThumbnailStatusGroup` does NOT hold a `FilterPanel` pointer and does NOT `#include "FilterPanel.h"`.
  - Takes all dependencies via parameters: `QVBoxLayout* contentLayout`, `FilterStateModel* filterModel`, `const QuarkMeta::ScanStats& currentStats`, `const FilterState& currentState`, and `AddFilterRowFunc addFilterRow`.
- **Exact Behavioral Preservation**:
  - Group title: `"缩略图状态"`.
  - Two checkboxes: `"有缩略图"` (`HasThumbnail`) and `"无缩略图 (提取失败)"` (`NoThumbnail`).
  - Mutual exclusion using `QButtonGroup` with `exclusive=false` + manual unchecking.
  - Both unchecked state corresponds to `ThumbAll`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/ThumbnailStatusGroup-1.md`.

## 2. Modified Files List
- `src/ui/ThumbnailStatusGroup.h` (New File)
- `src/ui/ThumbnailStatusGroup.cpp` (New File)
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/ThumbnailStatusGroup.h` (New File)

```cpp
#pragma once

#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class ThumbnailStatusGroup {
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

### 2. `src/ui/ThumbnailStatusGroup.cpp` (New File)

```cpp
#include "ThumbnailStatusGroup.h"
#include <QButtonGroup>

namespace QuarkMeta {

void ThumbnailStatusGroup::populate(QVBoxLayout* contentLayout,
                                   FilterStateModel* filterModel,
                                   const QuarkMeta::ScanStats& currentStats,
                                   const FilterState& currentState,
                                   AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* thumbGroup = new QButtonGroup(contentLayout->parentWidget());
    thumbGroup->setExclusive(false);

    QCheckBox* cbYes = addFilterRow(contentLayout, "有缩略图", currentStats.hasThumbnailCount);
    if (currentState.thumbnailPresence == FilterState::HasThumbnail) cbYes->setChecked(true);
    QObject::connect(cbYes, &QCheckBox::toggled, cbYes, [filterModel, thumbGroup, cbYes](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : thumbGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
            st.thumbnailPresence = FilterState::HasThumbnail;
        } else st.thumbnailPresence = FilterState::ThumbAll;
        filterModel->setState(st);
    });
    thumbGroup->addButton(cbYes);

    QCheckBox* cbNo = addFilterRow(contentLayout, "无缩略图 (提取失败)", currentStats.noThumbnailCount);
    if (currentState.thumbnailPresence == FilterState::NoThumbnail) cbNo->setChecked(true);
    QObject::connect(cbNo, &QCheckBox::toggled, cbNo, [filterModel, thumbGroup, cbNo](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : thumbGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
            st.thumbnailPresence = FilterState::NoThumbnail;
        } else st.thumbnailPresence = FilterState::ThumbAll;
        filterModel->setState(st);
    });
    thumbGroup->addButton(cbNo);
}

} // namespace QuarkMeta
```

### 3. `src/ui/FilterPanel.h`

```diff
<<<<<<< SEARCH
namespace QuarkMeta {

class FilterPanel : public QFrame {
=======
namespace QuarkMeta {

class ThumbnailStatusGroup;

class FilterPanel : public QFrame {
>>>>>>> REPLACE
```

### 4. `src/ui/FilterPanel.cpp`

```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
#include "../core/AppConfig.h"
=======
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 13. 缩略图状态 ───────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("缩略图状态", gl);

        QButtonGroup* thumbGroup = new QButtonGroup(g);
        thumbGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "有缩略图", m_currentStats.hasThumbnailCount);
        if (currentSt.thumbnailPresence == FilterState::HasThumbnail) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, thumbGroup, cbYes](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : thumbGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                st.thumbnailPresence = FilterState::HasThumbnail;
            } else st.thumbnailPresence = FilterState::ThumbAll;
            m_filterModel->setState(st);
        });
        thumbGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "无缩略图 (提取失败)", m_currentStats.noThumbnailCount);
        if (currentSt.thumbnailPresence == FilterState::NoThumbnail) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, thumbGroup, cbNo](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : thumbGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                st.thumbnailPresence = FilterState::NoThumbnail;
            } else st.thumbnailPresence = FilterState::ThumbAll;
            m_filterModel->setState(st);
        });
        thumbGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 13. 缩略图状态 ───────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("缩略图状态", gl);

        ThumbnailStatusGroup::populate(gl, m_filterModel, m_currentStats, currentSt,
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
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
=======
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
    src/ui/ThumbnailStatusGroup.cpp
    src/ui/ThumbnailStatusGroup.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open the right filter sidebar panel (`FilterPanel`):
   - Locate the `"缩略图状态"` group at the bottom.
   - Verify `"有缩略图"` displays `m_currentStats.hasThumbnailCount` and `"无缩略图 (提取失败)"` displays `m_currentStats.noThumbnailCount`.
   - Check `"有缩略图"` -> verify filtering takes effect.
   - Check `"无缩略图 (提取失败)"` -> verify `"有缩略图"` is automatically unchecked and filter updates.
   - Uncheck both -> verify filter state resets to `ThumbAll`.
   - Click "重置所有筛选条件" -> verify group resets properly.
   - Switch folders -> verify counts update dynamically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` without modifying their signatures or implementations.
- **Zero Redundancy**: Pure 1:1 code move with zero extra logic or altered parameters.

## 6. Header API Signature Verification
- `ThumbnailStatusGroup::populate(...)` in `src/ui/ThumbnailStatusGroup.h`.
- `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` signatures remain untouched.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `ThumbnailStatusGroup.h` / `.cpp` to `CMakeLists.txt`.
- Forward declared `class ThumbnailStatusGroup;` in `FilterPanel.h`.
- Included `"ThumbnailStatusGroup.h"` in `FilterPanel.cpp`.
