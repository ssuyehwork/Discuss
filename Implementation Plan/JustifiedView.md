# Implementation Plan - Unifying JustifiedView & Eliminating DropJustifiedView (`JustifiedView.md`)

## 1. Overview
This implementation plan resolves the redundant subclassing anti-pattern where `DropJustifiedView` existed solely as a wrapper subclass extending `JustifiedView` with `DragDropEventFilter::install(this)`.

Since `DragDropEventFilter` is an event filter designed to attach drag-and-drop capabilities to any `QWidget` without requiring inheritance, this plan merges the drag-and-drop initialization directly into `JustifiedView`, eliminates `DropJustifiedView.h` and `DropJustifiedView.cpp`, and cleans up all redundant `DropJustifiedView` references across the codebase.

---

## 2. Modified Files List
- `CMakeLists.txt` (Removed `src/ui/DropJustifiedView.h` and `src/ui/DropJustifiedView.cpp`)
- `src/ui/JustifiedView.h` (Added `pathsDropped` signal and `startDrag` override directly to `JustifiedView`)
- `src/ui/JustifiedView.cpp` (Installed `DragDropEventFilter` and implemented `startDrag` in `JustifiedView`)
- `src/ui/SectionedScrollCanvas.cpp` (Instantiated `JustifiedView` directly instead of `DropJustifiedView`)
- `src/ui/ContentPanel.h` (Updated pointer type from `DropJustifiedView*` to `JustifiedView*`)
- `src/ui/ContentPanel.cpp` (Removed `#include "DropJustifiedView.h"` header include and updated static casts)
- `src/ui/controllers/ContentViewCoordinator.cpp` (Removed `#include "../DropJustifiedView.h"`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/JustifiedView.h`
```
<<<<<<< SEARCH
signals:
    void totalHeightChanged(int height);
=======
signals:
    void totalHeightChanged(int height);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
=======
protected:
    void startDrag(Qt::DropActions supportedActions) override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
>>>>>>> REPLACE
```

### 3.2 `src/ui/JustifiedView.cpp`
```
<<<<<<< SEARCH
#include "JustifiedView.h"
#include "CardLayoutEngine.h"
=======
#include "JustifiedView.h"
#include "CardLayoutEngine.h"
#include "ViewDragDropHelper.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFrameShape(QFrame::NoFrame);
=======
JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFrameShape(QFrame::NoFrame);
    setDragEnabled(true);
    DragDropEventFilter::install(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
int JustifiedView::horizontalOffset() const { return 0; }
=======
void JustifiedView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

int JustifiedView::horizontalOffset() const { return 0; }
>>>>>>> REPLACE
```

### 3.3 `src/ui/SectionedScrollCanvas.cpp`
```
<<<<<<< SEARCH
#include "DropJustifiedView.h"
#include "JustifiedView.h"
=======
#include "JustifiedView.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (auto* dropFolder = qobject_cast<DropJustifiedView*>(folderView)) {
            connect(dropFolder, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropJustifiedView*>(fileView)) {
            connect(dropFile, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
=======
        if (auto* dropFolder = qobject_cast<JustifiedView*>(folderView)) {
            connect(dropFolder, &JustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<JustifiedView*>(fileView)) {
            connect(dropFile, &JustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
>>>>>>> REPLACE
```

---

## 4. SSOT API Reuse & Anti-Redundancy Verification
- **SSOT Event Filter Reuse**: Leveraged `DragDropEventFilter::install(this)` and `ViewDragDropHelper::executeStartDrag(this, supportedActions)` directly inside `JustifiedView`.
- **Anti-Redundancy**: Fully removed `DropJustifiedView.h` and `DropJustifiedView.cpp` dead code files.
