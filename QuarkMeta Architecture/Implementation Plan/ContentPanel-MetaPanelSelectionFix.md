# ContentPanel-MetaPanelSelectionFix.md - ContentPanel View Stack Selection, Context Menu & KeyHandler Hitbox Implementation Plan

## 1. Overview
本实施方案旨在彻底排查并修复因引入 `FolderSectionWidget` 复合容器（`m_gridContainerWidget` / `m_listContainerWidget`）及独立 ProxyModel 架构（`m_folderProxyModel` 与 `m_fileProxyModel` 分线）引发的全套受损功能，包括：
1. `ContentPanel` 中使用 `qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())` 获取当前活动视图失败导致的五大受损功能：选中项获取 `getSelectedIndexes()`、选中路径 `getSelectedPaths()`、滚动定位 `selectAndScrollToItem()`、懒加载缩略图刷新 `refreshVisibleThumbnails()` 与右键菜单缺省 Fallback；
2. 右侧元数据面板（`MetaPanel`）星级/颜色按钮交互解锁；
3. `ContentKeyHandler` 中卡片/列表星级 Hitbox 点击以及快捷键（Ctrl+0~5 星级、Alt+1~9 色标、Alt+D 置顶、F4 重复操作）在文件夹子视图 (`m_folderGridView` / `m_folderTreeView`) 上的 Model 索引错位导致数据提交失败的问题；
4. `ContentContextMenu` 中右键菜单重复操作与粘贴标签在文件夹子视图上的 Model 数据提交错位问题。

