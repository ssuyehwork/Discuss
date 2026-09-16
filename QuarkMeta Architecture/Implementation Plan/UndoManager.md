# Implementation Plan - Undo / Redo Architecture Normalization and Unified UI Refresh

## 1. Overview
When executing Undo or Redo operations (via Ctrl+Z / Ctrl+Y or the 7-second UndoToastOverlay button), state rollbacks (e.g., metadata, file movements, batch operations) occurred without a guaranteed UI refresh, leaving `ContentPanel` and `MetaPanel` showing stale information.

This implementation plan normalizes the Undo/Redo architecture by:
1. Introducing `AppEventType::UndoRedoPerformed` to `CentralEventHub`.
2. Publishing an `UndoRedoPerformed` event from `UndoManager::undo()` and `UndoManager::redo()`.
3. Listening to `UndoRedoPerformed` in `PanelMediator` to trigger `ContentPanel::refreshAll()`, ensuring in-place UI updates according to SSOT guidelines.

## 2. Modified Files List
- `src/core/CentralEventHub.h`
- `src/core/UndoManager.h`
- `src/ui/PanelMediator.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/core/CentralEventHub.h`
```diff
<<<<<<< SEARCH
    FavoritesUpdated,       // 收藏夹状态变更
    ItemsDeleted,           // 文件物理擦除/删除
    ItemsRenamed,           // 文件批量或单项重命名
    FilterStateChanged      // 条件筛选状态变更
};
=======
    FavoritesUpdated,       // 收藏夹状态变更
    ItemsDeleted,           // 文件物理擦除/删除
    ItemsRenamed,           // 文件批量或单项重命名
    FilterStateChanged,     // 条件筛选状态变更
    UndoRedoPerformed       // 撤销/重做事务完成
};
>>>>>>> REPLACE
```

### File: `src/core/UndoManager.h`
```diff
<<<<<<< SEARCH
#include "ActionCommand.h"
#include <deque>
#include <memory>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>
=======
#include "ActionCommand.h"
#include "CentralEventHub.h"
#include <deque>
#include <memory>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    void undo() {
        QMutexLocker lock(&m_mutex);
        if (m_undoStack.empty()) return;

        auto command = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        command->undo();
        m_redoStack.push_back(std::move(command));

        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(true);
    }

    void redo() {
        QMutexLocker lock(&m_mutex);
        if (m_redoStack.empty()) return;

        auto command = std::move(m_redoStack.back());
        m_redoStack.pop_back();

        command->redo();
        m_undoStack.push_back(std::move(command));

        emit canUndoChanged(true);
        emit canRedoChanged(!m_redoStack.empty());
    }
=======
    void undo() {
        QMutexLocker lock(&m_mutex);
        if (m_undoStack.empty()) return;

        auto command = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        command->undo();
        m_redoStack.push_back(std::move(command));

        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(true);

        CentralEventHub::instance().publishEvent({AppEventType::UndoRedoPerformed, "", {}, {}});
    }

    void redo() {
        QMutexLocker lock(&m_mutex);
        if (m_redoStack.empty()) return;

        auto command = std::move(m_redoStack.back());
        m_redoStack.pop_back();

        command->redo();
        m_undoStack.push_back(std::move(command));

        emit canUndoChanged(true);
        emit canRedoChanged(!m_redoStack.empty());

        CentralEventHub::instance().publishEvent({AppEventType::UndoRedoPerformed, "", {}, {}});
    }
>>>>>>> REPLACE
```

### File: `src/ui/PanelMediator.cpp`
```diff
<<<<<<< SEARCH
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
                   event.type == QuarkMeta::AppEventType::ItemsRenamed) {
            contentPanel->refreshAll();
        }
=======
        } else if (event.type == QuarkMeta::AppEventType::ItemsDeleted ||
                   event.type == QuarkMeta::AppEventType::ItemsRenamed ||
                   event.type == QuarkMeta::AppEventType::UndoRedoPerformed) {
            contentPanel->refreshAll();
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using standard CMake build commands:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Perform a batch metadata operation or move file operation.
3. Press `Ctrl+Z` or click the Undo button in `UndoToastOverlay`.
4. Verify that `CentralEventHub` receives `UndoRedoPerformed` and `PanelMediator` triggers `ContentPanel::refreshAll()`, updating the view automatically in place.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entrance**: All UI refreshes following Undo/Redo operations route through `CentralEventHub` -> `PanelMediator` -> `ContentPanel::refreshAll()`.
- **Anti-Redundancy**: Prevents manual, fragmented refresh calls inside individual command callbacks.

## 6. Header API Signature Verification
- `void CentralEventHub::publishEvent(const AppEvent& event);` (`src/core/CentralEventHub.h`, line 51)
- `void ContentPanel::refreshAll();` (`src/ui/ContentPanel.h`, line 166)
