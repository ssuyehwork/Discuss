# ContentPanel-9.md: ColumnView Multi-Selection Preservation & Async Timing Normalization

## 1. Overview
When switching view modes from Grid/List View to Column View (`ColumnView`), multi-selected items were lost because `ColumnViewPane::loadDirectory()` only checked `m_pendingSelectPath` upon asynchronous scan completion, omitting `tryPendingSelection()` for `m_pendingSelectNames`. Consequently, when `restoreSelections()` passed `m_pendingSelectNames` to `rightmostPane()`, `tryPendingSelection()` ran prematurely before disk scanning finished and returned without clearing or selecting items.

This implementation plan normalizes Column View selection restoration by:
1. Ensuring `tryPendingSelection()` is invoked inside `ColumnViewPane::loadDirectory()` after asynchronous data decoration and model assignment.
2. Updating `ColumnViewPane::tryPendingSelection()` to preserve `m_pendingSelectNames` if the proxy model is empty or zero-row, preventing premature clearing during asynchronous scanning.
3. Consolidating `ContentPanel::setViewMode()` selection restoration to pass `m_pendingSelectNames` through `restoreSelections()`.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.h`

<<<<<<< SEARCH
    void selectItemByPath(const QString& targetPath);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);

    DropListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

private slots:
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
=======
    void selectItemByPath(const QString& targetPath);
    void setPendingSelectNames(const QSet<QString>& names);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);

    DropListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }

signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);

private slots:
    void tryPendingSelection();

private:
    QString m_path;
    QString m_pendingSelectPath;
    QSet<QString> m_pendingSelectNames;
>>>>>>> REPLACE

---

### File 2: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::setPendingSelectNames(const QSet<QString>& names) {
    m_pendingSelectNames = names;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
>>>>>>> REPLACE

<<<<<<< SEARCH
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectNames.isEmpty()) {
        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemName = QFileInfo(idx.data(PathRole).toString()).fileName();
            if (m_pendingSelectNames.contains(itemName)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                m_listView->setCurrentIndex(lastIdx);
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectNames.clear();
            emit selectionChanged();
            return;
        }
    }
=======
void ColumnViewPane::tryPendingSelection() {
    if (!m_proxyModel || !m_listView) return;

    if (!m_pendingSelectNames.isEmpty() && m_proxyModel->rowCount() > 0) {
        QItemSelection sel;
        QModelIndex lastIdx;
        for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
            QModelIndex idx = m_proxyModel->index(r, 0);
            QString itemName = QFileInfo(idx.data(PathRole).toString()).fileName();
            if (m_pendingSelectNames.contains(itemName)) {
                sel.select(idx, idx);
                lastIdx = idx;
            }
        }
        if (!sel.isEmpty() && m_listView->selectionModel()) {
            m_listView->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                m_listView->setCurrentIndex(lastIdx);
                m_listView->scrollTo(lastIdx, QAbstractItemView::PositionAtCenter);
            }
            m_pendingSelectNames.clear();
            emit selectionChanged();
            return;
        }
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
=======
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                if (!weakSelf->m_pendingSelectNames.isEmpty()) {
                    weakSelf->tryPendingSelection();
                }
>>>>>>> REPLACE

---

### File 3: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
    // 🚀【视图切换选区无损同步】：在新激活的视图中同步恢复之前的选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        for (const QString& selPath : savedSelectedPaths) {
            selectAndScrollToPath(selPath);
        }
    }
=======
    // 🚀【视图切换选区无损同步】：在新激活的视图中同步恢复之前的选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        m_pendingSelectNames.clear();
        for (const QString& selPath : savedSelectedPaths) {
            m_pendingSelectNames.insert(QFileInfo(selPath).fileName());
        }
        restoreSelections();
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_pendingSelectNames.isEmpty()) return;
    QAbstractItemView* view = nullptr;
    DiskItemModel* diskModel = m_diskModel;
    QSortFilterProxyModel* proxy = m_proxyModel;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            view = m_columnView->rightmostPane()->listView();
            diskModel = m_columnView->rightmostPane()->model();
            proxy = m_columnView->rightmostPane()->proxyModel();
        }
    } else {
        view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    }
    if (view && view->selectionModel() && diskModel && proxy) {
        QItemSelection sel;
        QModelIndex lastIdx;
        const auto& recs = diskModel->allRecords();
        for (size_t i = 0; i < recs.size(); ++i) {
            if (m_pendingSelectNames.contains(QFileInfo(recs[i].path).fileName())) {
                QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
            }
        }
        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
    }
    m_pendingSelectNames.clear();
}
=======
void ContentPanel::restoreSelections() {
    if (m_pendingSelectNames.isEmpty()) return;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectNames(m_pendingSelectNames);
        }
        m_pendingSelectNames.clear();
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
            if (m_pendingSelectNames.contains(QFileInfo(recs[i].path).fileName())) {
                QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
            }
        }
        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
    }
    m_pendingSelectNames.clear();
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
1. Select multiple files in Grid View or List View.
2. Switch view mode to Column View (`ColumnView`).
3. Verify that the rightmost pane in Column View asynchronously loads records and highlights all selected files seamlessly.
4. Verify MetaPanel status bar shows the correct selection count.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Batch Selection Channel**: Leverages SSOT `m_pendingSelectNames` and `restoreSelections()` routing to ensure zero duplication and prevent clearing multi-selections during view mode transitions.

---

## 6. Header API Signature Verification
- `ColumnViewPane::setPendingSelectNames(const QSet<QString>& names)` -> `src/ui/ColumnViewWidget.h`
- `ColumnViewPane::tryPendingSelection()` -> `src/ui/ColumnViewWidget.h`
- `ContentPanel::restoreSelections()` -> `src/ui/ContentPanel.h`
