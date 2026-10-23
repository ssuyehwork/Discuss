# Implementation Plan - LocateFileAndGoUpSelection.md

## Overview
Fix and complete auto-selection and scroll-into-view behavior for "Locate File from Favorites" and "Go Up to Parent Directory" across all four view modes (List, Grid, Justified, and Column View) in QuarkMeta disk-direct mode.

---

## Verification Results for Mandatory Points

1. **`DiskItemModel::setRecords` Model Reset**:
   - `DiskItemModel::setRecords` wraps its data reload in `beginResetModel()` / `endResetModel()`.
   - In Column View, `ColumnViewPane::setPendingSelectPaths` was being called before `loadDirectory` (which invokes `setRecords`), so the model reset wiped out the newly assigned selection.
   - **Solution**: Pending target paths must be preserved in a pending state field on `ColumnViewPane` or `ContentPanel` and only consumed/applied *after* `setRecords` finishes and the proxy models build their mappings.

2. **`SectionProxyModel` Synchronous Rebuild**:
   - `SectionProxyModel::onSourceModelReset` synchronously executes `rebuildMapping()`, generating row mappings and `mapFromSource` indexes immediately during model reset.
   - `JustifiedView` uses `rowsInRange` based on geometry. In `restoreSelections`, `scrollTo` is invoked. `JustifiedView::scrollTo` checks item position. If layout is not ready, `JustifiedView::doItemsLayout` is called inside `scrollTo`.
   - No timers or polling loops are needed.

3. **`ItemRecord::path` Format & Normalization**:
   - `ItemRecord::path` stores native Windows paths (e.g., `C:\Folder\file.txt`).
   - Normalization function: `QDir::toNativeSeparators(QDir::cleanPath(path))` with case-insensitive comparison (`Qt::CaseInsensitive`).

4. **`restoreSelections` Signal Blockers & Re-entrancy (`m_isRestoringSelections`)**:
   - The original `QSignalBlocker` on `selectionModel()` in `ContentViewCoordinator::restoreSelections` prevented `selectionModel()->selectionChanged` from firing, leaving `MetaPanel` and status bar ("Selected N items") un-updated.
   - **Solution**: Remove `QSignalBlocker` from `restoreSelections`, but protect against re-entrant loops by using an atomic boolean flag (`m_isRestoringSelections`) or emitting `selectionChanged` once after `select(...)` finishes.

---

## Architectural Root Causes & Complete Design

### 1. Root Cause Analysis

- **Locate File from Favorites**:
  - `PanelMediator.cpp`: `target->setPendingSelectName(fi.fileName(), false)` used `m_currentPath + "/" + name`. `m_currentPath` was the *old* directory, making `pendingSelectPath` wrong.
  - Fix: Store the **full normalized target path** directly (`setPendingSelectPath(fi.absoluteFilePath(), false)`).

- **Return to Parent Directory ("Go Up")**:
  - `NavigationService::goUp()` navigated to `dir.absolutePath()` but discarded the source child directory path.
  - Entry points (`NavBarWidget` up button, Backspace key in `ContentKeyHandler`, Column View empty canvas click) did not pass the source child directory.
  - Fix: In `NavigationService::goUp()`, record the departing child path (e.g. `m_pendingGoUpChildPath = m_currentUrl`). When `navigateTo` triggers `currentUrlChanged`, pass `goUpChildPath` to `PanelMediator`, which delivers it as `pendingSelectPath` to the target `ContentPanel`.

- **Target Disambiguation & One-time Consumption**:
  - `ContentPanel` reused `m_selectionState.selectedPaths` for both view-mode switching selection state AND pending single-shot navigation targets.
  - Fix: Separate into `m_pendingSelectPath` (single QString, single-shot target) vs `m_selectionState` (active selection). `m_pendingSelectPath` is consumed and cleared immediately after one selection attempt.

