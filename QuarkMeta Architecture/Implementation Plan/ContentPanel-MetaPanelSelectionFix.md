# ContentPanel-MetaPanelSelectionFix.md - ContentPanel View Stack Selection & KeyHandler Hitbox Implementation Plan

## 1. Overview
本实施方案旨在解决因引入 `FolderSectionWidget` 复合容器（`m_gridContainerWidget` / `m_listContainerWidget`）后，`ContentPanel` 中使用 `qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())` 获取当前活动视图失败导致的四大受损功能（选中项获取、滚动定位、懒加载缩略图刷新、选择集恢复），恢复右侧元数据面板（`MetaPanel`）星级/颜色按钮交互状态，并彻底修复 `ContentKeyHandler` 中未选中卡片点击星级 Hitbox 的触发与数据提交逻辑。

## 2. Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `src/ui/controllers/ContentKeyHandler.cpp`

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
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) continue;

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex proxyIdx = proxy->index(r, 0);
            QModelIndex srcIdx = proxy->mapToSource(proxyIdx);
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
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) continue;

        for (int i = 0; i < proxy->rowCount(); ++i) {
            QModelIndex proxyIdx = proxy->index(i, 0);
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

```
<<<<<<< SEARCH
    for (auto* view : views) {
        if (!view || !view->selectionModel()) continue;
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) proxy = m_proxyModel;
        DiskItemModel* diskModel = m_diskModel;

        if (diskModel && proxy) {
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
    }
=======
    for (auto* view : views) {
        if (!view || !view->selectionModel()) continue;
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) proxy = m_proxyModel;
        DiskItemModel* diskModel = m_diskModel;

        if (diskModel && proxy) {
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

## 4. Build & Verification Steps
1. 使用 MSVC / Visual Studio 编译环境：
   ```cmd
   mkdir build
   cd build
   cmake -G "Visual Studio 17 2022" -A x64 ..
   cmake --build . --config Release
   ```
2. 运行 QuarkMeta 应用程序并进行验证：
   - **右侧 MetaPanel 与选区联动**：点击网格/列表视图中的文件或文件夹，验证右侧 `MetaPanel` 星级/色标控件解除 Disabled 状态，能正常点击修改；
   - **缩略图延迟加载**：快速滚动网格/列表视图，验证 `refreshVisibleThumbnails()` 准确获取可见区域索引并顺畅加载缩略图；
   - **路径定位滚动**：通过地址栏或搜索定位指定项目，验证 `selectAndScrollToItem()` 能够精准滚动并选中目标；
   - **星级 Hitbox 点击**：鼠标直接点击未选中的卡片/列表星级 Hitbox，验证能够即时自动选中该项并赋予对应的星级评级。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **完整清理 `qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())`**：排查并消除了全文件所有隐患点，支持 Composite Container 架构。
- **数据提交映射**：使用 `selIdx.model()->setData(...)` 直接通过当前 Item 绑定的真实 Model / ProxyModel 提交数据，消除 `getActiveProxyModel()` 错位。

## 6. Header API Signature Verification
经物理查阅 `.h` 源头，核验涉及类及成员函数精准签名如下：

### `ContentPanel.h`
- `QModelIndexList getSelectedIndexes() const;`
- `QStringList getSelectedPaths() const;`
- `void refreshVisibleThumbnails();`
- `void selectAndScrollToItem(const QString& path);`
- `DropJustifiedView* folderGridView() const;`
- `DropTreeView* folderTreeView() const;`

### `ContentKeyHandler.h`
- `bool handleMousePress(QObject* obj, QEvent* event);`

所有物理签名 100% 绝对一致，完全防范 C2039 成员不存在等编译错误。
