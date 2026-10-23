# Implementation Plan: ContextMenuFactory-2.md

## 1. Overview
本方案旨在彻底归一化全软件中“在资源管理器中显示” (`buildShowInExplorerAction`) 的菜单构建逻辑。
解决 `ContentContextMenu.cpp` 中空白区域右键（场景 4）与 `computer://` 根目录环境下，未统一调用 `ContextMenuFactory::buildShowInExplorerAction` 导致的独立手写、图标/文本/状态未能走 SSOT 工厂通道的问题。

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/controllers/ContentContextMenu.cpp`

```
<<<<<<< SEARCH
    // =========================================================================
    // 场景 3：点击空白处
    // =========================================================================
    else {
        if (isComputerRoot) {
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            QMenu* newMenu = menu.addMenu(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "新建...");
            UiHelper::applyMenuStyle(newMenu);
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ContentPanel::ActionNewFolder);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ContentPanel::ActionNewMd);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ContentPanel::ActionNewTxt);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);

            menu.addSeparator();
            QAction* actPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(m_panel->canPaste(currentPath));

            menu.addSeparator();
            bool isPhysicalPath = !currentPath.isEmpty() && !currentPath.contains("://") && QDir(currentPath).exists();
            QAction* actShowInExp = menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
            actShowInExp->setData(ContentPanel::ActionShowInExplorer);
            actShowInExp->setEnabled(isPhysicalPath);

            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        }
    }
=======
    // =========================================================================
    // 场景 3：点击空白处
    // =========================================================================
    else {
        if (isComputerRoot) {
            ContextMenuFactory::buildShowInExplorerAction(&menu, "computer://", m_panel);
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            QMenu* newMenu = menu.addMenu(UiHelper::getIcon("add", QColor("#EEEEEE"), 18), "新建...");
            UiHelper::applyMenuStyle(newMenu);
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ContentPanel::ActionNewFolder);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ContentPanel::ActionNewMd);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ContentPanel::ActionNewTxt);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);

            menu.addSeparator();
            QAction* actPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(m_panel->canPaste(currentPath));

            menu.addSeparator();
            bool isPhysicalPath = !currentPath.isEmpty() && !currentPath.contains("://") && QDir(currentPath).exists();
            QAction* actShowInExp = ContextMenuFactory::buildShowInExplorerAction(&menu, currentPath, m_panel);
            if (actShowInExp) {
                actShowInExp->setEnabled(isPhysicalPath);
            }

            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. 运行 CMake 构建验证项目能够成功编译：
   `cmake --build build`
2. 运行应用并右键点击空白区域（如物理目录空白处或“此电脑”根目录空白处）。
3. 验证菜单中“在“资源管理器”中显示”选项图标、文案与点击唤起系统资源管理器功能完全正常，非物理路径时置灰表现正确。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **是否复用既有 SSOT 接口**：是。统一复用了 `ContextMenuFactory::buildShowInExplorerAction(&menu, path, receiver)`。
- **是否消除另起炉灶**：是。彻底清除了 `ContentContextMenu.cpp` 中空白处右键手写 `menu.addAction(...)` 的重复构建代码。

## 6. Header API Signature Verification
- `ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver)`
  - 物理源头：`src/ui/controllers/ContextMenuFactory.h`
  - 完整物理签名：`static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);`

## 7. Header Inclusion Chain & Type Completeness Check
- `ContentContextMenu.cpp` 中已包含 `#include "ContextMenuFactory.h"`，类型定义完整，无破坏或遗漏。
