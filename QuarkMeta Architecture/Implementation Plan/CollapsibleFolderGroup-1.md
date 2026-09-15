# Implementation Plan - CollapsibleFolderGroup-1.md

## 1. Overview
In QuarkMeta, separating folders from regular files through hardcoded proxy sorting causes conflicts with sorting types (e.g., sort by size, modification date, or rating) and pin behavior.

This plan introduces a unified **Collapsible Folder Group (`子文件夹 (X) ▼`)** across all four view modes (`JustifiedView`, `GridView`, `DropTreeView` / `m_treeView`, and `ColumnViewWidget`).

### Key Features:
1. **Collapsible Folder Group Header**: A clickable group banner displaying `子文件夹 (X) ▼` (or `子文件夹 (X) ▶` when collapsed) placed right above the subfolders section.
2. **Independent Section Collapse**: Clicking the header folds/unfolds only the subfolders. The regular files section remains visible and unaffected below.
3. **Decoupled Sorting**: Sorting and pin features operate purely on data items without interfering with section boundaries.

## 2. Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.h` & `src/ui/JustifiedView.cpp` (GridView & JustifiedView)
Add `m_foldersCollapsed` state and `m_folderHeaderRect` for hit-testing in `JustifiedView`.

```
<<<<<<< SEARCH
    int m_totalHeight = 0;
    int m_anchorRow = -1;
=======
    int m_totalHeight = 0;
    int m_anchorRow = -1;
    bool m_foldersCollapsed = false;
    QRect m_folderHeaderRect;
>>>>>>> REPLACE
```

In `JustifiedView::doLayout()`: Reserve space at `currentY` for `m_folderHeaderRect` if folders exist (`folderCount > 0`). If `m_foldersCollapsed == true`, skip geometry assignment for folder rows.

In `JustifiedView::mousePressEvent()`: Check if click falls within `m_folderHeaderRect`. If so, toggle `m_foldersCollapsed = !m_foldersCollapsed;` and call `doLayout()`.

In `JustifiedView::paintEvent()`: Render the folder group header banner (`子文件夹 (X) ▼` / `▶`) inside `m_folderHeaderRect`.

### 2. `src/ui/DropTreeView.h` & `src/ui/DropTreeView.cpp` (ListView)
In `DropTreeView`, implement custom painting and click handling for row 0 if folders exist, or integrate a group header delegate that renders `子文件夹 (X) ▼` above folder items, allowing collapse/expand without modifying proxy sorting logic.

### 3. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp` (ColumnView)
In `ColumnViewPane`, add `m_foldersCollapsed` state and render a collapsible header banner `子文件夹 (X) ▼` at the top of each pane above folder items.

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run `cmake --build build` to verify clean compilation.
2. **Behavioral Verification**:
   - Open QuarkMeta in GridView / JustifiedView: Confirm `子文件夹 (X) ▼` header appears above subfolders. Click header to verify subfolders collapse while files remain visible.
   - Switch to ListView: Confirm `子文件夹 (X) ▼` collapsible header operates identically.
   - Switch to ColumnView: Confirm `子文件夹 (X) ▼` operates per column pane.
   - Change sorting or pin items: Verify sorting applies cleanly to files and folders without layout breaking.
