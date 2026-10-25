# 实施方案：FilterPanel 顶部按钮 ToolTipOverlay 悬停提示修复方案

## 1. 概述
在 QuarkMeta 的筛选面板 (`FilterPanel`) 顶部，包含了三个控制按钮：
- **② 锁定按钮 (`m_btnPin`)**：锁定/解锁当前筛选条件（目录切换不重置）。
- **③ 折叠/展开按钮 (`m_btnToggleGroups`)**：一键折叠或展开所有筛选分组。
- **④ 重置按钮 (`m_btnClearAll`)**：重置所有筛选条件。

目前这三个按钮虽然均通过 `setProperty("tooltipText", ...)` 设置了提示文本，并在事件过滤器中安装了 `installEventFilter(this)`，但在 `FilterPanel::eventFilter` 处理 `QEvent::HoverEnter` 事件时，原本的代码分支为空，未能调用 `ToolTipOverlay` 显示浮动提示，导致鼠标悬停时没有任何视觉提示反馈。

本方案在 `FilterPanel::eventFilter` 的悬停与进入事件中补充对 `ToolTipOverlay::instance()->showText(...)` 的调用，并在鼠标离开或点击时通过 `ToolTipOverlay::hideTip()` 及时隐藏提示，恢复符合 QuarkMeta 设计规范的悬停提示交互。

---

## 2. 修改文件列表
- `src/ui/FilterPanel.cpp`

---

## 3. 逐文件详细修改点

### 3.1 `src/ui/FilterPanel.cpp`

在 `FilterPanel::eventFilter` 方法中：
1. 将事件匹配扩展为 `QEvent::HoverEnter` 与 `QEvent::Enter`。
2. 在获取非空 `tooltipText` 属性后，调用 `ToolTipOverlay::instance()->showText(QCursor::pos(), text, 700)` 触发全局浮动提示。
3. 补充在 `QEvent::HoverLeave`、`QEvent::Leave`、`QEvent::MouseButtonPress` 以及 `QEvent::MouseButtonRelease` 时调用 `ToolTipOverlay::hideTip()`，保证提示的实时隐匿与干净清理。

```line
<<<<<<< SEARCH
    if (event->type() == QEvent::HoverEnter) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::MouseButtonRelease) {
        ToolTipOverlay::hideTip();
    }
=======
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
        QString text = watched->property("tooltipText").toString();
        if (!text.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, 700);
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave ||
               event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        ToolTipOverlay::hideTip();
    }
>>>>>>> REPLACE
```

---

## 4. 构建与验证步骤

1. **编译代码**：
   在 Qt Creator 或命令行环境中构建 QuarkMeta 项目，确认 `FilterPanel.cpp` 编译无误。
2. **测试场景 1（按钮 ② 锁定按钮悬停测试）**：
   - 鼠标悬停在筛选面板顶部的“钉子”图标按钮（按钮 ②）上。
   - 验证是否在 700ms 后出现 `ToolTipOverlay` 提示文本：“锁定当前筛选条件”。
   - 点击该按钮，再次悬停，验证提示文本是否动态更新为：“当前筛选条件已锁定（目录切换不重置）”。
3. **测试场景 2（按钮 ③ 折叠/展开按钮悬停测试）**：
   - 鼠标悬停在“双箭头”图标按钮（按钮 ③）上。
   - 验证提示文本显示“折叠所有分组”；点击切换后，再次悬停验证提示显示“展开所有分组”。
4. **测试场景 3（按钮 ④ 重置按钮悬停测试）**：
   - 鼠标悬停在“重置漏斗”图标按钮（按钮 ④）上。
   - 验证提示文本显示“重置所有筛选条件”。
5. **测试场景 4（鼠标离开与点击隐匿测试）**：
   - 当提示浮现时，移开鼠标或点击按钮，验证 ToolTipOverlay 是否立即干净隐藏，无遗留残影。

---

## 5. 官方 API 重用与防冗余声明
- 重用单例 `ToolTipOverlay::instance()->showText(QCursor::pos(), text, timeout)` 与 `ToolTipOverlay::hideTip()`，无任何新增或重复的 UI 提示类。

---

## 6. 头文件 API 签名核验
本次修改仅调整 `FilterPanel.cpp` 中的事件处理逻辑，未对 `FilterPanel.h` 或任何公开 API 接口签名做出更改。

---

## 7. 头文件包含链与类型完整性检查
- `FilterPanel.cpp` 第 19 行已包含 `#include "ToolTipOverlay.h"`，API 与类型均已完整就绪。

---

## 8. 数据 SSOT 声明表

| 数据项 | SSOT 源头 | 修改前处理方式 | 修改后处理方式 |
| :--- | :--- | :--- | :--- |
| 按钮悬停提示文本 | 按钮 QObject 属性 `"tooltipText"` | 仅存取属性，事件响应为空 | 读取 `"tooltipText"` 并透传给 `ToolTipOverlay::showText` 显示 |

---

## 9. 合规性自检表

| 检查项 | 状态 | 说明 |
| :--- | :--- | :--- |
| 防补丁贴布 (Anti-patching) | 通过 | 修复原本未完成的事件分支，实现与规范统一的 UI 反馈 |
| 线程安全 (Thread Safety) | 通过 | 属于主 UI 线程中的纯事件处理与控件重绘操作 |
| 死锁风险 (Deadlock Safety) | 通过 | 不涉及锁操作 |
| 逻辑隔离 (Logic Isolation) | 通过 | 不改动任何筛选核心算法与业务逻辑 |

---

## 10. 行为冻结声明表

| 原有行为 | 修改后行为 |
| :--- | :--- |
| 鼠标悬停在筛选面板顶部控制按钮上时无提示 | 鼠标悬停时弹出 `ToolTipOverlay` 提示，并在移开/点击时及时隐藏 |
