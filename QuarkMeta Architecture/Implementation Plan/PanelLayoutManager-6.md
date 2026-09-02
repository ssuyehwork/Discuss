# PanelLayoutManager-6.md Implementation Plan: Tab 沉浸式单栏模式切换与焦点安全避让

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **分栏布局与沉浸状态 SSOT**：`PanelLayoutManager` 是 5 栏分栏布局显隐状态、尺寸及沉浸模式切换的唯一权威掌控者。
- **持久化配置 SSOT**：`AppConfig` 保存 `MainWindow/SplitterState` 以及进入沉浸模式前记录的 4 栏显隐状态（`MainWindow/PreImmersiveNavVisible`, `MainWindow/PreImmersiveFavoriteVisible`, `MainWindow/PreImmersiveMetaVisible`, `MainWindow/PreImmersiveFilterVisible`）。

### 1.2 封装完整性 (Encapsulation Integrity)
- `AppShortcutController` 负责局内快捷键及按键事件过滤，通过 `isEditingFocus()` 执行焦点安全判定，并通过 `toggleImmersiveRequested` 信号解耦通知外部。
- `PanelLayoutManager` 封装 `toggleImmersiveMode()`、`isImmersiveMode()`、`savePreImmersiveState()` 与 `restorePreImmersiveState()`，内部管理分栏状态与提示 Overlay 展示，不越权直接修改不相关的 UI 组件。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **根因分析**：
  1. 若直接使用 `QShortcut(QKeySequence(Qt::Key_Tab))`，Qt 快捷键系统会在全局匹配阶段吞掉 `Tab` 按键事件，导致输入框（`QLineEdit`/`QTextEdit`）无法正常响应 Tab 缩进或焦点流转；
  2. 若焦点判定逻辑不全（如仅判断 `QLineEdit` 而遗漏了 delegate 行内重命名编辑器、`QTextEdit` 备注框、`QSpinBox` 等），会导致用户在编辑文本时按 `Tab` 误触发分栏隐藏。
- **解决方案**：
  1. 在 `AppShortcutController` 中建立静态焦点安全判定函数 `isEditingFocus()`，全面覆盖 `QLineEdit`、`QTextEdit`、`QPlainTextEdit`、`QAbstractSpinBox`、可编辑 `QComboBox` 以及处于 `QAbstractItemView::EditingState` 状态的列表/网格行内编辑器；
  2. 通过精确限定条件的事件过滤器拦截无修饰键的 `Qt::Key_Tab`：当 `isEditingFocus()` 为 `true` 时，事件过滤器直接返回 `false` 放行，使 `Tab` 保持默认文本/焦点流转逻辑；当为 `false` 时，拦截 `Tab` 并发射 `toggleImmersiveRequested()` 信号触发沉浸模式切换。

---

## 2. 详细实现方案

### 2.1 `PanelLayoutManager` 增加沉浸模式逻辑
在 `src/ui/PanelLayoutManager.h` 中添加公开接口与私有辅助方法：
```cpp
public:
    bool isImmersiveMode() const;
    void toggleImmersiveMode();

private:
    void savePreImmersiveState();
    void restorePreImmersiveState();
```

在 `src/ui/PanelLayoutManager.cpp` 中实现：
- `isImmersiveMode()`：当 `NavPanel`、`FavoritePanel`、`MetaPanel`、`FilterPanel` 4 个栏区均处于隐藏状态（`isHidden() == true`）时返回 `true`，否则返回 `false`。
- `toggleImmersiveMode()`：
  - 若 `isImmersiveMode()` 为 `true`（当前已是沉浸模式）：调用 `restorePreImmersiveState()` 从 `AppConfig` 恢复之前的 4 栏显隐状态；
  - 若 `isImmersiveMode()` 为 `false`（非沉浸模式）：先调用 `savePreImmersiveState()` 将当前 4 栏显隐状态写入 `AppConfig`，随后将 4 个栏区一键隐藏（`setPanelVisible`）；
  - 调用 `saveLayoutState()` 将最新 `SplitterState` 写入 `AppConfig`；
  - 调用 `ToolTipOverlay` 提示“已进入沉浸全屏模式”或“已恢复分栏布局”。