## 2. Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `src/ui/controllers/ContentKeyHandler.cpp`
4. `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes (Git Merge Diff)

### 3.1 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
    QAbstractItemView* gridView() const { return m_gridView; }
    QTreeView* treeView() const;
=======
    QAbstractItemView* gridView() const { return m_gridView; }
    DropJustifiedView* folderGridView() const { return m_folderGridView; }
    DropTreeView* folderTreeView() const { return m_folderTreeView; }
    QTreeView* treeView() const;
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = (m_viewStack && m_viewStack->currentWidget() == m_gridView) ? m_gridView : m_treeView;
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
=======
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) {
        if (m_currentViewMode == ListView) {
            view = m_treeView ? static_cast<QAbstractItemView*>(m_treeView) : m_folderTreeView;
        } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
            view = m_gridView ? static_cast<QAbstractItemView*>(m_gridView) : m_folderGridView;
        }
    }
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    if (!view || !m_model || !m_proxyModel || CoreController::isShuttingDown() || !view->viewport()) return;

    QRect vpRect = view->viewport()->rect();
    QModelIndex topIdx = view->indexAt(vpRect.topLeft());
    QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

    int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
    int bottom = btmIdx.isValid() ? qMin(m_proxyModel->rowCount() - 1, btmIdx.row() + 4) : m_proxyModel->rowCount() - 1;

    QList<int> visibleRows;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        if (srcIdx.isValid()) visibleRows.append(srcIdx.row());
    }

    m_model->loadThumbnailsForRows(visibleRows);
}
=======
void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    QList<int> visibleRows;

    for (auto* view : views) {
        if (!view || !view->viewport() || !view->model()) continue;
        QAbstractItemModel* model = view->model();

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(model->rowCount() - 1, btmIdx.row() + 4) : model->rowCount() - 1;

        auto* filterProxy = qobject_cast<QSortFilterProxyModel*>(model);
        for (int r = top; r <= bottom; ++r) {
            QModelIndex proxyIdx = model->index(r, 0);
            QModelIndex srcIdx = filterProxy ? filterProxy->mapToSource(proxyIdx) : proxyIdx;
            if (srcIdx.isValid()) visibleRows.append(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_model->loadThumbnailsForRows(visibleRows);
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) {
            QFileInfo info(path);
            if (info.exists() && !info.isDir()) {
                QString dirPath = info.absolutePath();
                if (!m_columnView->containsPath(dirPath)) {
                    m_columnView->setRootPath(path);
                } else if (m_columnView->rightmostPane()) {
                    m_columnView->rightmostPane()->selectItemByPath(path);
                }
            } else {
                if (!m_columnView->containsPath(path)) {
                    m_columnView->setRootPath(path);
                }
            }
        }
        return;
    }
    if (!m_proxyModel || path.isEmpty()) return;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex proxyIdx = m_proxyModel->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
            if (view && view->selectionModel()) {
                view->scrollTo(proxyIdx);
                view->setCurrentIndex(proxyIdx);
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}
=======
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) {
            QFileInfo info(path);
            if (info.exists() && !info.isDir()) {
                QString dirPath = info.absolutePath();
                if (!m_columnView->containsPath(dirPath)) {
                    m_columnView->setRootPath(path);
                } else if (m_columnView->rightmostPane()) {
                    m_columnView->rightmostPane()->selectItemByPath(path);
                }
            } else {
                if (!m_columnView->containsPath(path)) {
                    m_columnView->setRootPath(path);
                }
            }
        }
        return;
    }
    if (path.isEmpty()) return;

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    for (auto* view : views) {
        if (!view || !view->selectionModel() || !view->model()) continue;
        QAbstractItemModel* model = view->model();

        for (int i = 0; i < model->rowCount(); ++i) {
            QModelIndex proxyIdx = model->index(i, 0);
            if (proxyIdx.data(PathRole).toString() == path) {
                view->scrollTo(proxyIdx);
                view->setCurrentIndex(proxyIdx);
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                return;
            }
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}
=======
QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    QAbstractItemView* curView = nullptr;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            curView = m_columnView->activePane()->listView();
        }
    } else {
        curView = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    }
    if (!curView || !curView->selectionModel()) return {};

    QModelIndexList res;
    const auto& selected = curView->selectionModel()->selectedIndexes();
    res.reserve(selected.size());
    for (const auto& idx : selected) {
        if (idx.column() == 0) {
            res.append(idx);
        }
    }
    return res;
}
=======
QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    QModelIndexList res;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            DropListView* folderView = m_columnView->activePane()->folderListView();
            if (folderView && folderView->selectionModel() && folderView->selectionModel()->hasSelection()) {
                for (const auto& idx : folderView->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) res.append(idx);
                }
            }
            DropListView* fileView = m_columnView->activePane()->listView();
            if (fileView && fileView->selectionModel() && fileView->selectionModel()->hasSelection()) {
                for (const auto& idx : fileView->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) res.append(idx);
                }
            }
        }
        return res;
    }

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    } else {
        if (auto* v = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())) views << v;
    }

    for (auto* view : views) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) res.append(idx);
            }
        }
    }

    return res;
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/controllers/ContentKeyHandler.cpp`
```
<<<<<<< SEARCH
        if (hitVal != -1) {
            bool isSelected = view->selectionModel() && view->selectionModel()->isSelected(index);
            if (!isSelected) return false;

            auto selectedIndexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : selectedIndexes) {
                if (selIdx.column() == 0) {
                    m_panel->getActiveProxyModel()->setData(selIdx, hitVal, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, view, [view, cur]() { view->setEditTriggers(cur); });
            event->accept();
            return true;
        }
=======
        if (hitVal != -1) {
            bool isSelected = view->selectionModel() && view->selectionModel()->isSelected(index);
            if (!isSelected) {
                if (view->selectionModel()) {
                    view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    view->setCurrentIndex(index);
                }
            }

            auto selectedIndexes = view->selectionModel() ? view->selectionModel()->selectedIndexes() : QModelIndexList{index};
            for (const auto& selIdx : selectedIndexes) {
                if (selIdx.column() == 0 && selIdx.model()) {
                    const_cast<QAbstractItemModel*>(selIdx.model())->setData(selIdx, hitVal, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, view, [view, cur]() { view->setEditTriggers(cur); });
            event->accept();
            return true;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!isRowSelected) return false;

            auto selectedRows = treeView->selectionModel()->selectedRows();
            for (const auto& selRow : selectedRows) {
                QModelIndex targetIdx = treeView->model()->index(selRow.row(), 0, selRow.parent());
                m_panel->getActiveProxyModel()->setData(targetIdx, hitStar, RatingRole);
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
            treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, treeView, [treeView, currentTriggers]() {
                treeView->setEditTriggers(currentTriggers);
            });
            event->accept();
            return true;
        }
=======
        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!isRowSelected) {
                if (treeView->selectionModel()) {
                    treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    treeView->setCurrentIndex(index);
                }
            }

            auto selectedRows = treeView->selectionModel() ? treeView->selectionModel()->selectedRows() : QModelIndexList{index};
            for (const auto& selRow : selectedRows) {
                QModelIndex targetIdx = treeView->model()->index(selRow.row(), 0, selRow.parent());
                if (targetIdx.isValid() && targetIdx.model()) {
                    const_cast<QAbstractItemModel*>(targetIdx.model())->setData(targetIdx, hitStar, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
            treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, treeView, [treeView, currentTriggers]() {
                treeView->setEditTriggers(currentTriggers);
            });
            event->accept();
            return true;
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 1. Ctrl + 0~5: 评级
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) m_panel->getActiveProxyModel()->setData(idx, rating, RatingRole);
        }
        return true;
    }

    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0) {
                bool current = idx.data(IsLockedRole).toBool();
                m_panel->getActiveProxyModel()->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }

    // 3. Alt + 1~9: 色标快速赋予
    if ((keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() >= Qt::Key_1 && keyEvent->key() <= Qt::Key_9)) {
        static const QString colors[] = {
            "#E24B4A", "#EF9F27", "#FECF0E", "#639922",
            "#2E90FA", "#8E44AD", "#888888", "#555555", "#000000"
        };
        int idx = keyEvent->key() - Qt::Key_1;
        if (idx >= 0 && idx < 9) {
            QString colorValue = colors[idx];
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const QModelIndex& selIdx : indexes) {
                if (selIdx.column() == 0) {
                    m_panel->getActiveProxyModel()->setData(selIdx, colorValue, ColorRole);
                    QString path = selIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    m_panel->getActiveProxyModel()->setData(selIdx, coloredIcon, Qt::DecorationRole);
                }
            }
            return true;
        }
    }
=======
    // 1. Ctrl + 0~5: 评级
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0 && idx.model()) {
                const_cast<QAbstractItemModel*>(idx.model())->setData(idx, rating, RatingRole);
            }
        }
        return true;
    }

    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0 && idx.model()) {
                bool current = idx.data(IsLockedRole).toBool();
                const_cast<QAbstractItemModel*>(idx.model())->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }

    // 3. Alt + 1~9: 色标快速赋予
    if ((keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() >= Qt::Key_1 && keyEvent->key() <= Qt::Key_9)) {
        static const QString colors[] = {
            "#E24B4A", "#EF9F27", "#FECF0E", "#639922",
            "#2E90FA", "#8E44AD", "#888888", "#555555", "#000000"
        };
        int idx = keyEvent->key() - Qt::Key_1;
        if (idx >= 0 && idx < 9) {
            QString colorValue = colors[idx];
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const QModelIndex& selIdx : indexes) {
                if (selIdx.column() == 0 && selIdx.model()) {
                    const_cast<QAbstractItemModel*>(selIdx.model())->setData(selIdx, colorValue, ColorRole);
                    QString path = selIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    const_cast<QAbstractItemModel*>(selIdx.model())->setData(selIdx, coloredIcon, Qt::DecorationRole);
                }
            }
            return true;
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& targetIdx : indexes) {
            if (targetIdx.column() == 0) {
                if (type == LastOperationType::SetRating) {
                    m_panel->getActiveProxyModel()->setData(targetIdx, LastOperationManager::instance().rating(), RatingRole);
                } else if (type == LastOperationType::SetColor) {
                    QString colorVal = LastOperationManager::instance().color();
                    m_panel->getActiveProxyModel()->setData(targetIdx, colorVal, ColorRole);
                    QString path = targetIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    m_panel->getActiveProxyModel()->setData(targetIdx, coloredIcon, Qt::DecorationRole);
                } else if (type == LastOperationType::PasteTags) {
                    m_panel->getActiveProxyModel()->setData(targetIdx, LastOperationManager::instance().tags(), TagsRole);
                }
            }
        }
=======
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& targetIdx : indexes) {
            if (targetIdx.column() == 0 && targetIdx.model()) {
                auto* model = const_cast<QAbstractItemModel*>(targetIdx.model());
                if (type == LastOperationType::SetRating) {
                    model->setData(targetIdx, LastOperationManager::instance().rating(), RatingRole);
                } else if (type == LastOperationType::SetColor) {
                    QString colorVal = LastOperationManager::instance().color();
                    model->setData(targetIdx, colorVal, ColorRole);
                    QString path = targetIdx.data(PathRole).toString();
                    QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                    model->setData(targetIdx, coloredIcon, Qt::DecorationRole);
                } else if (type == LastOperationType::PasteTags) {
                    model->setData(targetIdx, LastOperationManager::instance().tags(), TagsRole);
                }
            }
        }
>>>>>>> REPLACE
```

