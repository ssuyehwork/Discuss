# Implementation Plan - CollapsibleFolderGroup-2.md

## 1. Overview
In QuarkMeta, folders and files must be strictly partitioned into two independent physical sections across all view modes (`JustifiedView` / GridView, `DropTreeView` / ListView, and `ColumnViewWidget` / ColumnView):
1. **Top Section (文件夹区)**: Dedicated exclusively to folders in the current directory. Includes a collapsible group header banner `文件夹 (X) ▼` (or `▶` when collapsed). Collapsing this section hides only the folders.
2. **Bottom Section (普通文件区)**: Dedicated exclusively to files (including recursive files when "显示子文件夹中的项目" is active). Rendered continuously below the folder section, remaining visible and unaffected when the folder section is collapsed.
3. **Pin & Sort Isolation**: Pinned items and sort order operate within their respective section boundaries (folders stay in the top section, files stay in the bottom section).

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/models/FilterProxyModel.cpp`
Unconditionally lock folders (`leftRec.isDir`) and pinned items (`leftPinned`) at the top of the proxy model, preventing descending sort order from inverting folder/file section order.

```
<<<<<<< SEARCH
    // 🚀【绝对权重 1：文件夹永远在最上方】：无视升序降序反转，文件夹永远第一顺位
    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) ? leftRec.isDir : !leftRec.isDir;
    }

    // 🚀【绝对权重 2：置顶/加密优先】：无视升序降序反转，置顶项永远置顶
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return (sortOrder() == Qt::AscendingOrder) ? leftPinned : !rightPinned;
    }
=======
    // 🚀【绝对权重 1：文件夹永远在最上方】：无视升序降序反转，文件夹永远第一顺位
    if (leftRec.isDir != rightRec.isDir) {
        return leftRec.isDir;
    }

    // 🚀【绝对权重 2：置顶/加密优先】：无视升序降序反转，置顶项永远置顶
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return leftPinned;
    }
>>>>>>> REPLACE
```

### 2. `src/ui/JustifiedView.h` & `src/ui/JustifiedView.cpp` (GridView)
Add `m_foldersCollapsed` and `m_folderHeaderRect` to `JustifiedView`.

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

In `JustifiedView::doLayout()`:
- Separate model items into `folderIndices` and `fileIndices`.
- If `folderIndices` is non-empty, reserve a 32px height header rect `m_folderHeaderRect` at `currentY` with text `文件夹 (X) ▼` / `▶`.
- If `!m_foldersCollapsed`, layout `folderIndices` into grid rows starting below `m_folderHeaderRect`.
- Layout `fileIndices` into grid rows starting below the folder section.

In `JustifiedView::mousePressEvent()`:
- Check if click falls within `m_folderHeaderRect`.
- Toggle `m_foldersCollapsed = !m_foldersCollapsed;` and call `doLayout()` and `update()`.

In `JustifiedView::paintEvent()`:
- Render the `文件夹 (X) ▼ / ▶` banner when `folderIndices` exist.

### 3. `src/ui/DropTreeView.h` & `src/ui/DropTreeView.cpp` (ListView)
In `DropTreeView`, manage collapsible folder rows:
- Add `m_foldersCollapsed` state and `m_folderHeaderRect`.
- When folders exist in the model (rows 0 to `folderCount - 1`), set row hidden state via `setRowHidden(i, m_foldersCollapsed)`.
- Render banner `文件夹 (X) ▼ / ▶` above row 0 in viewport and handle header click events.

### 4. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp` (ColumnView)
In `ColumnViewPane`:
- Add `m_foldersCollapsed` state and collapsible header rect `m_folderHeaderRect`.
- Partition pane items into folders (top section) and files (bottom section).
- When `m_foldersCollapsed` is true, hide folder items while preserving `文件夹 (X) ▶` header and file list underneath.

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run `cmake --build build` in bash session to ensure clean compilation without warnings or undefined symbols.
2. **Behavioral Verification**:
   - Open QuarkMeta in GridView (`JustifiedView`): Verify subfolders render under `文件夹 (X) ▼` header. Click header to fold/unfold. Verify files remain uncollapsed in the bottom section.
   - Enable "显示子文件夹中的项目": Verify all recursively loaded files flat-list in the bottom section while top section holds folders.
   - Test ListView (`DropTreeView`) & ColumnView (`ColumnViewWidget`): Verify identical two-section layout and collapsing behavior.
