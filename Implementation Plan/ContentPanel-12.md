# ContentPanel-12.md: Unified SelectionState Architecture & ColumnView Absolute-Path Restoration

## 1. Overview
Previously, selection state across view modes (`GridView`, `ListView`, and `ColumnView`) was loosely managed per view instance or restored via temporary name sets (`m_pendingSelectNames`). This led to view mode selection loss, selection hijacking by parent columns, and potential name collisions across different directories (e.g., `ProjectA/Readme.md` vs `ProjectB/Readme.md`).

This implementation plan establishes a unified Single Source of Truth (**SelectionState SSOT**) managed by `ContentPanel`. `SelectionState` stores the current folder, primary focused path, and absolute selected paths (`QSet<QString> selectedPaths`). Views act strictly as Consumers and Publishers of `SelectionState`.

When switching to `ColumnView`:
1. `ContentPanel` queries the global `SelectionState`.
2. `ColumnViewWidget::setRootPath(focusedPath)` rebuilds the ancestor column hierarchy leading to the focused item.
3. The rightmost column restores batch selections using absolute paths (`setPendingSelectPaths(selectedPaths)`).

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.h`

<<<<<<< SEARCH
    void selectItemByPath(const QString& targetPath);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);
=======
    void selectItemByPath(const QString& targetPath);
    void setPendingSelectPaths(const QSet<QString>& paths);
    void clearSelection();
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);
>>>>>>> REPLACE

<<<<<<< SEARCH
private:
    QString m_path;
    QString m_pendingSelectPath;
=======
private:
    QString m_path;
    QString m_pendingSelectPath;
    QSet<QString> m_pendingSelectPaths;
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

void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
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
>>>>>>> REPLACE

<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
=======
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                }
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
=======
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        if (pane == rightmostPane() || (pane->listView() && pane->listView()->hasFocus())) {
            m_activePaneIndex = pane->property("paneIndex").toInt();
        }
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
>>>>>>> REPLACE

---

### File 3: `src/ui/ContentPanel.h`

<<<<<<< SEARCH
    QSet<QString> m_pendingSelectNames;
=======
    struct SelectionState {
        QString currentFolder;
        QString focusedPath;
        QSet<QString> selectedPaths;
    };
    SelectionState m_selectionState;
>>>>>>> REPLACE

---

### File 4: `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }
    QStringList savedSelectedPaths = getSelectedPaths();
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 🚀【视图切换选区无损同步】：在新激活的视图中批量恢复之前全量选中高亮与聚焦位置
    if (!savedSelectedPaths.isEmpty()) {
        m_pendingSelectNames.clear();
        for (const QString& selPath : savedSelectedPaths) {
            m_pendingSelectNames.insert(QFileInfo(selPath).fileName());
        }
        restoreSelections();
    }
=======
void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }
    // 1. 在原视图中上报并更新 SelectionState (SSOT)
    m_selectionState.currentFolder = m_currentPath;
    m_selectionState.selectedPaths = QSet<QString>(getSelectedPaths().begin(), getSelectedPaths().end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }

    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
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
3. Confirm `m_selectionState` stores absolute paths (`selectedPaths`) and rebuilds the ancestor tree via `columnView->setRootPath(focusedPath)`.
4. Confirm `rightmostPane()` restores the 39 Markdown files cleanly without selecting parent folders or dropping selections.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Unified SSOT Channel**: Standardizes all selection restoration through `m_selectionState` across all four view modes.
- **Absolute Path Guarantee**: Uses `selectedPaths` instead of file names, preventing cross-directory file name collisions.

---

## 6. Header API Signature Verification
- `ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths)` -> `src/ui/ColumnViewWidget.h`
- `ContentPanel::restoreSelections()` -> `src/ui/ContentPanel.h`
