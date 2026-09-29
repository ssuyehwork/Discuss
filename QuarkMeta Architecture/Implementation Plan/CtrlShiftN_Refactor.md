# Implementation Plan - Ctrl+Shift+N Shortcut Architectural Refactoring (`CtrlShiftN_Refactor.md`)

## 1. Overview
The current `Ctrl+Shift+N` new folder workflow suffers from severe architectural defects:
1. **Full Directory Rescan (`loadDirectory`)**: Creating a single new folder triggers a complete asynchronous disk scan of the entire folder via `QtConcurrent`, resetting the entire model and re-sorting/filtering thousands of items.
2. **UI Thread Blocking I/O**: `ContentFileOpsHandler::createNewItem` executes `QDir::mkdir` synchronously on the main UI thread.
3. **Shortcut Registration Divergence**: `Ctrl+Shift+N` is trapped inside `ContentKeyHandler::eventFilter`, making it fail whenever keyboard focus moves outside item views.
4. **Timer Hacks**: Uses `QTimer::singleShot(0, ...)` microtasks inside `restoreSelections` to patch timing race conditions caused by the asynchronous directory rescan.

This implementation plan refactors `Ctrl+Shift+N` into a clean, incremental, event-driven architecture:
- **Centralized Window-Scoped Shortcut**: Registers `Ctrl+Shift+N` in `AppShortcutController` as a `WindowShortcut` routed through `PanelMediator` to the currently active `ContentPanel`.
- **Incremental Model Insertion (`DiskItemModel::addItemRecord`)**: Adds an `addItemRecord` method to `DiskItemModel` using `beginInsertRows` / `endInsertRows`, inserting the new item into the model incrementally without clearing or re-scanning the directory.
- **Asynchronous I/O**: Offloads folder/file creation to an asynchronous background task.
- **Immediate Inline Rename**: Directly triggers `QAbstractItemView::edit(index)` upon row insertion on the UI thread, eliminating all timer hacks (`QTimer::singleShot`).

---

## 2. Modified Files List
- `src/ui/models/DiskItemModel.h` (Declare `addItemRecord` for incremental model insertion)
- `src/ui/models/DiskItemModel.cpp` (Implement `addItemRecord` with `beginInsertRows` / `endInsertRows`)
- `src/ui/controllers/ContentFileOpsHandler.cpp` (Asynchronous creation & incremental model insertion & direct `view->edit`)
- `src/ui/controllers/ContentKeyHandler.cpp` (Remove obsolete local `Ctrl+Shift+N` key handling)
- `src/ui/AppShortcutController.h` (Declare `createNewFolderRequested` signal)
- `src/ui/AppShortcutController.cpp` (Register `Ctrl+Shift+N` as `QShortcut`)
- `src/ui/PanelMediator.cpp` (Route `createNewFolderRequested` signal to `m_activeContentPanel`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/models/DiskItemModel.h`
```
<<<<<<< SEARCH
    const std::vector<QuarkMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) override;
=======
    const std::vector<QuarkMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) override;
    void addItemRecord(const QuarkMeta::ItemRecord& record);
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
=======
void DiskItemModel::addItemRecord(const ItemRecord& record) {
    if (record.path.isEmpty()) return;
    QString nPath = QDir::toNativeSeparators(record.path);
    if (m_pathToIndex.find(nPath) != m_pathToIndex.end()) return;

    int newRow = static_cast<int>(m_allRecords.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_allRecords.push_back(record);
    m_pathToIndex[nPath] = newRow;
    endInsertRows();
}

void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
>>>>>>> REPLACE
```

### 3.3 `src/ui/AppShortcutController.h`
```
<<<<<<< SEARCH
signals:
    void toggleImmersiveRequested();
    void togglePinRequested();
=======
signals:
    void toggleImmersiveRequested();
    void togglePinRequested();
    void createNewFolderRequested();
>>>>>>> REPLACE
```

### 3.4 `src/ui/AppShortcutController.cpp`
```
<<<<<<< SEARCH
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        emit restoreClosedTabRequested();
    });
=======
    QShortcut* scRestoreTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), m_window);
    scRestoreTab->setContext(Qt::WindowShortcut);
    connect(scRestoreTab, &QShortcut::activated, this, [this]() {
        emit restoreClosedTabRequested();
    });

    QShortcut* scNewFolder = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), m_window);
    scNewFolder->setContext(Qt::WindowShortcut);
    connect(scNewFolder, &QShortcut::activated, this, [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    });
>>>>>>> REPLACE
```

### 3.5 `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
            if (layoutManager) layoutManager->toggleImmersiveMode();
        });
