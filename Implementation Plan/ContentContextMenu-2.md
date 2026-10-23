# Implementation Plan - ContentContextMenu-2.md

## 1. Overview
Fix the SSOT command pipeline defect for item pinning in `ContentContextMenu.cpp`:
Currently, `ContentContextMenu.cpp` calls `MetadataManager::instance().setPinned(...)` directly when toggling the pinned state of items from the context menu. This bypasses the `CoreEngine` command pipeline (`AppCommandType::SetPinned`), breaks the Undo/Redo stack integration, and fails to broadcast `AppEventType::MetadataUpdated` via `CentralEventHub` for cross-view synchronization.

This plan updates `ContentContextMenu.cpp` to execute pinning actions through `CoreEngine::instance().executeCommand(...)` with `AppCommandType::SetPinned`, restoring full Undo/Redo support and 0ms cross-view event synchronization.

---

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContentContextMenu.cpp`
In `ContentContextMenu::showMenu`, replace direct calls to `MetadataManager::instance().setPinned(...)` with `CoreEngine::instance().executeCommand(...)` using `AppCommandType::SetPinned`.

```
<<<<<<< SEARCH
            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            ContextMenuFactory::buildPinToggleAction(&menu, isPinned, [this, view](bool pin) {
                auto indexes = view->selectionModel()->selectedIndexes();
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
            }, m_panel);
=======
            bool isPinned = currentIndex.data(PinnedRole).toBool();
            ContextMenuFactory::buildPinToggleAction(&menu, isPinned, [this, view](bool pin) {
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targetPaths;
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) {
                        QString p = idx.data(PathRole).toString();
                        if (!p.isEmpty()) targetPaths << p;
                    }
                }
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetPinned;
                    cmd.targetPaths = targetPaths;
                    cmd.params["pinned"] = pin;
                    CoreEngine::instance().executeCommand(cmd);
                }
            }, m_panel);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and compile:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
2. Verification:
   - Right click any file/folder item and click "置顶" (Pin).
   - Verify that item pin status updates seamlessly across all open views/panels via `CoreEngine` event hub.
   - Verify that pressing `Ctrl+Z` (Undo) successfully reverts the pin state.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `CoreEngine::instance().executeCommand(...)` SSOT command entry point.
- Reused `AppCommandType::SetPinned` enum value.
- Reused `PinnedRole` instead of ambiguous `IsLockedRole`.

---

## 6. Header API Signature Verification
- `CoreEngine::executeCommand(const AppCommand& cmd)` -> `src/core/CoreEngine.h`
- `AppCommandType::SetPinned` -> `src/core/CoreEngine.h`
- `PinnedRole` -> `src/core/ModelContract.h`