- **Unified Selection & Path Comparison**:
  - Standardize path comparison helper across `ContentViewCoordinator` and `ColumnViewPane`:
    ```cpp
    static bool pathEquals(const QString& p1, const QString& p2) {
        return QString::compare(QDir::toNativeSeparators(QDir::cleanPath(p1)),
                               QDir::toNativeSeparators(QDir::cleanPath(p2)),
                               Qt::CaseInsensitive) == 0;
    }
    ```

- **Target Panel Consistency**:
  - `PanelMediator` had mismatched target panel checks: `requestLocateFile` used `m_activeContentPanel ? m_activeContentPanel : contentPanel`, whereas `currentUrlChanged` used `(m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel : contentPanel`.
  - Fix: Unify both to use a single helper method `activeOrMainContentPanel()`.

- **Column View Handling**:
  - Scenario A (Locate File): In `ContentPanel::loadDirectory`, if column view already contains the target directory in one of its panes, find the exact pane for that directory (not necessarily the rightmost) and call `setPendingSelectPath`.
  - Scenario B (Go Up): `ColumnViewWidget::goUpColumnFromIndex` trims rightmost pane and navigates. The new rightmost pane gets `setPendingSelectPath` for the trimmed folder, applied after `setRecords` finishes.

---

## Modified Files List

1. `src/core/NavigationService.h` & `src/core/NavigationService.cpp`
2. `src/ui/PanelMediator.h` & `src/ui/PanelMediator.cpp`
3. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
4. `src/ui/controllers/ContentViewCoordinator.h` & `src/ui/controllers/ContentViewCoordinator.cpp`
5. `src/ui/ColumnViewPane.h` & `src/ui/ColumnViewPane.cpp`
6. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/core/NavigationService.h` & `src/core/NavigationService.cpp`
Add `m_pendingGoUpChildPath` to track departing child folder when `goUp()` is invoked, and pass it in `currentUrlChanged` or via a new signal/getter.

```
<<<<<<< SEARCH
    void goUp();
=======
    void goUp();
    QString takePendingGoUpChildPath();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void NavigationService::goUp() {
    if (!canGoUp()) return;

    QDir dir(m_currentUrl);
    if (dir.isRoot()) {
        navigateTo("computer://");
        return;
    }

    if (dir.cdUp()) {
        navigateTo(dir.absolutePath());
    } else {
        navigateTo("computer://");
    }
}
=======
QString NavigationService::takePendingGoUpChildPath() {
    QString path = m_pendingGoUpChildPath;
    m_pendingGoUpChildPath.clear();
    return path;
}

void NavigationService::goUp() {
    if (!canGoUp()) return;

    QString departingPath = m_currentUrl;
    QDir dir(m_currentUrl);
    if (dir.isRoot()) {
        m_pendingGoUpChildPath.clear();
        navigateTo("computer://");
        return;
    }

    if (dir.cdUp()) {
        m_pendingGoUpChildPath = departingPath;
        navigateTo(dir.absolutePath());
    } else {
        m_pendingGoUpChildPath.clear();
        navigateTo("computer://");
    }
}
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
Add `m_pendingSelectPath` (QString) and `m_pendingIsEdit` (bool). Provide `setPendingSelectPath(const QString& fullPath, bool isEdit)`.

```
<<<<<<< SEARCH
    void setPendingSelectName(const QString& name, bool isEdit = false);
=======
    void setPendingSelectPath(const QString& fullPath, bool isEdit = false);
    void setPendingSelectName(const QString& name, bool isEdit = false);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::setPendingSelectName(const QString& name, bool isEdit) {
    if (name.isEmpty()) return;
    QString targetPath = m_currentPath + "/" + name;
    m_selectionState.selectedPaths.clear();
    m_selectionState.selectedPaths.insert(targetPath);
    m_isPendingEdit = isEdit;
}
=======
void ContentPanel::setPendingSelectPath(const QString& fullPath, bool isEdit) {
    if (fullPath.isEmpty()) return;
    m_pendingSelectPath = QDir::toNativeSeparators(QDir::cleanPath(fullPath));
    m_pendingIsEdit = isEdit;
}

void ContentPanel::setPendingSelectName(const QString& name, bool isEdit) {
    if (name.isEmpty()) return;
    setPendingSelectPath(QDir(m_currentPath).filePath(name), isEdit);
}
>>>>>>> REPLACE
```

