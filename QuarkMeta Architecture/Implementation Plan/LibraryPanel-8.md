# Implementation Plan - Drag Drop Hover Highlight SSOT Architecture Unification (`LibraryPanel-8.md`)

This plan provides the complete, unified architectural solution for making drag hover target highlights work reliably across all item views (including `LibraryPanel`, `DropTreeView`, `GridView`, `ListView`, and `ColumnView`) without depending on model-specific `IsDropTargetRole` implementations.

---

## 1. Overview & Architectural Principles

### 1.1 Problem Statement
When dragging files over custom categories in `LibraryPanel` (or other tree/list views backed by standard Qt models like `QStandardItemModel`), the drop target node fails to render the semi-transparent blue highlight background (`#3498db`, alpha 0.35).

### 1.2 Root Cause Analysis
1. `DragDropEventFilter` attempted to set `IsDropTargetRole` on `QStandardItemModel` via `setData()`. However, `QStandardItemModel` does not emit model update notifications or manage custom drop state roles natively.
2. Delegate drawing logic in `LibraryItemDelegate::paint` checked both `IsDropTargetRole` and `ViewDragDropHelper::isDropTarget()`.
3. `DragDropEventFilter::eventFilter` managed a separate local `m_currentHoverDropIdx` without updating `ViewDragDropHelper`'s static `s_hoverView` and `s_hoverIndex`, causing `ViewDragDropHelper::isDropTarget()` to return `false`.

### 1.3 Architectural Solution (Single Source of Truth)
1. **Unify SSOT**: Establish `ViewDragDropHelper::isDropTarget(widget, index)` as the **single source of truth** for drag hover drop target determination across the entire application.
2. **Synchronize Event Filter**: In `DragDropEventFilter::eventFilter`, whenever `DragMove` detects a valid targetable node (`nodeId > 0 || nodeId == -2` or `type == "category"` or `type == "folder"`), directly invoke `ViewDragDropHelper::setHoverTarget(m_targetView, hoverIdx)`.
3. **Model Decoupling**: Eliminate model-level `setData(IsDropTargetRole)` calls in `DragDropEventFilter`, decoupling drag target highlighting completely from model internals.

---

## 2. Modified Files List
1. `src/ui/ViewDragDropHelper.h` (Add static method `setHoverTarget`)
2. `src/ui/ViewDragDropHelper.cpp` (Implement `setHoverTarget` and update `DragDropEventFilter::eventFilter`)
3. `src/ui/LibraryPanel.cpp` (Ensure `LibraryItemDelegate::paint` uses `ViewDragDropHelper::isDropTarget(option.widget, index)`)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ViewDragDropHelper.h`
Add `setHoverTarget` static helper method.

```diff
<<<<<<< SEARCH
    static bool isDropTarget(const QWidget* widget, const QModelIndex& index);
    static void clearHover(QAbstractItemView* view = nullptr);
=======
    static bool isDropTarget(const QWidget* widget, const QModelIndex& index);
    static void setHoverTarget(QAbstractItemView* view, const QModelIndex& index);
    static void clearHover(QAbstractItemView* view = nullptr);
>>>>>>> REPLACE
```

### Change 2: `src/ui/ViewDragDropHelper.cpp`
Implement `setHoverTarget` and update `DragDropEventFilter::eventFilter` to synchronize `ViewDragDropHelper` state on `DragMove`.

```diff
<<<<<<< SEARCH
void ViewDragDropHelper::clearHover(QAbstractItemView* view) {
    if (view && s_hoverView != view) return;
    QAbstractItemView* oldView = s_hoverView;
    s_hoverView = nullptr;
    s_hoverIndex = QPersistentModelIndex();
    if (oldView && oldView->viewport()) {
        oldView->viewport()->update();
    }
}
=======
void ViewDragDropHelper::setHoverTarget(QAbstractItemView* view, const QModelIndex& index) {
    if (!view) return;
    if (s_hoverView != view || s_hoverIndex != index) {
        QAbstractItemView* oldView = s_hoverView;
        s_hoverView = view;
        s_hoverIndex = index;
        if (oldView && oldView->viewport()) oldView->viewport()->update();
        if (view->viewport()) view->viewport()->update();
    }
}

void ViewDragDropHelper::clearHover(QAbstractItemView* view) {
    if (view && s_hoverView != view) return;
    QAbstractItemView* oldView = s_hoverView;
    s_hoverView = nullptr;
    s_hoverIndex = QPersistentModelIndex();
    if (oldView && oldView->viewport()) {
        oldView->viewport()->update();
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
        QModelIndex hoverIdx = m_targetView->indexAt(moveEvent->position().toPoint());
        if (m_currentHoverDropIdx != hoverIdx) {
            clearDropHighlight();
            if (hoverIdx.isValid()) {
                int nodeId = hoverIdx.data(Qt::UserRole + 1).toInt();
                bool isTargetable = !hoverIdx.data(SectionHeaderRole).toBool() &&
                                    ((hoverIdx.data(TypeRole).toString() == "folder") ||
                                     (hoverIdx.data(TypeRole).toString() == "category") ||
                                     (nodeId > 0 || nodeId == -2) ||
                                     hoverIdx.data(Qt::UserRole + 2).toBool());
                if (isTargetable) {
                    m_currentHoverDropIdx = hoverIdx;
                    if (m_targetView->model()) {
                        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                        if (m_targetView->viewport()) m_targetView->viewport()->update();
                    }
                }
            }
        }
=======
        QModelIndex hoverIdx = m_targetView->indexAt(moveEvent->position().toPoint());
        if (hoverIdx.isValid()) {
            int nodeId = hoverIdx.data(Qt::UserRole + 1).toInt();
            QString typeStr = hoverIdx.data(TypeRole).toString();
            bool isTargetable = !hoverIdx.data(SectionHeaderRole).toBool() &&
                                (typeStr == "folder" || typeStr == "category" ||
                                 nodeId > 0 || nodeId == -2 ||
                                 hoverIdx.data(Qt::UserRole + 2).toBool());
            if (isTargetable) {
                ViewDragDropHelper::setHoverTarget(m_targetView, hoverIdx);
            } else {
                ViewDragDropHelper::clearHover(m_targetView);
            }
        } else {
            ViewDragDropHelper::clearHover(m_targetView);
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Re-build the application.
2. Open QuarkMeta and navigate to the "库" (Library) panel.
3. Drag a file from the content panel or external Explorer over a custom category or "未分类" category.
4. Confirm that a semi-transparent blue highlight box (`#3498db`, alpha 0.35) immediately and smoothly highlights the target category row.
5. Move the mouse away from the category and verify that the highlight clears instantly without leftover artifacts.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ViewDragDropHelper::isDropTarget` as the single source of truth across all delegates (`LibraryItemDelegate`, `TreeItemDelegate`, `ThumbnailDelegate`, `ColumnItemDelegate`).
- Decoupled model state from drag hover rendering, ensuring clean Clean Architecture layer separation.

---

## 6. Header API Signature Verification
- `ViewDragDropHelper::setHoverTarget(QAbstractItemView* view, const QModelIndex& index)` in `src/ui/ViewDragDropHelper.h`: Exact match.
- `ViewDragDropHelper::isDropTarget(const QWidget* widget, const QModelIndex& index)` in `src/ui/ViewDragDropHelper.h`: Exact match.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ViewDragDropHelper.cpp` includes `ViewDragDropHelper.h`, `ModelContract.h`, `<QAbstractItemView>`, `<QModelIndex>`.
- All required types are fully defined.
