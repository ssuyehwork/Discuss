# ContentPanel 双隐式容器完整装配实施方案 (ContentPanel-2.md)

## Overview
本实施方案旨在对 `ContentPanel`（内容面板）进行物理双独立容器（Dual Containers）的完整改动：
1. **保留全量业务**：100% 完整保留右键上下文菜单构建（`onCustomContextMenuRequested`）、双击进入/打开（`onDoubleClicked`）、拖拽导入（`onPathsDropped`）、全量键盘快捷键（Ctrl+C/V/Z, Delete, F2）及高级搜索筛选状态。
2. **双轨模型与双 View 容器装配**：在 `ContentPanel` 内部构建 `m_folderContainer`（文件夹专属）与 `m_fileContainer`（文件专属），上方绑 `FolderProxyModel`，下方绑 `FileProxyModel`。
3. **选区合并与智能隐形**：`getSelectedIndexes()` 自动双向合并，当目录中无文件夹或隐藏文件夹时，上方容器高度自动归零彻底隐形。

---

## Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`

---

## Build & Verification Steps
1. 编译验证：运行 `cmake --build build --config Release`，确保项目 100% 编译链接成功。
2. 物理防混排验证：进入同时包含文件与文件夹的目录，确认文件夹全部在上方容器排布，文件必定从下方容器的第 1 行最左侧重新开始排，绝对不可能挤在同一行。
