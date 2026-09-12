# InlineRenameF2Integration Implementation Plan

This implementation plan documents the architecture contract and physical design for the universal F2 inline rename workflow (`FileNameLineEdit`) across all view modes (**GridView**, **ListView**, **JustifiedView**, and **ViewModeColumn**).

## Overview
To prevent code duplication ("reinventing the wheel") and maintain consistent UX across the entire application, all view modes route inline file/folder renaming through a centralized pipeline:
1. **Key Handling**: `ContentKeyHandler` intercepts the `F2` key event globally across all item views and triggers `view->edit(nameIdx)`.
2. **Editor Delegation**: Each item view's custom delegate (`ThumbnailDelegate`, `TreeItemDelegate`, and `ColumnItemDelegate`) overrides `createEditor` to return a unified `FileNameLineEdit` instance.
3. **Behavior Consistency**: `FileNameLineEdit` encapsulates extension protection (selecting only the filename without the extension), direction key navigation interception (Up/Down arrow navigation, Esc cancellation, Return submission), and custom dark OS context menus.
4. **Data Persistence**: Edits are committed back through `DiskItemModel::setData`, executing physical renaming via `ShellHelper::renameItem` and syncing metadata cache in real time.

---

## Modified Files List
- `src/ui/controllers/ContentKeyHandler.cpp`
- `src/ui/ThumbnailDelegate.h`
- `src/ui/TreeItemDelegate.h`
- `src/ui/ColumnItemDelegate.h`
- `src/ui/FileNameLineEdit.h`

---

## Detailed Line-by-Line Changes

### 1. Centralized F2 Key Interception (`src/ui/controllers/ContentKeyHandler.cpp`)

```git
<<<<<<< SEARCH
    // 6. 基础文件操作键
    if (keyEvent->key() == Qt::Key_F2) {
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid() && view->selectionModel()) {
                auto selected = view->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) idx = selected.first();
            }
            if (idx.isValid()) {
                QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
                view->setCurrentIndex(nameIdx);
                view->edit(nameIdx);
            }
        }
        return true;
    }
=======
    // 6. 基础文件操作键：单选触发 F2 行内编辑 (FileNameLineEdit)，多选触发批量重命名对话框
    if (keyEvent->key() == Qt::Key_F2) {
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid() && view->selectionModel()) {
                auto selected = view->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) idx = selected.first();
            }
            if (idx.isValid()) {
                QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
                view->setCurrentIndex(nameIdx);
                view->edit(nameIdx);
            }
        }
        return true;
    }
>>>>>>> REPLACE
```

---

### 2. Universal Editor Creation in Item Delegates

#### A. Column View Delegate (`src/ui/ColumnItemDelegate.h`)

```git
<<<<<<< SEARCH
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
=======
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
>>>>>>> REPLACE
```

#### B. Tree / List View Delegate (`src/ui/TreeItemDelegate.h`)

```git
<<<<<<< SEARCH
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
=======
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
>>>>>>> REPLACE
```

#### C. Grid / Justified View Delegate (`src/ui/ThumbnailDelegate.h`)

```git
<<<<<<< SEARCH
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
=======
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        auto* edit = new FileNameLineEdit(parent);
        edit->setIsFolder(isDir);
        return edit;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **Verify Implementation Plan File Creation**:
   Ensure `QuarkMeta Architecture/Implementation Plan/InlineRenameF2Integration.md` exists and follows the repository's 4-section standard.
2. **Architecture Compliance Check**:
   Confirm all view modes delegate `createEditor` to `FileNameLineEdit` without creating standalone or custom QLineEdit implementations.
