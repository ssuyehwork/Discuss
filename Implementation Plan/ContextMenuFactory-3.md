# Implementation Plan: ContextMenuFactory-3.md

## 1. Overview
本方案旨在彻底归一化全软件中“复制完整路径” (`buildCopyPathAction`) 与“复制名称” (`buildCopyNameAction`) 的菜单项构建规范：
1. **NavPanel.cpp**：侧边栏目录树右键菜单在原有的 `buildCopyPathAction` 基础之上，补齐 `buildCopyNameAction`，保持路径/名称复制能力完整；
2. **AddressBar.cpp**：面包屑导航右键菜单在原有的 `buildCopyPathAction` 基础之上，补齐 `buildCopyNameAction`，确保与导航场景一致；
3. **QuickLookWindow.cpp**：核查与确认为 `m_currentPath` 规范调用 `buildCopyNameAction` 与 `buildCopyPathAction`。

## 2. Modified Files List
- `src/ui/NavPanel.cpp`
- `src/ui/AddressBar.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/NavPanel.cpp`

```
<<<<<<< SEARCH
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{path}, this);

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
=======
    FavoriteService::instance().buildFavoriteAction(&menu, path, this);
    ContextMenuFactory::buildCopyNameAction(&menu, QStringList{path}, this);
    ContextMenuFactory::buildCopyPathAction(&menu, QStringList{path}, this);

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
>>>>>>> REPLACE
```

### `src/ui/AddressBar.cpp`

```
<<<<<<< SEARCH
        FavoriteService::instance().buildFavoriteAction(&menu, nativePath, this);
        ContextMenuFactory::buildCopyPathAction(&menu, QStringList{nativePath}, this);

        menu.exec(globalPos);
=======
        FavoriteService::instance().buildFavoriteAction(&menu, nativePath, this);
        ContextMenuFactory::buildCopyNameAction(&menu, QStringList{nativePath}, this);
        ContextMenuFactory::buildCopyPathAction(&menu, QStringList{nativePath}, this);

        menu.exec(globalPos);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 构建：
   `cmake --build build`
2. 运行应用，右键侧边栏（NavPanel）任意文件夹项目，验证是否同时包含“复制名称”与“复制完整路径”，且点击后粘贴板内容符合预期。
3. 右键地址栏面包屑节点，验证“复制名称”与“复制完整路径”菜单节点显示与功能无误。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **是否复用既有 SSOT 接口**：是。统一复用了 `ContextMenuFactory::buildCopyNameAction` 与 `ContextMenuFactory::buildCopyPathAction`。
- **是否消除另起炉灶**：是。杜绝了各模块对路径/名称复制动作的不一致构建与缺少选项问题。

## 6. Header API Signature Verification
- `ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver)`
  - 物理源头：`src/ui/controllers/ContextMenuFactory.h`
  - 完整物理签名：`static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);`
- `ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver)`
  - 物理源头：`src/ui/controllers/ContextMenuFactory.h`
  - 完整物理签名：`static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);`

## 7. Header Inclusion Chain & Type Completeness Check
- `NavPanel.cpp` 和 `AddressBar.cpp` 中均已包含 `#include "controllers/ContextMenuFactory.h"`，头文件包含链闭合且类型定义完整。
