# ContentPaneSplitManager Implementation Plan - Drag Tab to ContentPanel Split Creation

## 1. Overview
This plan implements split pane creation by dragging a tab from `TabBarWidget` onto `ContentPanel` (or its edge overlay regions). 
When a tab is dragged over a `ContentPanel`, `ContentPanel` detects the `"application/x-quarkmeta-taburl"` MIME type, displays an edge overlay (top/bottom/left/right 25% edge zones), and upon drop creates a new split pane (up to `kMaxPanes = 4`) showing the dragged tab's URL.

### Key Safety Guarantees (Anti-Side-Effect Isolation):
1. **MIME Signature Strict Filtering**: Accepts strictly `"application/x-quarkmeta-taburl"` for split pane drag overlay & creation. Standard file/folder drag-and-drop operations (`hasUrls()`) bypass split overlay handling and remain 100% processed by `ViewDragDropHelper`.
2. **Hard-Capped Max Pane Limit**: Rejects tab drag-split if `paneCount() >= ContentPanel::kMaxPanes` (4 panes).
3. **Automatic Panel Wiring**: Relies on `ContentPanel::secondaryPaneCreated` signal which is already hooked into `PanelMediator` for automatic signal/slot setup of newly spawned secondary panels.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::dragEnterEvent(QDragEnterEvent* event) {
    QFrame::dragEnterEvent(event);
}

void ContentPanel::dragMoveEvent(QDragMoveEvent* event) {
    QFrame::dragMoveEvent(event);
}

void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    QFrame::dragLeaveEvent(event);
}

void ContentPanel::dropEvent(QDropEvent* event) {
    QFrame::dropEvent(event);
}
=======
void ContentPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            event->acceptProposedAction();
            return;
        }
    }
    QFrame::dragEnterEvent(event);
}

void ContentPanel::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            event->acceptProposedAction();
            updateDragOverlay(event->pos());
            return;
        }
    }
    QFrame::dragMoveEvent(event);
}

void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    hideDragOverlay();
    QFrame::dragLeaveEvent(event);
}

void ContentPanel::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            hideDragOverlay();
            QString tabUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
            if (tabUrl.isEmpty()) {
                tabUrl = event->mimeData()->text();
            }

            if (!tabUrl.isEmpty()) {
                QPoint pos = event->pos();
                int w = width();
                int h = height();
                Qt::Orientation orientation = Qt::Horizontal;

                if (pos.y() < h * 0.25 || pos.y() > h * 0.75) {
                    orientation = Qt::Vertical;
                } else {
                    orientation = Qt::Horizontal;
                }

                splitPane(orientation, tabUrl);
                event->acceptProposedAction();
                return;
            }
        }
    }
    hideDragOverlay();
    QFrame::dropEvent(event);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Check**:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. **Behavior Verification**:
   - **Tab Drag Split**: Drag a tab from `TabBarWidget` over `ContentPanel`. Move mouse near left/right/top/bottom 25% edge zones and verify blue split overlay is displayed.
   - **Drop Execution**: Drop tab on edge overlay and verify new split pane opens displaying the tab's path.
   - **Max Pane Limit Check**: Repeat split until 4 panes exist. Verify 5th tab drop is rejected and no overlay is shown.
   - **File Drag Side-Effect Check**: Drag a physical file/folder into `ContentPanel`. Verify standard file move/copy behavior executes normally without triggering split overlay.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **Split Manager API Reuse**:
  - Reuses `m_splitManager->updateDragOverlay(pos)` and `m_splitManager->hideDragOverlay()`.
  - Reuses `m_splitManager->splitPane(orientation, secondaryPath)`.
  - Reuses `m_splitManager->paneCount()` and `ContentPanel::kMaxPanes`.
- **No Parallel / Duplicate Implementations**:
  - Leverages existing `ContentPaneSplitManager` drag overlay & split mechanisms without creating any parallel logic.

---

## 6. Header API Signature Verification

| Class / Function Header Signature | File Origin | Result |
| :--- | :--- | :--- |
| `void ContentPanel::dragEnterEvent(QDragEnterEvent* event)` | `src/ui/ContentPanel.h:179` | Verified |
| `void ContentPanel::dragMoveEvent(QDragMoveEvent* event)` | `src/ui/ContentPanel.h:180` | Verified |
| `void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event)` | `src/ui/ContentPanel.h:181` | Verified |
| `void ContentPanel::dropEvent(QDropEvent* event)` | `src/ui/ContentPanel.h:182` | Verified |
| `int ContentPanel::paneCount() const` | `src/ui/ContentPanel.h:85` | Verified |
| `static constexpr int kMaxPanes = 4;` | `src/ui/ContentPanel.h:77` | Verified |
| `void ContentPanel::splitPane(Qt::Orientation, const QString&)` | `src/ui/ContentPanel.h:87` | Verified |
| `void ContentPanel::updateDragOverlay(const QPoint&)` | `src/ui/ContentPanel.h:264` | Verified |
| `void ContentPanel::hideDragOverlay()` | `src/ui/ContentPanel.h:265` | Verified |

---

## 7. Header Inclusion Chain & Type Completeness Check

- `QDragEnterEvent`, `QDragMoveEvent`, `QDragLeaveEvent`, `QDropEvent` are included in `ContentPanel.cpp` via `<QDropEvent>`, `<QDragEnterEvent>`, `<QDragMoveEvent>` headers.
- `QMimeData` is included via `<QMimeData>` header in `ContentPanel.cpp`.
- All types, functions, and enums used are completely defined and verified.
