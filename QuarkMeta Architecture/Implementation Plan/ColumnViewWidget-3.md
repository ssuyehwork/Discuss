# Implementation Plan: ColumnViewWidget-3.md (Column View Drag & Drop Target Path Alignment and Post-Drop Refresh Preservation)

## Overview
Fixes two critical issues in Column View (Miller Columns) during drag-and-drop operations:
1. **Target Path & Model Mismatch**: When dragging across columns (e.g. Column 3 to Column 2), the destination directory defaulted to `m_panel->currentPath()` instead of the specific `ColumnViewPane`'s directory, causing silent drop cancellations due to self-drop guards.
2. **Column 4 (and Sub-columns) Unexpected Closure Post-Drop**: After file operations complete, `DiskIoService` invoked `weakPanel->loadDirectory(...)`. In Column View mode, `loadDirectory` called `m_columnView->setRootPath(...)`, which forcibly wiped all open columns (`clearAllColumns()`) and rebuilt only up to the 3rd column, destroying the 4th column (and any deeper sub-columns).

### Solution
1. **In-Place Column Refresh**: Replace `weakPanel->loadDirectory(...)` with `weakPanel->refreshAll()` in `ContentFileOpsHandler::onPathsDropped`. `refreshAll()` safely delegates to `m_columnView->refreshAllColumns()`, reloading file data in-place without destroying open column widgets or truncating the column stack.
2. **Target Alignment**: Pass `targetDirOverride` (`m_path`) and `sourceModelOverride` (`m_proxyModel`) from `ColumnViewPane` to ensure drop operations accurately resolve destination directories and filter proxy indices.

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
Extend `onPathsDropped` signature with optional `targetDirOverride` and `sourceModelOverride` parameters.

```
<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), QAbstractItemModel* sourceModelOverride = nullptr);
>>>>>>> REPLACE
```

### 2. `src/ui/controllers/ContentFileOpsHandler.cpp`
Include `<QDebug>`, resolve target paths using overrides, and replace `loadDirectory` with `refreshAll` post-I/O execution.

```
<<<<<<< SEARCH
#include <QApplication>
#include <QPointer>
=======
#include <QApplication>
#include <QPointer>
#include <QDebug>
>>>>>>> REPLACE
```

```
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

```
<<<<<<< SEARCH
    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->loadDirectory(weakPanel->currentPath(), weakPanel->isRecursive());
            }
        });
    });
=======
    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->refreshAll();
            }
        });
    });
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.h`
Extend `onPathsDropped` in `ContentPanel`.

```
<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), QAbstractItemModel* sourceModelOverride = nullptr);
>>>>>>> REPLACE
```

### 4. `src/ui/ContentPanel.cpp`
Forward override parameters and protect `loadDirectory` in ColumnView mode when columns are already open.

```
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

```
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        m_isRecursive = recursive;
        if (m_columnView) {
            m_columnView->setRootPath(path);
            restoreSelections();
        }
        updateStatusBarStats();
        return;
    }
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        m_isRecursive = recursive;
        if (m_columnView) {
            if (m_columnView->containsPath(path)) {
                m_columnView->refreshAllColumns();
            } else {
                m_columnView->setRootPath(path);
                restoreSelections();
            }
        }
        updateStatusBarStats();
        return;
    }
>>>>>>> REPLACE
```

### 5. `src/ui/ColumnViewWidget.cpp`
Update `ColumnViewPane` signal connection to pass column-specific `m_path` and `m_proxyModel`.

```
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
   - Expand to 4 columns: Column 1 (`H:/`), Column 2 (`H:/Test`), Column 3 (`H:/Test/Test-3`), Column 4 (`H:/Test/Test-3/Test-4`).
   - Drag an item from Column 3 to Column 4 (or Column 3 to Column 2) and release left mouse button.
   - Verify that the item moves successfully.
   - Verify that Column 4 (and any open sub-columns) remains open and visible, and that all columns refresh in-place without flashing or collapsing.
