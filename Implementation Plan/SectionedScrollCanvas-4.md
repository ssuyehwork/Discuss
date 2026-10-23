# SectionedScrollCanvas-4.md Implementation Plan

## 1. Overview
This implementation plan resolves the severe performance degradation in thumbnail loading, rendering delays, and layout instability caused by forcing `QAbstractItemView` instances to expand to their total physical height (`setFixedHeight(totalHeight)`) and disabling their native scrollbars (`setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff)`).

### Root Causes Solved:
1. **Virtual Scrolling Destruction**: Forcing view widgets to expand to their full content height expands their `viewport()->rect()` to encompass all items in the folder (e.g. 3,000 items). This tricks `refreshVisibleThumbnails()` into treating all items as visible on screen, triggering thousands of concurrent thumbnail extraction requests and crashing CPU/Disk IO pipelines.
2. **Layout Storm & Re-entrant Layout Relayouts**: As each thumbnail finishes decoding asynchronously, `dataChanged` fires, triggering `JustifiedView::scheduleLayout()`. Re-calculating layout changes `m_totalHeight`, which emits `totalHeightChanged(int)`, calling `setFixedHeight()` and driving endless Qt layout re-evaluations and full-view redraws.
3. **Loss of Viewport Clipping**: Disabling native vertical scrollbars on item views and forcing full widget expansion bypasses Qt's internal hardware/software viewport painting clip region, causing excessive draw calls.

### Architecture 3-Question Answers (架构三问):
1. **SSOT Source**: The SSOT for item records and thumbnail pipeline state is held by `DiskItemModel` and `ThumbnailPipelineService`. Viewports should only reflect the physical viewing region without duplicating full-height geometry state.
2. **Black-Box Integrity**: Public `.h` contracts of `SectionedScrollCanvas`, `DualSectionPanel`, and `JustifiedView` remain unmodified and frozen. Viewport calculations rely strictly on Qt's public `QAbstractItemView::viewport()` API.
3. **Root Cause vs Symptom**: The root cause is layout force-expansion bypassing virtual scrolling. The fix restores native viewport scrolling and item-view viewport clipping, isolating layout re-calculations from thumbnail updates.

---

## 2. Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`
- `src/ui/DualSectionPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Modification 1: `src/ui/SectionedScrollCanvas.cpp`
Restores native vertical scrollbar policy on folder and file views, decouples `totalHeightChanged` from forcing full widget height, and lets item views manage internal viewport scrolling.

```
<<<<<<< SEARCH
        folderJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
=======
        folderJv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        folderTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
=======
        folderTv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        fileJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
=======
        fileJv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        fileTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
=======
        fileTv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                    m_panel->folderView()->setFixedHeight(height);
                }
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(height);
                }
            });
        }
    }
=======
    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int) {
                updateSectionCounts();
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int) {
                updateSectionCounts();
            });
        }
    }
>>>>>>> REPLACE
```

### Modification 2: `src/ui/DualSectionPanel.cpp`
Ensures `fileView` and `folderView` layouts stretch naturally within the layout using proper layout stretch factors rather than forcing fixed heights.

```
<<<<<<< SEARCH
    if (m_folderView) {
        m_folderView->setParent(this);
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
    }

    m_fileHeader = new FileSectionHeaderBar(this);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader, 0);

    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 0);
    }
=======
    if (m_folderView) {
        m_folderView->setParent(this);
        m_folderView->hide();
        m_layout->addWidget(m_folderView, 0);
    }

    m_fileHeader = new FileSectionHeaderBar(this);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader, 0);

    if (m_fileView) {
        m_fileView->setParent(this);
        m_layout->addWidget(m_fileView, 1);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command:
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Verification Methods:
1. Open a folder containing over 1,000 image/media files in Grid / Justified view mode.
2. Confirm that only the 20-30 thumbnails currently visible in the active viewport are requested from `ThumbnailPipelineService`.
3. Scroll rapidly down the view and verify smooth, non-blocking 0ms thumbnail loading without UI thread stutter or layout re-entrancy loops.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` / `refreshVisibleThumbnails()`**: Reuses the core SSOT thumbnail loading pipeline via `ThumbnailPipelineService::instance().loadBatchAsync`.
- **No Redundant Layout Hacks**: Completely eliminates `setFixedHeight` force-expansions.

---

## 6. Header API Signature Verification
- `DualSectionPanel::folderView() const`: Exact signature in `src/ui/DualSectionPanel.h`.
- `DualSectionPanel::fileView() const`: Exact signature in `src/ui/DualSectionPanel.h`.
- `SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model)`: Exact signature in `src/ui/SectionedScrollCanvas.h`.
- `JustifiedView::totalHeightChanged(int height)`: Exact signature in `src/ui/JustifiedView.h`.
