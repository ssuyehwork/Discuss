# FilterProxyModel-5.md

## Overview
修复 `FilterProxyModel` 中因保存独立成员 `m_sortOrder` 导致与 Qt 原生 `sortOrder()` 双真理源冲突，进而引发“降序选择后列表仍为升序”的 Bug。

具体改动：
1. 从 `FilterProxyModel` 中彻底移除私有成员 `m_sortOrder`，升降序状态 100% 信任与使用 Qt 框架原生 `sortOrder()`。
2. 移除 `FilterProxyModel` 中无外部调用的 `setSortType(int)` 与 `setSortOrder(Qt::SortOrder)` 冗余方法，统一收敛至唯一入口 `setSortCriteria(int type, Qt::SortOrder order)`。
3. `setSortCriteria(int type, Qt::SortOrder order)` 精准重排控制逻辑：
   - 记录 `m_sortType` 是否改变；
   - 若 `sortColumn() != 0` 或 `sortOrder() != order`，调用 `sort(0, order)` 驱动 Qt 更新原生 `sortOrder()` 与 `sortColumn()`；
   - 否则若仅 `m_sortType` 发生改变，调用 `invalidate()`；
   - 若类型与顺序均未发生改变，不做任何操作。

---

## Modified Files List
1. `src/ui/models/FilterProxyModel.h`
2. `src/ui/models/FilterProxyModel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/models/FilterProxyModel.h`
```
<<<<<<< SEARCH
    void setSortType(int type);
    void setSortOrder(Qt::SortOrder order);
    void setSortCriteria(int type, Qt::SortOrder order);


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
=======
    void setSortCriteria(int type, Qt::SortOrder order);


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
>>>>>>> REPLACE
```

### 2. `src/ui/models/FilterProxyModel.cpp`
```
<<<<<<< SEARCH
void FilterProxyModel::setSortType(int type) {
    if (m_sortType != type) {
        m_sortType = type;
        if (sortColumn() == -1) {
            sort(0, m_sortOrder);
        } else {
            invalidate();
        }
    }
}

void FilterProxyModel::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        if (sortColumn() == -1) {
            sort(0, m_sortOrder);
        } else {
            invalidate();
        }
    }
}

void FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order) {
    bool changed = (m_sortType != type || m_sortOrder != order);
    m_sortType = type;
    m_sortOrder = order;
    if (sortColumn() == -1) {
        sort(0, m_sortOrder);
    } else if (changed) {
        invalidate();
    }
}
=======
void FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order) {
    bool typeChanged = (m_sortType != type);
    m_sortType = type;

    if (sortColumn() != 0 || sortOrder() != order) {
        sort(0, order);
    } else if (typeChanged) {
        invalidate();
    }
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译步骤：
运行构建：
```bash
cmake --build build --config Debug
```

### 2. 验证步骤：
1. 启动应用，在网格视图、列表视图、分栏视图下分别测试排序菜单。
2. 切换“升序/降序”选项：验证列表是否即时反转；置顶项与文件夹区是否始终保持在最上方。
3. 切换“名称/大小/修改日期”等类型：验证视图是否即时重排。
4. 启动应用后首次点击“降序”，验证降序是否即时生效。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 彻底删除了重复维护升降序状态的私有成员 `m_sortOrder`，把升降序真理源唯一归还给 Qt 底层 `sortOrder()`。
- 物理清理了未使用的 `setSortType` 与 `setSortOrder` 方法。

---

## Header API Signature Verification
- `FilterProxyModel::setSortCriteria(int type, Qt::SortOrder order)` — `.h` 物理定义保持一致。
- `QSortFilterProxyModel::sort(int column, Qt::SortOrder order)` — Qt 框架标准 API。
- `QSortFilterProxyModel::sortColumn()` — Qt 框架标准 API。
- `QSortFilterProxyModel::sortOrder()` — Qt 框架标准 API。

---

## Header Inclusion Chain & Type Completeness Check
- `FilterProxyModel.h` 包含 `<QSortFilterProxyModel>`。
- 无增加外部依赖，包含链闭合且类型完整。
