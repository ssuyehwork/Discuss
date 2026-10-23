# Column View Thumbnail Refactoring Implementation Plan (`ColumnViewThumbnailRefactoring.md`)

## 1. Overview
This implementation plan addresses the issue where column view panes (`ColumnViewPane`) fail to display file thumbnails:
1. **Unified Visible Rows Calculation**: Extract a public static calculation function `ContentViewCoordinator::calculateVisibleRowsForView(QAbstractItemView*, DiskItemModel*)` so both `ContentViewCoordinator` and `ColumnViewPane` share the exact same visible row range calculation (extending 4 buffer rows up and down, skipping section headers).
2. **ColumnViewPane Thumbnail Refresh & Debouncing**: Upgrade `ColumnViewPane::refreshVisibleThumbnails()` to retrieve visible row indices using the unified helper and request thumbnail loads on its own `DiskItemModel`. Debounce refresh calls via a 60ms single-shot `QTimer` triggered by vertical scrolling, pane resizing, and proxy model layout/reset events.
3. **ColumnItemDelegate Thumbnail Rendering**: Update `ColumnItemDelegate::paint` to read `HasThumbnailRole`. When a thumbnail exists, render the thumbnail centered inside the 18x18 icon rectangle with a 3px rounded corner clip, consistent with list view styling.

## 2. Pre-Change Verification Findings
- **a) Other Thumbnail Loading Entries in Column View**: Confirmed `loadThumbnailsForRows` was previously only invoked by `ContentViewCoordinator` for `ContentPanel`'s own `diskModel()`. There were no thumbnail loading entries for `ColumnViewPane`'s independent `DiskItemModel` instances.
- **b) Sort / LayoutChanged Re-request Loop**: Confirmed no loop exists. `DiskItemModel::loadThumbnailsForRows` checks `m_requestedPaths` and `m_iconCache`. Any path already requested or cached is skipped, preventing duplicate or cyclic network/disk requests when `applySort` emits `layoutChanged`.
- **c) Thumbnail Disk Cache Sharing**: Confirmed `DiskMediaExtractor::getDiskThumbCachePath` is static and key-based (FRN / file path hash). Thumbnails generated in grid/list views are immediately hit from disk cache by `ColumnViewPane`'s `DiskItemModel`.

## 3. Modified Files List
- `src/ui/controllers/ContentViewCoordinator.h`
- `src/ui/controllers/ContentViewCoordinator.cpp`
- `src/ui/ColumnViewPane.h`
- `src/ui/ColumnViewPane.cpp`
- `src/ui/ColumnItemDelegate.cpp`

## 4. Detailed Line-by-Line Changes

### Change 1: Add `calculateVisibleRowsForView` static helper to `src/ui/controllers/ContentViewCoordinator.h`
```
<<<<<<< SEARCH
    static QModelIndex toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* target);

    QList<QAbstractItemView*> currentActiveViews() const;
=======
    static QModelIndex toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* target);
    static QSet<int> calculateVisibleRowsForView(QAbstractItemView* view, DiskItemModel* targetModel);

    QList<QAbstractItemView*> currentActiveViews() const;
>>>>>>> REPLACE
```

### Change 2: Implement `calculateVisibleRowsForView` and update `refreshVisibleThumbnails` in `src/ui/controllers/ContentViewCoordinator.cpp`
```
<<<<<<< SEARCH
        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(model->rowCount() - 1, btmIdx.row() + 4) : model->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex idx = model->index(r, 0);
            if (idx.data(SectionHeaderRole).toBool()) continue;
            QModelIndex srcIdx = toSourceIndex(idx, m_panel->diskModel());
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_panel->diskModel()->loadThumbnailsForRows(visibleRows.values());
    }
}
=======
        QSet<int> rows = calculateVisibleRowsForView(view, m_panel->diskModel());
        visibleRows.unite(rows);
    }

    if (!visibleRows.isEmpty()) {
        m_panel->diskModel()->loadThumbnailsForRows(visibleRows.values());
    }
}

QSet<int> ContentViewCoordinator::calculateVisibleRowsForView(QAbstractItemView* view, DiskItemModel* targetModel) {
    QSet<int> visibleRows;
    if (!view || !view->viewport() || !targetModel) return visibleRows;
    QAbstractItemModel* model = view->model();
    if (!model || model->rowCount() == 0) return visibleRows;

    QRect vpRect = view->viewport()->rect();
    QModelIndex topIdx = view->indexAt(vpRect.topLeft());
    QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

    int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
    int bottom = btmIdx.isValid() ? qMin(model->rowCount() - 1, btmIdx.row() + 4) : model->rowCount() - 1;

    for (int r = top; r <= bottom; ++r) {
        QModelIndex idx = model->index(r, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        QModelIndex srcIdx = toSourceIndex(idx, targetModel);
        if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
    }
    return visibleRows;
}
>>>>>>> REPLACE
```

