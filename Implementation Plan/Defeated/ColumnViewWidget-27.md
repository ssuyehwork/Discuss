# ColumnViewWidget Implementation Plan - Auto-Locate and Select Favorite Files

## 1. Overview
When a file item in Favorites or Recent Visited is clicked, `PanelMediator` sets the file name in `m_pendingSelectNames` and navigates to its parent directory. In Grid/List views, `restoreSelections()` locates and selects the file. In Column View, `restoreSelections()` was not called upon column load, and `selectAndScrollToItem()` pointed to `activePane()` instead of `rightmostPane()`.

This implementation plan adds `restoreSelections()` invocation on `activeColumnRecordsChanged`, updates `selectAndScrollToItem()` to target `rightmostPane()`, and improves `ColumnViewPane::tryPendingSelection()` to support filename-based matching.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```git
<<<<<<< SEARCH
void ColumnViewPane::tryPendingSelection() {
    if (m_pendingSelectPath.isEmpty() || !m_proxyModel || !m_listView) return;

    QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));

        if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0) {
            m_listView->setCurrentIndex(idx);
            if (m_listView->selectionModel()) {
                m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            m_pendingSelectPath.clear();
            emit selectionChanged();
            break;
        }
    }
}
=======
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
            m_listView->scrollTo(idx, QAbstractItemView::EnsureVisible);
            m_pendingSelectPath.clear();
            emit selectionChanged();
            break;
        }
    }
}
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```git
<<<<<<< SEARCH
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
        if (m_statsWorker && !records.empty()) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
    });
=======
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
        if (m_statsWorker && !records.empty()) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
        restoreSelections();
    });
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            m_columnView->activePane()->selectItemByPath(path);
        }
        return;
    }
=======
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->selectItemByPath(path);
        }
        return;
    }
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_pendingSelectNames.isEmpty()) return;
    QAbstractItemView* view = nullptr;
    DiskItemModel* diskModel = m_diskModel;
    QSortFilterProxyModel* proxy = m_proxyModel;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            view = m_columnView->activePane()->listView();
            diskModel = m_columnView->activePane()->model();
            proxy = m_columnView->activePane()->proxyModel();
        }
    } else {
        view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    }
=======
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
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Recompile project with CMake.
2. Launch QuarkMeta in Column View mode.
3. Click a file item from Favorites panel or Recent Visited panel.
4. Verify that the parent directory expands into the column view and the target file item is automatically selected and scrolled into view.
