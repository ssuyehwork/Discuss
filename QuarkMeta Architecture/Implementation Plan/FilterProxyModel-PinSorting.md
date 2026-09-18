# Implementation Plan - FilterProxyModel-PinSorting.md

## 1. Overview
In QuarkMeta, folders (`leftRec.isDir`) and pinned/encrypted items (`leftRec.pinned || leftRec.encrypted`) must **always remain locked at the top of the item list**, regardless of whether the user sets the sort order to Ascending (`Qt::AscendingOrder`) or Descending (`Qt::DescendingOrder`).

### Qt `QSortFilterProxyModel` Sorting Mechanics Analysis:
Qt's `QSortFilterProxyModel` sorting engine evaluates `lessThan(left, right)`. Crucially, when `sortOrder()` is `Qt::DescendingOrder`, Qt's C++ framework automatically inverts the boolean result returned by `lessThan(left, right)` (i.e. `!lessThan(left, right)`) to achieve descending order for standard comparisons.

Because of Qt's automatic inversion:
1. When `sortOrder() == Qt::AscendingOrder`: returning `leftRec.isDir` (true if left is dir) correctly puts directory `left` before file `right`.
2. When `sortOrder() == Qt::DescendingOrder`: Qt calls `lessThan(leftDir, rightFile)` -> if `lessThan` returns `true`, Qt **inverts it to `false`**, causing the directory `leftDir` to be placed **AFTER** `rightFile` (sinking to the bottom)!

To counteract Qt's automatic framework-level inversion on descending order and guarantee that folders and pinned items **never sink to the bottom**, `FilterProxyModel::lessThan` must account for Qt's descending inversion mechanism for the absolute priority checks.

---

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/models/FilterProxyModel.cpp`
In `FilterProxyModel::lessThan`, account for Qt's automatic `DescendingOrder` inversion during the absolute priority checks for directories and pinned items:

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
    // 物理机制：Qt 在 DescendingOrder 时会对 lessThan 返回值取反 (!lessThan)
    // 要让 leftRec.isDir 在降序时依然排在前面，必须在 DescendingOrder 时让 lessThan 返回 !leftRec.isDir，供 Qt 取反后恢复为 true
    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) ? leftRec.isDir : !leftRec.isDir;
    }

    // 🚀【绝对权重 2：置顶/加密优先】：无视升序降序反转，置顶项永远置顶
    bool leftPinned = leftRec.pinned || leftRec.encrypted;
    bool rightPinned = rightRec.pinned || rightRec.encrypted;
    if (leftPinned != rightPinned) {
        return (sortOrder() == Qt::AscendingOrder) ? leftPinned : !leftPinned;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and build:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
2. Verification:
   - Open any folder containing a mix of folders, pinned files, and unpinned files.
   - Toggle sort order between Ascending and Descending (e.g. by Name, Modification Date, Size).
   - Verify that folders and pinned items remain 100% locked at the top of the list in both Ascending and Descending modes, while regular unpinned items below them correctly reverse their sort order.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `FilterProxyModel::lessThan` SSOT sorting virtual method.
- Correctly handled Qt framework `sortOrder()` inversion mechanics.

---

## 6. Header API Signature Verification
- `FilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const` -> `src/ui/models/FilterProxyModel.h`
