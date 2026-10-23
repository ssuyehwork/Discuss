# Implementation Plan: PinAndFavoriteNormalization.md

## 1. Overview
本方案针对【置顶 / 取消置顶】与【收藏 / 取消收藏】在 `FavoritePanel.cpp` 中的归一化复用进行重构：
1. `FavoritePanel.cpp` 中非虚拟分类项目（即真实路径项目）右键菜单原先手写 `QAction* removeAct = menu.addAction(...)` 节点并硬编码“取消收藏”，现统一归一化为调用 `FavoriteService::instance().buildFavoriteAction(&menu, path, this)` SSOT 构建通道；
2. 对于虚拟分类（`isVirtual`），保留专属的 `removeFavoriteById(nodeId)` 分类删除动作。

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    // 4. 删除 / 取消收藏
    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), isVirtual ? "删除" : "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, nodeId, isVirtual, &isItemRemoved]() {
        isItemRemoved = true;
        if (isVirtual) {
            FavoriteService::instance().removeFavoriteById(nodeId);
        } else {
            removeFavoriteItem(path);
        }
    });
=======
    // 4. 删除 / 取消收藏
    if (isVirtual) {
        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除");
        connect(removeAct, &QAction::triggered, this, [this, nodeId, &isItemRemoved]() {
            isItemRemoved = true;
            FavoriteService::instance().removeFavoriteById(nodeId);
        });
    } else {
        QAction* favAct = FavoriteService::instance().buildFavoriteAction(&menu, path, this);
        if (favAct) {
            connect(favAct, &QAction::triggered, this, [&isItemRemoved]() {
                isItemRemoved = true;
            });
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 构建验证：
   `cmake --build build`
2. 运行应用，右键侧边栏收藏夹（FavoritePanel）中的真实文件夹项目，验证菜单项是否正确显示“从收藏夹移除”或“添加至收藏夹”及其对应图标，点击后触发全局信号同步刷新。
3. 右键虚拟分类文件夹，验证“删除”选项正常保留并有效。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **是否复用既有 SSOT 接口**：是。统一复用了 `FavoriteService::instance().buildFavoriteAction`。
- **是否消除另起炉灶**：是。彻底清除了 `FavoritePanel.cpp` 中针对真实文件/文件夹路径手写“取消收藏”Action 节点的重复代码。

## 6. Header API Signature Verification
- `FavoriteService::buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver)`
  - 物理源头：`src/meta/FavoriteService.h`
  - 完整物理签名：`QAction* buildFavoriteAction(QMenu* parentMenu, const QString& path, QObject* receiver = nullptr);`

## 7. Header Inclusion Chain & Type Completeness Check
- `FavoritePanel.cpp` 中已包含 `#include "../meta/FavoriteService.h"`，类型完整无误。
