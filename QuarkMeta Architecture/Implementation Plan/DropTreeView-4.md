# Implementation Plan - Split Pane Column Policy & Horizontal Scrollbar Refactoring

## 1. Overview
This implementation plan resolves the flaw in multi-pane split view column policy and horizontal scrollbar handling:
- **Flaw Fixed**: In 4-pane split view mode, narrow pane widths (~270px) previously triggered a threshold check in `DropTreeView::applyColumnPolicies()` that violently hid all extension columns (`Rating`, `Size`, `Date`, etc.) and left only the `Name` column stretched to fill the viewport. Because total content width equaled the viewport width, Qt's horizontal scrollbar was suppressed.
- **Refactoring**: Sets `setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded)` and `setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel)` across `DropTreeView`, `JustifiedView`, and `ContentPanel`. Refactors `applyColumnPolicies()` so that extension columns remain accessible and `QHeaderView` maintains a minimum 230px width for the `Name` column, allowing Qt's native horizontal scrollbar to appear when total column width exceeds the single pane viewport width.

---

## 2. Modified Files List
- `src/ui/DropTreeView.cpp`
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes in `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    DragDropEventFilter::install(this);
=======
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    DragDropEventFilter::install(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DropTreeView::applyColumnPolicies() {
    QHeaderView* hdr = header();
    if (!hdr) return;

    int currentWidth = viewport() ? viewport()->width() : width();
    const int minNameWidth = 230; // 强保名称列最小 230px 宽，不可更改
    int remainingForFixedColumns = currentWidth - minNameWidth;

    for (const auto& policy : kFileListColumnPolicies) {
        int colIdx = static_cast<int>(policy.column);
        if (policy.alwaysHidden) {
            setColumnHidden(colIdx, true);
            continue;
        }

        if (policy.column == FileListColumn::Name) {
            setColumnHidden(colIdx, false);
            hdr->setSectionResizeMode(colIdx, policy.resizeMode);
            continue;
        }

        // 针对固定宽度的扩展列：仅当剩余空间能够完满容纳该列时才展示，否则隐藏
        if (policy.fixedWidth > 0 && remainingForFixedColumns >= policy.fixedWidth) {
            setColumnHidden(colIdx, false);
            hdr->setSectionResizeMode(colIdx, policy.resizeMode);
            hdr->resizeSection(colIdx, policy.fixedWidth);
            remainingForFixedColumns -= policy.fixedWidth;
        } else {
            setColumnHidden(colIdx, true);
        }
    }
}
=======
void DropTreeView::applyColumnPolicies() {
    QHeaderView* hdr = header();
    if (!hdr) return;

    const int minNameWidth = 230; // 强保名称列最小 230px 宽
    hdr->setMinimumSectionSize(minNameWidth);

    for (const auto& policy : kFileListColumnPolicies) {
        int colIdx = static_cast<int>(policy.column);
        if (policy.alwaysHidden) {
            setColumnHidden(colIdx, true);
            continue;
        }

        setColumnHidden(colIdx, false);
        if (policy.column == FileListColumn::Name) {
            hdr->setSectionResizeMode(colIdx, QHeaderView::Interactive);
            if (hdr->sectionSize(colIdx) < minNameWidth) {
                hdr->resizeSection(colIdx, minNameWidth);
            }
        } else if (policy.fixedWidth > 0) {
            hdr->setSectionResizeMode(colIdx, policy.resizeMode);
            hdr->resizeSection(colIdx, policy.fixedWidth);
        }
    }
}
>>>>>>> REPLACE
```

---

### 3.2 Changes in `src/ui/JustifiedView.cpp`

```
<<<<<<< SEARCH
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setSingleStep(20);
=======
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    verticalScrollBar()->setSingleStep(20);
>>>>>>> REPLACE
```

---

### 3.3 Changes in `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    tree->header()->setMinimumSectionSize(0);
    tree->applyColumnPolicies();
    tree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
=======
    tree->header()->setMinimumSectionSize(230);
    tree->applyColumnPolicies();
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. Verify syntax in `src/ui/DropTreeView.cpp`, `src/ui/JustifiedView.cpp`, and `src/ui/ContentPanel.cpp`.
2. Confirm `CMakeLists.txt` builds `DropTreeView.cpp`, `JustifiedView.cpp`, and `ContentPanel.cpp`.
3. Verify that in 4-pane split view mode, when pane width is ~270px, extension columns remain visible and Qt's horizontal scrollbar appears on the bottom of the pane as needed.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT Reuse**: Uses Qt's native `QAbstractItemView` horizontal scrollbar policies (`Qt::ScrollBarAsNeeded`) and `QHeaderView` section sizing without creating custom scroll wrappers or duplicate layout calculators.
- **Anti-Redundancy**: Eliminates the flawed `remainingForFixedColumns` column hiding logic that violently concealed columns in narrow split panes.

---

## 6. Header API Signature Verification

| Class / Struct | Method / Member Signature | Status |
| :--- | :--- | :--- |
| `DropTreeView` | `void applyColumnPolicies()` | Existing method refactored |
| `DropTreeView` | `void resizeEvent(QResizeEvent* event) override` | Existing method maintained |
| `JustifiedView` | `JustifiedView(QWidget* parent = nullptr)` | Constructor modified |
| `ContentPanel` | `void initListView()` | Existing method modified |

---

## 7. Header Inclusion Chain & Type Completeness Check

- Verified `#include <QScrollBar>` in `src/ui/DropTreeView.cpp`, `src/ui/JustifiedView.cpp`, and `src/ui/ContentPanel.cpp`.
- Verified `#include <QHeaderView>` in `src/ui/DropTreeView.cpp`.
