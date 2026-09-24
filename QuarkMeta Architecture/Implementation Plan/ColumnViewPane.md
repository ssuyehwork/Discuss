# Implementation Plan - ColumnViewPane Clean Architecture Refactoring & Parameter Restoration

## 1. Overview
This implementation plan restores the exact original design philosophy, parameters, and behaviors of Column View (`ColumnViewPane` & `ColumnViewWidget`) from `Version-Old-6` and `Memories.md`. It aligns the implementation with QuarkMeta Clean Architecture and the Anti-Patch 5 Engineering Locks.

### Architecture 3-Question Answers
1. **SSOT Source**: The authoritative single source of truth for filesystem directory records is `DiskScanService` / `DiskItemModel` (Domain & Model layer), and metadata properties are held by `MetadataManager`. The UI (`ColumnViewPane`) is strictly a view subscriber.
2. **Black Box Integrity**: No friends or private state exposures are introduced. `ColumnViewPane` exposes public Qt slots and signals for communication, decoupling direct `ContentPanel` parent references.
3. **Root Cause**: The Column View parameters (such as row height 28px, min pane width 230px, selection color `#378ADD`, parent expanded highlight `#334455`, and thumbnail visibility calculation) strictly align with the `Version-Old-6` master specification.

---

## 2. Modified Files List
- `Memories.md`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `Memories.md` Specification Updates
```
<<<<<<< SEARCH
14. **分栏视图快捷键归一化与激活列路由契约 (Column View Keyboard Shortcuts Routing Contract)**：
   - 在分栏视图模式下，`Ctrl+V` (粘贴) 与 `Ctrl+Shift+N` / 新建命令必须动态感知并路由至当前激活列的路径 `activePane()->currentPath()`，禁止误粘贴或新建在最左侧根目录；
   - `Backspace` (退格键) 与 `Left` (左方向键) 必须拦截并路由至 `columnView()->goUpColumn()`，平滑裁撤收起最右侧子列，禁止触发全局顶层路径跳转。
=======
14. **分栏视图快捷键归一化与激活列路由契约 (Column View Keyboard Shortcuts Routing Contract)**：
   - 在分栏视图模式下，`Ctrl+V` (粘贴) 与 `Ctrl+Shift+N` / 新建命令必须动态感知并路由至当前激活列的路径 `activePane()->currentPath()`，禁止误粘贴或新建在最左侧根目录；
   - `Backspace` (退格键) 与 `Left` (左方向键) 必须拦截并路由至 `columnView()->goUpColumn()`，平滑裁撤收起最右侧子列，禁止触发全局顶层路径跳转。

15. **Version-Old-6 经典列视图顶层设计理念与架构契约汇总 (Version-Old-6 Column View Master Specification)**：
   - **单层 HBox 级联展开与祖先路径栈 (Ancestor Path Stack)**：所有列面板 (`ColumnViewPane`) 挂载于单层 `QHBoxLayout` 中。传入深层路径时向上拆分完整的祖先路径栈，从根目录开始逐级构建多列分栏，并自动定位高亮选中子目录；展开子列后父列对应项通过 `IsParentExpandedRole` 保持深青灰高亮 (`#334455`)；
   - **文件夹/文件物理双分栏与折叠 (Dual-Section Panel)**：每个列面板内部由 `DualSectionPanel` 承载，拆分为上方文件夹专用列表（`m_folderProxyModel`）与下方文件专用列表（`m_fileProxyModel`），配合折叠标头 (`FolderSectionHeaderBar`) 根据项目数自适应动态计算列表高度；
   - **智能等比例宽度均分与延伸留白画布 (Smart Width Allocation & Blank Canvas)**：各列设定 230px 刚性最小宽度，视口充足时按 `viewportWidth / count` 自动等均分，最后一列吸收余数像素；最右侧剥离独立的延伸留白区 (`ColumnBlankCanvasWidget`)，支持拖拽放下与双击背景回退；
   - **严格的单击/双击语义分工**：单击文件夹/文件仅选中高亮并切换焦点，不裁撤右侧子列；双击文件夹触发 `folderExpandRequested`，裁撤当前列右侧所有子列并在右侧展开新子列；双击文件触发激活/打开；
   - **双击空白处精准回退降级**：最右侧列或留白区双击逐级关闭最右侧列；中间父列空白处双击裁撤该列右侧所有子列；仅剩最后一列时双击空白处自动降级退回 `computer://`；
   - **极简单行 Delegate 绘制 (28px Row Height)**：每行固定 28px 高度，包含左侧 8px 留白、5px 垂直色条（若有）、18x18px 图标/缩略图、文本区（`ElideRight`）、右侧星级评分及文件夹最右侧 `chevron_right` 级联指示箭头（空文件夹绘制虚线框及 `#41F2F2` 箭羽）。
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake --build build --config Release
```

### Verification Steps
1. **Physical Parameter Verification**:
   - Verify Column View item height is locked to `28px` in `ColumnItemDelegate`.
   - Verify minimum column width is locked to `230px` in `ColumnViewWidget`.
   - Verify selection color is `#378ADD` (18% alpha) and parent expanded item highlight is `#334455`.
2. **Behavioral Verification**:
   - Single-click on a folder/file highlights the item without clearing right-hand sub-columns.
   - Double-click on a folder expands the sub-column and updates the active path.
   - Double-click on empty space in the rightmost column or canvas steps back up one directory level (`goUpColumn`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` SSOT Channel**: Column View refresh operations strictly reuse `ContentPanel::refreshAll()` / `refreshAllColumns()`.
- **`AppConfig` SSOT**: Global settings and state persistence strictly route through `AppConfig::instance()`.
- **Zero Redundant Code**: All dispersed legacy scan implementations route through `DiskScanService` and `DiskItemModel`.

---

## 6. Header API Signature Verification
| Class | Member Function / Signal Signature | Status |
| :--- | :--- | :--- |
| `ColumnViewPane` | `void folderExpandRequested(const QString& folderPath, int paneIndex)` | Verified in `ColumnViewPane.h` |
| `ColumnViewPane` | `void setFilterState(const FilterState& state)` | Verified in `ColumnViewPane.h` |
| `ColumnViewWidget` | `void goUpColumnFromIndex(int paneIndex)` | Verified in `ColumnViewWidget.h` |
| `ColumnViewWidget` | `ColumnViewPane* activePane() const` | Verified in `ColumnViewWidget.h` |
| `DiskScanService` | `static std::vector<ItemRecord> scanDirectory(...)` | Verified in `DiskScanService.h` |
