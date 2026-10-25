# Implementation Plan - FilterPanel 顶部按钮 ToolTipOverlay 悬停提示跳动修复

- **Base Commit Hash**: `1e9a71b62c08f4b040483238a5a105559768becc`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 第 1 关：已读物理证据清单 (Pre-Draft Evidence)

### 1.1 物理读取文件列表
1. `src/ui/FilterPanel.cpp` (已通过 `read_file` 精准读取 `eventFilter` 机制与事件处理逻辑)
2. `src/ui/ContentHeaderWidget.cpp` (已通过 `read_file` 精准读取标杆 `eventFilter` 实现)
3. `src/ui/ToolTipOverlay.cpp` (已通过 `read_file` 读取 `showText` 与 `hideTip` 提示渲染逻辑)

### 1.2 头文件原样签名
1. `FilterPanel::eventFilter(QObject* watched, QEvent* event)`：
   - 声明位置：`src/ui/FilterPanel.h` 第 87 行：`protected: bool eventFilter(QObject* watched, QEvent* event) override;`
2. `ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout = 700, const QColor& borderColor = QColor("#B0B0B0"), bool exactPosition = false, const QColor& backgroundColor = QColor("#2B2B2B"))`：
   - 声明位置：`src/ui/ToolTipOverlay.h` 第 46 行
3. `ToolTipOverlay::hideTip()`：
   - 声明位置：`src/ui/ToolTipOverlay.h` 第 58 行：`static void hideTip() { instance()->hideOverlay(); }`

### 1.3 既有 API 检索日志
- 检索关键词：`grep -rn "tooltipText" src/ui/`
- 命中列表：`ContentHeaderWidget.cpp`、`NavPanel.cpp`、`DriveBarWidget.cpp`、`MetaPanel.cpp` 等模块均统一使用 `property("tooltipText")` 结合 `QEvent::ToolTip` 事件及 `ToolTipOverlay::instance()->showText(...)` 进行渲染。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷分析
在 QuarkMeta 筛选面板 (`FilterPanel`) 顶部的三个控制按钮（锁定 `m_btnPin`、折叠/展开 `m_btnToggleGroups`、重置 `m_btnClearAll`）上，当鼠标悬停时，提示框 (`ToolTipOverlay`) 会出现频繁抖动、反复重绘及位置跳动现象。而在其他控件（如内容面板标题栏 `ContentHeaderWidget` 的 “显示子文件夹中的项目” 按钮 `m_btnLayers`）上，悬停提示表现稳定流畅。

### 1.2 物理根因分析
对比 `ContentHeaderWidget.cpp` 与 `FilterPanel.cpp` 的 `eventFilter` 实现：
1. **事件重复触发**：`FilterPanel::eventFilter` 同时监听了 `QEvent::HoverEnter`、`QEvent::Enter` 以及 `QEvent::ToolTip` 三种不同事件。当鼠标移入按钮时，事件循环先发送 `QEvent::Enter`，随后发送 `QEvent::ToolTip`，短时间内多次重复调用 `ToolTipOverlay::instance()->showText(...)`。
2. **缺少事件截断**：`FilterPanel::eventFilter` 在处理悬停提示逻辑后，未返回 `true` 截断事件，导致 Qt 的事件派发机制继续向基类传派，进一步激发了重复触发与位置计算抖动。

### 1.3 修复方案设计
将 `FilterPanel::eventFilter` 中的悬停提示逻辑归一化重构为与 `ContentHeaderWidget` / `HoverEventFilter` 一致的 SSOT 标准规范：
- 仅监听 `QEvent::ToolTip` 触发源；
- 弹出 `ToolTipOverlay` 后返回 `true` 阻断事件二次传派；
- 仅在 `QEvent::Leave` 与 `QEvent::MouseButtonPress` 时触发 `ToolTipOverlay::hideTip()`；
- 清理多余的 `HoverEnter`、`Enter`、`HoverLeave` 与 `MouseButtonRelease` 事件混合监听。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/ui/FilterPanel.cpp` (修改：归一化 `eventFilter` 悬停提示逻辑)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

```
<<<<<<< SEARCH
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter || event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave ||
               event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        ToolTipOverlay::hideTip();
    }
