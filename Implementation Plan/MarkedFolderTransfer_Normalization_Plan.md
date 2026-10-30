# Implementation Plan - MarkedFolderService Transfer Normalization

## 1. Overview & Goal
This plan addresses the fragmentation and code duplication in marked folder transfer dispatches (`Alt+V` key shortcut vs drag-and-drop onto empty area in `MarkedPanel`).
By encapsulating selection resolution, empty checks, toast notifications, and sequential async waterfall dispatches into a single authoritative entry in `MarkedFolderService::executeTransferToMarked`, both key shortcuts and UI drop handlers delegate to a single unified API.

---

## 2. Affected Files
1. `src/meta/MarkedFolderService.h`
2. `src/meta/MarkedFolderService.cpp`
3. `src/ui/PanelMediator.cpp`
4. `src/ui/MarkedPanel.cpp`

---

## 3. Detailed Changes & Search/Replace Blocks

### 3.1 `src/meta/MarkedFolderService.cpp`
Enhance `executeTransferToMarked` so that when `sourcePaths` is empty, it automatically queries `PanelMediator::instance()->activeContentPanel()` for selected item paths, issuing unified toast feedback if no selection is present.

```
<<<<<<< SEARCH
bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs) {
    if (sourcePaths.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选择任何要迁移的项目", 2000, QColor("#e81123"));
        return false;
    }
=======
bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs) {
    QStringList resolvedSources = sourcePaths;
    if (resolvedSources.isEmpty()) {
        if (auto* mediator = PanelMediator::instance()) {
            if (auto* panel = mediator->activeContentPanel()) {
                resolvedSources = panel->getSelectedPaths();
            }
        }
    }

    if (resolvedSources.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选择任何要迁移的文件或文件夹", 2000, QColor("#e81123"));
        return false;
    }
>>>>>>> REPLACE
```

In `MarkedFolderService.cpp`, ensure `#include "../ui/PanelMediator.h"` and `#include "../ui/ContentPanel.h"` are included.

### 3.2 `src/ui/PanelMediator.cpp`
Simplify shortcut handler for `transferToMarkedRequested` to a single call to `MarkedFolderService::instance().executeTransferToMarked({})`.

```
<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::transferToMarkedRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                QStringList sel = target->getSelectedPaths();
                if (!sel.isEmpty()) {
                    QuarkMeta::MarkedFolderService::instance().executeTransferToMarked(sel);
                } else {
                    ToolTipOverlay::instance()->showToast("未选择任何要迁移的文件或文件夹", 2000, QColor("#e81123"));
                }
            }
        });
=======
        connect(shortcutController, &AppShortcutController::transferToMarkedRequested, this, []() {
            QuarkMeta::MarkedFolderService::instance().executeTransferToMarked({});
        });
>>>>>>> REPLACE
```

---

## 4. Verification Plan
1. Check that compiling with CMake succeeds without warnings.
2. Verify that pressing `Alt+V` without any selection produces the unified toast message.
3. Verify that dragging files to `MarkedPanel` empty space routes through `executeTransferToMarked` cleanly.