- `setPanelVisible()`：在显示/隐藏状态变更时显式调用 `saveLayoutState()`，保证每一次分栏变动均即时持久化。

### 2.2 `AppShortcutController` 增加焦点安全判定与 `Tab` 事件拦截
在 `src/ui/AppShortcutController.h` 中添加：
```cpp
public:
    static bool isEditingFocus();

signals:
    void toggleImmersiveRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
```

在 `src/ui/AppShortcutController.cpp` 中实现：
- `isEditingFocus()` 静态函数：
  1. 获取 `QApplication::focusWidget()`；若为空返回 `false`；
  2. 检查 `focusW` 是否为 `QLineEdit` / `QTextEdit` / `QPlainTextEdit` / `QAbstractSpinBox` 或可编辑 `QComboBox`；
  3. 检查 `focusW` 的元对象继承关系；
  4. 遍历 `focusW` 的父级链，检查是否存在处于 `QAbstractItemView::EditingState` 的 `QAbstractItemView`；
  5. 满足上述任意一条即返回 `true`（安全避让），否则返回 `false`。
- 在 `AppShortcutController` 构造函数中为 `qApp` 安装事件过滤器（或监听 `m_window`），在 `eventFilter` 中拦截 `QEvent::KeyPress`：
  - 判定条件：`event->type() == QEvent::KeyPress` 且 `key == Qt::Key_Tab` 且 `modifiers == Qt::NoModifier` 且 `m_window` 为激活窗口/祖先；
  - 若 `!isEditingFocus()`：发射 `toggleImmersiveRequested()` 信号，`event->accept()` 并返回 `true`（拦截 `Tab`）；
  - 若 `isEditingFocus()`：返回 `false`（放行 `Tab`）。

### 2.3 `MainWindow` 信号连接
在 `src/ui/MainWindow.cpp` 中：
```cpp
connect(m_shortcutController, &AppShortcutController::toggleImmersiveRequested,
        m_panelLayoutManager, &PanelLayoutManager::toggleImmersiveMode);
```

---

## 3. 验证与测试计划

1. **编辑状态安全避让测试**：
   - 焦点在地址栏输入框时按 `Tab` -> 验证正常焦点移动/文本操作，**不触发**分栏隐藏。
   - 焦点在搜索栏输入框时按 `Tab` -> 验证正常焦点移动，**不触发**分栏隐藏。
   - F2 触发文件行内重命名时按 `Tab` -> 验证正常完成/切换行内编辑，**不触发**分栏隐藏。
   - 焦点在元数据面板备注 `QTextEdit` 时按 `Tab` -> 验证正常输入，**不触发**分栏隐藏。

2. **沉浸模式切换与恢复测试**：
   - 焦点处于文件列表/网格/空白区域/工具栏按钮时按 `Tab` -> 4 个周边栏区一键隐藏，中间 `ContentPanel` 独占主窗口宽度。
   - 再次按 `Tab` -> 4 个周边栏区一键恢复先前的展开状态。
   - 修改部分分栏显隐（如只显示导航与元数据栏），按 `Tab` 进入沉浸模式，再次按 `Tab` 退出沉浸模式 -> 验证精准恢复之前“只显示导航与元数据栏”的状态。

3. **持久化与重启测试**：
   - 在沉浸单栏状态下关闭应用，重新打开应用 -> 验证启动时保持沉浸单栏状态，按 `Tab` 键可成功退出沉浸模式并恢复先前的分栏布局。

---

## 4. 检查与合规声明
- 公开 API 签名冻结：无任何破坏性接口签名修改。
- 主题与 QSS 规范：不涉及 inline setStyleSheet，所有 UI 动态提示使用统一的 `ToolTipOverlay`。
