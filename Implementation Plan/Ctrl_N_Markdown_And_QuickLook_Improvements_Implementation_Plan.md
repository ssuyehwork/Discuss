# Implementation Plan: Ctrl+N 新建 Markdown 与 QuickLookWindow 双击关闭及连续预览实施方案

本实施方案旨在严格遵循工业级 C++/Qt 架构规范与项目设计准则，在 `QuarkMeta_Source` 桌面应用中完整实现：
1. **全局快捷键 `Ctrl+N` 创建 Markdown 文件**：在 `AppShortcutController` 中注册非编辑态快捷键 `Ctrl+N`，通过 `PanelMediator` 调度活跃内容面板执行 `target->createNewItem("md")`；
2. **解决 `QuickLookWindow` 空白内容无法双击关闭的交互缺陷**：为预览界面内的所有展示容器（`m_lblEmptyPrompt`、`m_container` 以及窗口自身）挂载双击关闭事件拦截，无论是否有文本内容，双击任意区域均无条件关闭预览窗口；
3. **`QuickLookWindow` 删除项目后连续平滑预览**：重构删除槽函数时序，在执行物理删除前预先计算下一个有效文件（末项时向前回溯寻找上一个有效文件），并结合 `setIgnoreDeactivate` 锁定失焦门禁，彻底解决因数据提前移除与视图刷新抢焦点导致的跳转失败或闪退问题；
4. **侧边栏历史选项卡 (`HistoryPanel`) 支持多选与批量操作**：将树视图模式配置为 `ExtendedSelection`，右键上下文菜单全量支持批量操作，修复野指针二次释放 (Double Free) 导致的闪退问题。
