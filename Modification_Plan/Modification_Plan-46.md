# FERREX-META 版本 Ctrl+滚轮键 联动滑杆逻辑移植 —— Modification_Plan-46.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在将卡片尺寸调节水平滑杆（`m_sizeSlider`）移植到 `MainWindow` 自定义标题栏之后，用户希望进一步将 `FERREX-META` 版本中极为稳定的 Ctrl+滚轮 联动机制移入当前版本（对应用户原话：“下一步单独将FERREX-META版本里的Ctrl+滚轮键逻辑移植到当前版本”）。按住 Ctrl 并滚动滚轮时，将不再直接调整行高，而是统一调整顶部滑杆的 value，从而实现集中控制和完美的线性无缝调节。

## 2. 问题定位
我们需要打通 `ContentPanel` 事件过滤器/原生事件 与 `MainWindow` 中 `m_sizeSlider` 的连接。
1. **内容面板事件过滤器**：`src/ui/ContentPanel.cpp` 的 `ContentPanel::eventFilter`。
2. **内容面板原生滚轮事件**：`src/ui/ContentPanel.cpp` 的 `ContentPanel::wheelEvent`。
3. **滑杆控制接口**：需要在 `MainWindow` 中提供一个公共方法或公开成员，方便 `ContentPanel` 递增/递减滑杆数值。

通过调用该方法，Ctrl+滚轮在被消费后，安全地控制滑杆每次变化 ±10 像素。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 将FERREX-META版本里的Ctrl+滚轮键逻辑移植到当前版本 | 捕获 `QEvent::Wheel` 的 `Qt::ControlModifier`。向上滚动，使滑杆的当前值增加 10；向下滚动，使滑杆当前值减少 10。 | ✅ |

## 4. 详细解决方案

### 4.1 在 `MainWindow.h` 中提供修改滑杆值的接口
在 `MainWindow.h` 的 `public` 作用域内，公开一个方法以便 `ContentPanel` 调用：
```cpp
// src/ui/MainWindow.h
public:
    void stepSizeSlider(int direction) {
        if (m_sizeSlider) {
            int step = 10;
            int newVal = m_sizeSlider->value() + (direction > 0 ? step : -step);
            m_sizeSlider->setValue(qBound(m_sizeSlider->minimum(), newVal, m_sizeSlider->maximum()));
        }
    }
```

### 4.2 拦截并转发 `ContentPanel::eventFilter` 中的滚轮事件
在 `src/ui/ContentPanel.cpp` 的 `ContentPanel::eventFilter` 函数中，对属于视图或其 viewport 的 `QEvent::Wheel` 事件进行物理拦截：
```cpp
    if ((obj == m_gridView || obj == m_gridView->viewport() || obj == m_treeView || obj == m_treeView->viewport()) && event->type() == QEvent::Wheel) {
        QWheelEvent* wEvent = reinterpret_cast<QWheelEvent*>(event);
        if (wEvent->modifiers() & Qt::ControlModifier) {
            int delta = wEvent->angleDelta().y();
            // 向上滚动为正值，向下为负值
            if (auto* mainWin = qobject_cast<MainWindow*>(window())) {
                mainWin->stepSizeSlider(delta > 0 ? 1 : -1);
            }
            return true; // 拦截并消费该事件，不继续向底层滚动条派发
        }
    }
```

### 4.3 修改 `ContentPanel::wheelEvent`
在 `src/ui/ContentPanel.cpp` 的 `ContentPanel::wheelEvent(QWheelEvent* event)` 函数中，同样加入对 Ctrl+滚轮的拦截：
```cpp
void ContentPanel::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int delta = event->angleDelta().y();
        if (auto* mainWin = qobject_cast<MainWindow*>(window())) {
            mainWin->stepSizeSlider(delta > 0 ? 1 : -1);
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}
```

通过这一重构，双向滚轮输入全部分流到 `stepSizeSlider` 方法，统一推动滑杆改变，避免了多套缩放机制的交叉感染，达到极简和稳固的效果。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/MainWindow.h` 声明 `stepSizeSlider` 接口。
- [ ] 模块/文件：`src/ui/ContentPanel.cpp` 的 `eventFilter` 与 `wheelEvent`，捕获 `Qt::ControlModifier` 并控制滑杆递增。

**明确禁止越界修改的范围：**
- [ ] MFT/USN 监控扫描底层 —— 严禁触碰
- [ ] 各种代理模型的过滤策略、数据库处理链 —— 严禁触碰

## 6. 实现准则与预警【核心】
1. **防止跨类型访问错误**：`ContentPanel.cpp` 需要包含主窗口头文件 `#include "MainWindow.h"` 才能成功通过 `qobject_cast<MainWindow*>(window())` 的编译。
2. **防死循环**：滑杆的 valueChanged 会触发 `ContentPanel::setZoomLevel` 从而调整行高/卡片大小并调用 `updateGridSize()`。在这个链条里，滚轮仅仅是个“触发源”（发射改变命令），不会存在任何递归。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 窗口置顶 | 一律使用 Win32 原生 SetWindowPos（HWND_TOPMOST / HWND_NOTOPMOST），严禁使用 setWindowFlag(Qt::WindowStaysOnTopHint) 或任何导致窗口重建的操作，调用时必须配合 SWP_NOSENDCHANGING 标志 | ✅（本方案不修改置顶逻辑） |
| 输入框清除功能 | 一律使用 Qt 原生 setClearButtonEnabled(true)，严禁通过 addAction、手动绘图或自定义按钮模拟清除逻辑 | ✅（本方案不涉及输入框） |

## 8. 待确认事项（可选）
（无）
