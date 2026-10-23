# ContentPanel-8.md: ColumnView Multi-Selection Preservation & Selection Normalization

## 1. Overview
When switching view modes from Grid/List View to Column View (`ColumnView`), multi-selected items (e.g., Markdown files) were previously lost, and only parent folders in intermediate columns remained expanded/highlighted. This occurred because:
1. `ColumnViewPane` only supported single-path pending selection restoration (`m_pendingSelectPath` string) rather than batch set selection (`QSet<QString>`).
2. In `ContentPanel::setViewMode()`, switching to `ColumnView` called `selectAndScrollToPath()` iteratively, clearing previous selections on each call.
3. In `ContentPanel::restoreSelections()`, inspects of `rightmostPane()` were executed synchronously before the rightmost column finished its asynchronous disk scanning (`loadDirectory()`).

This implementation plan normalizes Column View selection restoration by introducing `QSet<QString> m_pendingSelectNames` to `ColumnViewPane`, routing multi-selection restoration through `ColumnViewWidget::rightmostPane()`, and ensuring batch selection is applied upon asynchronous directory data load completion.

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
    if (m_pendingSelectPath.isEmpty() || !m_proxyModel || !m_listView) return;

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
=======
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
1. Select 30 files in Grid View or List View.
2. Click TitleBar or press shortcut to switch to Column View (`ColumnView`).
3. Verify that the rightmost column in `ColumnView` asynchronously completes scan and highlights all 30 files with multi-selection.
4. Verify that MetaPanel accurately reflects the batch selection count.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Batch Selection Entry Point**: Reuses `m_pendingSelectNames` and `restoreSelections()` SSOT restoration channels.
- **Data Normalization**: Eliminates iterative `selectAndScrollToPath()` calls that wiped previous selections in favor of unified batch `QItemSelection`.

---

## 6. Header API Signature Verification
- `ColumnViewPane::setPendingSelectNames(const QSet<QString>& names)` -> `src/ui/ColumnViewWidget.h`
- `ColumnViewWidget::rightmostPane()` -> `src/ui/ColumnViewWidget.h`
- `ContentPanel::restoreSelections()` -> `src/ui/ContentPanel.h`
