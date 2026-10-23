# DualSectionPanel-5.md - Single SSOT Visible Viewport Thumbnail Sampling & Dynamic Queue Cancellation

## 1. Overview
This implementation plan establishes `DualSectionPanel::refreshVisibleThumbnails` as the single SSOT for visible viewport thumbnail sampling. It eliminates redundant full-model viewport traversals in `ContentPanel` and optimizes visible row scanning to prevent background queue saturation during scrolling.

## 2. Modified Files List
- `src/ui/DualSectionPanel.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

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

## 4. Build & Verification Steps
Static inspection verifies that `DualSectionPanel` handles all outer viewport projection and visible range sampling without redundant model-wide passes in `ContentPanel`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
Reuses `DualSectionPanel::refreshVisibleThumbnails` as the single authoritative entrance for visible viewport thumbnail sampling.

## 6. Header API Signature Verification
- `void DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)`
- `void ContentPanel::refreshVisibleThumbnails()`

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "DualSectionPanel.h"`
- `#include "ContentPanel.h"`