=======
        connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
            if (layoutManager) layoutManager->toggleImmersiveMode();
        });
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this]() {
            if (m_activeContentPanel) {
                m_activeContentPanel->createNewItem("folder");
            }
        });
>>>>>>> REPLACE
```

### 3.6 `src/ui/controllers/ContentKeyHandler.cpp`
```
<<<<<<< SEARCH
    // 面板级全局快捷键（独立于 QAbstractItemView 焦点状态）
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if ((keyEvent->modifiers() & Qt::ShiftModifier) && keyEvent->key() == Qt::Key_N) {
            m_panel->createNewItem("folder");
            return true;
        }
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
    }
=======
    // 面板级全局快捷键（独立于 QAbstractItemView 焦点状态）
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
    }
>>>>>>> REPLACE
```

### 3.7 `src/ui/controllers/ContentFileOpsHandler.cpp`
```
<<<<<<< SEARCH
void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = currentPath + "/" + finalName;
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = currentPath + "/" + finalName;
    }

    if (type == "folder") {
        QDir(currentPath).mkdir(finalName);
    } else {
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
        }
    }

    m_panel->setPendingSelectName(finalName, true);
    if (m_panel->currentViewMode() == ContentPanel::ColumnView && m_panel->columnView()) {
        m_panel->columnView()->refreshActiveColumn();
    } else {
        m_panel->loadDirectory(currentPath, m_panel->isRecursive());
    }
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

    QPointer<ContentPanel> weakPanel(m_panel);
    QtConcurrent::run([weakPanel, currentPath, finalName, fullPath, type]() {
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

        ItemRecord rec = ItemRecord::create(fullPath);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, rec, fullPath]() {
            if (!weakPanel) return;

            if (weakPanel->currentViewMode() == ContentPanel::ColumnView && weakPanel->columnView()) {
                weakPanel->columnView()->refreshActiveColumn();
                return;
            }

            if (DiskItemModel* diskModel = qobject_cast<DiskItemModel*>(weakPanel->model())) {
                diskModel->addItemRecord(rec);
            }

            weakPanel->applySort();
            weakPanel->applyFilters();
            weakPanel->recalculateAndEmitStats();

            QSortFilterProxyModel* proxy = weakPanel->getActiveProxyModel();
            QAbstractItemView* view = weakPanel->activeItemView();
            if (proxy && view) {
                for (int i = 0; i < proxy->rowCount(); ++i) {
                    QModelIndex proxyIdx = proxy->index(i, 0);
                    if (proxyIdx.data(PathRole).toString() == fullPath) {
                        view->setFocus();
                        view->scrollTo(proxyIdx);
                        view->setCurrentIndex(proxyIdx);
                        view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        view->edit(proxyIdx);
                        break;
                    }
                }
            }
        });
    });
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and compile using CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Run QuarkMeta application.
3. Verify `Ctrl+Shift+N` anywhere in the window (even when item views do not have direct focus):
   - A new folder `"新建文件夹"` is created asynchronously without blocking UI thread.
   - The new folder is incrementally inserted into `DiskItemModel` (`beginInsertRows` / `endInsertRows`).
   - `loadDirectory` is **never called** (no full disk rescan).
   - The new folder is selected, scrolled to, and inline rename editor is opened instantly (`view->edit(proxyIdx)`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Reuse**: Reuses `DiskItemModel` as the sole model source of truth, adding incremental row insertion `addItemRecord`.
- **Anti-Redundancy**: Completely eliminates full directory rescans (`loadDirectory`) during item creation.
- **Central Shortcut Entry**: Reuses `AppShortcutController` window shortcut mechanism rather than fragmented key handlers.

---

## 6. Header API Signature Verification
- `DiskItemModel::addItemRecord(const QuarkMeta::ItemRecord& record)` -> `src/ui/models/DiskItemModel.h`
- `AppShortcutController::createNewFolderRequested()` -> `src/ui/AppShortcutController.h`
- `ContentFileOpsHandler::createNewItem(const QString& type)` -> `src/ui/controllers/ContentFileOpsHandler.h`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/controllers/ContentFileOpsHandler.cpp` includes `<QtConcurrent/QtConcurrent>`, `<QPointer>`, `<QMetaObject>`, `<QCoreApplication>`, `<QDir>`, `DiskItemModel.h`.
- `src/ui/models/DiskItemModel.h` already includes `ItemRecord` via `ItemModelBase.h`.
- `src/ui/AppShortcutController.cpp` includes `<QShortcut>`.
