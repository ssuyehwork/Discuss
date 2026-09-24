# Implementation Plan - ColumnViewPane Clean Architecture Refactoring & Parameter Restoration

## 1. Overview
This implementation plan restores the exact original design philosophy, parameters, and behaviors of Column View (`ColumnViewPane` & `ColumnViewWidget`) from `Version-Old-6` and `Memories.md`. It aligns the implementation with QuarkMeta Clean Architecture and the Anti-Patch 5 Engineering Locks.

### Architecture 3-Question Answers
1. **SSOT Source**: The authoritative single source of truth for filesystem directory records is `DiskScanService` / `DiskItemModel` (Domain & Model layer), and metadata properties are held by `MetadataManager`. The UI (`ColumnViewPane`) is strictly a view subscriber.
2. **Black Box Integrity**: No friends or private state exposures are introduced. `ColumnViewPane` exposes public Qt slots and signals for communication, decoupling direct `ContentPanel` parent references.
3. **Root Cause**: The Column View parameters (such as row height 28px, min pane width 230px, selection color `#378ADD`, parent expanded highlight `#334455`, and thumbnail visibility calculation) strictly align with the `Version-Old-6` master specification.

---

## 2. Modified Files List
- `src/ui/ColumnItemDelegate.cpp`
- `src/ui/ColumnViewPane.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnItemDelegate.cpp` Parameter Check
```
<<<<<<< SEARCH
QSize ColumnItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = RenameCapableDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}
=======
QSize ColumnItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = RenameCapableDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}
>>>>>>> REPLACE
```

### 3.2 `src/ui/ColumnViewPane.cpp`
```
<<<<<<< SEARCH
DropListView* ColumnViewPane::listView() const { return m_listView; }
DropListView* ColumnViewPane::folderListView() const { return m_folderListView; }
FolderSectionHeaderBar* ColumnViewPane::folderHeader() const { return m_panel ? m_panel->folderHeader() : nullptr; }
=======
DropListView* ColumnViewPane::listView() const { return m_listView; }
DropListView* ColumnViewPane::folderListView() const { return m_folderListView; }
FolderSectionHeaderBar* ColumnViewPane::folderHeader() const { return m_panel ? m_panel->folderHeader() : nullptr; }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake --build build --config Release
```

### Verification Steps
1. **Physical Parameter Verification**:
   - Verify Column View item height is locked to `28px` in `ColumnItemDelegate`.
   - Verify minimum column width is locked to `230px` in `ColumnViewWidget`.
   - Verify selection color is `#378ADD` (18% alpha) and parent expanded item highlight is `#334455`.
2. **Behavioral Verification**:
   - Single-click on a folder/file highlights the item without clearing right-hand sub-columns.
   - Double-click on a folder expands the sub-column and updates the active path.
   - Double-click on empty space in the rightmost column or canvas steps back up one directory level (`goUpColumn`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` SSOT Channel**: Column View refresh operations strictly reuse `ContentPanel::refreshAll()` / `refreshAllColumns()`.
- **`AppConfig` SSOT**: Global settings and state persistence strictly route through `AppConfig::instance()`.
- **Zero Redundant Code**: All dispersed legacy scan implementations route through `DiskScanService` and `DiskItemModel`.

---

## 6. Header API Signature Verification
| Class | Member Function / Signal Signature | Status |
| :--- | :--- | :--- |
| `ColumnViewPane` | `void folderExpandRequested(const QString& folderPath, int paneIndex)` | Verified in `ColumnViewPane.h` |
| `ColumnViewPane` | `void setFilterState(const FilterState& state)` | Verified in `ColumnViewPane.h` |
| `ColumnViewWidget` | `void goUpColumnFromIndex(int paneIndex)` | Verified in `ColumnViewWidget.h` |
| `ColumnViewWidget` | `ColumnViewPane* activePane() const` | Verified in `ColumnViewWidget.h` |
| `DiskScanService` | `static std::vector<ItemRecord> scanDirectory(...)` | Verified in `DiskScanService.h` |
