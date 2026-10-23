# Implementation Plan - ContentFileOpsHandler-1.md

## 1. Overview
This implementation plan fixes the issue where creating a new folder (or file via Ctrl+Shift+N / menu) fails to reliably enter inline rename mode (`view->edit()`) due to timing脱节 and state fragility in the asynchronous disk rescan pipeline.

### Root Cause Analysis
Previously, `createNewItem` created the physical directory, set `m_isPendingEdit = true` on `ContentPanel`, and called `refreshAll()`.
`refreshAll()` triggered an asynchronous background thread disk scan (`QtConcurrent::run`). By the time the background scan completed and `restoreSelections()` was invoked asynchronously on the main thread:
1. `m_isPendingEdit` could be cleared or overwritten by intermediate UI events (focus changes, mouse hovers, filter updates).
2. The active view (`activeItemView()`) had often lost keyboard focus, causing Qt's `QAbstractItemView::edit()` to fail silently without opening the inline rename line editor.

### Solution
1. **Direct Synchronous Model Append**: When a new item is created, synchronously append the new `ItemRecord` into the active `DiskItemModel` on the main thread.
2. **Immediate Focus & Edit Trigger (`selectAndEditPath`)**: Immediately set focus to the active view (`view->setFocus()`), select the new item, and call `view->edit(proxyIndex)`.
3. **100% Deterministic & Non-Fragile**: Eliminates the asynchronous state dependency completely, ensuring inline renaming is triggered instantly and reliably every single time.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/ContentFileOpsHandler-1.md`.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```diff
<<<<<<< SEARCH
    void selectAndScrollToPath(const QString& path);
    void selectAndScrollToItem(const QString& path);
=======
    void selectAndScrollToPath(const QString& path);
    void selectAndScrollToItem(const QString& path);
    void selectAndEditPath(const QString& path);
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`

```diff
<<<<<<< SEARCH
void ContentPanel::selectAndScrollToPath(const QString& path) { selectAndScrollToItem(path); }
void ContentPanel::selectAndScrollToItem(const QString& path) {
=======
void ContentPanel::selectAndEditPath(const QString& path) {
    QSortFilterProxyModel* proxy = getActiveProxyModel();
    QAbstractItemView* view = activeItemView();
    if (!proxy || !view || path.isEmpty()) return;

    for (int i = 0; i < proxy->rowCount(); ++i) {
        QModelIndex proxyIdx = proxy->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            view->setFocus();
            view->scrollTo(proxyIdx);
            view->setCurrentIndex(proxyIdx);
            if (view->selectionModel()) {
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            view->edit(proxyIdx);
            break;
        }
    }
}

void ContentPanel::selectAndScrollToPath(const QString& path) { selectAndScrollToItem(path); }
void ContentPanel::selectAndScrollToItem(const QString& path) {
>>>>>>> REPLACE
```

### 3. `src/ui/controllers/ContentFileOpsHandler.cpp`

```diff
<<<<<<< SEARCH
void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = QDir(currentPath).filePath(finalName);
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = QDir(currentPath).filePath(finalName);
    }

    QPointer<ContentPanel> weakPanel(m_panel);
    (void)QtConcurrent::run([weakPanel, currentPath, finalName, fullPath, type]() {
        bool success = false;
        if (type == "folder") {
            success = QDir(currentPath).mkdir(finalName);
        } else {
            QFile f(fullPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.close();
                success = true;
            }
        }

        if (!success) return;

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, finalName]() {
            if (!weakPanel) return;
            weakPanel->setPendingSelectName(finalName, true);
            weakPanel->refreshAll();
        });
    });
}
=======
void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = QDir(currentPath).filePath(finalName);
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = QDir(currentPath).filePath(finalName);
    }

    bool success = false;
    if (type == "folder") {
        success = QDir(currentPath).mkdir(finalName);
    } else {
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
            success = true;
        }
    }

    if (!success) return;

    // 1. 同步将新项目追加至模型，避免全盘异步重扫造成的时序脱节
    ItemRecord newRec = ItemRecord::create(fullPath);
    if (m_panel->model()) {
        m_panel->model()->appendRecord(newRec);
    }
    m_panel->applyFilters();

    // 2. 强锁定焦点并即时触发代理行内编辑框 (100% 稳固)
    m_panel->selectAndEditPath(fullPath);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, navigate to any folder:
   - Press `Ctrl+Shift+N` (or right-click menu -> "新建文件夹").
   - Verify that the new folder is instantly created and 100% reliably enters inline rename mode (`view->edit()`), opening the `FileNameLineEdit` text box with "新建文件夹" highlighted.
   - Type a new name and press Enter to save.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `DiskItemModel::appendRecord`, `QAbstractItemView::edit()`, and `RenameCapableDelegate`.
- **Zero Redundancy**: Directly triggers edit mode synchronously instead of relying on fragile asynchronous scan flags.

## 6. Header API Signature Verification
- `ContentPanel::selectAndEditPath(const QString& path)` in `src/ui/ContentPanel.h`.
- `DiskItemModel::appendRecord(const ItemRecord& rec)` in `src/ui/models/DiskItemModel.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- All header inclusions closed and verified.
