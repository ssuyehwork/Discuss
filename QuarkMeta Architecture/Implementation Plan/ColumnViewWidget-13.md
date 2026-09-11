# ColumnViewWidget Implementation Plan - Metadata Enrichment & Clean Architecture

## 1. Overview
This implementation plan addresses the "detached architecture" issue in `ColumnViewWidget` / `ColumnViewPane` and removes invasive workaround patches from `ContentPanel.cpp`.
When directory scanning is performed in `ColumnViewPane::loadDirectory()`, the raw `ItemRecord` objects lack visual metadata (rating, color, tags, pinned status, encryption, notes). This plan enriches the records with `MetadataManager::instance().getMeta(...)` before setting them on the `DiskItemModel`. Additionally, bogus "self-healing" workarounds in `ContentPanel.cpp` are cleaned up.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### A. Data Enrichment in `ColumnViewPane::loadDirectory`
File: `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
=======
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        for (auto& item : items) {
            RuntimeMeta meta = MetadataManager::instance().getMeta(item.path.toStdWString());
            item.rating = meta.rating;
            item.manualColor = QString::fromStdWString(meta.color);
            item.tags = meta.tags;
            item.pinned = meta.pinned;
            item.encrypted = meta.encrypted;
            item.note = meta.note;
        }
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
>>>>>>> REPLACE
```

### B. Header Include Verification in `ColumnViewWidget.cpp`
File: `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
#include "ColumnViewWidget.h"
#include "ContentPanel.h"
#include "../core/DiskScanService.h"
=======
#include "ColumnViewWidget.h"
#include "ContentPanel.h"
#include "../core/DiskScanService.h"
#include "../meta/MetadataManager.h"
>>>>>>> REPLACE
```

### C. Remove Workaround Branch in `ContentPanel::setViewMode`
File: `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ViewModeColumn && mode != ViewModeColumn) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }
=======
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build the project using CMake:
   ```bash
   cmake --build --preset x64-Debug  # or standard cmake --build build
   ```
2. Verify that `ColumnViewWidget` displays item ratings, color tags, and custom metadata accurately.
3. Switch between Grid View and Column View to ensure seamless mode switching without redundant scans.
