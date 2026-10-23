# Implementation Plan - ContentPanel Multi-Selection Preservation Across View Mode Switching Fix

## 1. Overview
When a user selects multiple items (e.g., 100 items) in the content panel and switches view modes (e.g. between Grid View, List View, and Column View), only 1 item remained selected while the other 99 items lost their selection highlight.

The root cause was traced to `ContentPanel::setViewMode()` in `src/ui/ContentPanel.cpp`, where `selectAndScrollToPath()` was called in a loop for each path in `savedSelectedPaths`. Because `selectAndScrollToPath()` uses `QItemSelectionModel::ClearAndSelect`, each iteration cleared all previous selections and selected only the current item, leaving only the final item selected.

This implementation plan fixes multi-selection preservation across view mode changes by populating `m_pendingSelectNames` with all selected filenames and invoking `restoreSelections()`, which applies a single batch `QItemSelection` to preserve all selections simultaneously.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.cpp`

```diff
<<<<<<< SEARCH
    // 🚀【视图切换选区无损同步】：在新激活的视图中同步恢复之前的选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        for (const QString& selPath : savedSelectedPaths) {
            selectAndScrollToPath(selPath);
        }
    }
=======
    // 🚀【视图切换选区无损同步】：在新激活的视图中批量恢复之前全量选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        m_pendingSelectNames.clear();
        for (const QString& selPath : savedSelectedPaths) {
            m_pendingSelectNames.insert(QFileInfo(selPath).fileName());
        }
        restoreSelections();
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using standard CMake build commands:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. In Grid View or List View, select 100 items (e.g., via Ctrl+A).
3. Switch view mode to List View or Column View.
4. Verify that all 100 items remain selected and highlighted in the newly activated view.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Method**: `restoreSelections()` in `ContentPanel` is already the SSOT API for restoring batch item selections into `QItemSelection`.
- **Divergent Implementation Cleaned**: Replaced the item-by-item loop calling `selectAndScrollToPath()` (which used `ClearAndSelect` per item) with batch selection via `restoreSelections()`.

## 6. Header API Signature Verification
- `void ContentPanel::restoreSelections();` (`src/ui/ContentPanel.h`, line 188)
- `QStringList ContentPanel::getSelectedPaths() const;` (`src/ui/ContentPanel.h`, line 140)
