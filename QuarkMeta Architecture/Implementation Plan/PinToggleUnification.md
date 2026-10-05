# Implementation Plan - Pin Toggle Execution Unification (SetPinned SSOT)

## 1. Overview
Currently, toggling the "Pin/Unpin" (置顶 / 取消置顶) state of file items is implemented in three disconnected ways across the application:
1. **Right-Click Context Menu (`buildPinToggleAction`)**: Constructs an `AppCommandType::SetPinned` command and dispatches it to `CoreEngine::instance().executeCommand(cmd)` (lines 185 and 310 of `src/ui/controllers/ContentContextMenu.cpp`).
2. **Context Menu Actions (`ActionPin` / `ActionUnpin`)**: Directly manipulates `MetadataManager::instance().setPinned(...)` and manually calls `m_panel->updateItemMetadata(p)` and `m_panel->refreshAll()` (lines 670–685 of `src/ui/controllers/ContentContextMenu.cpp`).
3. **Keyboard Shortcut (`Alt + D`)**: Directly calls `model->setData(idx, !current, IsLockedRole)` on ViewModel items (lines 297–305 of `src/ui/controllers/ContentKeyHandler.cpp`).

This fragmentation creates duplicate implementations, introduces role mismatches (`IsLockedRole` vs `PinnedRole`), and risks out-of-sync metadata state.

This implementation plan unifies all Pin/Unpin actions through `CoreEngine::instance().executeCommand(AppCommandType::SetPinned)` by introducing a static SSOT helper:
`ContextMenuFactory::togglePinState(const QStringList& paths, bool pin)`

---

## 2. Modified Files List
1. `src/ui/controllers/ContextMenuFactory.h` (Public static `togglePinState` declaration)
2. `src/ui/controllers/ContextMenuFactory.cpp` (Static `togglePinState` implementation & `buildPinToggleAction` refactoring)
3. `src/ui/controllers/ContentKeyHandler.cpp` (`Alt + D` hotkey handler refactoring to delegate to `ContextMenuFactory::togglePinState`)
4. `src/ui/controllers/ContentContextMenu.cpp` (`ActionPin` and `ActionUnpin` cases refactoring to delegate to `ContextMenuFactory::togglePinState`)

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContextMenuFactory.h`

<<<<<<< SEARCH
class ContextMenuFactory {
public:
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};
=======
class ContextMenuFactory {
public:
    /**
     * @brief 置顶/取消置顶 SSOT 统一执行入口 (支持单文件与多文件聚合广播)
     */
    static bool togglePinState(const QStringList& paths, bool pin);

    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};
>>>>>>> REPLACE

---

### File 2: `src/ui/controllers/ContextMenuFactory.cpp`

<<<<<<< SEARCH
#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
#include "../StyleLibrary.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
=======
#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
#include "../StyleLibrary.h"
#include "../../core/CoreEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
bool ContextMenuFactory::togglePinState(const QStringList& paths, bool pin) {
    if (paths.isEmpty()) return false;

    AppCommand cmd;
    cmd.type = AppCommandType::SetPinned;
    cmd.targetPaths = paths;
    cmd.params["pinned"] = pin;
    return CoreEngine::instance().executeCommand(cmd);
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
>>>>>>> REPLACE

---

### File 3: `src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        QAbstractItemModel* model = view->model();
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
                bool current = idx.data(IsLockedRole).toBool();
                model->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }
=======
    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targetPaths;
        bool anyUnpinned = false;
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) {
                    targetPaths << p;
                    if (!idx.data(PinnedRole).toBool()) anyUnpinned = true;
                }
            }
        }
        if (!targetPaths.isEmpty()) {
            ContextMenuFactory::togglePinState(targetPaths, anyUnpinned);
        }
        return true;
    }
>>>>>>> REPLACE

---

### File 4: `src/ui/controllers/ContentContextMenu.cpp`

<<<<<<< SEARCH
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (!targetPaths.isEmpty()) {
                for (const QString& p : targetPaths) {
                    MetadataManager::instance().setPinned(p.toStdWString(), pin);
                    if (m_panel) m_panel->updateItemMetadata(p);
                }
                if (m_panel) m_panel->refreshAll();
            }
            break;
        }
=======
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) {
                targetPaths << path;
            }
            if (!targetPaths.isEmpty()) {
                ContextMenuFactory::togglePinState(targetPaths, pin);
            }
            break;
        }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods
1. **HotKey `Alt + D` Test**: Select single or multiple items, press `Alt + D`. Confirm items are pinned and `CentralEventHub` publishes `AppEventType::MetadataUpdated`.
2. **Context Menu Test**: Select items, right-click, choose "置顶" or "取消置顶". Verify identical behavior and immediate UI update.
3. **SSOT Inspection**: Confirm all pin state changes flow exclusively through `ContextMenuFactory::togglePinState` -> `CoreEngine::executeCommand(SetPinned)`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - `CoreEngine::instance().executeCommand(cmd)` with `AppCommandType::SetPinned`.
  - `ContextMenuFactory::togglePinState(targetPaths, pin)`.
- **Anti-Redundancy**: Removed duplicate calls to `MetadataManager::instance().setPinned(...)` and direct model `setData(..., IsLockedRole)` calls.

---

## 6. Header API Signature Verification

| Class / Component | Function / Method Signature | Header File Path | Status |
| :--- | :--- | :--- | :--- |
| `ContextMenuFactory` | `static bool togglePinState(const QStringList& paths, bool pin)` | `src/ui/controllers/ContextMenuFactory.h` | New Addition |
| `CoreEngine` | `bool executeCommand(const AppCommand& cmd)` | `src/core/CoreEngine.h` | Verified Existing |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `src/ui/controllers/ContextMenuFactory.h`:
  - Contains `#include <QStringList>`.
- `src/ui/controllers/ContextMenuFactory.cpp`:
  - Includes `"../../core/CoreEngine.h"`.
- `src/ui/controllers/ContentKeyHandler.cpp`:
  - Includes `"ContextMenuFactory.h"`.
- `src/ui/controllers/ContentContextMenu.cpp`:
  - Includes `"ContextMenuFactory.h"`.
