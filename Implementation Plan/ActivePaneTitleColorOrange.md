# 多窗格模式下激活窗格标题颜色高亮与移除边框高亮线重构实施方案

- **Base Commit Hash**: `3e304b14270588e6d40fe9c9cbf79a617f73a932`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

### 已读物理证据清单
1. **物理读取文件列表**：
   - `src/ui/ContentHeaderWidget.h`
   - `src/ui/ContentHeaderWidget.cpp`
   - `resources/style.qss`
2. **头文件原样签名验证**：
   - `void ContentHeaderWidget::setActive(bool active)` -> `src/ui/ContentHeaderWidget.h:26`
3. **既有 API 检索日志**：
   - `grep -rn "ContentHeaderTitle" resources/style.qss` -> 确认标题 Label QSS 选择器
   - `grep -rn "activePane" resources/style.qss` -> 确认边框高亮线 QSS 选择器位于第 142 行 `#EditorContainer[activePane="true"], ContentPanel[activePane="true"]`
   - `grep -rn "activePane" src/ui/ContentHeaderWidget.cpp` -> 确认属性刷新逻辑

---

## Chapter 1: Overview（概述与根因分析）

在 QuarkMeta 的多窗格（分栏/双视口）浏览模式下，原本通过窗格外框的高亮边线（`border: 1px solid #555555;`）来指示焦点状态。为了提供更简洁清爽的视觉界面并强化视口标题栏的焦点反馈，用户明确要求**彻底移除激活窗格的外框视觉高亮线，不可保留**，并改用视口标题栏文字的颜色高亮来指示当前激活窗格。

本方案通过：
1. 从 `resources/style.qss` 中**彻底移除**针对激活窗格的外框边框高亮规则（`#EditorContainer[activePane="true"], ContentPanel[activePane="true"] { border: 1px solid #555555; }`）。
2. 在 `src/ui/ContentHeaderWidget.cpp` 中，给当前**激活（获得焦点）**的窗格顶部标题栏（`ContentHeaderWidget`）的文件夹名称标签（`m_titleLabel`）动态设置 `activePane="true"` 属性，并在 `resources/style.qss` 中定义专用的 QSS 样式选择器（`QLabel#ContentHeaderTitle[activePane="true"]`），使激活窗格的标题文字高亮显示为醒目的**橙红色 `#ff551c`**；非激活窗格标题文字则保持默认的**灰色 `#888888`**。

---

## Chapter 2: Modified Files List（影响文件清单）

* **修改的文件**：
  - `src/ui/ContentHeaderWidget.cpp`
  - `resources/style.qss`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. 修改 `src/ui/ContentHeaderWidget.cpp` 中的 `setActive` 逻辑，同步刷新 `m_titleLabel` 属性与 QSS 样式

```
<<<<<<< SEARCH
void ContentHeaderWidget::setActive(bool active) {
    setProperty("activePane", active ? "true" : "false");
    style()->unpolish(this);
    style()->polish(this);
}
=======
void ContentHeaderWidget::setActive(bool active) {
    setProperty("activePane", active ? "true" : "false");
    if (m_titleLabel) {
        m_titleLabel->setProperty("activePane", active ? "true" : "false");
        m_titleLabel->style()->unpolish(m_titleLabel);
        m_titleLabel->style()->polish(m_titleLabel);
    }
    style()->unpolish(this);
    style()->polish(this);
}
>>>>>>> REPLACE
```

### 2. 修改 `resources/style.qss` 彻底移除外框高亮线规则并添加 `QLabel#ContentHeaderTitle[activePane="true"]` 文字高亮选择器

```
<<<<<<< SEARCH
#EditorContainer[activePane="true"], ContentPanel[activePane="true"] {
    border: 1px solid #555555;
}

#EditorContainer[isHostPanel="true"] {
=======
#EditorContainer[isHostPanel="true"] {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QLabel#ContentPanelTitleLabel {
    font-size: 13px;
    font-weight: bold;
    color: #888888;
    background: transparent;
    border: none;
}
=======
QLabel#ContentPanelTitleLabel {
    font-size: 13px;
    font-weight: bold;
    color: #888888;
    background: transparent;
    border: none;
}

QLabel#ContentHeaderTitle[activePane="true"] {
    color: #ff551c;
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
1. **外框高亮线完全移除验证**：开启分栏双窗格模式，观察激活窗格周围，确认没有任何 1px 灰色或暗色外框边线（`border: 1px solid #555555` 已物理清空）。
2. **标题文字颜色高亮验证**：点击左侧窗格，左侧窗格顶部“自建”标题文字瞬间变为醒目的**橙红色 `#ff551c`**；右侧“Z 暂时保留”保持为**灰色 `#888888`**。
3. **焦点切换表现**：点击右侧窗格，右侧“Z 暂时保留”标题文字瞬间变为**橙红色 `#ff551c`**；左侧“自建”标题恢复为**灰色 `#888888`**。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **Qt Dynamic Property 机制**：利用 Qt 原生 `QObject::setProperty("activePane", ...)` 与 `QStyle::unpolish`/`polish`，实现无侵入式的 QSS 动态属性切换。
2. **全局样式表定义**：所有的样式规则彻底定义在 `resources/style.qss` 中，无任何 C++ 内联 `setStyleSheet` 硬编码，且外框高亮线样式规则已完全移除。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 观察者 / 视图 | 刷新方向 |
| :--- | :--- | :--- | :--- |
| **窗格激活状态 (Active Pane)** | `ContentPaneSplitManager` / `PanelMediator` | `ContentHeaderWidget::setActive(bool)` | 信号驱动更新 `activePane` dynamic property |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 无任何新增或修改的 `.h` 接口签名，`ContentHeaderWidget::setActive(bool active)` 的物理签名在 `ContentHeaderWidget.h:26` 保持 100% 只读冻结与向后兼容。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 未修改任何 `#include` 语句，依赖与类型完整性保持不变。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 详细说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否未对源代码进行直接修改 | **满足** | 本方案为 Markdown 实施方案，存放于 `Implementation Plan/` 目录 |
| 2 | 是否回应了架构三问 | **满足** | Chapter 1 阐明了高亮状态驱动机制与边框高亮线的彻底移除 |
| 3 | 是否无 C++ 硬编码内联 QSS | **满足** | 样式全量收敛于 `resources/style.qss` |
| 4 | 是否未动 Freeze 的公共接口 | **满足** | 无任何 `.h` 接口签名修改 |
| 5 | 是否包含 SEARCH 块真实性核查 | **满足** | 所有 SEARCH 块均与目标 Commit 物理源码及 QSS 1:1 匹配 |
| 6 | 是否包含构建与运行时验收标准 | **满足** | Chapter 4 提供分步编译命令与运行时验收测试用例 |
| 7 | 是否包含数值零篡改表 | **满足** | Chapter 10 提供完整对照表 |
| 8 | 语言规范是否合规 | **满足** | 正文及注释严格遵守中文规范 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 属性 / 交互点 | 原始配置 | 重构后配置 | 一致性 / 变化确认 |
| :--- | :--- | :--- | :--- |
| **激活窗格外框视觉高亮线** | `border: 1px solid #555555;` | **彻底移除 (无边框)** | **按用户明确要求彻底清除** |
| **激活窗格标题文字颜色** | 默认灰色 `#888888` | **橙红色 `#ff551c`** | **显式高亮优化** |
| **非激活窗格标题文字颜色** | 默认灰色 `#888888` | 默认灰色 `#888888` | **保持原样** |
| **标题栏字体尺寸与样式** | `13px`, `bold` | `13px`, `bold` | **100% 保持一致** |
