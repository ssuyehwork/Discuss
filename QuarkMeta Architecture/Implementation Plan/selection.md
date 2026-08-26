# Implementation Plan - Fix ContentPanel Selection Compatibility with GridView (`selection.md`)

## 1. Overview
This implementation plan fixes the bug where selecting items in `ContentPanel` under GridView (card grid mode) fails to update `MetaPanel` (Metadata Panel). The issue stemmed from `getSelectedIndexes()` returning `selectedRows(0)`, which returns an empty list `[]` in `QListView`/`JustifiedView` because grid views operate on cell items rather than rows. This caused `PanelMediator` to abort early and fail to populate item attributes.

The fix ensures `getSelectedIndexes()` handles both `QTreeView` (returning `selectedRows(0)`) and grid views (filtering selected indexes for `column == 0`). Additionally, `PanelMediator` is updated so that item attribute lookup reliably fallback to index-based or path-based retrieval without getting blocked by empty row queries.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.h`
```git
<<<<<<< SEARCH
    QModelIndexList getSelectedIndexes() const {
        if (!m_viewStack) return {};
        QItemSelectionModel* selModel = (m_viewStack->currentWidget() == m_gridView) ?
                m_gridView->selectionModel() : m_treeView->selectionModel();
        if (!selModel) return {};
        // 核心优化：高并发防卡死，仅获取第 0 列单元格索引（而非全列索引集合），性能提升数十倍
        return selModel->selectedRows(0);
    }
=======
    QModelIndexList getSelectedIndexes() const {
        if (!m_viewStack) return {};
        bool isGrid = (m_viewStack->currentWidget() == m_gridView);
        QItemSelectionModel* selModel = isGrid ? m_gridView->selectionModel() : m_treeView->selectionModel();
        if (!selModel) return {};

        if (isGrid) {
            // 网格视图 (GridView/JustifiedView): 提取 column == 0 的单元格索引，保证在卡片模式下正确获取选中项
            QModelIndexList result;
            const QModelIndexList selected = selModel->selectedIndexes();
            result.reserve(selected.size());
            for (const QModelIndex& idx : selected) {
                if (idx.column() == 0) {
                    result.append(idx);
                }
            }
            return result;
        } else {
            // 列表视图 (TreeView): 高并发防卡死，仅获取第 0 列行索引
            return selModel->selectedRows(0);
        }
    }
>>>>>>> REPLACE
```

### `src/ui/PanelMediator.cpp`
```git
<<<<<<< SEARCH
            } else {
                auto indexes = contentPanel->getSelectedIndexes();
                if (indexes.isEmpty()) return;

                QModelIndex idx = indexes.first();
                QString path = paths.first();
=======
            } else {
                auto indexes = contentPanel->getSelectedIndexes();
                QModelIndex idx;
                if (!indexes.isEmpty()) {
                    idx = indexes.first();
                }
                QString path = paths.first();
                QFileInfo fi(path);

                QString name;
                QString type;
                QString sizeStr;
                QString mtimeStr;

                if (idx.isValid()) {
                    name = idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString();
                    type = (idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件";
                    sizeStr = idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString();
                    mtimeStr = idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString();
                }

                if (name.isEmpty()) name = fi.fileName();
                if (type.isEmpty()) type = fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件";
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Open application in GridView mode (`Ctrl+\` or toggle view mode).
2. Click on any file/folder in `ContentPanel`.
3. Verify that `MetaPanel` (on the right side) immediately updates and displays preview, ratings, colors, tags, size, and metadata without needing to click `FavoritePanel`.
4. Switch to ListView mode and verify seamless metadata synchronization when selecting items.
