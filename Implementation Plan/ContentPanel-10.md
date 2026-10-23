# ContentPanel-10.md: ColumnView Multi-Selection Preservation & Folder Re-selection Race Fix

## 1. Overview
Runtime logs revealed why `ContentPanel-9.md` lost multi-selection highlights when switching to Column View:
```
[18:23:44.742] [ContentPanel Debug] restoreSelections in ColumnView, count: 39
[18:23:44.742] [ColumnViewPane Debug] setPendingSelectNames count: 39 path: "H:/测试/测试-删除"
[18:23:44.812] [ColumnViewPane Debug] tryPendingSelection rowCount: 39 pendingNames: 39 -> Selected 39 files!
...
[18:23:45.127] [ColumnViewPane Debug] tryPendingSelection rowCount: 2503 pendingNames: 0 pendingPath: "H:/测试/测试-删除"
```

When switching to `ColumnView`:
1. `setRootPath("H:/测试/测试-删除")` creates columns for `H:/`, `H:/测试`, and `H:/测试/测试-删除`.
2. Parent column `H:/测试` starts an asynchronous `loadDirectory()`.
3. Meanwhile, `restoreSelections()` sets `pendingNames: 39` on `H:/测试/测试-删除`, which completes loading and selects the 39 Markdown files!
4. Slightly later, parent column `H:/测试` finishes its async `loadDirectory()` and executes `selectItemByPath("H:/测试/测试-删除")`.
5. Selecting `"H:/测试/测试-删除"` in the parent column triggers its `selectionChanged` / `folderSelected` signals, executing `dismissSubColumns()` or clearing deep column selections, wiping out the 39 selected files in the rightmost pane!

This implementation plan resolves the race condition by:
1. Updating `ColumnViewPane::selectItemByPath()` so that if selecting a parent directory in an upstream column would re-expand a child column that is already open and valid, it suppresses redundant `folderSelected` re-triggering and selection wipes on child columns.
2. Preserving the rightmost column's active multi-selection when ancestor directory columns finish asynchronous loads.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    // 静默选中父级列项目，防止二次触发 folderSelected 擦除最右列的多选集合
    if (m_listView && m_listView->selectionModel()) {
        m_listView->selectionModel()->blockSignals(true);
        tryPendingSelection();
        m_listView->selectionModel()->blockSignals(false);
    } else {
        tryPendingSelection();
    }
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Select 39 Markdown files in Grid View or List View.
2. Switch view mode to Column View (`ColumnView`).
3. Verify that parent column async scanning finishes without wiping the 39 selected Markdown files in the rightmost pane.
4. Verify MetaPanel status bar continues to display "39 items selected".

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `QItemSelectionModel::blockSignals` mechanism to decouple parent column expansion highlights from sub-column selection resets.

---

## 6. Header API Signature Verification
- `ColumnViewPane::selectItemByPath(const QString& targetPath)` -> `src/ui/ColumnViewWidget.h`
- `ColumnViewPane::tryPendingSelection()` -> `src/ui/ColumnViewWidget.h`
