# Implementation Plan - Fixing Inline Rename Trigger in `createNewItem` (`ContentFileOpsHandler-CreateItemInlineRenameFix.md`)

## 1. Overview
When creating a new file/folder via `Ctrl+Shift+N` or context menu in `ContentFileOpsHandler::createNewItem`, four specific issues prevented Qt's `QAbstractItemView::edit(index)` from activating inline renaming:
1. **Path Format Discrepancy**: `fullPath` was generated with forward slashes while `ItemRecord` stored paths with native separators, causing `proxyIdx.data(PathRole).toString() == fullPath` comparison to fail.
2. **Qt View Layout Engine Race Condition**: Directly invoking `view->edit(proxyIdx)` during model row insertion failed because `QAbstractItemView` marks layout as dirty (`d->delayedPendingLayout = true`) and yields invalid `visualRect(proxyIdx)` until the event loop processes layout.
3. **ColumnView Logical Branch Early Exit**: `ColumnView` branch refreshed column data and immediately returned without selecting or triggering inline rename.

This implementation plan fixes all 3 issues by normalizing paths with `QDir::cleanPath`, integrating `ColumnView` inline editing, and deferring `edit(proxyIdx)` via `QTimer::singleShot(0, ...)` microtasks after layout completion.

---

## 2. Modified Files List
- `src/ui/controllers/ContentFileOpsHandler.cpp` (Fix path normalization, defer `edit()`, support ColumnView)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentFileOpsHandler.cpp`
```
<<<<<<< SEARCH
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
=======
            QString cleanFullPath = QDir::cleanPath(fullPath);

            if (weakPanel->currentViewMode() == ContentPanel::ColumnView && weakPanel->columnView()) {
                weakPanel->columnView()->refreshActiveColumn();
                QTimer::singleShot(0, weakPanel, [weakPanel, cleanFullPath]() {
                    if (!weakPanel || !weakPanel->columnView() || !weakPanel->columnView()->activePane()) return;
                    weakPanel->columnView()->activePane()->selectItemByPath(cleanFullPath);
                    QAbstractItemView* listView = weakPanel->columnView()->activePane()->listView();
                    if (listView && listView->selectionModel()) {
                        QModelIndex curIdx = listView->currentIndex();
                        if (curIdx.isValid()) {
                            listView->setFocus();
                            listView->edit(curIdx);
                        }
                    }
                });
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
                    QString itemPath = QDir::cleanPath(proxyIdx.data(PathRole).toString());
                    if (itemPath == cleanFullPath) {
                        QPointer<QAbstractItemView> weakView(view);
                        QTimer::singleShot(0, weakPanel, [weakView, proxyIdx]() {
                            if (weakView && proxyIdx.isValid()) {
                                weakView->setFocus();
                                weakView->scrollTo(proxyIdx);
                                weakView->setCurrentIndex(proxyIdx);
                                weakView->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                                weakView->edit(proxyIdx);
                            }
                        });
                        break;
                    }
                }
            }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure CMake build environment:
   ```bash
   cmake -B build -S .
   ```
2. Build executable:
   ```bash
   cmake --build build --config Debug
   ```
3. Run QuarkMeta application and test `Ctrl+Shift+N`.
4. Verify inline rename edit opens in GridView, ListView, and ColumnView.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Path Normalization**: Uses standard `QDir::cleanPath` across all path comparisons.
- **Microtask Deferral**: Leverages `QTimer::singleShot(0, ...)` to align with Qt layout completion.
