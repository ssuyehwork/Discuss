# Implementation Plan - LinkStatusGroup.md

## 1. Overview
This implementation plan extracts the "链接" (Link Status) filter group from `rebuildGroups()` in `FilterPanel.cpp` into an independent class **`LinkStatusGroup`** (`src/ui/LinkStatusGroup.h` / `.cpp`).

### Refactoring Rules & Constraints
- **Direct Refactoring Only**: Moving the exact code as-is with zero logic changes, zero UI changes, and zero optimization.
- **Allowed Files**:
  - New Files: `src/ui/LinkStatusGroup.h`, `src/ui/LinkStatusGroup.cpp`
  - Modified Files: `src/ui/FilterPanel.h`, `src/ui/FilterPanel.cpp`, `CMakeLists.txt`
- **Decoupled Design**:
  - `LinkStatusGroup` does NOT hold a `FilterPanel` pointer and does NOT `#include "FilterPanel.h"`.
  - Takes all dependencies via parameters: `QWidget* parentWidget`, `QVBoxLayout* contentLayout`, `FilterStateModel* filterModel`, `const QuarkMeta::ScanStats& currentStats`, `const FilterState& currentState`, and `addFilterRow`.
- **Exact Behavioral & Structure Preservation**:
  - Group title: `"链接"`.
  - Two checkboxes written out separately: `"有链接"` (`hasLinkCount`, `Yes`) and `"无链接"` (`noLinkCount`, `No`).
  - Checking logic sets `setChecked(true)` BEFORE `QObject::connect` signal attachment.
  - Mutual exclusion using `QButtonGroup` with `exclusive=false` + manual unchecking.
  - Both unchecked state corresponds to `All`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LinkStatusGroup.md`.

## 2. Modified Files List
- `src/ui/LinkStatusGroup.h` (New File)
- `src/ui/LinkStatusGroup.cpp` (New File)
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/LinkStatusGroup.h` (New File)

```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <functional>
#include "FilterStateModel.h"
#include "ScanStatsEngine.h"

namespace QuarkMeta {

class LinkStatusGroup {
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

### 2. `src/ui/LinkStatusGroup.cpp` (New File)

```cpp
#include "LinkStatusGroup.h"
#include <QButtonGroup>

namespace QuarkMeta {

void LinkStatusGroup::populate(QWidget* parentWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const QuarkMeta::ScanStats& currentStats,
                               const FilterState& currentState,
                               AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* linkGroup = new QButtonGroup(parentWidget ? parentWidget : contentLayout->parentWidget());
    linkGroup->setExclusive(false);

    QCheckBox* cbYes = addFilterRow(contentLayout, "有链接", currentStats.hasLinkCount);
    if (currentState.linkPresence == FilterState::Yes) cbYes->setChecked(true);
    QObject::connect(cbYes, &QCheckBox::toggled, cbYes, [filterModel, linkGroup, cbYes](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : linkGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
            st.linkPresence = FilterState::Yes;
        } else st.linkPresence = FilterState::All;
        filterModel->setState(st);
    });
    linkGroup->addButton(cbYes);

    QCheckBox* cbNo = addFilterRow(contentLayout, "无链接", currentStats.noLinkCount);
    if (currentState.linkPresence == FilterState::No) cbNo->setChecked(true);
    QObject::connect(cbNo, &QCheckBox::toggled, cbNo, [filterModel, linkGroup, cbNo](bool on) {
        FilterState st = filterModel->state();
        if (on) {
            for (QAbstractButton* b : linkGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
            st.linkPresence = FilterState::No;
        } else st.linkPresence = FilterState::All;
        filterModel->setState(st);
    });
    linkGroup->addButton(cbNo);
}

} // namespace QuarkMeta
```

### 3. `src/ui/FilterPanel.h`

```diff
<<<<<<< SEARCH
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;

class FilterPanel : public QFrame {
=======
namespace QuarkMeta {

class ThumbnailStatusGroup;
class DuplicateStatusGroup;
class LinkStatusGroup;

class FilterPanel : public QFrame {
>>>>>>> REPLACE
```

### 4. `src/ui/FilterPanel.cpp`

```diff
<<<<<<< SEARCH
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "../core/AppConfig.h"
=======
#include "FilterPanel.h"
#include "ThumbnailStatusGroup.h"
#include "DuplicateStatusGroup.h"
#include "LinkStatusGroup.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // ── 7. 链接 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("链接", gl);

        QButtonGroup* linkGroup = new QButtonGroup(g);
        linkGroup->setExclusive(false);

        QCheckBox* cbYes = addFilterRow(gl, "有链接", m_currentStats.hasLinkCount);
        if (currentSt.linkPresence == FilterState::Yes) cbYes->setChecked(true);
        connect(cbYes, &QCheckBox::toggled, this, [this, linkGroup, cbYes](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : linkGroup->buttons()) if (b != cbYes && b->isChecked()) b->setChecked(false);
                st.linkPresence = FilterState::Yes;
            } else st.linkPresence = FilterState::All;
            m_filterModel->setState(st);
        });
        linkGroup->addButton(cbYes);

        QCheckBox* cbNo = addFilterRow(gl, "无链接", m_currentStats.noLinkCount);
        if (currentSt.linkPresence == FilterState::No) cbNo->setChecked(true);
        connect(cbNo, &QCheckBox::toggled, this, [this, linkGroup, cbNo](bool on) {
            FilterState st = m_filterModel->state();
            if (on) {
                for (QAbstractButton* b : linkGroup->buttons()) if (b != cbNo && b->isChecked()) b->setChecked(false);
                st.linkPresence = FilterState::No;
            } else st.linkPresence = FilterState::All;
            m_filterModel->setState(st);
        });
        linkGroup->addButton(cbNo);

        m_containerLayout->insertWidget(m_containerLayout->count() - 1, g);
    }
=======
    // ── 7. 链接 ──────────────────────────────────────────
    {
        QVBoxLayout* gl = nullptr;
        QWidget* g = buildGroup("链接", gl);

        LinkStatusGroup::populate(g, gl, m_filterModel, m_currentStats, currentSt,
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
    src/ui/DuplicateStatusGroup.cpp
    src/ui/DuplicateStatusGroup.h
=======
    src/ui/DuplicateStatusGroup.cpp
    src/ui/DuplicateStatusGroup.h
    src/ui/LinkStatusGroup.cpp
    src/ui/LinkStatusGroup.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open `FilterPanel`:
   - Locate the `"链接"` group.
   - Verify `"有链接"` displays `m_currentStats.hasLinkCount` and `"无链接"` displays `m_currentStats.noLinkCount`.
   - Check `"有链接"` -> verify link filtering takes effect.
   - Check `"无链接"` -> verify `"有链接"` is automatically unchecked and filter updates.
   - Uncheck both -> verify filter state resets to `All`.
   - Click "重置所有筛选条件" -> verify group resets properly.
   - Switch folders -> verify counts update dynamically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` without modifying their signatures or implementations.
- **Zero Redundancy**: Pure 1:1 code move keeping the two separate checkbox blocks without merging into loops or generic functions.

## 6. Header API Signature Verification
- `LinkStatusGroup::populate(...)` in `src/ui/LinkStatusGroup.h`.
- `FilterPanel::buildGroup` and `FilterPanel::addFilterRow` signatures remain untouched.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `LinkStatusGroup.h` / `.cpp` to `CMakeLists.txt`.
- Forward declared `class LinkStatusGroup;` in `FilterPanel.h`.
- Included `"LinkStatusGroup.h"` in `FilterPanel.cpp`.
