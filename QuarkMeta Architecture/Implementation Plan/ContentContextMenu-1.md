# ContentContextMenu-1 Implementation Plan

## 1. Overview
This implementation plan unifies item renaming in the right-click context menu and keyboard shortcuts (`F2`).
It abolishes the standalone "批量重命名 (Ctrl+Shift+R)" right-click menu item and the `Ctrl+Shift+R` shortcut.

### Behaviors:
1. The right-click menu always shows a single unified "重命名" menu option regardless of whether a file or folder is selected, or whether single/multiple items are selected.
2. The shortcut is unified strictly to **`F2`**.
3. **Smart Dispatching**:
   - If **1 item** is selected: Triggers inline file/folder name editing (`view->edit(...)`).
   - If **> 1 items** are selected: Triggers the Batch Rename Dialog (`m_panel->performBatchRename()`).

---

## 2. Modified Files List
1. `src/ui/controllers/ContentContextMenu.cpp`
2. `src/ui/controllers/ContentKeyHandler.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentContextMenu.cpp`
Unify the right-click menu option to always show "重命名", and automatically route to batch rename if more than 1 item is selected.

<<<<<<< SEARCH
            int selectedCount = 0;
            for (const auto& selIdx : view->selectionModel()->selectedIndexes()) {
                if (selIdx.column() == 0 && !selIdx.data(PathRole).toString().isEmpty()) selectedCount++;
            }

            if (selectedCount <= 1) {
                menu.addAction("重命名")->setData(ContentPanel::ActionRename);
            }
            if (isFolder || selectedCount > 1) {
                menu.addAction("批量重命名 (Ctrl+Shift+R)")->setData(ContentPanel::ActionBatchRename);
            }
=======
            menu.addAction("重命名")->setData(ContentPanel::ActionRename);
>>>>>>> REPLACE

<<<<<<< SEARCH
        case ContentPanel::ActionBatchRename:
            m_panel->performBatchRename();
            break;
        case ContentPanel::ActionRename:
            view->edit(currentIndex);
            break;
=======
        case ContentPanel::ActionBatchRename:
        case ContentPanel::ActionRename: {
            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.size() > 1) {
                m_panel->performBatchRename();
            } else {
                view->edit(currentIndex);
            }
            break;
        }
>>>>>>> REPLACE

---

### 3.2 `src/ui/controllers/ContentKeyHandler.cpp`
Abolish the `Ctrl+Shift+R` shortcut and enhance `F2` to handle smart dispatching.

<<<<<<< SEARCH
    // 4. Ctrl + Shift + C: 复制路径列表
    if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (keyEvent->key() == Qt::Key_C) {
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) paths << QDir::toNativeSeparators(idx.data(PathRole).toString());
            }
            if (!paths.isEmpty()) QApplication::clipboard()->setText(paths.join("\r\n"));
            return true;
        }
        if (keyEvent->key() == Qt::Key_R) {
            m_panel->performBatchRename();
            return true;
        }
    }

    // 5. 基础文件操作键
    if (keyEvent->key() == Qt::Key_F2) {
        view->edit(view->currentIndex());
        return true;
    }
=======
    // 4. Ctrl + Shift + C: 复制路径列表
    if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (keyEvent->key() == Qt::Key_C) {
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) paths << QDir::toNativeSeparators(idx.data(PathRole).toString());
            }
            if (!paths.isEmpty()) QApplication::clipboard()->setText(paths.join("\r\n"));
            return true;
        }
    }

    // 5. 基础文件操作键 (F2: 选中 1 项进入行内重命名，选中多项进入批量重命名)
    if (keyEvent->key() == Qt::Key_F2) {
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
            view->edit(view->currentIndex());
        }
        return true;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Configure and build target:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Verify that right-clicking a single folder or file shows only "重命名".
3. Verify pressing `F2` or clicking "重命名" on 1 item triggers inline editing.
4. Verify pressing `F2` or clicking "重命名" on multiple items opens the Batch Rename Dialog.
