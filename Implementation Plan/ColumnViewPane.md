# Implementation Plan - ColumnViewPane Clean Architecture Refactoring & Parameter Restoration

## 1. Overview
This implementation plan provides the complete, non-redundant refactoring strategy for `ColumnViewPane` and `ColumnViewWidget`. It restores the exact original design parameters (28px row height, `#378ADD` selection highlight, `#334455` parent expanded state, 230px minimum column width, and `chevron_right` 14px arrows) from `Version-Old-6` and `Memories.md`, while documenting the decoupling of UI rendering from async directory scanning.

### Architecture 3-Question Answers
1. **SSOT Source**: The authoritative single source of truth for filesystem directory records is `DiskScanService` / `DiskItemModel` (Domain & Model layer), and metadata properties are held by `MetadataManager`. The UI (`ColumnViewPane`) is strictly a view subscriber.
2. **Black Box Integrity**: No friends or private state exposures are introduced. `ColumnViewPane` exposes public Qt slots and signals for communication, decoupling direct `ContentPanel` parent references.
3. **Root Cause**: Previously, `ColumnViewPane` directly triggered multi-threaded disk scans (`QtConcurrent::run`) and metadata decoration inside the Widget class itself. The refactoring decouples the UI layer from IO operations and restores all physical parameters.

---

## 2. Modified Files List
- `Memories.md`
- `src/ui/ColumnViewPane.h`
- `src/ui/ColumnViewPane.cpp`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnViewPane.h` Decoupling Signal Interface
```
<<<<<<< SEARCH
signals:
    void folderClicked(const QString& folderPath, int paneIndex);
    void fileClicked(const QString& filePath, int paneIndex);
    void folderExpandRequested(const QString& folderPath, int paneIndex);
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);
=======
signals:
    void folderClicked(const QString& folderPath, int paneIndex);
    void fileClicked(const QString& filePath, int paneIndex);
    void folderExpandRequested(const QString& folderPath, int paneIndex);
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);
    void contextMenuRequested(QAbstractItemView* view, const QPoint& pos);
    void pathsDroppedSignal(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDir);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ColumnViewPane.cpp` Event Filter & Connection Decoupling
```
<<<<<<< SEARCH
        connect(m_folderListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_folderListView, pos);
            }
        });
        connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_listView, pos);
            }
        });
=======
        connect(m_folderListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_folderListView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_folderListView, pos);
            }
        });
        connect(m_listView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_listView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_listView, pos);
            }
        });
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
| `ColumnViewPane` | `void contextMenuRequested(QAbstractItemView* view, const QPoint& pos)` | Added & Verified in `ColumnViewPane.h` |
| `ColumnViewPane` | `void folderExpandRequested(const QString& folderPath, int paneIndex)` | Verified in `ColumnViewPane.h` |
| `ColumnViewWidget` | `void goUpColumnFromIndex(int paneIndex)` | Verified in `ColumnViewWidget.h` |
| `ColumnViewWidget` | `ColumnViewPane* activePane() const` | Verified in `ColumnViewWidget.h` |
| `DiskScanService` | `static std::vector<ItemRecord> scanDirectory(...)` | Verified in `DiskScanService.h` |
