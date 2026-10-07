# ModelContract Implementation Plan - Column Order Refactoring (CreatedDate before ModifiedDate)

This implementation plan adjusts the standard list view column order so that "创建日期" (CreatedDate) is placed before "修改日期" (ModifiedDate):
Target Column Order: Name → Status (Hidden) → Rating → Dimension → Type → Size → CreatedDate → ModifiedDate.

---

## 1. Overview
The list view column order contract needs to be updated across the entire application:
1. In `src/core/ModelContract.h`, the enum values of `FileListColumn::CreatedDate` and `FileListColumn::ModifiedDate` are swapped (`CreatedDate = 6`, `ModifiedDate = 7`).
2. In `src/ui/DropTreeView.cpp`, `kFileListColumnPolicies` column order is swapped so `CreatedDate` comes before `ModifiedDate`.
3. In `src/ui/models/DiskItemModel.cpp`, column checking and formatting in `headerData` and `data` strictly use `FileListColumn` enum switches. `CreatedDate` reads `record.ctime` and `ModifiedDate` reads `record.mtime`. All numeric column check literals (`index.column() == 0`) are replaced with `FileListColumn::Name`.
4. In `src/ui/PanelMediator.cpp`, `updateMetaPanelFromPanel` uses `FileListColumn::Name`, `Type`, `Size`, and `ModifiedDate` enum values instead of hardcoded numbers `0, 4, 5, 6`. Since `ModifiedDate` is now column `7`, passing `FileListColumn::ModifiedDate` guarantees that the Meta Panel metadata continues to correctly point to the modification date.
5. All numeric column literals across `TreeItemDelegate.h`, `ContentContextMenu.cpp`, `ContentViewCoordinator.cpp`, `ContentKeyHandler.cpp`, `ContentFileOpsHandler.cpp`, `ViewDragDropHelper.cpp`, `ClipboardService.cpp`, `ColumnViewWidget.cpp`, and `ContentPanel.cpp` are replaced with `FileListColumn` enum members.

---

## 2. Modified Files List
- `src/core/ModelContract.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/controllers/ContentViewCoordinator.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`
- `src/ui/ViewDragDropHelper.cpp`
- `src/core/ClipboardService.cpp`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/core/ModelContract.h`

```
<<<<<<< SEARCH
enum class FileListColumn : int {
    Name = 0,        // 名称 (微卡片 + 文本)
    Status = 1,      // 状态 (固定 40px，默认常态隐藏)
    Rating = 2,      // 评分 (固定 100px)
    Dimension = 3,   // 尺寸 (固定 100px)
    Type = 4,        // 类型 (固定 60px)
    Size = 5,        // 大小 (固定 80px)
    ModifiedDate = 6,// 修改日期 (固定 130px)
    CreatedDate = 7, // 创建日期 (固定 130px)
    Count = 8
};
=======
enum class FileListColumn : int {
    Name = 0,        // 名称 (微卡片 + 文本)
    Status = 1,      // 状态 (固定 40px，默认常态隐藏)
    Rating = 2,      // 评分 (固定 100px)
    Dimension = 3,   // 尺寸 (固定 100px)
    Type = 4,        // 类型 (固定 60px)
    Size = 5,        // 大小 (固定 80px)
    CreatedDate = 6, // 创建日期 (固定 130px)
    ModifiedDate = 7,// 修改日期 (固定 130px)
    Count = 8
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Fixed, false },
    { FileListColumn::Status,       40,  QHeaderView::Fixed, true  },
    { FileListColumn::Rating,       100, QHeaderView::Fixed, false },
    { FileListColumn::Dimension,    100, QHeaderView::Fixed, false },
    { FileListColumn::Type,         60,  QHeaderView::Fixed, false },
    { FileListColumn::Size,         80,  QHeaderView::Fixed, false },
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed, false },
    { FileListColumn::CreatedDate,  130, QHeaderView::Fixed, false },
};
=======
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Fixed, false },
    { FileListColumn::Status,       40,  QHeaderView::Fixed, true  },
    { FileListColumn::Rating,       100, QHeaderView::Fixed, false },
    { FileListColumn::Dimension,    100, QHeaderView::Fixed, false },
    { FileListColumn::Type,         60,  QHeaderView::Fixed, false },
    { FileListColumn::Size,         80,  QHeaderView::Fixed, false },
    { FileListColumn::CreatedDate,  130, QHeaderView::Fixed, false },
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed, false },
};
>>>>>>> REPLACE
```

### 3.3 `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
QVariant DiskItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<FileListColumn>(section)) {
            case FileListColumn::Name: return QString("名称");
            case FileListColumn::Status: return QString("状态");
            case FileListColumn::Rating: return QString("评分");
            case FileListColumn::Dimension: return QString("尺寸");
            case FileListColumn::Type: return QString("类型");
            case FileListColumn::Size: return QString("大小");
            case FileListColumn::ModifiedDate: return QString("修改日期");
            case FileListColumn::CreatedDate: return QString("创建日期");
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
=======
QVariant DiskItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<FileListColumn>(section)) {
            case FileListColumn::Name: return QString("名称");
            case FileListColumn::Status: return QString("状态");
            case FileListColumn::Rating: return QString("评分");
            case FileListColumn::Dimension: return QString("尺寸");
            case FileListColumn::Type: return QString("类型");
            case FileListColumn::Size: return QString("大小");
            case FileListColumn::CreatedDate: return QString("创建日期");
            case FileListColumn::ModifiedDate: return QString("修改日期");
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}
=======
Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == static_cast<int>(FileListColumn::Name)) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            case FileListColumn::ModifiedDate: {
                return formatDateTime(record.mtime);
            }
            case FileListColumn::CreatedDate: {
                return formatDateTime(record.ctime);
            }
