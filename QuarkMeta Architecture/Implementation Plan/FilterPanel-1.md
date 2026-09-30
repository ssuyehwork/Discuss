# FilterPanel-1 Implementation Plan

## 1. Overview
修复筛选器（FilterPanel）在勾选条件后内容面板卡片无反应（不刷新）的致命 Bug，以及颜色筛选勾选框无法正确反显颜色的隐蔽 Bug。
- **根因 1（内容面板 0 反应）**：`src/ui/models/FilterProxyModel.cpp` 中 `updateFilter()` 仅调用了 `beginFilterChange()` 与 `endFilterChange()`。当仅变更自定义 `currentFilter` 结构体而不调用 Qt 原生固定字符串筛选 API 时，Qt 底层 `endFilterChange()` 误判筛选条件未改变而直接 return，没有重新计算 `filterAcceptsRow`，导致视图彻底不刷新。
- **根因 2（颜色反显失效）**：`src/ui/FilterPanel.cpp` 中 `syncUIFromFilterState()` 使用 unqualified `findChild<QLabel*>()` 误取到了先添加的 `FilterItemDot`（颜色小圆点），导致获取到的文本始终为空 `""`，从而使颜色匹配失败、无法反显勾选。

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.cpp`
- `src/ui/FilterPanel.cpp`

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/models/FilterProxyModel.cpp`
```
<<<<<<< SEARCH
void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}
=======
void FilterProxyModel::updateFilter() {
    invalidateFilter();
}
>>>>>>> REPLACE
```

### Change 2: `src/ui/FilterPanel.cpp`
```
<<<<<<< SEARCH
        QLabel* labelWidget = row->findChild<QLabel*>();
=======
        QLabel* labelWidget = row->findChild<QLabel*>("FilterItemLabel");
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 构建脚本验证 MSVC 编译：
   `cmake --build QuarkMeta_Build --config Release`
2. 确认在筛选器中勾选评级、类型、颜色等条件时，代理模型触发 `invalidateFilter()` 重新判定，内容面板卡片实时过滤刷新；
3. 确认颜色筛选勾选框能根据当前 `FilterState` 正常反显勾选状态。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 使用 Qt 官方推荐的 `QSortFilterProxyModel::invalidateFilter()` 强制重刷映射表，不私造重刷事件或重新设置 SourceModel。
- 精准利用 `FilterItemLabel` 对象名查找子控件，符合现有 Qt 控件命名规范。

## 6. Header API Signature Verification
- `QSortFilterProxyModel::invalidateFilter()` 为 `QSortFilterProxyModel` 基类的标准 `public slot` 函数。
- `QWidget::findChild<T>(const QString& name)` 为 `QObject` 标准泛型查找 API。

## 7. Header Inclusion Chain & Type Completeness Check
- `FilterProxyModel.cpp` 包含 `<QSortFilterProxyModel>`（位于 `FilterProxyModel.h`），包含链完整。
- `FilterPanel.cpp` 包含 `<QLabel>`，类型完全闭合。