### Change 3: Add `QTimer* m_thumbDebounceTimer` and `scheduleRefreshThumbnails` to `src/ui/ColumnViewPane.h`
```
<<<<<<< SEARCH
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSet>
#include <QPointer>
=======
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSet>
#include <QPointer>
#include <QTimer>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FolderSectionHeaderBar* folderHeader() const { return nullptr; }

    void refreshVisibleThumbnails();
=======
    FolderSectionHeaderBar* folderHeader() const { return nullptr; }

    void refreshVisibleThumbnails();
    void scheduleRefreshThumbnails();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QScrollArea* m_paneScrollArea = nullptr;
    DropListView* m_folderListView = nullptr;
    DropListView* m_listView = nullptr;
};
=======
    QScrollArea* m_paneScrollArea = nullptr;
    DropListView* m_folderListView = nullptr;
    DropListView* m_listView = nullptr;
    QTimer* m_thumbDebounceTimer = nullptr;
};
>>>>>>> REPLACE
```

### Change 4: Connect debounced triggers and implement `refreshVisibleThumbnails` in `src/ui/ColumnViewPane.cpp`
```
<<<<<<< SEARCH
#include "ColumnViewPane.h"
#include "ContentPanel.h"
#include "models/SectionProxyModel.h"
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
=======
#include "ColumnViewPane.h"
#include "ContentPanel.h"
#include "controllers/ContentViewCoordinator.h"
#include "models/SectionProxyModel.h"
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, [this]() { tryPendingSelection(); });
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, [this]() { tryPendingSelection(); });
=======
    m_thumbDebounceTimer = new QTimer(this);
    m_thumbDebounceTimer->setSingleShot(true);
    m_thumbDebounceTimer->setInterval(60);
    connect(m_thumbDebounceTimer, &QTimer::timeout, this, &ColumnViewPane::refreshVisibleThumbnails);

    connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged, this, &ColumnViewPane::scheduleRefreshThumbnails);

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, [this]() {
        tryPendingSelection();
        scheduleRefreshThumbnails();
    });
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, [this]() {
        tryPendingSelection();
        scheduleRefreshThumbnails();
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewPane::refreshVisibleThumbnails() {
    if (m_listView && m_listView->viewport()) {
        m_listView->viewport()->update();
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}
=======
void ColumnViewPane::scheduleRefreshThumbnails() {
    if (m_thumbDebounceTimer && !m_thumbDebounceTimer->isActive()) {
        m_thumbDebounceTimer->start();
    }
}

void ColumnViewPane::refreshVisibleThumbnails() {
    if (!m_listView || !m_model) return;
    QSet<int> rows = ContentViewCoordinator::calculateVisibleRowsForView(m_listView, m_model);
    if (!rows.isEmpty()) {
        m_model->loadThumbnailsForRows(rows.values());
    }
    if (m_listView->viewport()) {
        m_listView->viewport()->update();
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    scheduleRefreshThumbnails();
    update();
}
>>>>>>> REPLACE
```

