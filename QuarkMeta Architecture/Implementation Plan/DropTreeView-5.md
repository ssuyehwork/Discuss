# DropTreeView Implementation Plan - Fixed Column Widths & Dynamic Horizontal Scroll Bar

This implementation plan refactors the column sizing and horizontal scrolling behavior of `DropTreeView`:
- Removes `minContainerWidth` threshold filtering so all columns (except `Status` which is always hidden) remain visible regardless of pane width.
- Sets `Name` column width = `std::max(ContentPanel::kMinPaneWidth, viewportWidth - totalVisibleFixedWidth)`.
- Replaces `ScrollBarAlwaysOff` with `ScrollBarAsNeeded` and `ScrollPerPixel`.
- Unifies `setMinimumSectionSize(0)` exclusively in `DropTreeView::applyColumnPolicies()`.

---

## 1. Overview
Previously, `DropTreeView` dynamically hid columns as the container width decreased based on `minContainerWidth` thresholds, and disabled the horizontal scroll bar with `setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff)`.

To ensure consistent column availability and smooth scrolling in narrow split panes:
1. Column policies are simplified: all non-Status columns are kept visible (`setColumnHidden(col, false)`).
2. The `Name` column uses `QHeaderView::Fixed` resize mode and its width is calculated as:
   `max(ContentPanel::kMinPaneWidth, viewportWidth - totalFixedWidths)`
   - When pane is wide (>= ~830px): Name column fills remaining space, total width equals viewport width, no scroll bar.
   - When pane is narrow (< ~830px): Name column stops at `ContentPanel::kMinPaneWidth` (230px), total width exceeds viewport, triggering `Qt::ScrollBarAsNeeded`.
3. Horizontal scrollbar policy is set to `Qt::ScrollBarAsNeeded` with `ScrollPerPixel`.
4. Header section minimum size `setMinimumSectionSize(0)` is single-sourced in `applyColumnPolicies()`, removing duplicate calls in `ContentPanel::initListView()`.
5. Reentrancy guard `m_isApplyingPolicies` prevents recursive `resizeSection` layout events.

---

## 2. Modified Files List
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DropTreeView.h`

```
<<<<<<< SEARCH
struct ColumnPolicy {
    FileListColumn column;
    int fixedWidth;                     // 固定宽度（Stretch 列为 0）
    QHeaderView::ResizeMode resizeMode; // Stretch 或 Fixed
    int minContainerWidth;              // 容器达到多少宽度时才激活展示 (0 表示始终保留)
    bool alwaysHidden;                  // 是否常态隐藏 (如 Status 列)
};
=======
struct ColumnPolicy {
    FileListColumn column;
    int fixedWidth;                     // 固定宽度（Name 列为 0）
    QHeaderView::ResizeMode resizeMode; // Fixed
    bool alwaysHidden;                  // 是否常态隐藏 (如 Status 列)
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
    int m_bottomMargin = 0;
=======
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
    int m_bottomMargin = 0;
    bool m_isApplyingPolicies = false;
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
static const std::vector<ColumnPolicy> kFileListColumnPolicies = {
    { FileListColumn::Name,         0,   QHeaderView::Stretch, 0,   false }, // 始终显示并拉伸 (强保最小 230px)
    { FileListColumn::Status,       40,  QHeaderView::Fixed,   0,   true  }, // 恒定隐藏
    { FileListColumn::Rating,       100, QHeaderView::Fixed,   0,   false }, // 按空间动态计算 (>= 230 + 100)
    { FileListColumn::Dimension,    100, QHeaderView::Fixed,   0,   false }, // 按空间动态计算
    { FileListColumn::Type,         60,  QHeaderView::Fixed,   0,   false }, // 按空间动态计算
    { FileListColumn::Size,         80,  QHeaderView::Fixed,   0,   false }, // 按空间动态计算
    { FileListColumn::ModifiedDate, 130, QHeaderView::Fixed,   0,   false }, // 按空间动态计算
    { FileListColumn::CreatedDate,  130, QHeaderView::Fixed,   0,   false }, // 按空间动态计算
};
=======
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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
=======
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
void DropTreeView::applyColumnPolicies() {
    if (m_isApplyingPolicies) return;
    m_isApplyingPolicies = true;

    QHeaderView* hdr = header();
    if (!hdr) {
        m_isApplyingPolicies = false;
        return;
    }

    hdr->setMinimumSectionSize(0);

    int totalVisibleFixedWidth = 0;
    for (const auto& policy : kFileListColumnPolicies) {
        if (policy.alwaysHidden) continue;
        if (policy.column != FileListColumn::Name) {
            totalVisibleFixedWidth += policy.fixedWidth;
        }
    }

    int availWidth = viewport() ? viewport()->width() : width();
    int minPaneW = ContentPanel::kMinPaneWidth;
    int calculatedNameWidth = std::max(minPaneW, availWidth - totalVisibleFixedWidth);

    for (const auto& policy : kFileListColumnPolicies) {
        int colIdx = static_cast<int>(policy.column);
        if (policy.alwaysHidden) {
            setColumnHidden(colIdx, true);
            continue;
        }

        setColumnHidden(colIdx, false);
        hdr->setSectionResizeMode(colIdx, QHeaderView::Fixed);

        if (policy.column == FileListColumn::Name) {
            hdr->resizeSection(colIdx, calculatedNameWidth);
        } else {
            hdr->resizeSection(colIdx, policy.fixedWidth);
        }
    }

    m_isApplyingPolicies = false;
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    tree->header()->setMinimumSectionSize(0);
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Since Qt6 dependencies are not available in the headless sandbox, compilation is skipped.
Functional verification checklist for native build:
1. Launch QuarkMeta in List View.
2. At normal window widths (>= 830px pane width), verify no horizontal scroll bar appears, and Name column stretches to fill all available space up to the right edge.
3. Split view into 4 panes (~270px width per pane).
4. Verify all columns (Name, Rating, Dimension, Type, Size, Modified Date, Created Date) remain visible with fixed widths, Name column stays at 230px, and horizontal scrollbar appears.
5. Scroll horizontally using mouse wheel (`Shift` + scroll) or dragging scrollbar. Verify header text and column divider lines stay 100% synchronized with item rows.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Column Policies SSOT**: `kFileListColumnPolicies` in `DropTreeView.cpp` remains the single source of truth for column widths and visibility rules.
- **Minimum Pane Width SSOT**: `ContentPanel::kMinPaneWidth` (230px) is referenced directly without hardcoded `230` magic numbers.
- **Header Section Size SSOT**: `hdr->setMinimumSectionSize(0)` is single-sourced in `applyColumnPolicies()`, eliminating redundant calls in `ContentPanel.cpp`.

---

## 6. Header API Signature Verification
- `DropTreeView::applyColumnPolicies()` -> Existing public member function in `DropTreeView.h`.
- `ContentPanel::kMinPaneWidth` -> Existing static constant in `ContentPanel.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `DropTreeView.cpp` includes `ContentPanel.h` which defines `ContentPanel::kMinPaneWidth`.
- `FileListColumn` is defined in `ModelContract.h` which is included in `DropTreeView.h`.
- All types (`DropTreeView`, `QHeaderView`, `FileListColumn`, `ContentPanel`) have complete type definitions.
