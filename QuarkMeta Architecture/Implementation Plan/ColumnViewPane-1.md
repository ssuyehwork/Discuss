# Implementation Plan - ColumnViewPane Blue Top-Line Design Constraint Guard

## 1. Overview（概述与解决的问题）

### 问题背景

在 `ColumnViewPane::paintEvent` 中，存在一条**蓝色顶部焦点提示线**（`#3498db`，1px）：

- **唯一合法语义**：仅用于标识"列视图（Column View）中当前活跃列（ColumnViewPane）"的焦点状态。
- **严禁挪用场景**：`ContentPanel`（内容面板/窗格）、`ContentHeaderWidget`（内容面板标题栏）或任何其他非列视图列的 Widget。

经过物理核查，当前代码层面**蓝线从未被错误应用到窗格上**，现状是正确的。但由于：
1. `ContentPanel` 也拥有独立的"活跃窗格"状态机（`setActivePane`）；
2. `ContentPaneSplitManager` 管理多窗格分屏（`splitPane`）；
3. 两者在命名上（"active pane" / "active"）与 `ColumnViewPane::m_isActive` 高度相似；

未来 AI 或开发者极易将蓝线逻辑误移植到窗格层，**本方案通过在代码中植入不可忽略的防护注释来锁定这条设计约束**，作为架构永久性红线标记。

### Architecture 3-Question Answers（架构三问）

1. **SSOT 真理源溯源**：列焦点状态唯一真理源为 `ColumnViewPane::m_isActive`，由 `ColumnViewWidget::setActivePaneIndex` 统一驱动；窗格活跃状态唯一真理源为 `ContentPaneSplitManager::setActivePane`，两者互相独立，绝对不可混用。
2. **黑盒完整性**：`ColumnViewPane` 对外只暴露 `setActive(bool)` 接口，蓝线绘制逻辑封装在 `paintEvent` 内部，外部不可访问，完全符合黑盒隔离原则。
3. **根因溯源**：无代码 Bug，纯粹的防御性设计注释植入，防止未来重构时发生语义污染。

---

## 2. Modified Files List（影响文件清单）

| 文件 | 修改类型 | 说明 |
| :--- | :--- | :--- |
| `src/ui/ColumnViewPane.cpp` | 防护注释植入 | 在 `paintEvent` 蓝线绘制处植入架构红线注释 |
| `src/ui/ColumnViewPane.h` | 防护注释植入 | 在 `setActive` / `m_isActive` 声明处植入约束注释 |

> [!IMPORTANT]
> **本方案不修改任何逻辑、参数或接口签名**，属于纯注释植入，零行为变化。

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ColumnViewPane.h` — `setActive` 接口声明处植入设计约束注释

```
<<<<<<< SEARCH
    bool isActive() const { return m_isActive; }
    void setActive(bool active);
=======
    bool isActive() const { return m_isActive; }
    // 【架构红线】setActive / m_isActive 专属于列视图（Column View）中单列（ColumnViewPane）的焦点激活状态。
    // 顶部蓝色焦点提示线（#3498db, 1px）由 paintEvent 根据此状态绘制。
    // 严禁将此接口或蓝线绘制逻辑挪用至 ContentPanel（内容面板/窗格）或
    // ContentHeaderWidget（内容面板标题栏）。窗格活跃状态由 ContentPaneSplitManager::setActivePane
    // 通过 QSS property "activePane" 独立管理，两者完全正交，绝对不可混用。
    void setActive(bool active);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ColumnViewPane.cpp` — `paintEvent` 蓝线绘制处植入设计约束注释

```
<<<<<<< SEARCH
void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    if (m_isActive) {
        painter.setPen(QPen(QColor("#3498db"), 1));
        painter.drawLine(0, 0, width(), 0);
    }
=======
void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    // 【架构红线 - 蓝色顶部焦点提示线设计约束】
    // 此处绘制的顶部蓝线（颜色 #3498db，宽度 1px）是"列视图（Column View）中活跃列"的焦点视觉标识。
    // ─────────────────────────────────────────────────────────────────────────────────
    // ✅ 唯一合法应用范围：ColumnViewPane（列视图中的单列窗格），当 m_isActive == true 时绘制。
    // ❌ 严禁应用范围：
    //    - ContentPanel（内容面板/窗格）     → 其活跃状态由 ContentPaneSplitManager::setActivePane
    //                                          通过 QSS property "activePane" 独立管理（border样式）。
    //    - ContentHeaderWidget（面板标题栏） → 其活跃状态由 ContentHeaderWidget::setActive
    //                                          通过 QSS property "activePane" 独立管理。
    //    - 任何其他非 ColumnViewPane 类型的 Widget。
    // ─────────────────────────────────────────────────────────────────────────────────
    // 两条"活跃"状态机（列焦点 vs 窗格焦点）完全正交，绝对不可混用或合并。
    if (m_isActive) {
        painter.setPen(QPen(QColor("#3498db"), 1));
        painter.drawLine(0, 0, width(), 0);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 编译命令
```bash
cmake --build build --config Release
```

### 验证步骤
1. **编译通过验证**：纯注释修改，编译必须零警告零错误通过。
2. **运行时视觉验证**：
   - 进入列视图模式（Column View），点击不同列 → 被点击列**顶部出现蓝线**，其他列无蓝线。
   - 在分屏双窗格模式下，点击不同窗格 → 窗格边框变化（`#555555`），**绝对不出现蓝线**。
   - `ContentHeaderWidget` 标题栏在任何状态下 → **绝对不出现蓝线**。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（防另起炉灶自查）

| 检查项 | 结论 |
| :--- | :--- |
| 是否新增/修改任何业务逻辑 | ❌ 否，纯注释植入，零行为变化 |
| 是否复用既有 SSOT 通道 | ✅ 不涉及任何数据流或刷新操作 |
| 是否私自调用 `loadDirectory` 另起炉灶 | ❌ 不涉及 |
| 是否破坏对外公开接口签名 | ❌ 不修改任何函数签名 |

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 类 | 物理签名（已核查 `.h` 文件） | 核查结论 |
| :--- | :--- | :--- |
| `ColumnViewPane` | `bool isActive() const` | ✅ 已验证，`ColumnViewPane.h` L26 |
| `ColumnViewPane` | `void setActive(bool active)` | ✅ 已验证，`ColumnViewPane.h` L27 |
| `ContentPanel` | `void setActivePane(bool active)` | ✅ 已验证，`ContentPanel.h` L91 |
| `ContentHeaderWidget` | `void setActive(bool active)` | ✅ 已验证，`ContentHeaderWidget.h` L26 |
| `ContentPaneSplitManager` | `void setActivePane(bool active)` | ✅ 已验证，`ContentPaneSplitManager.h` L32 |

---

> [!NOTE]
> 本方案为纯防御性注释植入，不涉及任何逻辑修改。执行者只需在对应代码位置插入注释块，无需修改 `CMakeLists.txt`（无新增 `Q_OBJECT` 类），无需其他文件变更。
