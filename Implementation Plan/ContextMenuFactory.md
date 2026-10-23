# Implementation Plan - ContextMenuFactory Normalization

## 1. Overview
This implementation plan unifies all context menu action creation across the application into `ContextMenuFactory`.
Specifically:
1. Create `ContextMenuFactory` (`src/ui/controllers/ContextMenuFactory.h` and `src/ui/controllers/ContextMenuFactory.cpp`) to generate standardized actions for:
   - "在“资源管理器”中显示" (`folder_search` icon, opens path in explorer)
   - "复制完整路径" (`link` icon, copies native path to clipboard with Toast feedback)
   - "复制名称" (`text` icon, copies filename to clipboard with Toast feedback)
   - "置顶" / "取消置顶" (`pin_vertical` / `pin_tilted` icons)
2. Register `ContextMenuFactory` in `CMakeLists.txt`.
3. Refactor `ContentContextMenu`, `NavPanel`, `AddressBar`, `QuickLookWindow`, and `FavoritePanel` to use `ContextMenuFactory`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/controllers/ContextMenuFactory.h` (New File)
- `src/ui/controllers/ContextMenuFactory.cpp` (New File)
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/NavPanel.cpp`
- `src/ui/AddressBar.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Add `ContextMenuFactory.h` and `ContextMenuFactory.cpp` to `SOURCES`:

<<<<<<< SEARCH
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ContentContextMenu.h
=======
    src/ui/controllers/ContentContextMenu.cpp
    src/ui/controllers/ContentContextMenu.h
    src/ui/controllers/ContextMenuFactory.h
    src/ui/controllers/ContextMenuFactory.cpp
>>>>>>> REPLACE

---

### 3.2 Create `src/ui/controllers/ContextMenuFactory.h`

```cpp
#pragma once

#include <QMenu>
#include <QAction>
#include <QStringList>
#include <functional>

namespace QuarkMeta {

class ContextMenuFactory {
public:
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};

} // namespace QuarkMeta
```

---

### 3.3 Create `src/ui/controllers/ContextMenuFactory.cpp`

```cpp
#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
#include "../StyleLibrary.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path]() {
        ShellHelper::openInExplorer(path);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList cleanPaths;
        for (const auto& p : paths) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
        QApplication::clipboard()->setText(cleanPaths.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList names;
        for (const auto& p : paths) {
            names << QFileInfo(p).fileName();
        }
        QApplication::clipboard()->setText(names.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}

QAction* ContextMenuFactory::buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver) {
    if (!menu) return nullptr;

    QIcon icon = UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18);
    QString text = isPinned ? "取消置顶" : "置顶";

    QAction* action = menu->addAction(icon, text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [isPinned, onToggle]() {
        if (onToggle) onToggle(!isPinned);
    });
    return action;
}

} // namespace QuarkMeta
```

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Test context menus in Content View, Nav Panel, Address Bar, QuickLook Window, and Favorite Panel.
4. Verify uniform wording ("在“资源管理器”中显示", "复制完整路径", "复制名称") and Toast feedback across all components.
