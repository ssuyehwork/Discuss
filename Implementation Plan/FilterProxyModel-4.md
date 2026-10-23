# FilterProxyModel-4.md

## Overview
在选择右键菜单中的排序类型（例如按“名称”、“创建日期”、“修改日期”、“扩展名”、“大小”、“尺寸”、“评分”）时，`ContentSortController` 会调用 `FilterProxyModel::setSortType(type)` 并随后执行 `proxyModel->sort(0, sortOrder)`。
由于列索引 `0` 与排序方向 `sortOrder` 可能未发生改变，Qt 原生 `QSortFilterProxyModel::sort()` 会判定参数与当前一致而直接 `return` 短路，导致修改后的 `m_sortType` 无法驱动 `lessThan()` 重新计算与视图重绘。

本方案在 `FilterProxyModel` 中增加 `setSortCriteria(type, order)` 合并设置方法，并在 `m_sortType` 或 `m_sortOrder` 改变时显式调用 `invalidate()`，同时同步更新 `ContentSortController::applySortToModel` 与 `ColumnViewPane::applySort` 调用链，确保当排序类型或排序方向改变时，单次平滑触发代理模型重排与视图刷新。

---

## Modified Files List
1. `src/ui/models/FilterProxyModel.h`
2. `src/ui/models/FilterProxyModel.cpp`
3. `src/ui/controllers/ContentSortController.cpp`
4. `src/ui/ColumnViewPane.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/models/FilterProxyModel.h`
```
<<<<<<< SEARCH
    void setSortType(int type) { m_sortType = type; }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; }
=======
    void setSortType(int type);
    void setSortOrder(Qt::SortOrder order);
    void setSortCriteria(int type, Qt::SortOrder order);
>>>>>>> REPLACE
```

### 2. `src/ui/models/FilterProxyModel.cpp`
```
<<<<<<< SEARCH
void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}
=======
void FilterProxyModel::setSortType(int type) {
    if (m_sortType != type) {
        m_sortType = type;
        invalidate();
    }
}

void FilterProxyModel::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        invalidate();
    }
}

void FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order) {
    bool changed = (m_sortType != type || m_sortOrder != order);
    m_sortType = type;
    m_sortOrder = order;
    if (changed) {
        invalidate();
    }
}

void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContentSortController.cpp`
```
<<<<<<< SEARCH
void ContentSortController::applySortToModel(QSortFilterProxyModel* proxyModel) {
    if (!proxyModel) return;
    if (auto* proxy = qobject_cast<FilterProxyModel*>(proxyModel)) {
        proxy->setSortType(static_cast<int>(m_sortType));
        proxy->setSortOrder(m_sortOrder);
    }
    proxyModel->sort(0, m_sortOrder);
}
=======
void ContentSortController::applySortToModel(QSortFilterProxyModel* proxyModel) {
    if (!proxyModel) return;
    if (auto* proxy = qobject_cast<FilterProxyModel*>(proxyModel)) {
        proxy->setSortCriteria(static_cast<int>(m_sortType), m_sortOrder);
    } else {
        proxyModel->sort(0, m_sortOrder);
    }
}
>>>>>>> REPLACE
```

### 4. `src/ui/ColumnViewPane.cpp`
```
<<<<<<< SEARCH
void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortType(sortType);
        m_proxyModel->sort(static_cast<int>(FileListColumn::Name), sortOrder);
    }
}
=======
void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_proxyModel) {
        m_proxyModel->setSortCriteria(sortType, sortOrder);
    }
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译步骤：
在中运行 CMake 构建：
```bash
cmake --build build --config Debug
```

### 2. 验证步骤：
1. 打开 QuarkMeta 应用程序，进入任意包含多个文件的文件夹。
2. 右键调出上下文菜单，展开“排序”子菜单。
3. 当前默认为“名称”排序，保持“降序”不变，选择“创建日期”或“大小”。
4. **验证**：文件列表应当立即重新计算并更新排列顺序，无需二次切换“升序/降序”。
5. 切换至“分栏视图 (`ColumnView`)”，重复上述操作，**验证**分栏视图各列同样即时刷新。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 本次修改直接在 `FilterProxyModel` 代理模型内部使用 Qt 官方 SSOT 入口 `invalidate()` 强制重置排序索引。
- `ContentSortController::applySortToModel` 归一化使用 `setSortCriteria(type, order)` 单次提交，彻底消除双重 `invalidate()` 开销。
- 未私自另起炉灶或在外部 UI 层编写局部的 `clear()` / `loadDirectory()` 视图重建代码。

---

## Header API Signature Verification
- `FilterProxyModel::setSortType(int type)` — `.h` 明确物理签名。
- `FilterProxyModel::setSortOrder(Qt::SortOrder order)` — `.h` 明确物理签名。
- `FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order)` — 新增公有成员函数。
- `QSortFilterProxyModel::invalidate()` — Qt 官方底层 API。

---

## Header Inclusion Chain & Type Completeness Check
- `FilterProxyModel.h` 已包含 `<QSortFilterProxyModel>`。
- `FilterProxyModel.cpp` 包含 `"FilterProxyModel.h"`。
- `ContentSortController.cpp` 已包含 `"FilterProxyModel.h"`。
- `ColumnViewPane.cpp` 已包含 `"FilterProxyModel.h"`。
- 无任何间接头文件断裂或未声明标识符隐患。
