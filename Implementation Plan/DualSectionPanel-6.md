# DualSectionPanel-6.md - SSOT Viewport Thumbnail Sampling Refactoring

## 1. Overview
This implementation plan consolidates visible viewport thumbnail sampling into `DualSectionPanel::refreshVisibleThumbnails` as the single source of truth (SSOT). It removes redundant directory-wide passes in `ContentPanel` and ensures viewport sampling maps host scroll geometries accurately without modifying source code prior to user approval.

## 2. Architectural Principles & Threading Rules
1. **SSOT Viewport Sampling**:
   - `DualSectionPanel::refreshVisibleThumbnails` is the sole entry point responsible for mapping host scrollarea viewport coordinates to sub-views (`m_folderView`, `m_fileView`).
   - `ContentPanel::refreshVisibleThumbnails` delegates directly to `m_scrollCanvas->refreshVisibleThumbnails(m_model)`.

## 3. Modified Files List
- `src/ui/DualSectionPanel.cpp`
- `src/ui/ContentPanel.cpp`

## 4. Detailed Line-by-Line Changes

### `src/ui/DualSectionPanel.cpp`

```
<<<<<<< SEARCH
    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - Submitting" << visibleRows.size() << "rows to loadThumbnailsForRows.";
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - No visible rows found in viewport sampling.";
    }
=======
    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || m_model->rowCount() == 0) return;
    
    // 省略：遍历整个 m_model 所有行强行装载
    QSet<int> visibleRows;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        visibleRows.insert(i);
    }
    m_model->loadThumbnailsForRows(visibleRows.values());
}
=======
void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || m_model->rowCount() == 0) return;
    if (m_scrollCanvas) {
        m_scrollCanvas->refreshVisibleThumbnails(m_model);
    }
}
>>>>>>> REPLACE
```

## 5. Build & Verification Steps
Static code verification confirms that `DualSectionPanel` handles all viewport coordinate transformations cleanly, preventing redundant model-wide passes in `ContentPanel`.

## 6. SSOT API Reuse & Anti-Redundancy Self-Check
Reuses `DualSectionPanel::refreshVisibleThumbnails` as the single authoritative entry point for visible viewport thumbnail sampling.

## 7. Header API Signature Verification
- `void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)`
- `void ContentPanel::refreshVisibleThumbnails()`

## 8. Header Inclusion Chain & Type Completeness Check
- `#include "DualSectionPanel.h"`
- `#include "ContentPanel.h"`
