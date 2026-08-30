# TreeItemDelegate-1 Implementation Plan

## 1. Overview
This implementation plan unifies the empty folder visual indicator across all view modes.
Even when grid cards are hidden or when using the list view (`DropTreeView`), empty folders will render a distinct cyan/blue dashed border (`#41F2F2` Qt::DashLine) around their 1:1 implicit square icon grid (`squareRect`).

### Behavior:
1. In list view (`TreeItemDelegate`), if an item is a folder (`TypeRole == "folder"`) and is empty (`IsEmptyRole == true`), a cyan dashed rounded rectangle (`#41F2F2`) is painted over its 1:1 square icon bounding box.
2. Unifies visual empty folder cues seamlessly across both Grid View and List View.

---

## 2. Modified Files List
1. `src/ui/TreeItemDelegate.h`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TreeItemDelegate.h`
Add cyan dashed border rendering (`#41F2F2` Qt::DashLine) over the 1:1 implicit square icon rect for empty folders in column 0.

<<<<<<< SEARCH
            // 3. 文本排版向右偏移
            QString name = index.data(Qt::DisplayRole).toString();
=======
            // 3. 空文件夹绘制青蓝色虚线框 (#41F2F2 Qt::DashLine)
            bool isFolder = (index.data(TypeRole).toString() == "folder");
            bool isEmpty = index.data(IsEmptyRole).toBool();
            if (isFolder && isEmpty) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(squareRect, 4, 4);
                painter->restore();
            }

            // 4. 文本排版向右偏移
            QString name = index.data(Qt::DisplayRole).toString();
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Configure and build:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Switch to List View (`ListView`).
3. Locate an empty folder and verify that a cyan dashed border (`#41F2F2`) is drawn around its 1:1 square icon grid in the leftmost column.