In `ContentPanel::restoreSelections()`:
```
<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_viewCoordinator) {
        m_viewCoordinator->restoreSelections(m_selectionState.selectedPaths, m_isPendingEdit);
    }
}
=======
void ContentPanel::restoreSelections() {
    if (!m_viewCoordinator) return;

    if (!m_pendingSelectPath.isEmpty()) {
        QString targetPath = m_pendingSelectPath;
        bool isEdit = m_pendingIsEdit;
        m_pendingSelectPath.clear();
        m_pendingIsEdit = false;

        QSet<QString> targetSet;
        targetSet.insert(targetPath);
        m_viewCoordinator->restoreSelections(targetSet, isEdit);
    } else if (!m_selectionState.selectedPaths.isEmpty()) {
        m_viewCoordinator->restoreSelections(m_selectionState.selectedPaths, false);
    }
}
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContentViewCoordinator.cpp`
Standardize normalized path matching in `restoreSelections`, remove `QSignalBlocker` on selectionModel, and emit `selectionChanged` explicitly once after selection is set.

```
<<<<<<< SEARCH
        QSignalBlocker blocker(view->selectionModel());
        QItemSelection sel;
        QModelIndex lastIdx;

        int total = viewModeModel->rowCount();
        for (int r = 0; r < total; ++r) {
            QModelIndex idx = viewModeModel->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) continue;
            QString p = idx.data(PathRole).toString();
            if (selectedPaths.contains(p)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }

        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
=======
        QItemSelection sel;
        QModelIndex lastIdx;

        auto matchesAny = [&selectedPaths](const QString& itemPath) {
            QString nItem = QDir::toNativeSeparators(QDir::cleanPath(itemPath));
            for (const QString& sp : selectedPaths) {
                QString nTarget = QDir::toNativeSeparators(QDir::cleanPath(sp));
                if (QString::compare(nItem, nTarget, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
            return false;
        };

        int total = viewModeModel->rowCount();
        for (int r = 0; r < total; ++r) {
            QModelIndex idx = viewModeModel->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) continue;
            QString p = idx.data(PathRole).toString();
            if (matchesAny(p)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }

        if (lastIdx.isValid()) {
            view->setCurrentIndex(lastIdx);
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            view->scrollTo(lastIdx);
            if (m_panel) {
                emit m_panel->selectionChanged(m_panel->getSelectedPaths());
            }

            if (isPendingEdit) {
                QPointer<QAbstractItemView> weakView(view);
                QTimer::singleShot(0, m_panel, [weakView, lastIdx]() {
                    if (weakView && lastIdx.isValid()) {
                        weakView->setFocus();
                        weakView->setCurrentIndex(lastIdx);
                        weakView->edit(lastIdx);
                    }
                });
            }
        }
>>>>>>> REPLACE
```

### 4. `src/ui/PanelMediator.cpp`
Unify target panel selection helper:

```
<<<<<<< SEARCH
        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            QFileInfo fi(path);
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectName(fi.fileName(), false);
            }
            NavigationService::instance().navigateTo(fi.absolutePath());
        });
=======
        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            ContentPanel* target = (m_activeContentPanel && m_activeContentPanel->isVisible()) ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectPath(path, false);
            }
            NavigationService::instance().navigateTo(QFileInfo(path).absolutePath());
        });
>>>>>>> REPLACE
```

In `currentUrlChanged` signal handler in `PanelMediator.cpp`:
```
<<<<<<< SEARCH
        if (targetPanel) {
            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url);
            }
        }
=======
        if (targetPanel) {
            QString goUpChild = NavigationService::instance().takePendingGoUpChildPath();
            if (!goUpChild.isEmpty() && url != "computer://" && url != "trash://") {
                targetPanel->setPendingSelectPath(goUpChild, false);
            }

            if (url == "computer://") {
                targetPanel->loadDirectory("computer://");
            } else if (url == "trash://") {
                targetPanel->loadCategory("trash");
            } else {
                targetPanel->loadDirectory(url);
            }
        }
>>>>>>> REPLACE
```

