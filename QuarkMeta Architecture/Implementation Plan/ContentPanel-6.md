# Implementation Plan - ContentPanel Selection Signal Truncation Fix

## 1. Overview
When a user selects multiple items (e.g. 100 items) in `ContentPanel`, executing a batch action (such as setting 2-star rating) in `MetaPanel` only processes 50 items.
The root cause was traced to `ContentPanel::emitSelectionChangedSignal()` in `src/ui/ContentPanel.cpp`, where `paths` was artificially capped at 50 items via `if (paths.size() >= 50) break;`.

This implementation plan removes the artificial 50-item limit and unifies path gathering by calling `getSelectedPaths()`, ensuring all selected item paths are broadcast to listeners such as `MetaPanel`.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
void ContentPanel::emitSelectionChangedSignal() {
    QList<QModelIndex> indexes = getSelectedIndexes();
    QStringList paths;
    paths.reserve(qMin(indexes.size(), 50));
    for (const auto& idx : indexes) {
        if (idx.isValid()) paths.append(idx.data(PathRole).toString());
        if (paths.size() >= 50) break;
    }
    emit selectionChanged(paths);
    updateStatusBarStats();
}
=======
void ContentPanel::emitSelectionChangedSignal() {
    QStringList paths = getSelectedPaths();
    emit selectionChanged(paths);
    updateStatusBarStats();
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using standard CMake build commands:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Launch the application, select 100 items in the main content panel.
3. Observe that the right-side `MetaPanel` shows "已选中 100 个项目" instead of 50.
4. Perform a batch metadata operation (e.g., set 2-star rating) and verify that all 100 items are updated.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Method**: `getSelectedPaths()` in `ContentPanel` is already the SSOT API for obtaining selected file paths without duplicate logic or arbitrary caps.
- **Divergent Implementation Cleaned**: Removed the custom loop that imposed an hardcoded 50-item limit in `emitSelectionChangedSignal()`.

## 6. Header API Signature Verification
- `QStringList ContentPanel::getSelectedPaths() const;` (`src/ui/ContentPanel.h`, line 140)
- `void ContentPanel::updateStatusBarStats();` (`src/ui/ContentPanel.h`)
- `void ContentPanel::selectionChanged(const QStringList& paths);` (`src/ui/ContentPanel.h`, line 149)
