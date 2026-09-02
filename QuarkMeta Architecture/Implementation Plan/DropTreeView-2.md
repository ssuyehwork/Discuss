# Implementation Plan - DropTreeView-2

This implementation plan decouples list view column index magic numbers, introduces standard `FileListColumn` column contracts, establishes a static `ColumnPolicy` descriptor table (Single Source of Truth), and encapsulates responsive column visibility and header resizing within `DropTreeView` / `ContentHeaderView`.

## 1. Overview & Architecture Decision
- **Single Source of Truth (SSOT)**: Define `FileListColumn` enum in `src/core/ModelContract.h` and static `kFileListColumnPolicies` descriptor table in `src/ui/DropTreeView.h` / `src/ui/DropTreeView.cpp`.
- **Encapsulation Integrity**: Move list column width initialization and responsive viewport width auto-hide/show rules into `DropTreeView`. Remove hardcoded section resizing loops and column magic numbers (`0, 1, 2, 3, 4, 5, 6`) from `ContentPanel.cpp` and `initListView()`.
- **Public API Contract**: Maintain `DiskItemModel::columnCount()` bound to `FileListColumn::Count`. Update `TreeItemDelegate` and `ContentKeyHandler` to reference `FileListColumn::Name` explicitly instead of column `0`.

## 2. Modified Files List
- `src/core/ModelContract.h`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentKeyHandler.cpp`

## 3. Detailed Changes Plan

### 1. Define `FileListColumn` Enum in `src/core/ModelContract.h`
```cpp
enum class FileListColumn : int {
    Name = 0,        // 名称 (微卡片 + 文本)
    Status = 1,      // 状态 (固定 40px，默认常态隐藏)
    Rating = 2,      // 评分 (固定 100px)
    Dimension = 3,   // 尺寸 (固定 100px)
    Type = 4,        // 类型 (固定 60px)
    Size = 5,        // 大小 (固定 80px)
    ModifiedDate = 6,// 修改日期 (固定 130px)
    Count = 7
};
```

### 2. Define `ColumnPolicy` and Column Descriptor Table in `DropTreeView.h/cpp`
```cpp
struct ColumnPolicy {
    FileListColumn column;
    int fixedWidth;                     // 固定宽度（Stretch 列为 0）
    QHeaderView::ResizeMode resizeMode; // Stretch 或 Fixed
    int minContainerWidth;              // 容器达到多少宽度时才激活展示 (0 表示始终保留)
    bool alwaysHidden;                  // 是否常态隐藏 (如 Status 列)
};
```
Inside `DropTreeView::resizeEvent(QResizeEvent* event)`:
- Query `viewport()->width()`.
- Iterate through `kFileListColumnPolicies`.
- Call `setColumnHidden(colIdx, isHidden)` and `header()->setSectionResizeMode(colIdx, mode)`.

### 3. Simplify `ContentPanel::initListView()`
- Remove hardcoded `header->resizeSection(0, ...)` loops.
- Delegate list column layout policies entirely to `DropTreeView::applyColumnPolicies()`.

### 4. Update References in `ContentKeyHandler.cpp` and `DiskItemModel.cpp`
- Replace column `0` with `static_cast<int>(FileListColumn::Name)`.
- Replace `columnCount()` returns in `DiskItemModel` with `static_cast<int>(FileListColumn::Count)`.

## 4. Verification Routine
1. Verify compilation and symbol definitions.
2. Verify list view mode in `ContentPanel` resizes smoothly, automatically showing/hiding Rating, Dimension, Type, Size, and ModifiedDate columns as the panel splitter width changes.
