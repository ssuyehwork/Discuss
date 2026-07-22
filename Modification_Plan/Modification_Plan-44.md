# 物理根除内容面板 Ctrl+滚轮 自动切换视图及缩放的交互逻辑 —— Modification_Plan-44.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在项目运行与启动排查过程中，发现内容面板（ContentPanel）中原有的 Ctrl+滚轮 自动切换列表/卡片视图及连续缩放逻辑存在较多复杂的视图升降级跳转。为了提升程序的启动与运行稳定性，并净化交互行为，需要将当前版本中的 Ctrl+滚轮 自动缩放和视图跳转代码进行物理根除（对应用户原话：“先将当前版本的 Ctrl+滚轮键 缩放/切换视图逻辑彻底根除”）。

## 2. 问题定位
Ctrl+滚轮的事件分发与拦截控制分散在以下两个位置：
1. **内容面板事件过滤器拦截**：`src/ui/ContentPanel.cpp` 中的 `ContentPanel::eventFilter`。在该过滤器中，捕获了 `m_gridView`/`m_treeView` 及其 `viewport` 上的 `QEvent::Wheel` 事件。当包含 `Qt::ControlModifier` 键盘修饰符时，会直接计算 `angleDelta().y()` 并修改 `m_zoomLevel`、调用 `setViewMode` 自主切换视图，且返回 `true` 消费掉该事件。
2. **内容面板原生滚轮事件**：`src/ui/ContentPanel.cpp` 中的 `ContentPanel::wheelEvent`。在控件本身收到滚轮事件时，如果携带 `Qt::ControlModifier`，会进行同样的 `m_zoomLevel` 与 `setViewMode` 复杂分流处理。

这两处拦截代码导致了非预期的卡片无级缩放和视图强制降级/升级。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 将当前版本的 Ctrl+滚轮键 缩放/切换视图逻辑彻底根除 | 物理删除 `ContentPanel::eventFilter` 中拦截 `QEvent::Wheel` 且包含 `Qt::ControlModifier` 的代码块，不再消耗该事件。 | ✅ |
| 2    | 彻底将当前版本内容面板中臃肿复杂的 Ctrl+滚轮键 处理逻辑完全删除、物理根除 | 物理删除 `ContentPanel::wheelEvent` 中包含 `Qt::ControlModifier` 的判断和复杂分流代码，使其仅调用基类的原生滚轮事件。 | ✅ |

## 4. 详细解决方案
本方案不涉及添加任何新组件，仅对指定范围内的拦截代码进行物理删除与退化还原。

### 4.1 修改 `ContentPanel::eventFilter`
定位到以下代码块：
```cpp
    if ((obj == m_gridView || obj == m_gridView->viewport() || obj == m_treeView || obj == m_treeView->viewport()) && event->type() == QEvent::Wheel) {
        QWheelEvent* wEvent = reinterpret_cast<QWheelEvent*>(event);
        if (wEvent->modifiers() & Qt::ControlModifier) {
            // ... 内部的缩放和 setViewMode 切换逻辑 ...
            return true;
        }
    }
```
将其中的 `if (wEvent->modifiers() & Qt::ControlModifier)` 这一层包裹判断及其内部所有缩放、设置模式代码完全移除，使 `QEvent::Wheel` 在被 Ctrl 修饰时不作任何拦截，直接流转到后续默认的分发流程中。

### 4.2 修改 `ContentPanel::wheelEvent`
定位到 `ContentPanel::wheelEvent(QWheelEvent* event)` 成员函数：
```cpp
void ContentPanel::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // ... 内部复杂的 zoomLevel 和 setViewMode 处理 ...
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}
```
将其中的 `if (event->modifiers() & Qt::ControlModifier)` 整个分支完全移除，函数体直接退化为仅调用基类事件：
```cpp
void ContentPanel::wheelEvent(QWheelEvent* event) {
    QWidget::wheelEvent(event);
}
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.cpp` 中的 `ContentPanel::eventFilter` 函数（涉及行号约 1436-1485）与 `ContentPanel::wheelEvent` 函数（涉及行号约 1686-1722）

**明确禁止越界修改的范围：**
- [ ] 核心模型层：`FerrexVirtualDbModel` —— 不修改
- [ ] 顶层视图及代理：`GridItemDelegate` 与 `ThumbnailDelegate` —— 不修改
- [ ] 筛选逻辑：`FilterState` 及其关联过滤链 —— 不修改

## 6. 实现准则与预警【核心】
1. **依赖核对**：物理删除过程中无需引入任何新的头文件。需保留现有的 `<QWheelEvent>` 以确保基类方法重写能够正常编译。
2. **安全还原**：在删除 `eventFilter` 里的 `Qt::ControlModifier` 拦截判断时，必须注意保留外层的 `if (event->type() == QEvent::KeyPress)` 等其他重要交互拦截逻辑，避免误删相邻代码。
3. **开箱即用**：修改后的代码保证没有任何悬空的变量或未闭合的括号，编译验证必须一次通过。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 输入框清除功能 | 一律使用 Qt 原生 setClearButtonEnabled(true)，严禁通过 addAction、手动绘图或自定义按钮模拟清除逻辑 | ✅（本方案不新增或修改输入框） |
| 窗口置顶 | 一律使用 Win32 原生 SetWindowPos（HWND_TOPMOST / HWND_NOTOPMOST），严禁使用 setWindowFlag(Qt::WindowStaysOnTopHint) 或任何导致窗口重建的操作，调用时必须配合 SWP_NOSENDCHANGING 标志 | ✅（本方案不涉及窗口置顶功能） |
| 标题栏按钮样式 | 悬停：#3E3E42（Style::HoverBackground），按下：#4E4E52（Style::PressedBackground），严禁使用 rgba 蒙版 | ✅（本方案不新增标题栏按钮） |

## 8. 待确认事项（可选）
（无）
