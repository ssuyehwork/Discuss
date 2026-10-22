# SearchControllerNormalization.md - SearchController 搜索框清理与聚焦 SSOT 入口归一化实施方案

## Chapter 1: Pre-Draft Evidence（第 1 关：起草前核查）

### 1. 已读文件清单
- `src/ui/SearchController.h`（已用工具物理读取）
- `src/ui/SearchController.cpp`（已用工具物理读取）
- `src/ui/PanelMediator.cpp`（已用工具物理读取第 238~245 行）
- `src/ui/AppShortcutController.cpp`（已用工具物理读取第 145~151 行）

### 2. 物理头文件签名原文摘录
- `SearchController.h`:
  `QLineEdit* searchEdit() const { return m_searchEdit; }`
  `void doSearch(const QString& keyword);`
- `AppShortcutController.cpp`:
  `if (m_searchController && m_searchController->searchEdit())`
- `PanelMediator.cpp`:
  `if (searchController && searchController->searchEdit())`

### 3. 既有 API 检索记录
- 使用 `git grep "searchEdit"` 检索到全工程对 `SearchController::searchEdit()` 的下钻调用共 2 处（`PanelMediator.cpp` 与 `AppShortcutController.cpp`）。其余 `TagManagerDialog` 与 `TagSelectorOverlay` 中的同名变量属于局部私有控件，不冲突。

---

## Chapter 2: Overview（概述与解决的问题）

### 核心问题
1. **违背 SSOT 与黑盒隔离**：`SearchController` 对外暴露了内部控件指针 `searchEdit()`。外部调用方 `PanelMediator` 在路径切换时，自己手写了 `searchController->searchEdit()->blockSignals(true); searchController->searchEdit()->clear(); searchController->searchEdit()->blockSignals(false);`，越权下钻操作私有组件，导致搜索框清理逻辑产生了分散的实现与静默/显式清理的不一致。
2. **快捷键直接下钻**：`AppShortcutController` 在处理 `Ctrl+F` 快捷键时，直接使用 `m_searchController->searchEdit()->setFocus(...)` 与 `selectAll()` 下钻，缺少统一的控制层入口。

### 重构方案
1. **提供统一的 SSOT 清理与聚焦接口**：
   - 在 `SearchController` 中提供公有方法 `void clearSearch(bool blockSignals = false)`，统一负责搜索框文本清空、清除按钮自动隐藏与信号控制。
   - 在 `SearchController` 中提供公有方法 `void focusSearch()`，统一负责搜索框焦点获取与全选操作。
2. **彻底消灭外部下钻**：
   - 将 `PanelMediator.cpp` 中的强行 `blockSignals` + `clear()` 替换为直调 `searchController->clearSearch(true)`。
   - 将 `AppShortcutController.cpp` 中的 `setFocus` / `selectAll` 替换为直调 `m_searchController->focusSearch()`。

---

## Chapter 3: Modified Files List（影响文件清单）
1. `src/ui/SearchController.h`（新增 `clearSearch` 与 `focusSearch` 公有接口）
2. `src/ui/SearchController.cpp`（实现 `clearSearch` 与 `focusSearch` 方法）
3. `src/ui/PanelMediator.cpp`（替换下钻 `clear()` 为 SSOT 入口 `clearSearch(true)`）
4. `src/ui/AppShortcutController.cpp`（替换下钻 `setFocus()` 为 SSOT 入口 `focusSearch()`）

---

## Chapter 4: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/SearchController.h`
```
<<<<<<< SEARCH
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }
=======
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }

    // 搜索框 SSOT 统一控制接口
    void clearSearch(bool blockSignals = false);
    void focusSearch();
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
void SearchController::doSearch(const QString& keyword) {
=======
void SearchController::clearSearch(bool blockSignals) {
    if (!m_searchEdit) return;
    if (blockSignals) {
        m_searchEdit->blockSignals(true);
        m_searchEdit->clear();
        m_searchEdit->blockSignals(false);
    } else {
        m_searchEdit->clear();
    }
}

void SearchController::focusSearch() {
    if (!m_searchEdit) return;
    m_searchEdit->setFocus(Qt::ShortcutFocusReason);
    m_searchEdit->selectAll();
}

void SearchController::doSearch(const QString& keyword) {
>>>>>>> REPLACE
```

### 3. `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
        if (searchController && searchController->searchEdit()) {
            searchController->searchEdit()->blockSignals(true);
            searchController->searchEdit()->clear();
            searchController->searchEdit()->blockSignals(false);
        }
=======
        if (searchController) {
            searchController->clearSearch(true);
        }
>>>>>>> REPLACE
```

### 4. `src/ui/AppShortcutController.cpp`
```
<<<<<<< SEARCH
    // 5. Ctrl+F: 局内聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    });
=======
    // 5. Ctrl+F: 局内聚焦搜索栏
    registerShortcut(QKeySequence::Find, [this]() {
        if (m_searchController) {
            m_searchController->focusSearch();
        }
    });
>>>>>>> REPLACE
```

---

## Chapter 5: Build & Verification Steps（编译命令与验证方法）

### 编译步骤
```bash
cmake -B build -G "Ninja"
cmake --build build --config Release
```

### 验证方法
1. **搜索框清理功能验证**：在地址栏输入关键字搜索后，切换到任意其他文件夹或视图，验证搜索框被平滑清空且清除按钮同时自动隐藏，系统无崩溃或信号死循环。
2. **Ctrl+F 快捷键验证**：按下 `Ctrl+F`，验证搜索输入框准确获取焦点并高亮全选当前文本。

---

## Chapter 6: SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）
- 统一收紧至 `SearchController` 控制层，彻底消灭外部直接针对 `m_searchEdit` 私有组件的越权下钻和分散逻辑。

---

## Chapter 7: Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / 函数 | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `SearchController::clearSearch` | `src/ui/SearchController.h` | `void clearSearch(bool blockSignals = false);` | 物理新增定义一致 |
| `SearchController::focusSearch` | `src/ui/SearchController.h` | `void focusSearch();` | 物理新增定义一致 |

---

## Chapter 8: Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）
- `PanelMediator.cpp` 与 `AppShortcutController.cpp` 均已包含 `#include "SearchController.h"`，头文件包含链完备闭合。

---

## Chapter 9: Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `SearchControllerNormalization.md` 对应 `SearchController` 核心类 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/SearchControllerNormalization.md` |
| 所有 SEARCH 块代码在仓库真实存在 | 3.2 | 满足 | 第 4 章 SEARCH 块逐字存在于源码中 |
| 第 1~10 章齐全 | 3.4 | 满足 | 第 1~10 章齐全 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 第 6 章 |

---

## Chapter 10: Zero-Value-Alteration Check（数值零篡改核对）
- 本方案不涉及任何 UI 尺寸或样式参数变动，填写：**不涉及**。