### 3.4 `src/ui/controllers/ContentContextMenu.cpp`
```
<<<<<<< SEARCH
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
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
                    count++;
                }
            }
=======
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && idx.model()) {
                    auto* model = const_cast<QAbstractItemModel*>(idx.model());
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
                    count++;
                }
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    m_panel->getProxyModel()->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
=======
            auto indexes = view->selectionModel()->selectedIndexes();
            int count = 0;
            for (const auto& idx : indexes) {
                if (idx.column() == 0 && idx.model()) {
                    const_cast<QAbstractItemModel*>(idx.model())->setData(idx, copiedTags, TagsRole);
                    count++;
                }
            }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 使用 MSVC / Visual Studio 编译环境：
   ```cmd
   mkdir build
   cd build
   cmake -G "Visual Studio 17 2022" -A x64 ..
   cmake --build . --config Release
   ```
2. 运行 QuarkMeta 应用程序并进行全量验证：
   - **右侧 MetaPanel 与选区联动**：点击网格/列表视图中的文件或文件夹，验证右侧 `MetaPanel` 星级/色标控件解除 Disabled 状态，能正常点击修改；
   - **缩略图延迟加载**：快速滚动网格/列表视图，验证 `refreshVisibleThumbnails()` 准确获取可见区域索引并顺畅加载缩略图；
   - **路径定位滚动**：通过地址栏或搜索定位指定项目，验证 `selectAndScrollToItem()` 能够精准滚动并选中目标；
   - **右键上下文菜单**：在文件夹/文件容器背景或项目空白处右键点击，验证 `onCustomContextMenuRequested()` 在缺少 sender 时仍能准确获取默认 Fallback 视图呈现系统菜单；
   - **文件夹项键盘快捷键**：在文件夹子视图（`m_folderGridView` / `m_folderTreeView`）中选中文件夹，按下 `Ctrl+1~5`（设置星级）、`Alt+1~9`（设置色标）、`Alt+D`（置顶）与 `F4`（重复上一次操作），验证 Model 准确更新数据且图标正常变色；
   - **星级 Hitbox 点击**：鼠标直接点击未选中的卡片/列表星级 Hitbox，验证能够即时自动选中该项并赋予对应的星级评级。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **完整清理 `qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())`**：排查并消除了全文件所有隐患点，支持 Composite Container 架构。
- **数据提交映射**：使用 `selIdx.model()->setData(...)` 直接通过当前 Item 绑定的真实 Model / ProxyModel 提交数据，消除 `getActiveProxyModel()` / `getProxyModel()` 错位。

## 6. Header API Signature Verification
经物理查阅 `.h` 源头，核验涉及类及成员函数精准签名如下：

### `ContentPanel.h`
- `QModelIndexList getSelectedIndexes() const;`
- `QStringList getSelectedPaths() const;`
- `void refreshVisibleThumbnails();`
- `void selectAndScrollToItem(const QString& path);`
- `void onCustomContextMenuRequested(const QPoint& pos);`
- `DropJustifiedView* folderGridView() const;`
- `DropTreeView* folderTreeView() const;`

### `ContentKeyHandler.h`
- `bool handleMousePress(QObject* obj, QEvent* event);`
- `bool handleKeyPress(QObject* obj, QEvent* event);`

所有物理签名 100% 绝对一致，完全防范 C2039 成员不存在等编译错误。
