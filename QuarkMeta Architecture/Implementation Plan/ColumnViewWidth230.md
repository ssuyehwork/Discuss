# ColumnViewWidth230.md Implementation Plan

## Overview
本实施方案旨在将列视图 (Column View) 每一列 (`ColumnViewPane`) 的物理列宽统一归一化调整为 **`230px`**，彻底消除与系统全应用各大栏区 (FavoritePanel, NavPanel, ContentPanel, FilterPanel, MetaPanel) `230px` 物理基准的不一致脱节问题。

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```diff
<<<<<<< SEARCH
    setFixedWidth(240);
=======
    setFixedWidth(230);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译验证
使用 CMake 构建应用程序，确保编译零 Warning / Error：
```bash
cmake --build build --config Release
```

### 2. 界面与对齐验证
1. 切换至列视图 (Column View) 模式。
2. 检查每一列的物理宽度，确认精准呈现为 **230px**，与系统全局各主面板的 230px 物理下限完全一致对齐。
