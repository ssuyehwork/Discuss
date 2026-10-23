# Implementation Plan - Refactoring New Item Creation & Unifying Inline Rename State Machine (`CreateItemInlineRenameRefactor.md`)

## 1. Overview
When creating new items (via `Ctrl+Shift+N` or context menu), inline rename editing failed across views due to three core architectural defects:
1. **Ad-Hoc Duplicate Logic in `ContentFileOpsHandler::createNewItem`**: `createNewItem` ignored the existing `setPendingSelectName(name, edit)` state machine on `ContentPanel` and implemented 40 lines of custom model insertion, path comparisons (which failed due to forward/backslash slashes mismatches), and synchronous `view->edit()` calls before Qt completed layout pass (`delayedPendingLayout`).
2. **Path Normalization Bug in `setPendingSelectName`**: `ContentPanel::setPendingSelectName` generated paths using string concatenation (`m_currentPath + "/" + name`), which produced mixed forward slashes `D:/test/新建文件夹` while `DiskItemModel` stored native separators `D:\test\新建文件夹`.
3. **Missing `m_isPendingEdit` Handling in `ColumnViewPane`**: In Column View, `restoreSelections()` passed pending paths to `ColumnViewPane` but discarded `m_isPendingEdit`.

This plan refactors new item creation to strictly follow SSOT principles by:
1. Normalizing path formatting in `setPendingSelectName` using `QDir::toNativeSeparators(QDir::cleanPath(...))`.
2. Eliminating the 40-line ad-hoc logic in `ContentFileOpsHandler::createNewItem`, replacing it with `setPendingSelectName(finalName, true)` and `refreshAll()`.
3. Extending `ColumnViewPane` to support inline editing when pending selections are pending edit.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp` (Normalize path in `setPendingSelectName`)
- `src/ui/controllers/ContentFileOpsHandler.cpp` (Remove ad-hoc code, invoke `setPendingSelectName` & `refreshAll`)
- `src/ui/ColumnViewPane.h` / `src/ui/ColumnViewPane.cpp` (Support pending edit in Column View)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_selectionState.selectedPaths.clear();
    if (!name.isEmpty()) {
        QString fullPath = m_currentPath + "/" + name;
        m_selectionState.selectedPaths.insert(fullPath);
        m_selectionState.focusedPath = fullPath;
    }
    m_isPendingEdit = edit;
}
=======
void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_selectionState.selectedPaths.clear();
    if (!name.isEmpty()) {
        QString fullPath = QDir::toNativeSeparators(QDir::cleanPath(QDir(m_currentPath).filePath(name)));
        m_selectionState.selectedPaths.insert(fullPath);
        m_selectionState.focusedPath = fullPath;
    }
    m_isPendingEdit = edit;
}
>>>>>>> REPLACE
```

### 3.2 `src/ui/controllers/ContentFileOpsHandler.cpp`
```
<<<<<<< SEARCH
        ItemRecord rec = ItemRecord::create(fullPath);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, rec, fullPath]() {
            if (!weakPanel) return;

            if (weakPanel->currentViewMode() == ContentPanel::ColumnView && weakPanel->columnView()) {
                weakPanel->columnView()->refreshActiveColumn();
                return;
            }

            if (DiskItemModel* diskModel = qobject_cast<DiskItemModel*>(weakPanel->model())) {
                diskModel->addItemRecord(rec);
            }

            weakPanel->applySort();
            weakPanel->applyFilters();
            weakPanel->recalculateAndEmitStats();

            QSortFilterProxyModel* proxy = weakPanel->getActiveProxyModel();
            QAbstractItemView* view = weakPanel->activeItemView();
            if (proxy && view) {
                for (int i = 0; i < proxy->rowCount(); ++i) {
                    QModelIndex proxyIdx = proxy->index(i, 0);
                    if (proxyIdx.data(PathRole).toString() == fullPath) {
                        view->setFocus();
                        view->scrollTo(proxyIdx);
                        view->setCurrentIndex(proxyIdx);
                        view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        view->edit(proxyIdx);
                        break;
                    }
                }
            }
        });
=======
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, finalName]() {
            if (!weakPanel) return;
            weakPanel->setPendingSelectName(finalName, true);
            weakPanel->refreshAll();
        });
>>>>>>> REPLACE
```

### 3.3 `src/ui/ColumnViewPane.h`
```
<<<<<<< SEARCH
    void setPendingSelectPaths(const QSet<QString>& paths);
=======
    void setPendingSelectPaths(const QSet<QString>& paths, bool edit = false);
>>>>>>> REPLACE
```

### 3.4 `src/ui/ColumnViewPane.cpp`
```
<<<<<<< SEARCH
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    tryPendingSelection();
}
=======
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths, bool edit) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    m_isPendingEdit = edit;
    tryPendingSelection();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure CMake:
   ```bash
   cmake -B build -S .
   ```
2. Build executable:
   ```bash
   cmake --build build --config Debug
   ```
3. Test creating folders/files in Grid, List, and Column views using `Ctrl+Shift+N` and context menu.
4. Verify that inline rename automatically opens and functions seamlessly across all view modes.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Reuse**: Both `ContentFileOpsHandler` and `ContentPanel` route item creation selections through `setPendingSelectName(name, edit)` and `refreshAll()`.
- **Eliminated Dead Code**: Physically removed 40 lines of duplicate model/view logic in `ContentFileOpsHandler::createNewItem`.
