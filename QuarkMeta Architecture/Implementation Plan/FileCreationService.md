# Implementation Plan - FileCreationService.md

## 1. Overview
This implementation plan extracts folder and file creation (`createNewItem`) into a dedicated, independent, high-cohesion core service module: **`FileCreationService`** (`src/core/FileCreationService.h` / `.cpp`).

### Architecture Benefits & Single Responsibility Principle
1. **Decoupling**: Fully decouples new folder/file creation logic from `ContentFileOpsHandler` (which handles drag-drop, collision dialogs, and batch renaming), preventing inline renameDelegate editing from being broken during future handler refactorings.
2. **Synchronous Model Append**: When a new item is created, `FileCreationService` synchronously appends the new `ItemRecord` into `DiskItemModel` on the main thread, avoiding asynchronous scan race conditions.
3. **Deterministic Delegate Editing**: `FileCreationService` immediately sets focus to the active view and calls `view->edit(proxyIndex)`, guaranteeing 100% reliable inline renaming every single time.
4. **Unified SSOT Entry Point**: Shortcuts (`Ctrl+Shift+N`), context menus ("新建文件夹"), and TitleBar creation buttons all route through `FileCreationService::instance().createNewItem(panel, type)`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/FileCreationService.md`.

## 2. Modified Files List
- `src/core/FileCreationService.h` (New File)
- `src/core/FileCreationService.cpp` (New File)
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 1. `src/core/FileCreationService.h` (New File)

```cpp
#pragma once

#include <QObject>
#include <QString>

namespace QuarkMeta {

class ContentPanel;

class FileCreationService : public QObject {
    Q_OBJECT

public:
    static FileCreationService& instance();

    bool createNewItem(ContentPanel* panel, const QString& type = "folder");

private:
    explicit FileCreationService(QObject* parent = nullptr);
    ~FileCreationService() override = default;
};

} // namespace QuarkMeta
```

### 2. `src/core/FileCreationService.cpp` (New File)

```cpp
#include "FileCreationService.h"
#include "../ui/ContentPanel.h"
#include "../ui/models/DiskItemModel.h"
#include "../core/ItemRecord.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>

namespace QuarkMeta {

FileCreationService& FileCreationService::instance() {
    static FileCreationService inst;
    return inst;
}

FileCreationService::FileCreationService(QObject* parent) : QObject(parent) {}

bool FileCreationService::createNewItem(ContentPanel* panel, const QString& type) {
    if (!panel) return false;
    QString currentPath = panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return false;

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

    if (!success) return false;

    // 1. 同步将新项目追加至 Model，彻底消除全盘异步扫描的时序脱节与状态遗失
    ItemRecord newRec = ItemRecord::create(fullPath);
    if (panel->model()) {
        panel->model()->appendRecord(newRec);
    }
    panel->applyFilters();

    // 2. 强锁定焦点并即时触发 Delegate 代理重命名编辑框 (100% 稳固)
    panel->selectAndEditPath(fullPath);
    return true;
}

} // namespace QuarkMeta
```

### 3. `src/ui/ContentPanel.h`

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

### 4. `src/ui/ContentPanel.cpp`

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

### 5. `src/ui/controllers/ContentFileOpsHandler.cpp`

```diff
<<<<<<< SEARCH
#include "ContentFileOpsHandler.h"
#include "../ContentPanel.h"
=======
#include "ContentFileOpsHandler.h"
#include "../ContentPanel.h"
#include "../../core/FileCreationService.h"
>>>>>>> REPLACE
```

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
    FileCreationService::instance().createNewItem(m_panel, type);
}
>>>>>>> REPLACE
```

### 6. `CMakeLists.txt`

```diff
<<<<<<< SEARCH
    src/core/DiskScanService.cpp
    src/core/DiskScanService.h
=======
    src/core/DiskScanService.cpp
    src/core/DiskScanService.h
    src/core/FileCreationService.cpp
    src/core/FileCreationService.h
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open any folder:
   - Press `Ctrl+Shift+N` (or click "新建文件夹").
   - Verify `FileCreationService` is invoked, creating the directory, appending to `DiskItemModel`, setting focus, and immediately calling `view->edit(proxyIdx)` to open the `FileNameLineEdit` delegate editor with "新建文件夹" text highlighted.
   - Type a new name and press Enter to save.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `DiskItemModel::appendRecord`, `QAbstractItemView::edit()`, and `RenameCapableDelegate`.
- **Zero Redundancy**: Establishes `FileCreationService` as the single source of truth for folder/file creation.

## 6. Header API Signature Verification
- `FileCreationService::createNewItem(ContentPanel* panel, const QString& type)` in `src/core/FileCreationService.h`.
- `ContentPanel::selectAndEditPath(const QString& path)` in `src/ui/ContentPanel.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- Added `FileCreationService.h` / `.cpp` to `CMakeLists.txt`.
- Type completeness verified for `FileCreationService`, `ContentPanel`, and `DiskItemModel`.
