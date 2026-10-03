# Implementation Plan - NoteStatusGroup.md

## 1. Overview
This implementation plan extracts the "备注" (Note Status) filter group from `rebuildGroups()` in `FilterPanel.cpp` into an independent class **`NoteStatusGroup`** (`src/ui/NoteStatusGroup.h` / `.cpp`).

### Refactoring Rules & Constraints
- **Direct Refactoring Only**: Moving the exact code as-is with zero logic changes, zero UI changes, and zero optimization.
- **Allowed Files**:
  - New Files: `src/ui/NoteStatusGroup.h`, `src/ui/NoteStatusGroup.cpp`
  - Modified Files: `src/ui/FilterPanel.h`, `src/ui/FilterPanel.cpp`, `CMakeLists.txt`
- **Decoupled Design**:
  - `NoteStatusGroup` does NOT hold a `FilterPanel` pointer and does NOT `#include "FilterPanel.h"`.
  - Takes all dependencies via parameters: `QWidget* parentWidget`, `QVBoxLayout* contentLayout`, `FilterStateModel* filterModel`, `const QuarkMeta::ScanStats& currentStats`, `const FilterState& currentState`, and `addFilterRow`.
- **Exact Behavioral & Structure Preservation**:
  - Group title: `"备注"`.
  - Two checkboxes written out separately: `"有备注"` (`hasNoteCount`, `Yes`) and `"无备注"` (`noNoteCount`, `No`).
  - Checking logic sets `setChecked(true)` BEFORE `QObject::connect` signal attachment.
  - Mutual exclusion using `QButtonGroup` with `exclusive=false` + manual unchecking.
  - Both unchecked state corresponds to `All`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/NoteStatusGroup.md`.

## 2. Modified Files List
- `src/ui/NoteStatusGroup.h` (New File)
- `src/ui/NoteStatusGroup.cpp` (New File)
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/NoteStatusGroup.h` (New File)

```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class NoteStatusGroup {
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

### 2. `src/ui/NoteStatusGroup.cpp` (New File)

```cpp
#include "NoteStatusGroup.h"
#include <QButtonGroup>

namespace QuarkMeta {

void NoteStatusGroup::populate(QWidget* parentWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const QuarkMeta::ScanStats& currentStats,
                               const FilterState& currentState,
                               AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* noteGroup = new QButtonGroup(parentWidget ? parentWidget : contentLayout->parentWidget());
    noteGroup->setExclusive(false);

    QCheckBox* cbYes = addFilterRow(contentLayout, "有备注", currentStats.hasNoteCount);
    if (currentState.notePresence == FilterState::Yes) cbYes->setChecked(true);
    QObject::connect(cbYes, &QCheckBox::toggled, cbYes, [filterModel, noteGroup, cbYes](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : noteGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
            st.notePresence = FilterState::Yes;
        } else st.notePresence = FilterState::All;
        filterModel->setState(st);
    });
    noteGroup->addButton(cbYes);

    QCheckBox* cbNo = addFilterRow(contentLayout, "无备注", currentStats.noNoteCount);
    if (currentState.notePresence == FilterState::No) cbNo->setChecked(true);
    QObject::connect(cbNo, &QCheckBox::toggled, cbNo, [filterModel, noteGroup, cbNo](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : noteGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
            st.notePresence = FilterState::No;
        } else st.notePresence = FilterState::All;
        filterModel->setState(st);
    });
    noteGroup->addButton(cbNo);
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

class FilterPanel : public QFrame {
=======
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;
class LinkStatusGroup;
class NoteStatusGroup;

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
#include "../core/AppConfig.h"
=======
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "LinkStatusGroup.h"
#include "NoteStatusGroup.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 8. 备注 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("备注", gl);

        QButtonGroup* noteGroup = new QButtonGroup(g);
        noteGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "有备注", m_currentStats.hasNoteCount);
        if (currentSt.notePresence == FilterState::Yes) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, noteGroup, cbYes](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : noteGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                st.notePresence = FilterState::Yes;
            } else st.notePresence = FilterState::All;
            m_filterModel->setState(st);
        });
        noteGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "无备注", m_currentStats.noNoteCount);
        if (currentSt.notePresence == FilterState::No) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, noteGroup, cbNo](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : noteGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                st.notePresence = FilterState::No;
            } else st.notePresence = FilterState::All;
            m_filterModel->setState(st);
        });
        noteGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 8. 备注 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("备注", gl);

        NoteStatusGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
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
    src/ui/LinkStatusGroup.cpp
    src/ui/LinkStatusGroup.h
=======
    src/ui/LinkStatusGroup.cpp
    src/ui/LinkStatusGroup.h
    src/ui/NoteStatusGroup.cpp
    src/ui/NoteStatusGroup.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open `FilterPanel`:
   - Locate the `"备注"` group.
   - Verify `"有备注"` displays `m_currentStats.hasNoteCount` and `"无备注"` displays `m_currentStats.noNoteCount`.
   - Check `"有备注"` -> verify note filtering takes effect.
   - Check `"无备注"` -> verify `"有备注"` is automatically unchecked and filter updates.
   - Uncheck both -> verify filter state resets to `All`.
   - Click "重置所有筛选条件" -> verify group resets properly.
   - Switch folders -> verify counts update dynamically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` without modifying their signatures or implementations.
- **Zero Redundancy**: Pure 1:1 code move keeping the two separate checkbox blocks without merging into loops or generic functions.

## 6. Header API Signature Verification
- `NoteStatusGroup::populate(...)` in `src/ui/NoteStatusGroup.h`.
- `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` signatures remain untouched.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `NoteStatusGroup.h` / `.cpp` to `CMakeLists.txt`.
- Forward declared `class NoteStatusGroup;` in `FilterPanel.h`.
- Included `"NoteStatusGroup.h"` in `FilterPanel.cpp`.
