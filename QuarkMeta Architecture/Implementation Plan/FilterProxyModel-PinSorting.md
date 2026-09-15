# Implementation Plan - FilterProxyModel-PinSorting.md

## 1. Overview
In QuarkMeta, pinned items (`record.pinned` or `record.encrypted`) and directories (`record.isDir`) should always stay at the top of the item list, regardless of the active sort order (Ascending or Descending) or sort type (by Name, Date, Size, Rating, etc.).

Currently, in `FilterProxyModel::lessThan`, when the sort order is set to `Qt::DescendingOrder`, the ternary check `(sortOrder() == Qt::AscendingOrder) ? leftPinned : !rightPinned` reverses the weight of pinned items and directories, forcing pinned items and folders to sink to the bottom of the list when sorted descending.

This implementation plan fixes `FilterProxyModel::lessThan` so that pinned items and directories remain permanently anchored at the top regardless of sort direction.

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/models/FilterProxyModel.cpp`
In `FilterProxyModel::lessThan`: Unconditionally return `leftRec.isDir` and `leftPinned` when comparing items with differing directory or pinned statuses.

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

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run `cmake --build build` to verify clean compilation.
2. **Behavioral Verification**:
   - Open QuarkMeta and view a folder containing pinned items or folders alongside regular files.
   - Switch sort order to DescendingOrder (降序).
   - Verify that folders and pinned items remain locked at the top of the list, while their internal ordering and the ordering of regular files below them correctly reverses.
