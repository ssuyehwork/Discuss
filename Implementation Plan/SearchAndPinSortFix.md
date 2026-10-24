# Implementation Plan - 搜索与置顶排序逻辑修复 (SearchAndPinSortFix.md)

- **Base Commit Hash**: `687093f93e2ad7abd6d3ad5053964eb3ffc6a97a`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 已读物理证据清单 (Gate 1: Pre-Draft Evidence)

### 1. 物理读取文件列表
- `src/ui/models/FilterProxyModel.h`
- `src/ui/models/FilterProxyModel.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/SearchController.cpp`

### 2. 头文件原样签名
- `src/ui/models/FilterProxyModel.h`:
  ```cpp
  void updateFilter();
  bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
  ```
- `src/ui/ContentPanel.h`:
  ```cpp
  void search(const QString& query);
  void applyFilters();
  void applySort();
  ```

### 3. 既有 API 检索日志
- `grep -n -C 10 "void FilterProxyModel::updateFilter()" src/ui/models/FilterProxyModel.cpp`
- `grep -n -C 30 "lessThan" src/ui/models/FilterProxyModel.cpp`

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题根因分析
在当前 `FilterProxyModel` 的实现中，当用户在搜索框输入关键词（`SearchController::doSearch` -> `ContentPanel::search` -> `applyFilters`）或清空搜索框时，`ContentPanel` 会调用 `FilterProxyModel::updateFilter()`。
在现有的 `FilterProxyModel::updateFilter()` 中，仅调用了 `beginFilterChange()` 与 `endFilterChange()`。受限于 Qt 的 `QSortFilterProxyModel` 内部工作机制，`beginFilterChange()` / `endFilterChange()` 仅仅更新了行的过滤显隐判定（`filterAcceptsRow`），**并未强制重新构建或重新排序代理模型的索引映射（Sort Invalidation）**。

虽然 `FilterProxyModel::lessThan` 内部已经写明了优先级判定规则（`leftRec.isDir` 文件夹优先 -> `leftRec.pinned` 置顶优先 -> 具体属性排序），但由于搜索或清空搜索时没有触发索引重排（Invalidation），导致匹配出的项目在视图呈现时丢失了对 `pinned` 置顶特性的排序约束，出现了置顶项目混杂在普通结果中的现象。

### 1.2 重构设计目标
1. 修改 `FilterProxyModel::updateFilter()` 逻辑，在过滤条件变动（包含搜索词输入、清空及高级筛选变动）时，调用 `invalidate()` 以同时重刷新过滤（Filter）与排序（Sort）映射。
2. 确保在搜索模式下（输入任意搜索关键词）以及清空搜索词恢复全量视图时，所有被标记为置顶（`pinned == true`）的项目在搜索结果中始终严格归位置顶，且全量规则 100% 保持一致。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/ui/models/FilterProxyModel.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

```
<<<<<<< SEARCH
void FilterProxyModel::updateFilter() {
    beginFilterChange();
    endFilterChange();
}
=======
void FilterProxyModel::updateFilter() {
    invalidate();
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译构建命令
在仓库根目录下执行 CMake 编译构建：
```bash
cmake --build build --config Release
```

### 4.2 运行时验收标准
1. **搜索状态下置顶项归位校验**：
   - 在某个文件夹中将其中 1~2 个文件/文件夹设为“置顶”（Pinned）。
   - 在右上角搜索框中输入能匹配到置顶项及普通项的通用关键词。
   - 观察搜索结果视图：匹配到的置顶项必须无条件排列在搜索结果的最顶部。
2. **清空搜索词恢复校验**：
   - 点击搜索框清除按钮或按 Backspace 清空搜索词。
   - 观察内容视图：全量视图恢复，且置顶项依然稳定停留在列表/网格的最顶部，排序规则未发生任何错乱。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **复用 Qt 官方 `QSortFilterProxyModel::invalidate()` API**：
   直接使用 Qt 内置的 `invalidate()` 替换局部的 `beginFilterChange()`/`endFilterChange()`，彻底打通过滤与排序的联动重算，无需手写任何额外的重新排序算法。
2. **不新增任何重复的逻辑**：
   完全继承 `FilterProxyModel::lessThan` 已有的 `leftRec.pinned != rightRec.pinned` 核心置顶判定逻辑，保持单一点 SSOT。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 项置顶状态 (`pinned`) | `ItemRecord::pinned` | `FilterProxyModel::lessThan` 比较入口 | 读取 `ItemRecord` -> `lessThan` 计算视图顺序 |
| 搜索关键词 (`keyword`) | `FilterState::keyword` | `FilterProxyModel::currentFilter` | `ContentPanel::search` -> `updateFilter()` -> `invalidate()` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **对外接口签名控制**：
   `FilterProxyModel::updateFilter()` 接口声明保持不变（`void updateFilter()`），物理 `.h` 无需任何动码，100% 冻结对外契约。
2. **跨类访问权限检查**：
   `invalidate()` 为 `QSortFilterProxyModel` 的 `protected` 成员函数，`FilterProxyModel` 继承自 `QSortFilterProxyModel`，在其成员函数 `updateFilter()` 内直接调用属于合法访问。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. **头文件修改说明**：
   本次改动完全位于 `FilterProxyModel.cpp` 内部实现中，未增加或减少任何 `#include` 头文件包含。
2. **类型完整性**：
   `FilterProxyModel.cpp` 既有包含已闭合，`invalidate()` 函数定义完全可解析。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否只允许使用中文（除英文标题/代码） | 满足 | 方案全篇统一中文 |
| 2 | 是否放置于 `Implementation Plan/` 物理目录 | 满足 | `Implementation Plan/SearchAndPinSortFix.md` |
| 3 | 物理文件名是否使用语义化英文命名 | 满足 | `SearchAndPinSortFix.md` |
| 4 | 是否包含 Base Commit Hash 锚点 | 满足 | `687093f93e2ad7abd6d3ad5053964eb3ffc6a97a` |
| 5 | 是否逐字核对 SEARCH 块在源码中的唯一匹配性 | 满足 | `FilterProxyModel.cpp` 23~26 行唯一匹配 |
| 6 | 是否未触及 `.h` 接口签名（契约锁） | 满足 | `.h` 未做任何更改 |
| 7 | 是否复用既有官方 API（防另起炉灶） | 满足 | 复用 `QSortFilterProxyModel::invalidate()` |
| 8 | 是否包含完整 10 大章节 | 满足 | 1~10 章完整 |
| 9 | 是否包含构建命令与验收用例 | 满足 | Chapter 4 |
| 10 | 是否无跨线程 UI 直接操作 | 满足 | 均在主线程事件循环同步执行 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性 / 行为 | 重构前表现 | 重构后表现 | 是否一致 / 说明 |
| :--- | :--- | :--- | :--- |
| 视图排序规则 | 文件夹优先 -> 置顶优先 -> 属性排序 | 文件夹优先 -> 置顶优先 -> 属性排序 | 100% 一致 |
| 搜索过滤逻辑 | 匹配文件名、扩展名、备注等 | 匹配文件名、扩展名、备注等 | 100% 一致 |
| 样式与数值 | 无修改 | 无修改 | 100% 一致 |
