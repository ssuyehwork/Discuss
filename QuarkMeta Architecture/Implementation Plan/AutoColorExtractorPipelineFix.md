# Implementation Plan - Auto Color Extractor Pipeline Wiring Fix

## Architecture Gate 3-Question Answers (架构三问回答)
1. **SSOT Source (真理源溯源)**:
   The extracted media features (width, height, `autoColor`, and `palettes`) are owned by `MetadataManager` (and cached in `MetaMemoryCache` & persisted in `.QuarkMeta.json` sidecar files). `MediaExtractorPipeline` acts as the single background extraction pipeline.
2. **Black-box Integrity (黑盒完整性)**:
   `ContentDataLoader` communicates exclusively via `MetadataManager::registerItemsAsync` facade API. `MetadataManager` handles background queueing and emits standard `notifyUI(RefreshLevel::PathUpdate)` signals upon completion without breaking component encapsulation.
3. **Root Cause Analysis (根因 vs 症状)**:
   The background extraction pipeline (`MediaExtractorPipeline`) was left unconnected after directory scanning in `ContentDataLoader`. Scanned file paths were never enqueued, and `updateExtractedMediaFeaturesBatch` in `MetadataManager` lacked UI refresh notifications (`notifyUI`), causing `auto_color` to remain empty in `.QuarkMeta.json` and UI.

---

## 1. Overview
This plan connects the auto color extraction pipeline when directory scanning completes in `ContentDataLoader`, enqueuing file items into `MetadataManager::registerItemsAsync`. It also updates `MetadataManager::updateExtractedMediaFeaturesBatch` to emit `notifyUI(RefreshLevel::PathUpdate)` upon feature extraction completion, allowing auto-extracted dominant colors (`auto_color`) and palettes to be persisted and rendered in the UI seamlessly.

---

## 2. Modified Files List
1. `src/ui/controllers/ContentDataLoader.cpp`
2. `src/meta/MetadataManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/controllers/ContentDataLoader.cpp`
Add `#include "../../meta/MetadataManager.h"` and enqueue scanned file paths to `registerItemsAsync` after directory scan completes.

```
<<<<<<< SEARCH
#include "../../meta/DiskTrashRepo.h"
#include "../../meta/MetaCacheDecorator.h"
#include "../../meta/MediaExtractorPipeline.h"
#include "../../util/ThumbnailPipelineService.h"
=======
#include "../../meta/DiskTrashRepo.h"
#include "../../meta/MetaCacheDecorator.h"
#include "../../meta/MediaExtractorPipeline.h"
#include "../../meta/MetadataManager.h"
#include "../../util/ThumbnailPipelineService.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, allItems, reqId]() {
            if (panelPtr && panelPtr->loadRequestId() == reqId) {
                if (panelPtr->model()) {
                    panelPtr->model()->setRecords(allItems);
                }
                panelPtr->applySort();
                panelPtr->setLoading(false);
                panelPtr->recalculateAndEmitStats();
                panelPtr->applyFilters();
                panelPtr->restoreSelections();
                panelPtr->startVisibleTimer();
            }
        }, Qt::QueuedConnection);
=======
        QMetaObject::invokeMethod(QCoreApplication::instance(), [panelPtr, allItems, reqId]() {
            if (panelPtr && panelPtr->loadRequestId() == reqId) {
                if (panelPtr->model()) {
                    panelPtr->model()->setRecords(allItems);
                }
                panelPtr->applySort();
                panelPtr->setLoading(false);
                panelPtr->recalculateAndEmitStats();
                panelPtr->applyFilters();
                panelPtr->restoreSelections();
                panelPtr->startVisibleTimer();

                // Enqueue scanned files for background media feature & auto-color extraction
                QStringList filePaths;
                for (const auto& item : allItems) {
                    if (!item.isDir && !item.path.isEmpty()) {
                        filePaths.append(item.path);
                    }
                }
                if (!filePaths.isEmpty()) {
                    MetadataManager::instance().registerItemsAsync(filePaths);
                }
            }
        }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

### File 2: `src/meta/MetadataManager.cpp`
Notify UI of path updates after updating extracted media features in batch.

```
<<<<<<< SEARCH
        // 落地写入 .QuarkMeta.json 侧车文件
        QuarkMetaJsonStore::instance().updateItemMeta(nPath, [&item](ItemMeta& meta) {
            meta.width = item.width;
            meta.height = item.height;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.push_back({p.first, p.second});
            }
        });
    }
}
=======
        // 落地写入 .QuarkMeta.json 侧车文件
        QuarkMetaJsonStore::instance().updateItemMeta(nPath, [&item](ItemMeta& meta) {
            meta.width = item.width;
            meta.height = item.height;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.push_back({p.first, p.second});
            }
        });

        notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build the project using `mkdir build && cd build && cmake .. && cmake --build .`.
2. Ensure there are zero C2039/C2027 compilation errors.
3. Open a directory containing image files (e.g., PNG, JPG) in QuarkMeta.
4. Verify that background thread extracts dominant colors (`autoColor`), updates `.QuarkMeta.json` sidecar files (`"auto_color": "#HEX"`), and broadcasts UI updates to update cards/filters dynamically.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Reused**: Reused official `MetadataManager::instance().registerItemsAsync(paths)` and `notifyUI(RefreshLevel::PathUpdate, path)`.
- **Anti-Redundancy Check**: No second extraction queue created. Wired directly into the existing `MediaExtractorPipeline` engine.

---

## 6. Header API Signature Verification
| Class / Function | Declared Header | Verified Exact Signature |
| :--- | :--- | :--- |
| `MetadataManager::registerItemsAsync` | `src/meta/MetadataManager.h` | `void registerItemsAsync(const QStringList& paths);` |
| `MetadataManager::notifyUI` | `src/meta/MetadataManager.h` | `void notifyUI(RefreshLevel level, const QString& path = "");` |
| `MetadataManager::updateExtractedMediaFeaturesBatch` | `src/meta/MetadataManager.h` | `void updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items);` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/controllers/ContentDataLoader.cpp`: Explicitly added `#include "../../meta/MetadataManager.h"` to ensure `MetadataManager::instance()` is fully defined.
