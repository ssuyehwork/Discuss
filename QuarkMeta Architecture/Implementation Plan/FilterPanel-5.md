# FilterPanel.md - 库分类加载重置与筛选锁定保护实施方案

## 【第 1 关：起草前核查（Pre-Draft Evidence）】

### 1. 已读文件清单
- `src/ui/PanelMediator.cpp` (已用 read_file 物理读取)
- `src/ui/FilterPanel.h` (已用 read_file 物理读取)
- `src/ui/FilterPanel.cpp` (已用 read_file 物理读取)

### 2. 签名原文摘录
- `FilterPanel.h`:
  ```cpp
  void clearAllFilters(bool force = false);
  ```
- `PanelMediator.cpp` (针对 `libraryPanel` 的处理断点):
  ```cpp
  if (libraryPanel) {
      connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel](const QStringList& paths) {
          ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
          if (target) {
              target->setCurrentCategoryType("library");
              target->setLastLoadedLibraryPaths(paths);
              target->loadPaths(paths);
          }
      });
  }
  ```

### 3. 既有 API 检索记录
- 检索关键词：`categoryPathsSelected`, `clearAllFilters`, `m_isFilterPinned`
- 检索结果：
  - 在 `PanelMediator.cpp` 中，`NavigationService::currentUrlChanged`（地址栏/目录导航）以及 `ColumnViewWidget::pathNavigated` 均调用了 `filterPanel->clearAllFilters(false)`。
  - 唯独 `libraryPanel::categoryPathsSelected` 连接在响应【库】分类点击时，漏掉了对 `filterPanel->clearAllFilters(false)` 的调用，导致非锁定状态下带入上一分类的筛选条件。

### 4. Memories.md 与 AGENTS.md 相关条款
- **唯一真理源 (SSOT)**：通用行为走统一控制管道，`FilterPanel::clearAllFilters(false)` 为筛选清空与锁定拦截的唯一合法入口。
- **用户意图与流程契约**：未锁定状态下加载分类必须自动重置筛选条件，已锁定状态下保留当前筛选条件。

---

## 1. Overview（概述与解决的问题）

### 解决的问题
在筛选器上勾选任意条件后，点击【库】侧边栏中的分类时，由于 `PanelMediator` 未在 `categoryPathsSelected` 回调中触发筛选重置机制，导致系统在未锁定的情况下仍把上一上下文的筛选条件带入新加载的库分类中。

### 核心重构方案
在 `PanelMediator::setupConnections()` 中，为 `libraryPanel::categoryPathsSelected` 触发回调补齐 `filterPanel->clearAllFilters(false)`。
- **未锁定状态（`m_isFilterPinned == false`）**：自动清空/重置筛选条件，完整展示该库分类下的所有内容；
- **已锁定状态（`m_isFilterPinned == true`）**：`clearAllFilters(false)` 内部防御机制会自动拦截重置，继续保留当前筛选条件。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/ui/PanelMediator.cpp`
在 `libraryPanel::categoryPathsSelected` 信号关联的 Lambda 闭包中，增加对 `filterPanel->clearAllFilters(false)` 的调用：

```
<<<<<<< SEARCH
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel](const QStringList& paths) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadPaths(paths);
                }
            });
        }
=======
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel, filterPanel](const QStringList& paths) {
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadPaths(paths);
                }
            });
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译核查**：
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ```
2. **功能验证**：
   - 在筛选器面板中勾选任意条件（如“5星”或某个颜色标记）；
   - 在未锁定状态下，点击【库】侧边栏的不同分类，验证筛选器面板自动恢复为重置状态，新分类展示全部项；
   - 点击筛选器顶部的“钉子”按钮锁定当前筛选条件；
   - 点击【库】侧边栏的不同分类，验证当前筛选条件保持生效。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 入口复用自查）

- 本次修改是否复用了既有 SSOT 官方 API：是，复用了 `FilterPanel::clearAllFilters(false)` 既有标准入口。
- 是否存在私自另起炉灶行为：否，没有手动修改或覆盖 `FilterState`。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用函数 | 声明所在头文件 | 头文件中的精准原样签名 | 核查状态 |
|---|---|---|---|
| `clearAllFilters` | `src/ui/FilterPanel.h` | `void clearAllFilters(bool force = false);` | 100% 物理原生匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

| 修改/引用的文件 | 修改/增加的 `#include` 行 | 核查理由与完整类型保障 |
|---|---|---|
| `src/ui/PanelMediator.cpp` | 无（已显式包含 `"FilterPanel.h"`） | `PanelMediator.cpp` 已具备 `FilterPanel` 的完整类型定义 |

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 核对结论 (满足/不满足/不涉及) | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `FilterPanel.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 章节 3.1 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 本次不新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节 1~9 完整覆盖 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 章节 6 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 章节 7 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 章节 5 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不修改样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 行为 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不包含批量数据变动 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 完全符合与用户达成的共识逻辑 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅改动 `PanelMediator.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 本方案不涉及任何 UI 尺寸、颜色、Hex、px 或边距参数更改（填写：**不涉及**）。