### 5. `src/ui/ColumnViewPane.cpp` & `src/ui/ColumnViewWidget.cpp`
Ensure column view handles pane-specific selection and `setPendingSelectPath` consumption post-load.

In `ColumnViewPane::tryPendingSelection`:
```
<<<<<<< SEARCH
    if (m_pendingSelectPaths.isEmpty()) return;

    QSet<QString> targets = m_pendingSelectPaths;
    bool doEdit = m_pendingIsEdit;
    m_pendingSelectPaths.clear();
    m_pendingIsEdit = false;
=======
    if (m_pendingSelectPaths.isEmpty()) return;

    QSet<QString> targets = m_pendingSelectPaths;
    bool doEdit = m_pendingIsEdit;
    m_pendingSelectPaths.clear();
    m_pendingIsEdit = false;

    QItemSelection sel;
    QModelIndex foundIdx;

    auto matchesTarget = [&targets](const QString& path) {
        QString nPath = QDir::toNativeSeparators(QDir::cleanPath(path));
        for (const QString& t : targets) {
            QString nTarget = QDir::toNativeSeparators(QDir::cleanPath(t));
            if (QString::compare(nPath, nTarget, Qt::CaseInsensitive) == 0) return true;
        }
        return false;
    };

    int rows = m_sectionProxyModel ? m_sectionProxyModel->rowCount() : m_model->rowCount();
    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = m_sectionProxyModel ? m_sectionProxyModel->index(r, 0) : m_model->index(r, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        if (matchesTarget(idx.data(PathRole).toString())) {
            foundIdx = idx;
            break;
        }
    }

    if (foundIdx.isValid()) {
        m_listView->setCurrentIndex(foundIdx);
        m_listView->selectionModel()->select(foundIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_listView->scrollTo(foundIdx, QAbstractItemView::PositionAtCenter);
        emit selectionChanged();

        if (doEdit) {
            QPointer<QListView> weakView(m_listView);
            QTimer::singleShot(0, this, [weakView, foundIdx]() {
                if (weakView && foundIdx.isValid()) {
                    weakView->setFocus();
                    weakView->edit(foundIdx);
                }
            });
        }
    }
>>>>>>> REPLACE
```

---

## Deliverables & Analysis Checklist

1. **4 Mandatory Verification Points**:
   - `DiskItemModel::setRecords`: Uses `beginResetModel()`/`endResetModel()`. Target path is saved in pending state and consumed *after* `setRecords` completes.
   - `SectionProxyModel`: Synchronously rebuilds mapping on model reset; `JustifiedView::scrollTo` computes layout immediately if not ready.
   - `ItemRecord::path`: Normalized via `QDir::toNativeSeparators(QDir::cleanPath(path))` with case-insensitive compare.
   - Re-entrancy & Notification: `QSignalBlocker` removed from `restoreSelections`, replaced with single explicit `selectionChanged` emission after selection applies.

2. **Root Cause of Selection Failure**:
   - `setPendingSelectName` was using `m_currentPath` (the old directory) instead of the target file's absolute path.
   - `NavigationService::goUp()` did not record or pass the departing child folder path.

3. **Go-Up Path Tracking**:
   - Tracked in `NavigationService::goUp()` via `m_pendingGoUpChildPath`.
   - Handled via `takePendingGoUpChildPath()` in `PanelMediator::setupConnections` when `currentUrlChanged` is received.
   - Covers all go-up entry points (`m_btnUp`, `Key_Backspace`, Column View empty canvas, double-click empty canvas).

4. **Modified Files**:
   - `NavigationService.h` & `.cpp`
   - `PanelMediator.cpp`
   - `ContentPanel.h` & `.cpp`
   - `ContentViewCoordinator.cpp`
   - `ColumnViewPane.cpp`
