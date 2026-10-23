# Implementation Plan - Column View Full Integration Fix (ColumnViewWidget-28.md)

## 1. Overview
Fix critical Column View functional degradations and missing UI integration capabilities:
1. **Item Auto-Location & Highlighting**: Ensure navigating from Favorites/Recent Visited to a file in Column View automatically focuses, scrolls to, and highlights the target file item in `rightmostPane()`.
2. **Double-Click File Opening & Activation**: Wire double-click events in `ColumnViewPane` to `ContentPanel::onDoubleClicked` so files open with default app / QuickLook.
3. **ContextMenu Model Binding Fix**: Fix `ContentContextMenu` to operate on `view->model()` instead of hardcoded `m_panel->getProxyModel()`, restoring color marking, rating, pinning, tags, and rename features in Column View.
4. **Header Operations Support**: Support recursive display (`m_isRecursive`), toggle hidden items (`showHidden`), sorting (`applySort`), and item pinning/filtering across Column View panes.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/ColumnViewWidget.h`
```
<<<<<<< SEARCH
    void setFilterState(const FilterState& state);

    QListView* listView() const { return m_listView; }
=======
    void setFilterState(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);

    QListView* listView() const { return m_listView; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
=======
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
    void applySort(int sortType, Qt::SortOrder sortOrder);
>>>>>>> REPLACE
```

### File: `src/ui/ColumnViewWidget.cpp`
```
<<<<<<< SEARCH
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (!isDir) {
            emit fileSelected(itemPath, paneIdx);
        }
    });
=======
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (m_contentPanel && index.isValid()) {
            m_contentPanel->onDoubleClicked(index);
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

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
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortType(sortType);
        m_proxyModel->sort(0, sortOrder);
    }
}

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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
}
=======
void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
}

void ColumnViewWidget::applySort(int sortType, Qt::SortOrder sortOrder) {
    for (auto* pane : m_panes) {
        if (pane) pane->applySort(sortType, sortOrder);
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
=======
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    pane->loadDirectory();
=======
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    if (m_contentPanel) {
        pane->applySort(m_contentPanel->currentSortType(), m_contentPanel->currentSortOrder());
    }
    pane->loadDirectory();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
=======
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            FilterState parentFilter;
            parentFilter.showHidden = m_currentFilter.showHidden;
            m_panes[i]->setFilterState(parentFilter);
        }
    }
>>>>>>> REPLACE
```

### File: `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        if (m_currentPath != path) {
            m_currentPath = path;
            if (m_columnView) m_columnView->setRootPath(path);
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        m_isRecursive = recursive;
        if (m_columnView) {
            m_columnView->setRootPath(path);
            restoreSelections();
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::applySort() {
    if (m_sortController) {
        m_sortController->applySortToModel(m_proxyModel);
    }
}
=======
void ContentPanel::applySort() {
    if (m_sortController) {
        m_sortController->applySortToModel(m_proxyModel);
        if (m_columnView) {
            m_columnView->applySort(m_sortController->sortType(), m_sortController->sortOrder());
        }
    }
}
>>>>>>> REPLACE
```

### File: `src/ui/controllers/ContentContextMenu.cpp`
```
<<<<<<< SEARCH
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });
=======
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto* model = view->model();
                if (!model) return;
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) model->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });
=======
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto* model = view->model();
                if (!model) return;
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) model->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            for (const QModelIndex& idx : indexes) {
                if (idx.column() == 0) {
                    m_panel->getProxyModel()->setData(idx, pin, IsLockedRole);
                }
            }
            m_panel->getProxyModel()->invalidate();
            m_panel->getProxyModel()->sort(0, m_panel->getProxyModel()->sortOrder());
            break;
        }
=======
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto* model = view->model();
            auto* proxy = qobject_cast<QSortFilterProxyModel*>(model);
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            for (const QModelIndex& idx : indexes) {
                if (idx.column() == 0 && model) {
                    model->setData(idx, pin, IsLockedRole);
                }
            }
            if (proxy) {
                proxy->invalidate();
                proxy->sort(0, proxy->sortOrder());
            }
            break;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                    if (type == LastOperationType::SetRating) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().rating(), RatingRole);
                    } else if (type == LastOperationType::SetColor) {
                        QString colorVal = LastOperationManager::instance().color();
                        m_panel->getProxyModel()->setData(idx, colorVal, ColorRole);
                        QString itemPath = idx.data(PathRole).toString();
                        QIcon coloredIcon = ShellIconManager::getFileIcon(itemPath, 128);
                        m_panel->getProxyModel()->setData(idx, coloredIcon, Qt::DecorationRole);
                    } else if (type == LastOperationType::PasteTags) {
                        m_panel->getProxyModel()->setData(idx, LastOperationManager::instance().tags(), TagsRole);
                    }
=======
                    auto* model = view->model();
                    if (!model) continue;
                    if (type == LastOperationType::SetRating) {
                        model->setData(idx, LastOperationManager::instance().rating(), RatingRole);
                    } else if (type == LastOperationType::SetColor) {
                        QString colorVal = LastOperationManager::instance().color();
                        model->setData(idx, colorVal, ColorRole);
                        QString itemPath = idx.data(PathRole).toString();
                        QIcon coloredIcon = ShellIconManager::getFileIcon(itemPath, 128);
                        model->setData(idx, coloredIcon, Qt::DecorationRole);
                    } else if (type == LastOperationType::PasteTags) {
                        model->setData(idx, LastOperationManager::instance().tags(), TagsRole);
                    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionPasteTags: {
            QStringList copiedTags = ClipboardService::instance().copiedTags();
            if (copiedTags.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
                break;
            }
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    m_panel->getProxyModel()->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
=======
        case ContentPanel::ActionPasteTags: {
            QStringList copiedTags = ClipboardService::instance().copiedTags();
            if (copiedTags.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "剪贴板无有效标签", 1500, QColor("#e81123"));
                break;
            }
            auto* model = view->model();
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && model) {
                    model->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Execute compilation build check:
   ```bash
   mkdir -p build && cd build && cmake .. && make -j$(nproc)
   ```
2. Run test execution or run binary if available.
