# Implementation Plan - TagStatusGroup.md

## 1. Overview
This implementation plan extracts the "标签" (Tag Status) filter group from `rebuildGroups()` in `FilterPanel.cpp` into an independent class **`TagStatusGroup`** (`src/ui/TagStatusGroup.h` / `.cpp`).

### Refactoring Rules & Constraints
- **Direct Refactoring Only**: Moving the exact code as-is with zero logic changes, zero UI changes, and zero optimization.
- **Allowed Files**:
  - New Files: `src/ui/TagStatusGroup.h`, `src/ui/TagStatusGroup.cpp`
  - Modified Files: `src/ui/FilterPanel.h`, `src/ui/FilterPanel.cpp`, `CMakeLists.txt`
- **Decoupled Design**:
  - `TagStatusGroup` does NOT hold a `FilterPanel` pointer and does NOT `#include "FilterPanel.h"`.
  - Takes all dependencies via parameters: `QWidget* parentWidget`, `QVBoxLayout* contentLayout`, `FilterStateModel* filterModel`, `const QuarkMeta::ScanStats& currentStats`, `const FilterState& currentState`, and `addFilterRow`.
- **Exact Behavioral & Order Preservation**:
  - Group title: `"标签"`.
  - Must remain the first `buildGroup("标签", gl)` call in `rebuildGroups()` to maintain `isFirst` top border styling logic.
  - Two checkboxes written out separately: `"已标签"` (`hasTagCount`, `Yes`) and `"未标签"` (`noTagCount`, `No`).
  - Checking logic sets `setChecked(true)` BEFORE `QObject::connect` signal attachment.
  - Mutual exclusion using `QButtonGroup` with `exclusive=false` + manual unchecking.
  - Both unchecked state corresponds to `All`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/TagStatusGroup.md`.

## 2. Modified Files List
- `src/ui/TagStatusGroup.h` (New File)
- `src/ui/TagStatusGroup.cpp` (New File)
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/TagStatusGroup.h` (New File)

```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class TagStatusGroup {
public:
    using AddFilterRowFunc = std::function<QCheckBox*(QVBoxLayout* layout, const QString& label, int count)>;

    static void populate(QWidget* parentWidget,
                         QVBoxLayout* contentLayout,
                         FilterStateModel* filterModel,
                         const QuarkMeta::ScanStats& currentStats,
                         const FilterState& currentState,
                         AddFilterRowFunc addFilterRow);
};

} // namespace QuarkMeta
```

### 2. `src/ui/TagStatusGroup.cpp` (New File)

```cpp
#include "TagStatusGroup.h"
#include <QButtonGroup>

namespace QuarkMeta {

void TagStatusGroup::populate(QWidget* parentWidget,
                              QVBoxLayout* contentLayout,
                              FilterStateModel* filterModel,
                              const QuarkMeta::ScanStats& currentStats,
                              const FilterState& currentState,
                              AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* tagGroup = new QButtonGroup(parentWidget ? parentWidget : contentLayout->parentWidget());
    tagGroup->setExclusive(false);

    QCheckBox* cbYes = addFilterRow(contentLayout, "已标签", currentStats.hasTagCount);
    if (currentState.tagPresence == FilterState::Yes) cbYes->setChecked(true);
    QObject::connect(cbYes, &QCheckBox::toggled, cbYes, [filterModel, tagGroup, cbYes](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : tagGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
            st.tagPresence = FilterState::Yes;
        } else st.tagPresence = FilterState::All;
        filterModel->setState(st);
    });
    tagGroup->addButton(cbYes);

    QCheckBox* cbNo = addFilterRow(contentLayout, "未标签", currentStats.noTagCount);
    if (currentState.tagPresence == FilterState::No) cbNo->setChecked(true);
    QObject::connect(cbNo, &QCheckBox::toggled, cbNo, [filterModel, tagGroup, cbNo](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : tagGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
            st.tagPresence = FilterState::No;
        } else st.tagPresence = FilterState::All;
        filterModel->setState(st);
    });
    tagGroup->addButton(cbNo);
}

} // namespace QuarkMeta
```

### 3. `src/ui/FilterPanel.h`

```diff
<<<<<<< SEARCH
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;
class LinkStatusGroup;
class NoteStatusGroup;

class FilterPanel : public QFrame {
=======
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;
class LinkStatusGroup;
class NoteStatusGroup;
class TagStatusGroup;

class FilterPanel : public QFrame {
>>>>>>> REPLACE
```

### 4. `src/ui/FilterPanel.cpp`

```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "LinkStatusGroup.h"
#include "NoteStatusGroup.h"
#include "../core/AppConfig.h"
=======
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "LinkStatusGroup.h"
#include "NoteStatusGroup.h"
#include "TagStatusGroup.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 1. 标签 ──────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("标签", gl);

        QButtonGroup* tagGroup = new QButtonGroup(g);
        tagGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "已标签", m_currentStats.hasTagCount);
        if (currentSt.tagPresence == FilterState::Yes) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, tagGroup, cbYes](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : tagGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                st.tagPresence = FilterState::Yes;
            } else st.tagPresence = FilterState::All;
            m_filterModel->setState(st);
        });
        tagGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "未标签", m_currentStats.noTagCount);
        if (currentSt.tagPresence == FilterState::No) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, tagGroup, cbNo](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : tagGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                st.tagPresence = FilterState::No;
            } else st.tagPresence = FilterState::All;
            m_filterModel->setState(st);
        });
        tagGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 1. 标签 ──────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("标签", gl);

        TagStatusGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
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
    src/ui/NoteStatusGroup.cpp
    src/ui/NoteStatusGroup.h
=======
    src/ui/NoteStatusGroup.cpp
    src/ui/NoteStatusGroup.h
    src/ui/TagStatusGroup.cpp
    src/ui/TagStatusGroup.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open `FilterPanel`:
   - Verify `"标签"` remains the topmost group with top border styling (`isFirst`).
   - Verify `"已标签"` displays `m_currentStats.hasTagCount` and `"未标签"` displays `m_currentStats.noTagCount`.
   - Check `"已标签"` -> verify tag filtering takes effect.
   - Check `"未标签"` -> verify `"已标签"` is automatically unchecked and filter updates.
   - Uncheck both -> verify filter state resets to `All`.
   - Click "重置所有筛选条件" -> verify group resets properly.
   - Switch folders -> verify counts update dynamically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` without modifying their signatures or implementations.
- **Zero Redundancy**: Pure 1:1 code move keeping the two separate checkbox blocks without merging into loops or generic functions.

## 6. Header API Signature Verification
- `TagStatusGroup::populate(...)` in `src/ui/TagStatusGroup.h`.
- `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` signatures remain untouched.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `TagStatusGroup.h` / `.cpp` to `CMakeLists.txt`.
- Forward declared `class TagStatusGroup;` in `FilterPanel.h`.
- Included `"TagStatusGroup.h"` in `FilterPanel.cpp`.
