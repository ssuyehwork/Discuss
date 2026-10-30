# Implementation Plan - 快速预览 QuickLook 右键菜单【打开方式】归一化重构方案

## 1. 方案概述 (Overview)
本方案旨在解决快速预览窗口（`QuickLookWindow`）右键上下文菜单中“打开方式”选项未生成级联子菜单的离散另起炉灶问题。
重构将物理删除 `QuickLookWindow.cpp` 中单项 `addAction` 硬编码的“打开方式”动作，全面归一化委托给全软件统一的 `ContextMenuFactory::buildOpenWithMenu(&menu, m_currentPath, this)` 工厂函数，生成包含系统关联软件、选择其他应用等完整的级联子菜单。

---

## 2. 问题分析与根因溯源 (Analysis & Root Cause)
1. **离散另起炉灶**：`QuickLookWindow.cpp` 中（第 478 行）手写了 `QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");`，直接生成了一个没有子菜单的叶子 Action，并在点击事件响应中仅调用了 `QDesktopServices::openUrl`。
2. **违背 SSOT 通道**：`ContextMenuFactory::buildOpenWithMenu` 已经提供了全软件统一的“打开方式”级联子菜单构建逻辑（含 `ShellHelper::getAssociatedApps` 系统关联应用列表与“选择其他应用...”），`QuickLookWindow` 绕过该通道独自实现导致视觉与功能严重脱节。

---

## 3. 修改文件范围 (Scope of Changes)

### 核心代码文件
- `src/ui/QuickLookWindow.cpp`：物理替换单项 `actOpenDefault` 的创建与触发分支，统一改用 `ContextMenuFactory::buildOpenWithMenu(&menu, m_currentPath, this)` 构建标准级联子菜单。

---

## 4. 架构契约与规范核查 (Architecture Checks)
- **【契约锁】SSOT 唯一真理源**：所有右键“打开方式”级联菜单的构建统一委托给 `ContextMenuFactory::buildOpenWithMenu`，绝对禁止私建单项 Action。
- **【依赖锁】单向依赖**：`QuickLookWindow` 作为视图/窗口层，调用控制层的 `ContextMenuFactory`，符合 Clean Architecture 单向依赖。
- **【门禁锁】中性色契约**：级联菜单图标统一使用 `#EEEEEE` 中性单色。

---

## 5. 详细实现方案 (Implementation Details)

### 5.1 归一化替换逻辑
在 `QuickLookWindow::showContextMenu` 中：
```cpp
// 替换前：
QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");

// 替换后：
ContextMenuFactory::buildOpenWithMenu(&menu, m_currentPath, this);
```
并在 Action 选中处理逻辑中移除对 `actOpenDefault` 的分支判断。

---

## 6. 代码修改 Blocks (Search & Replace Blocks)

### Block 1: `src/ui/QuickLookWindow.cpp`
<<<<<<< SEARCH
    QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "打开方式");
    ContextMenuFactory::buildShowInExplorerAction(&menu, m_currentPath, this);
    menu.addSeparator();
=======
    ContextMenuFactory::buildOpenWithMenu(&menu, m_currentPath, this);
    ContextMenuFactory::buildShowInExplorerAction(&menu, m_currentPath, this);
    menu.addSeparator();
>>>>>>> REPLACE

---

### Block 2: `src/ui/QuickLookWindow.cpp`
<<<<<<< SEARCH
    } else if (selected == actFit) {
        m_graphicsView->fitImage();
    } else if (selected == actOpenDefault) {
        HistoryService::instance().recordFileAccess(m_currentPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentPath));
    } else if (selected == actCopy) {
=======
    } else if (selected == actFit) {
        m_graphicsView->fitImage();
    } else if (selected == actCopy) {
>>>>>>> REPLACE

---

## 7. 三道关自检与验证步骤 (Verification)

### 1. 第一道关：静态编译核查
确保修改后无编译语法错误，无未定义变量引用。

### 2. 第二道关：自检脚本验证
使用方案自检脚本 `tools/plan_verify/verify_plan.py` 验证本文件的完整性：
```bash
python3 tools/plan_verify/verify_plan.py "Implementation Plan/QuickLook_OpenWith_Normalization_Plan.md"
```

---

## 8. 总结 (Summary)
本实施方案彻底消除了快速预览窗口右键菜单中“打开方式”离散单项动作的缺陷，将其全面归一化回流至 `ContextMenuFactory::buildOpenWithMenu` SSOT 通道，完全满足工业级架构治理规范。
