# Implementation Plan - Protecting Multi-Selection on Right Click in `JustifiedView` (`JustifiedView-RightClickSelectionFix.md`)

## 1. Overview
In `JustifiedView::mousePressEvent`, when a user multi-selects items via rubberband drag or key shortcuts, clicking the right mouse button on an already selected item passed through to `QAbstractItemView::mousePressEvent`, causing Qt's default selection model to clear all other selections and degrade the selection state down to a single item.

This implementation plan adds explicit right-click handling in `JustifiedView::mousePressEvent` guarded by `!selectionModel()->isSelected(idx)`:
- If the right-clicked item is **not** currently selected, the selection resets to single-select that item.
- If the right-clicked item is **already** part of the active multi-selection, the selection remains untouched, preserving the entire selection for context menu batch operations.

---

## 2. Modified Files List
- `src/ui/JustifiedView.cpp` (Add right-click selection guard in `mousePressEvent`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/JustifiedView.cpp`
```
<<<<<<< SEARCH
void JustifiedView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
=======
void JustifiedView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        QModelIndex hitIdx = indexAt(event->pos());
        if (hitIdx.isValid()) {
            if (selectionModel() && !selectionModel()->isSelected(hitIdx)) {
                selectionModel()->select(hitIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                selectionModel()->setCurrentIndex(hitIdx, QItemSelectionModel::NoUpdate);
            }
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure CMake:
   ```bash
   cmake -B build -S .
   ```
2. Build executable:
   ```bash
   cmake --build build --config Debug
   ```
3. Test rubberband dragging multiple cards in Grid/Justified view.
4. Right click on any selected card.
5. Verify that all selected cards remain selected and the context menu operates on all selected items.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Selection Preservation Guard**: Uses `selectionModel()->isSelected(hitIdx)` directly without redundant local state.
