# Implementation Plan - LibraryPanel-5.md

## 1. Overview
This implementation plan adds drag-and-drop target highlight support for category items in `LibraryPanel`.

When dragging files or items over a category node in the "库" (Library) tree view, the target category node should visually highlight (semi-transparent blue background `#3498db` with alpha 0.35), consistent with `ColumnItemDelegate`, `TreeItemDelegate`, and `ThumbnailDelegate`.

In accordance with AGENTS.md rules:
- Source files are NOT directly modified in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LibraryPanel-5.md`.

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`
- `src/ui/ViewDragDropHelper.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/LibraryPanel.cpp`

```diff
<<<<<<< SEARCH
#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "ColorPicker.h"
#include "ShellIconManager.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../core/CoreEngine.h"
=======
#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "PresetTagsDialog.h"
#include "ColorPicker.h"
#include "ShellIconManager.h"
#include "ViewDragDropHelper.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include "../core/CoreEngine.h"
#include "../core/ModelContract.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void LibraryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
=======
void LibraryItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(qobject_cast<const QAbstractItemView*>(option.widget), index);

    if (isDropTarget) {
        QColor dropBg("#3498db");
        dropBg.setAlphaF(0.35f);
        painter->fillRect(opt.rect, dropBg);
    } else if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor("#37373D"));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#2A2D2E"));
    } else {
        painter->fillRect(opt.rect, Qt::transparent);
    }
>>>>>>> REPLACE
```

### 2. `src/ui/ViewDragDropHelper.cpp`

```diff
<<<<<<< SEARCH
        if (m_currentHoverDropIdx != hoverIdx) {
            clearDropHighlight();
            if (hoverIdx.isValid()) {
                bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
                if (isFolder) {
                    m_currentHoverDropIdx = hoverIdx;
                    if (m_targetView->model()) {
                        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                        if (m_targetView->viewport()) m_targetView->viewport()->update();
                    }
                }
            }
        }
=======
        if (m_currentHoverDropIdx != hoverIdx) {
            clearDropHighlight();
            if (hoverIdx.isValid()) {
                bool isTargetable = (hoverIdx.data(TypeRole).toString() == "folder") ||
                                    (hoverIdx.data(TypeRole).toString() == "category") ||
                                    (hoverIdx.data(IdRole).toInt() > 0) ||
                                    hoverIdx.data(Qt::UserRole + 2).toBool();
                if (isTargetable) {
                    m_currentHoverDropIdx = hoverIdx;
                    if (m_targetView->model()) {
                        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                        if (m_targetView->viewport()) m_targetView->viewport()->update();
                    }
                }
            }
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open the "库" tab in the left sidebar:
   - Drag files/items from the content panel or desktop over any category node in the Library tree view.
   - Verify that the target category node under the cursor immediately shows a blue hover highlight (`#3498db` with 0.35 alpha).
   - Drop the items onto the category node and verify that items are successfully added to the category and the highlight is cleared.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Standard `ViewDragDropHelper::isDropTarget` and `IsDropTargetRole` SSOT entries from `ModelContract.h`.
- **Zero Redundancy**: Reuses the exact hover highlight logic and color schema (`#3498db` with 0.35 alpha) used across `ColumnItemDelegate`, `TreeItemDelegate`, and `ThumbnailDelegate`.

## 6. Header API Signature Verification
- `ViewDragDropHelper::isDropTarget` signature in `src/ui/ViewDragDropHelper.h`:
  `static bool isDropTarget(const QAbstractItemView* view, const QModelIndex& index);`
- `LibraryItemDelegate::paint` signature in `src/ui/LibraryPanel.h`:
  `void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;`

## 7. Header Inclusion Chain & Type Completeness Check
- Added `#include "ViewDragDropHelper.h"` and `#include "../core/ModelContract.h"` in `src/ui/LibraryPanel.cpp`.
- Type completeness checked for `ViewDragDropHelper` and `IsDropTargetRole`.
