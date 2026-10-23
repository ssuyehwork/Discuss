# Implementation Plan - Unifying JustifiedView & Eliminating DropJustifiedView (`DropJustifiedView.md`)

## 1. Overview
This implementation plan resolves the redundant subclassing anti-pattern where `DropJustifiedView` existed solely as a 17-line subclass wrapping `JustifiedView` with `DragDropEventFilter::install(this)`. 

Since `DragDropEventFilter` is already a decoupled event filter designed to attach drag-and-drop capabilities to any `QWidget` without requiring inheritance, this plan merges the drag-and-drop initialization directly into `JustifiedView`, eliminates `DropJustifiedView.h` and `DropJustifiedView.cpp`, and cleans up all redundant `qobject_cast<DropJustifiedView*>` calls in `SectionedScrollCanvas.cpp` and `ContentPanel.cpp`.

---

## 2. Modified Files List
- `CMakeLists.txt` (Remove `src/ui/DropJustifiedView.h` and `src/ui/DropJustifiedView.cpp`)
- `src/ui/JustifiedView.h` (Add `pathsDropped` signal directly to `JustifiedView`)
- `src/ui/JustifiedView.cpp` (Install `DragDropEventFilter` and handle drag start in `JustifiedView`)
- `src/ui/SectionedScrollCanvas.cpp` (Instantiate `JustifiedView` directly instead of `DropJustifiedView`)
- `src/ui/ContentPanel.h` (Update pointer type from `DropJustifiedView*` to `JustifiedView*`)
- `src/ui/ContentPanel.cpp` (Remove `DropJustifiedView.h` header include and update casts)
- `src/ui/controllers/ContentViewCoordinator.cpp` (Remove `#include "../DropJustifiedView.h"`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/ui/DropJustifiedView.h
    src/ui/DropJustifiedView.cpp
=======
>>>>>>> REPLACE
```

### 3.2 `src/ui/JustifiedView.h`
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

### 3.3 `src/ui/JustifiedView.cpp`
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

### 3.4 `src/ui/SectionedScrollCanvas.cpp`
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
        auto* folderJv = new DropJustifiedView();
=======
        auto* folderJv = new JustifiedView();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        auto* fileJv = new DropJustifiedView();
=======
        auto* fileJv = new JustifiedView();
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

### 3.5 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
class DropJustifiedView;
=======
class JustifiedView;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    DropJustifiedView* m_folderGridView = nullptr;
=======
    JustifiedView* m_folderGridView = nullptr;
>>>>>>> REPLACE
```

### 3.6 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
#include "DropJustifiedView.h"
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_folderGridView = static_cast<DropJustifiedView*>(m_gridCanvas->folderView());
=======
    m_folderGridView = static_cast<JustifiedView*>(m_gridCanvas->folderView());
>>>>>>> REPLACE
```

### 3.7 `src/ui/controllers/ContentViewCoordinator.cpp`
```
<<<<<<< SEARCH
#include "../DropJustifiedView.h"
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Delete `src/ui/DropJustifiedView.h` and `src/ui/DropJustifiedView.cpp`.
2. Configure CMake:
   ```bash
   cmake -B build -S .
   ```
3. Compile the target:
   ```bash
   cmake --build build --config Debug
   ```
4. Verify application launch and drag-and-drop operations in Grid/Justified view modes.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Event Filter Reuse**: Leveraged `DragDropEventFilter::install(this)` and `ViewDragDropHelper::executeStartDrag(this, supportedActions)` directly inside `JustifiedView`.
- **Anti-Redundancy**: Fully removed `DropJustifiedView.h` and `DropJustifiedView.cpp` dead code.

---

## 6. Header API Signature Verification
- `DragDropEventFilter::install(QObject*)` -> `src/ui/ViewDragDropHelper.h` (Verified)
- `ViewDragDropHelper::executeStartDrag(QAbstractItemView*, Qt::DropActions)` -> `src/ui/ViewDragDropHelper.h` (Verified)
- `JustifiedView::pathsDropped(const QStringList&, const QModelIndex&)` -> `src/ui/JustifiedView.h` (Verified)