=======
            case FileListColumn::CreatedDate: {
                return formatDateTime(record.ctime);
            }
            case FileListColumn::ModifiedDate: {
                return formatDateTime(record.mtime);
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else if (role == Qt::DecorationRole && index.column() == 0) {
=======
    } else if (role == Qt::DecorationRole && index.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

### 3.4 `src/ui/PanelMediator.cpp`

```
<<<<<<< SEARCH
            QString name = idx.isValid() ? idx.sibling(idx.row(), 0).data(Qt::DisplayRole).toString() : fi.fileName();
            QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), 4).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
            QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), 5).data(Qt::DisplayRole).toString() : "-";
            QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), 6).data(Qt::DisplayRole).toString() : "-";
=======
            QString name = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name)).data(Qt::DisplayRole).toString() : fi.fileName();
            QString type = idx.isValid() ? ((idx.data(TypeRole).toString() == "folder") ? "文件夹" : idx.sibling(idx.row(), static_cast<int>(FileListColumn::Type)).data(Qt::DisplayRole).toString() + " 文件") : (fi.isDir() ? "文件夹" : fi.suffix().toUpper() + " 文件");
            QString sizeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::Size)).data(Qt::DisplayRole).toString() : "-";
            QString mtimeStr = idx.isValid() ? idx.sibling(idx.row(), static_cast<int>(FileListColumn::ModifiedDate)).data(Qt::DisplayRole).toString() : "-";
>>>>>>> REPLACE
```

### 3.5 `src/ui/TreeItemDelegate.h`

```
<<<<<<< SEARCH
        // 2026-06-16 按照 8 列架构重构：第 1, 2, 3 列由代理独立绘制；第 0 列作为名称列，具有微型圆角卡片预览（最左侧看片）
        int col = index.column();
        if (col == 0 && m_drawMiniCards) {
=======
        // 按照 8 列架构重构：状态列、评分列由代理独立绘制；Name 列作为名称列，具有微型圆角卡片预览
        int col = index.column();
        if (col == static_cast<int>(FileListColumn::Name) && m_drawMiniCards) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        } else if (col == 1 || col == 2) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            QModelIndex idx0 = index.model()->index(index.row(), 0);

            if (col == 1) { // 🚨 物理修复 ①：状态列图标在单元格内部 100% 水平+垂直绝对居中！
                bool isPinned = idx0.data(IsLockedRole).toBool();

                int iconSize = 16;
                // 计算单元格物理中心坐标
                QRect centeredRect(option.rect.left() + (option.rect.width() - iconSize) / 2,
                                   option.rect.top() + (option.rect.height() - iconSize) / 2,
                                   iconSize, iconSize);

                if (isPinned) {
                    UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                }
            } else if (col == 2) { // 星级列
=======
        } else if (col == static_cast<int>(FileListColumn::Status) || col == static_cast<int>(FileListColumn::Rating)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            QModelIndex idx0 = index.model()->index(index.row(), static_cast<int>(FileListColumn::Name));

            if (col == static_cast<int>(FileListColumn::Status)) { // 状态列图标在单元格内部 100% 水平+垂直绝对居中
                bool isPinned = idx0.data(IsLockedRole).toBool();

                int iconSize = 16;
                QRect centeredRect(option.rect.left() + (option.rect.width() - iconSize) / 2,
                                   option.rect.top() + (option.rect.height() - iconSize) / 2,
                                   iconSize, iconSize);

                if (isPinned) {
                    UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                }
            } else if (col == static_cast<int>(FileListColumn::Rating)) { // 星级列
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() == 0 && m_drawMiniCards) {
=======
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() == static_cast<int>(FileListColumn::Name) && m_drawMiniCards) {
>>>>>>> REPLACE
```

### 3.6 `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
    QModelIndex col0Index = onItem ? currentIndex.sibling(currentIndex.row(), 0) : QModelIndex();
=======
    QModelIndex col0Index = onItem ? currentIndex.sibling(currentIndex.row(), static_cast<int>(FileListColumn::Name)) : QModelIndex();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                    if (idx.column() == 0) {
=======
                    if (idx.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

### 3.7 `src/ui/controllers/ContentViewCoordinator.cpp`

```
<<<<<<< SEARCH
                    if (idx.column() == 0) res.append(idx);
=======
                    if (idx.column() == static_cast<int>(FileListColumn::Name)) res.append(idx);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                if (idx.column() == 0) {
=======
                if (idx.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (idx.column() == 0) {
=======
        if (idx.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

### 3.8 `src/ui/controllers/ContentKeyHandler.cpp`

```
<<<<<<< SEARCH
        if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
=======
        if (idx.column() == static_cast<int>(FileListColumn::Name) && !idx.data(SectionHeaderRole).toBool()) {
>>>>>>> REPLACE
```

### 3.9 `src/ui/controllers/ContentFileOpsHandler.cpp`

```
<<<<<<< SEARCH
        if (idx.column() == 0) {
=======
        if (idx.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

### 3.10 `src/ui/ViewDragDropHelper.cpp`

```
<<<<<<< SEARCH
        if (idx.column() != 0) continue;
=======
        if (idx.column() != static_cast<int>(FileListColumn::Name)) continue;
>>>>>>> REPLACE
```

### 3.11 `src/core/ClipboardService.cpp`

```
<<<<<<< SEARCH
#include "ClipboardService.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
=======
#include "ClipboardService.h"
#include "ModelContract.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (targetIdx.column() == 0 && !targetIdx.data(SectionHeaderRole).toBool()) {
=======
        if (targetIdx.column() == static_cast<int>(FileListColumn::Name) && !targetIdx.data(SectionHeaderRole).toBool()) {
>>>>>>> REPLACE
```

### 3.12 `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
=======
            if (idx.column() == static_cast<int>(FileListColumn::Name) && !idx.data(SectionHeaderRole).toBool()) {
>>>>>>> REPLACE
```

### 3.13 `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
        if (idx.column() == 0) {
=======
        if (idx.column() == static_cast<int>(FileListColumn::Name)) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Skipped since Qt6 environment is unavailable in headless sandbox.
Verification checklist on native build:
1. Open List View and inspect column headers: Order must be Name → Rating → Dimension → Type → Size → Created Date → Modified Date.
2. Verify "创建日期" displays creation time (`ctime`) and "修改日期" displays modification time (`mtime`).
3. Click "创建日期" and "修改日期" header sections: Verify sorting arrow appears on the clicked section and sorts by creation time / modification time respectively.
4. Select a single file and inspect Meta Panel: Verify modification time displays the correct modification date.
5. Verify column view, grid view, justified view, and split panes operate normally without issues.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Enum SSOT**: `FileListColumn` is the single source of truth for column indices across the entire application.
- **Sort Mapping SSOT**: `ContentSortController::columnForSortType` and `sortTypeForColumn` map sort types to `FileListColumn` enum members.

---

## 6. Header API Signature Verification
- `FileListColumn::CreatedDate` -> `ModelContract.h`
- `FileListColumn::ModifiedDate` -> `ModelContract.h`
- `ContentSortController::columnForSortType(SortType)` -> `ContentSortController.h`
- `ContentSortController::sortTypeForColumn(FileListColumn)` -> `ContentSortController.h`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "ModelContract.h"` added to `ClipboardService.cpp`.
- All affected `.cpp` files include `ModelContract.h` either directly or via `DiskItemModel.h` / `ContentPanel.h`.
