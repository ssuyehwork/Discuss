# Implementation Plan - JustifiedView & ContentPanel Debug Logging Integration (JustifiedView-7.md)

## 1. Overview
本实施方案旨在为 `JustifiedView`（网格/自适应视图）、`ContentPanel`（内容面板）与 `DiskItemModel`（磁盘模型）补充结构化诊断日志（通过 `Logger::log`）。
通过包含行移除范围、模型总行数、视图重绘保护防护触发点以及元数据更新广播路径等日志信息，帮助开发者在调试与线上运行中实时追踪并定位视图重排、筛选联动与闪退拦截的真实轨迹。

---

## 2. Modified Files List
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/models/DiskItemModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/JustifiedView.cpp`
```
<<<<<<< SEARCH
#include "JustifiedView.h"
#include "CardLayoutEngine.h"
=======
#include "JustifiedView.h"
#include "CardLayoutEngine.h"
#include "Logger.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    doLayout();
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}
=======
void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    Logger::log(QString("[JustifiedView] rowsAboutToBeRemoved range: %1 ~ %2, current model rowCount: %3")
                .arg(start).arg(end).arg(model() ? model()->rowCount() : 0));
    doLayout();
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QModelIndex idx = model()->index(geo.index, 0);
        if (!idx.isValid()) continue;
=======
        QModelIndex idx = model()->index(geo.index, 0);
        if (!idx.isValid()) {
            Logger::log(QString("[JustifiedView::paintEvent] Guarded invalid index at geo.index: %1 (model rowCount: %2)")
                        .arg(geo.index).arg(model() ? model()->rowCount() : 0));
            continue;
        }
>>>>>>> REPLACE
```

### File 2: `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
#include "UiHelper.h"
#include "ToolTipOverlay.h"
=======
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "Logger.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_folderGridView && m_folderGridView->viewport()) m_folderGridView->viewport()->update();
=======
void ContentPanel::updateItemMetadata(const QString& path) {
    Logger::log(QString("[ContentPanel::updateItemMetadata] Updating metadata for path: %1").arg(path));
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_folderGridView && m_folderGridView->viewport()) m_folderGridView->viewport()->update();
>>>>>>> REPLACE
```

### File 3: `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
#include "CoreController.h"
#include "DiskMediaExtractor.h"
#include "FileOperationHelper.h"
#include "MetadataManager.h"
#include "DriveMetaDao.h"
#include "../../core/LastOperationManager.h"
=======
#include "CoreController.h"
#include "DiskMediaExtractor.h"
#include "FileOperationHelper.h"
#include "MetadataManager.h"
#include "DriveMetaDao.h"
#include "../../core/LastOperationManager.h"
#include "Logger.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            emit dataChanged(index(i, 0), index(i, columnCount() - 1), {Qt::DisplayRole, Qt::DecorationRole, RatingRole, ColorRole, HasThumbnailRole});
        }
    }
}
=======
            Logger::log(QString("[DiskItemModel::updateRecordMetadata] Refreshed row %1 for path: %2").arg(i).arg(nPath));
            emit dataChanged(index(i, 0), index(i, columnCount() - 1), {Qt::DisplayRole, Qt::DecorationRole, RatingRole, ColorRole, HasThumbnailRole});
        }
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. 在代码中核查 `Logger.h` 包含及 `Logger::log` 日志输出。
2. 运行应用并执行筛选与元数据修改操作，检查 `quarkmeta_debug.log` 中的实时追踪日志信息。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- 严格使用项目中既有的唯一日志组件 `QuarkMeta::Logger::log`。

---

## 6. Header API Signature Verification
- `Logger::log(const QString& msg)` 物理声明于 `src/ui/Logger.h`。

---

## 7. Header Inclusion Chain & Type Completeness Check
- `JustifiedView.cpp`、`ContentPanel.cpp` 和 `DiskItemModel.cpp` 均显式包含 `"Logger.h"`，类型定义闭合。
