# ContentPanel 双隐式容器架构实施方案 (ContentPanel-1.md)

## Overview
本实施方案旨在对 `ContentPanel` 内容区域进行架构升级，建立“物理双独立容器 (Dual Containers)”架构：
1. **数据源物理双轨分流**：建立 `FolderProxyModel`（专责 `isDir == true`）与 `FileProxyModel`（专责 `isDir == false`），从数据模型层 100% 物理阻断文件与文件夹的混杂可能。
2. **上下独立隐式容器装配**：在 `ContentPanel` 中构建“上方文件夹容器（高度自适应）”与“下方文件容器（撑满剩余空间）”，无文件夹或隐藏文件夹时上方容器高度自动为 0 彻底隐形。
3. **独立置顶与统一排序**：文件夹在上方容器独立置顶，文件在下方容器独立置顶，互不跨界；排序指令全局统一广播，双容器各自在组内独立排序。
4. **选区与接口 100% 向后兼容**：`getSelectedIndexes()` 自动合并双容器选中项，对外信号（`selectionChanged`、`requestQuickLook`、`directorySelected`）保持 100% 契约不变。

---

## Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
```cpp
// 在 ContentPanel.h 中引入 FolderProxyModel 与 FileProxyModel，声明双独立容器与双代理模型：
#include "models/FolderProxyModel.h"
#include "models/FileProxyModel.h"

// 替换原有的单个 m_proxyModel 与 m_viewStack，声明双容器组件：
QWidget* m_folderContainer = nullptr;
QAbstractItemView* m_folderGridView = nullptr;
QTreeView* m_folderTreeView = nullptr;
QStackedWidget* m_folderViewStack = nullptr;

QWidget* m_fileContainer = nullptr;
QAbstractItemView* m_fileGridView = nullptr;
QTreeView* m_fileTreeView = nullptr;
QStackedWidget* m_fileViewStack = nullptr;

FolderProxyModel* m_folderProxyModel = nullptr;
FileProxyModel*   m_fileProxyModel = nullptr;
```

---

## Build & Verification Steps
1. 编译验证：运行 `cmake --build build --config Release`，确保无编译与链接错误。
2. 逻辑验证：打开包含文件夹与文件的目录，确认上方文件夹排完后，下方文件必定从第一行最左侧重新开始，绝不混排同一行。
