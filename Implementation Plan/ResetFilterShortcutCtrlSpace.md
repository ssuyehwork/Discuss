# 筛选器标题栏重置筛选条件绑定 Ctrl+Space 快捷键重构实施方案

- **Base Commit Hash**: `3e304b14270588e6d40fe9c9cbf79a617f73a932`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

### 已读物理证据清单
1. **物理读取文件列表**：
   - `src/ui/AppShortcutController.h`
   - `src/ui/AppShortcutController.cpp`
   - `src/ui/FilterPanel.h`
   - `src/ui/FilterPanel.cpp`
   - `src/ui/PanelMediator.h`
   - `src/ui/PanelMediator.cpp`
2. **头文件原样签名验证**：
   - `void FilterPanel::clearAllFilters(bool force = false)` -> `src/ui/FilterPanel.h:60`
   - `AppShortcutController::AppShortcutController(QWidget* targetWindow, SearchController* searchController, QObject* parent = nullptr)` -> `src/ui/AppShortcutController.h:19`
3. **既有 API 检索日志**：
   - `grep -rn "clearAllFilters" src/ui/FilterPanel.h` -> 确认清空筛选条件的槽函数及默认参数
   - `grep -rn "toggleRecursiveRequested" src/ui/AppShortcutController.h` -> 确认既有快捷键信号定义模式

---

## Chapter 1: Overview（概述与根因分析）

在 QuarkMeta 界面设计中，筛选面板（`FilterPanel`）标题栏右上角设置有“重置所有筛选条件”按钮（`m_btnClearAll`），用于快速清空用户选中的标签、颜色、文件类型、日期等过滤条件。目前该功能仅支持鼠标点击触发，在键盘流高效操作场景下缺乏对应的快捷键。

为了提升用户体验并严格遵循系统 SSOT（单一事实来源）架构，本方案将在局内快捷键控制器 `AppShortcutController` 中注册 `Ctrl+Space` 快捷键，并通过 `PanelMediator` 路由将快捷键信号与 `FilterPanel::clearAllFilters(true)` 进行编排绑定，实现按下 `Ctrl+Space` 时即刻清空所有筛选条件。

---

## Chapter 2: Modified Files List（影响文件清单）

* **修改的文件**：
  - `src/ui/AppShortcutController.h`
  - `src/ui/AppShortcutController.cpp`
  - `src/ui/PanelMediator.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. 修改 `src/ui/AppShortcutController.h`，声明 `resetFilterRequested` 信号

```
<<<<<<< SEARCH
    /**
     * @brief Ctrl+R 触发切换递归显示子文件夹中的项目
     */
    void toggleRecursiveRequested();
=======
    /**
     * @brief Ctrl+R 触发切换递归显示子文件夹中的项目
     */
    void toggleRecursiveRequested();

    /**
     * @brief Ctrl+Space 触发重置所有筛选条件
     */
    void resetFilterRequested();
>>>>>>> REPLACE
```

### 2. 修改 `src/ui/AppShortcutController.cpp`，在 `initShortcuts` 中注册 `Ctrl+Space` 快捷键

```
<<<<<<< SEARCH
    // 11. Ctrl+R: 切换递归显示子文件夹中的项目
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() {
        if (!isEditingFocus()) {
            emit toggleRecursiveRequested();
        }
    });
}
=======
    // 11. Ctrl+R: 切换递归显示子文件夹中的项目
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), [this]() {
        if (!isEditingFocus()) {
            emit toggleRecursiveRequested();
        }
    });

    // 12. Ctrl+Space: 局内重置所有筛选条件
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space), [this]() {
        if (!isEditingFocus()) {
            emit resetFilterRequested();
        }
    });
}
>>>>>>> REPLACE
```

### 3. 修改 `src/ui/PanelMediator.cpp`，在 `setupConnections` 中编排绑定信号与 `FilterPanel::clearAllFilters`

```
<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::toggleRecursiveRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setRecursive(!target->isRecursive());
            }
        });
=======
        connect(shortcutController, &AppShortcutController::toggleRecursiveRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setRecursive(!target->isRecursive());
            }
        });

        if (filterPanel) {
            connect(shortcutController, &AppShortcutController::resetFilterRequested, filterPanel, [filterPanel]() {
                filterPanel->clearAllFilters(true);
            });
        }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 分步构建命令
```bash
cmake --build build --config Release -j 8
```

### 运行时验收标准
1. **快捷键触发验证**：在筛选面板中勾选任意筛选条件（如选择某种颜色或文件类型），在主视口非文本编辑状态下按下 `Ctrl+Space`。
2. **重置结果验证**：确认所有选中的筛选勾选项瞬间清空，重置图标恢复默认状态，主视口恢复显示全部项目。
3. **编辑状态防误触验证**：在搜索框（`SearchEdit`）或重命名输入框（`FileNameLineEdit`）中处于文本编辑输入状态时，按下 `Ctrl+Space`，确认不触发重置筛选操作（由 `isEditingFocus()` 防护拦截）。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **统一快捷键总线**：基于 `AppShortcutController` 内置的 `registerShortcut` 与跨平台防抢占钩子机制（`LocalShortcutInterceptor`）进行注册，无私挂事件过滤器或破坏统一快捷键管理。
2. **既有 API 直通**：直接复用 `FilterPanel::clearAllFilters(true)` 公共槽函数，零冗余新增逻辑。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 观察者 / 视图 | 刷新方向 |
| :--- | :--- | :--- | :--- |
| **快捷键触发源** | `AppShortcutController` | `PanelMediator` -> `FilterPanel` | 信号驱动调用 `clearAllFilters(true)` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `FilterPanel::clearAllFilters(bool force)` 公共槽函数原样物理签名保持 100% 只读冻结。
- `AppShortcutController` 新增公共信号 `void resetFilterRequested()`，符合 Qt 标准信号扩展规范。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何 `#include` 依赖关系，类型完整性与头文件包含链保持不变。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 详细说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否未对源代码进行直接修改 | **满足** | 本方案为 Markdown 实施方案，存放于 `Implementation Plan/` 目录 |
| 2 | 是否回应了架构三问 | **满足** | Chapter 1 阐明了快捷键分发与编排总线 |
| 3 | 是否无 C++ 硬编码内联 QSS | **满足** | 零 QSS 修改 |
| 4 | 是否未动 Freeze 的公共接口 | **满足** | 既有接口保持只读冻结，仅按规范添加新信号 |
| 5 | 是否包含 SEARCH 块真实性核查 | **满足** | 所有 SEARCH 块均与目标 Commit 物理源码 1:1 匹配 |
| 6 | 是否包含构建与运行时验收标准 | **满足** | Chapter 4 提供编译命令与运行时测试标准 |
| 7 | 是否包含数值零篡改表 | **满足** | Chapter 10 提供完整对照表 |
| 8 | 语言规范是否合规 | **满足** | 正文及注释严格遵守中文规范 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性 / 交互点 | 原始配置 | 重构后配置 | 一致性 / 变化确认 |
| :--- | :--- | :--- | :--- |
| **鼠标点击重置按钮** | 触发 `clearAllFilters(true)` | 触发 `clearAllFilters(true)` | **100% 保持一致** |
| **Ctrl+Space 快捷键** | 未绑定/无响应 | **触发 `clearAllFilters(true)`** | **按用户要求新增键盘流快捷键** |
| **输入框编辑焦点下的 Ctrl+Space** | 忽略/由编辑框处理 | **忽略（由 `isEditingFocus()` 保护）** | **防止输入法/编辑误触** |