### Change 5: Update thumbnail drawing in `src/ui/ColumnItemDelegate.cpp`
```
<<<<<<< SEARCH
    // 2. 左侧图标 (文件 / 文件夹)
    bool isFolder = (index.data(TypeRole).toString() == "folder");
    bool isEmpty = index.data(IsEmptyRole).toBool();
    QVariant deco = index.data(Qt::DecorationRole);

    int iconSize = 18;
    QRect iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);

    if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
        deco.value<QIcon>().paint(painter, iconRect, Qt::AlignCenter);
    } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
        QPixmap pix = deco.value<QPixmap>();
        painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QIcon fallbackIcon = UiHelper::getIcon(isFolder ? "folder" : "file", QColor("#888888"), 18);
        fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
    }
=======
    // 2. 左侧图标 (文件 / 文件夹)
    bool isFolder = (index.data(TypeRole).toString() == "folder");
    bool isEmpty = index.data(IsEmptyRole).toBool();
    bool hasThumbnail = index.data(HasThumbnailRole).toBool();
    QVariant deco = index.data(Qt::DecorationRole);

    int iconSize = 18;
    QRect iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);

    if (hasThumbnail) {
        QPixmap pix;
        if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
            pix = deco.value<QIcon>().pixmap(iconRect.size());
        } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
            pix = deco.value<QPixmap>();
        }
        if (!pix.isNull()) {
            painter->save();
            QPainterPath clipPath;
            clipPath.addRoundedRect(iconRect, 3, 3);
            painter->setClipPath(clipPath);

            QSize scaledSize = pix.size().scaled(iconRect.size(), Qt::KeepAspectRatio);
            QRect targetRect(
                iconRect.left() + (iconRect.width() - scaledSize.width()) / 2,
                iconRect.top() + (iconRect.height() - scaledSize.height()) / 2,
                scaledSize.width(),
                scaledSize.height()
            );
            painter->drawPixmap(targetRect, pix);
            painter->restore();
        } else {
            QIcon fallbackIcon = UiHelper::getIcon("file", QColor("#888888"), 18);
            fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
        }
    } else {
        if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
            deco.value<QIcon>().paint(painter, iconRect, Qt::AlignCenter);
        } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
            QPixmap pix = deco.value<QPixmap>();
            painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            QIcon fallbackIcon = UiHelper::getIcon(isFolder ? "folder" : "file", QColor("#888888"), 18);
            fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
        }
    }
>>>>>>> REPLACE
```

## 5. Build & Verification Steps
1. **Compilation**:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **Verification Checklist**:
   - Open a directory containing image files in column view mode. Verify thumbnails appear in the 18x18 icon area with 3px rounded corners.
   - Scroll up and down. Confirm thumbnails for newly visible files load smoothly without stutter.
   - Expand multiple columns side-by-side. Confirm each column pane independently loads its own visible thumbnails.
   - Switch between Grid, List, Justified, and Column view modes to confirm no regression in other views.

## 6. SSOT API Reuse & Anti-Redundancy Self-Check
- **Single SSOT Function for Visible Rows**: `ContentViewCoordinator::calculateVisibleRowsForView` is the single SSOT entry point used across all views and column panes.
- **No Duplicate Request Guard**: Relies directly on `DiskItemModel::loadThumbnailsForRows`'s internal `m_requestedPaths` and `m_iconCache` check, avoiding redundant layers.

## 7. Header API Signature Verification
| Class / Function Name | Declaration File (`.h`) | Physical Exact Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `ContentViewCoordinator::calculateVisibleRowsForView` | `src/ui/controllers/ContentViewCoordinator.h` | `static QSet<int> calculateVisibleRowsForView(QAbstractItemView* view, DiskItemModel* targetModel);` | Added as new public static method |
| `ColumnViewPane::scheduleRefreshThumbnails` | `src/ui/ColumnViewPane.h` | `void scheduleRefreshThumbnails();` | Added as new public method |
| `DiskItemModel::loadThumbnailsForRows` | `src/ui/models/DiskItemModel.h` | `void loadThumbnailsForRows(const QList<int>& rows) override;` | Matched 100% |

## 8. Header Inclusion Chain & Type Completeness Check
- `src/ui/controllers/ContentViewCoordinator.h` includes `<QSet>`, `<QList>`, `<QModelIndex>`. Requires forward declaration `class DiskItemModel;`.
- `src/ui/ColumnViewPane.h` includes `<QTimer>`.
- `src/ui/ColumnViewPane.cpp` includes `"controllers/ContentViewCoordinator.h"`.
- `src/ui/ColumnItemDelegate.cpp` includes `"ColumnItemDelegate.h"`, `<QPainterPath>`.