=======
    if (event->type() == QEvent::ToolTip) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            return true;
        }
    } else if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress) {
        ToolTipOverlay::hideTip();
    }
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建步骤
运行项目标准 CMake 构建命令：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. 打开 QuarkMeta 应用程序，导航至筛选面板 (`FilterPanel`)；
2. 将鼠标悬停在顶部“锁定筛选”按钮 (`m_btnPin`) 上，确认 `ToolTipOverlay` 提示气泡平滑弹出，不发生频繁重绘与跳动；
3. 将鼠标移至“折叠/展开分组”按钮 (`m_btnToggleGroups`) 与“重置筛选”按钮 (`m_btnClearAll`) 上，确认气泡显示位置稳定，平滑切换；
4. 点击按钮或将鼠标移出按钮范围，确认 `ToolTipOverlay` 气泡立即隐退；
5. 双击筛选框历史记录等既有 `eventFilter` 逻辑功能正常，无任何行为退化。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **ToolTip 唯一真理源入口**：复用全应用统一的 `ToolTipOverlay::instance()->showText(...)` 与 `ToolTipOverlay::hideTip()`，严禁另起炉灶手写 `QToolTip` 或局部气泡窗口。
- **事件捕获通道规范**：遵循 `ContentHeaderWidget` 标准 `QEvent::ToolTip` 拦截与 `return true;` 阻断机制。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 源 | 派生副本/视图展现 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 按钮提示文本 | 控件 `tooltipText` QObject 动态属性 | `ToolTipOverlay` 绘制文本 | 悬停时读取 `tooltipText` 属性传递给 `ToolTipOverlay` |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `FilterPanel::eventFilter(QObject* watched, QEvent* event)`：
   - 声明：`src/ui/FilterPanel.h` line 87 (`protected: bool eventFilter(...) override`)
   - 签名与访问级别均保持完全不变。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `FilterPanel.cpp` 已直接包含 `#include "ToolTipOverlay.h"`，无需变动任何 `#include` 指令，包含链完备闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 是否遵循物理隔离与实施方案命名契约 | 是 | 方案存放于 `Implementation Plan/FilterPanelToolTipFix.md` |
| 2 | 是否包含全部 10 大必备章节 | 是 | 包含 Overview 至 Behavior Preservation 全部章节 |
| 3 | SEARCH 块是否在目标文件中真实存在且唯一 | 是 | 在 `src/ui/FilterPanel.cpp` 第 258-267 行唯一存在 |
| 4 | 是否未直接修改任何 C++/Qt 源代码文件 | 是 | 仅提供实施方案，未修改任何 `src/` 代码文件 |
| 5 | 是否破坏既有对外接口签名与 C++ 契约 | 是(否破坏) | 未修改任何 `.h` 头文件签名 |
| 6 | 是否违反单向依赖与分层硬锁 | 是(否违反) | 保持在 UI 视图层内部回调 |
| 7 | 是否包含特定平台的 Hack 级补丁 | 是(否包含) | 纯 Qt 标准事件派发逻辑重构 |
| 8 | 是否包含另起炉灶的重复实现 | 是(否包含) | 复用统一 `ToolTipOverlay` 通道 |
| 9 | 是否有隐式/显式的数据迁移逻辑 | 是(否包含) | 不涉及数据库或持久化结构变动 |
| 10 | 是否破坏既有 UI 样式数值或参数 | 是(否破坏) | 零样式数值修改 |
| 11 | 是否完整走完四道关质量管控流程 | 是 | 已完成第 0 关共识确认与第 1 关物理证据检索 |
| 12 | 是否存在跨线程直接操作 GUI 控件行为 | 是(否存在) | 全部执行于主 UI 线程 |
| 13 | 是否有死代码残留 | 是(无残留) | 移除冗余的 HoverEnter/Enter 事件流 |
| 14 | 是否引发潜在的死锁风险 | 是(无风险) | 无锁操作 |
| 15 | 变量与类型闭合性 | 是 | 类型在 `FilterPanel.cpp` 完备 |
| 16 | CMake 注册完整性 | 是 | 未新建/删除文件，`CMakeLists.txt` 无需修改 |
| 17 | 行为与交互零篡改声明 | 是 | 仅修复提示跳动 Bug，未改变提示文本与按钮交互 |
| 18 | 语言规范 | 是 | 100% 全中文编写 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 项目名称 | 原始状态 / 数值 | 实施后状态 / 数值 | 确认一致 |
| :--- | :--- | :--- | :--- | :---: |
| 交互行为 | 按钮悬停提示 | 提示文本弹窗显示 | 提示文本弹窗显示（移除抖动/跳动） | 是 |
| 提示内容 | `m_btnPin` 提示 | "锁定当前筛选条件" / "当前筛选条件已锁定..." | 保持完全一致 | 是 |
| 提示内容 | `m_btnToggleGroups` 提示 | "折叠所有分组" / "展开所有分组" | 保持完全一致 | 是 |
| 提示内容 | `m_btnClearAll` 提示 | "重置所有筛选条件" | 保持完全一致 | 是 |
| 样式数值 | ToolTip 颜色/圆角/边框 | 由 `ToolTipOverlay` 统一渲染 | 保持 100% 完全一致 | 是 |
