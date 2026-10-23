# Implementation Plan: ColumnViewWidget-2.md (Column View Drag & Drop Target Path Alignment)

## Overview
Fixes the issue where drag-and-drop operations in Column View (Miller Columns) fail when dragging items across columns (e.g., from Column 3 `H:/Test/Test-3` to Column 2 `H:/Test`). 

### Root Cause
1. In `ContentFileOpsHandler::onPathsDropped`, the target directory defaults to `m_panel->currentPath()`, which represents the active/rightmost column path rather than the specific column (`ColumnViewPane`) onto which the drag-and-drop event was released.
2. When resolving a valid target item index (`targetIndex`), `m_panel->getProxyModel()` was used. In Column View, each `ColumnViewPane` possesses its own independent `FilterProxyModel`. Attempting to map a pane-specific `targetIndex` using the global proxy model failed, returning an invalid source index.
3. As a result, dropping onto Column 2 incorrectly resolved its destination back to Column 3 (`H:/Test/Test-3`). The same-directory self-drop guard detected that `srcDir == destDir` and silently aborted the operation.

### Solution
1. Extend `ContentFileOpsHandler::onPathsDropped` and `ContentPanel::onPathsDropped` with optional parameters: `targetDirOverride` and `sourceModelOverride`.
2. Update `ColumnViewPane` in `ColumnViewWidget.cpp` to forward its specific column path (`m_path`) and proxy model (`m_proxyModel`) when emitting drop events.
3. Add debug logging in `ContentFileOpsHandler::onPathsDropped` to verify source paths, targeted column directory, resolved destination directory, and drag-and-drop execution results.

---

## Modified Files List
- `src/ui/controllers/ContentFileOpsHandler.h`
- `src/ui/controllers/ContentFileOpsHandler.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/controllers/ContentFileOpsHandler.h`
Add `targetDirOverride` and `sourceModelOverride` parameters to `onPathsDropped`.

```cpp
<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), QAbstractItemModel* sourceModelOverride = nullptr);
>>>>>>> REPLACE
```

### 2. `src/ui/controllers/ContentFileOpsHandler.cpp`
Update `onPathsDropped` implementation to utilize `targetDirOverride` and `sourceModelOverride`, include `<QDebug>`, and add debug logging.

```cpp
<<<<<<< SEARCH
#include <QApplication>
#include <QPointer>
=======
#include <QApplication>
#include <QPointer>
#include <QDebug>
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (!m_panel || paths.isEmpty()) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString destDir = currentPath;
    if (targetIndex.isValid() && m_panel->getProxyModel()) {
        QModelIndex srcIdx = m_panel->getProxyModel()->mapToSource(targetIndex);
        if (srcIdx.isValid() && QFileInfo(srcIdx.data(PathRole).toString()).isDir()) {
            destDir = srcIdx.data(PathRole).toString();
        }
    }
=======
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, QAbstractItemModel* sourceModelOverride) {
    if (!m_panel || paths.isEmpty()) return;
    
    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
    if (baseDir.isEmpty() || baseDir == "computer://") return;

    QString destDir = baseDir;
    QAbstractItemModel* proxyModel = sourceModelOverride ? sourceModelOverride : m_panel->getProxyModel();

    if (targetIndex.isValid() && proxyModel) {
        QModelIndex srcIdx = targetIndex;
        if (auto* filterProxy = qobject_cast<QSortFilterProxyModel*>(proxyModel)) {
            srcIdx = filterProxy->mapToSource(targetIndex);
        }
        if (srcIdx.isValid() && QFileInfo(srcIdx.data(PathRole).toString()).isDir()) {
            destDir = srcIdx.data(PathRole).toString();
        }
    }

    qDebug() << "[ColumnView DragDrop Debug] Sources:" << paths 
             << "| TargetDirOverride:" << targetDirOverride 
             << "| Final DestDir:" << destDir;
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.h`
Add `targetDirOverride` and `sourceModelOverride` parameters to `onPathsDropped`.

```cpp
<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), QAbstractItemModel* sourceModelOverride = nullptr);
>>>>>>> REPLACE
```

### 4. `src/ui/ContentPanel.cpp`
Pass `targetDirOverride` and `sourceModelOverride` in `ContentPanel::onPathsDropped`.

```cpp
<<<<<<< SEARCH
void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex);
}
=======
void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, QAbstractItemModel* sourceModelOverride) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex, targetDirOverride, sourceModelOverride);
}
>>>>>>> REPLACE
```

### 5. `src/ui/ColumnViewWidget.cpp`
Update `ColumnViewPane` signal connection for `pathsDropped` to forward `m_path` and `m_proxyModel`.

```cpp
<<<<<<< SEARCH
        connect(m_listView, &DropListView::pathsDropped, m_contentPanel, &ContentPanel::onPathsDropped);
=======
        connect(m_listView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_proxyModel);
            }
        });
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **Compilation**:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ```

2. **Verification**:
   - Open Column View in QuarkMeta.
   - Expand multiple columns (e.g., Column 1: `H:/`, Column 2: `H:/Test`, Column 3: `H:/Test/Test-3`).
   - Drag an item from Column 3 (`H:/Test/Test-3`) and drop it onto the blank space of Column 2 (`H:/Test`).
   - Observe debug logs outputting `[ColumnView DragDrop Debug] Sources: ("...") | TargetDirOverride: "H:/Test" | Final DestDir: "H:/Test"`.
   - Confirm the item is successfully moved from `H:/Test/Test-3` to `H:/Test` and both columns refresh automatically.
