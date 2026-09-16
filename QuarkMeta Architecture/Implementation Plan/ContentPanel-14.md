# ContentPanel-14.md: Selection Loop Fix & ColumnView Performance Optimization

## 1. Overview
When selecting all items in ContentPanel (e.g., in Grid View or List View) and then switching to Column View, the application froze (hung) and crashed due to a stack overflow / infinite signal recursion loop between `restoreSelections()` and `selectionChanged` signals.

Specifically:
1. `ContentPanel::setViewMode()` calls `restoreSelections()`.
2. `restoreSelections()` invokes `ColumnViewPane::setPendingSelectPaths()`, which selects items via `selectionModel()->select(...)`.
3. Selecting items triggers `ColumnViewPane::selectionChanged` -> `ColumnViewWidget::activeColumnRecordsChanged` -> `ContentPanel` lambda.
4. `ContentPanel`'s lambda unconditionally re-invokes `restoreSelections()`, causing an endless recursive signal loop.

Additionally, `ColumnViewPane::tryPendingSelection()` performed an $O(N \times M)$ nested loop comparison with redundant `QDir::cleanPath(...)` normalization calls per item, causing severe CPU spikes on bulk selection.

This implementation plan fixes both issues:
- Adds an `m_isRestoringSelections` re-entrancy guard lock in `ContentPanel` and uses `QSignalBlocker` during selection restoration.
- Optimizes `ColumnViewPane::tryPendingSelection()` from $O(N \times M)$ to $O(N)$ by pre-normalizing pending selection paths into a `QSet<QString>`.
- Eliminates duplicate/manual `emit selectionChanged()` calls when `QItemSelectionModel` already auto-emits the signal.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.h`

<<<<<<< SEARCH
    struct SelectionState {
        QString currentFolder;
        QString focusedPath;
        QSet<QString> selectedPaths;
    };
    SelectionState m_selectionState;
    bool m_isPendingEdit = false;
=======
    struct SelectionState {
        QString currentFolder;
        QString focusedPath;
        QSet<QString> selectedPaths;
    };
    SelectionState m_selectionState;
    bool m_isRestoringSelections = false;
    bool m_isPendingEdit = false;
>>>>>>> REPLACE

---

### File 2: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty()) return;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
        }
        return;
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    DiskItemModel* diskModel = m_diskModel;
    QSortFilterProxyModel* proxy = m_proxyModel;

    if (view && view->selectionModel() && diskModel && proxy) {
        QItemSelection sel;
        QModelIndex lastIdx;
        const auto& recs = diskModel->allRecords();
        for (size_t i = 0; i < recs.size(); ++i) {
            if (m_selectionState.selectedPaths.contains(recs[i].path)) {
                QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
            }
        }
        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
    }
}
=======
void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
        }
        m_isRestoringSelections = false;
        return;
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    DiskItemModel* diskModel = m_diskModel;
    QSortFilterProxyModel* proxy = m_proxyModel;

    if (view && view->selectionModel() && diskModel && proxy) {
        QSignalBlocker blocker(view->selectionModel());
        QItemSelection sel;
        QModelIndex lastIdx;
        const auto& recs = diskModel->allRecords();
        for (size_t i = 0; i < recs.size(); ++i) {
            if (m_selectionState.selectedPaths.contains(recs[i].path)) {
                QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
            }
        }
        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
    }

    m_isRestoringSelections = false;
}
>>>>>>> REPLACE

---

### File 3: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty() && m_proxyModel->rowCount() > 0) {
        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            for (const QString& p : m_pendingSelectPaths) {
                QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
                if (QString::compare(itemPath, cleanP, Qt::CaseInsensitive) == 0) {
                    sel.select(idx, idx);
                    lastIdx = idx;
                    break;
                }
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                m_listView->setCurrentIndex(lastIdx);
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectPaths.clear();
            emit selectionChanged();
            return;
        }
    }

    if (!m_pendingSelectPath.isEmpty()) {
        QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
        QString targetName = QFileInfo(cleanTarget).fileName();

        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            QString itemName = QFileInfo(itemPath).fileName();

            if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                m_listView->setCurrentIndex(idx);
                if (m_listView->selectionModel()) {
                    m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
                m_listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                m_pendingSelectPath.clear();
                emit selectionChanged();
                break;
            }
        }
    }
}
=======
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty() && m_proxyModel->rowCount() > 0) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
            if (normalizedPending.contains(itemPath)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            {
                QSignalBlocker blocker(m_listView->selectionModel());
                m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            if (lastIdx.isValid()) {
                m_listView->setCurrentIndex(lastIdx);
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectPaths.clear();
            emit selectionChanged();
            return;
        }
    }

    if (!m_pendingSelectPath.isEmpty()) {
        QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
        QString targetName = QFileInfo(cleanTarget).fileName();

        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            QString itemName = QFileInfo(itemPath).fileName();

            if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                m_listView->setCurrentIndex(idx);
                if (m_listView->selectionModel()) {
                    QSignalBlocker blocker(m_listView->selectionModel());
                    m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
                m_listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                m_pendingSelectPath.clear();
                emit selectionChanged();
                break;
            }
        }
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
1. Launch QuarkMeta, navigate to a directory containing numerous files (e.g. 500+ items).
2. Switch to Grid View or List View, press `Ctrl+A` to select all items.
3. Switch view mode from Grid/List View to Column View.
4. Verify that the UI remains responsive, no crash / freeze occurs, and CPU usage stays normal.
5. Verify that selected items and focus path are correctly restored in Column View.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Retains `m_selectionState` as the Single Source of Truth for selection state across view mode switches.
- Uses `QSignalBlocker` and `m_isRestoringSelections` guard instead of destroying/clearing `m_selectionState.selectedPaths`.

---

## 6. Header API Signature Verification
- `ContentPanel::restoreSelections()` -> `src/ui/ContentPanel.h`
- `ColumnViewPane::tryPendingSelection()` -> `src/ui/ColumnViewWidget.h`
