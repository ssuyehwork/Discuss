# QuarkMeta 内容区双隐式容器架构实施方案 (Dual Implicit Containers)

## Overview
本实施方案旨在对 `ContentPanel` 内容区域进行架构升级，建立“双隐式容器 (Dual Implicit Containers)”架构：
1. **数据源物理双轨分流**：建立 `FolderProxyModel`（专责 `isDir == true`）与 `FileProxyModel`（专责 `isDir == false`），从数据模型层 100% 物理阻断文件与文件夹的混杂可能。
2. **上下独立隐式容器装配**：在 `ContentPanel` 中构建“上方文件夹容器（高度自适应）”与“下方文件容器（撑满剩余空间）”，无文件夹或隐藏文件夹时上方容器高度自动为 0 彻底隐形。
3. **独立置顶与统一排序**：文件夹在上方容器独立置顶，文件在下方容器独立置顶，互不跨界；排序指令全局统一广播，双容器各自在组内独立排序。
4. **选区与接口 100% 向后兼容**：`getSelectedIndexes()` 自动合并双容器选中项，对外信号（`selectionChanged`、`requestQuickLook`、`directorySelected`）保持 100% 契约不变。

---

## Modified Files List
1. `src/ui/models/FolderProxyModel.h` (新建)
2. `src/ui/models/FolderProxyModel.cpp` (新建)
3. `src/ui/models/FileProxyModel.h` (新建)
4. `src/ui/models/FileProxyModel.cpp` (新建)
5. `src/ui/ContentPanel.h`
6. `src/ui/ContentPanel.cpp`
7. `CMakeLists.txt`

---

## Detailed Line-by-Line Changes

### 1. `CMakeLists.txt` 注册
```cmake
set(SOURCES
    # ...
    src/ui/models/FolderProxyModel.h
    src/ui/models/FolderProxyModel.cpp
    src/ui/models/FileProxyModel.h
    src/ui/models/FileProxyModel.cpp
    # ...
)
```

## Build & Verification Steps

1. **构建工程**：
   在 MSVC 编译环境下运行：
   ```bash
   cmake --build build --config Release
   ```
2. **功能验证**：
   - 切换到含有文件夹和文件的普通目录，确认上方平铺显示文件夹，下方独立平铺显示文件。
   - 切换到无文件夹的纯文件目录，确认上方文件夹容器高度自动收缩为 0 彻底隐形。
   - 测试文件或文件夹置顶（pinned/encrypted），确认文件夹置顶仅作用于上方容器，文件置顶仅作用于下方容器，互不跨界干扰。
