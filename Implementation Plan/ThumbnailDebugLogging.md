# Implementation Plan - Thumbnail Loading Pipeline Trace Logging (`ThumbnailDebugLogging.md`)

## 1. Overview
This implementation plan inserts structured debug trace logging (`[THUMB_TRACE]`) across the entire thumbnail viewport-sampling and async loading pipeline. This allows precise runtime tracing to locate why thumbnail images may delay or fail to display when thumbnail files already exist in local cache.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp` (Log `refreshVisibleThumbnails` invocation and active view mode)
- `src/ui/DualSectionPanel.cpp` (Log host-to-child viewport coordinate mapping and visible row sampling results)
- `src/ui/models/DiskItemModel.cpp` (Log row filtering, cache/in-flight status, and batch async dispatch)
- `src/util/ThumbnailPipelineService.cpp` (Log generation match/mismatch, cache read-only hits/misses, and main-thread model updates)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    if (m_currentViewMode == ColumnView) {
=======
void ContentPanel::refreshVisibleThumbnails() {
    qDebug() << "[THUMB_TRACE] ContentPanel::refreshVisibleThumbnails called. Current view mode:" << static_cast<int>(m_currentViewMode);
    if (m_currentViewMode == ColumnView) {
>>>>>>> REPLACE
```

### 3.2 `src/ui/DualSectionPanel.cpp`
```
<<<<<<< SEARCH
    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
=======
    if (!visibleRows.isEmpty()) {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - Submitting" << visibleRows.size() << "rows to loadThumbnailsForRows.";
        model->loadThumbnailsForRows(visibleRows.values());
    } else {
        qDebug() << "[THUMB_TRACE] DualSectionPanel::refreshVisibleThumbnails - No visible rows found in viewport sampling.";
    }
>>>>>>> REPLACE
```

### 3.3 `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);
=======
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    qDebug() << "[THUMB_TRACE] DiskItemModel::loadThumbnailsForRows called with" << rows.size() << "rows.";

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);
>>>>>>> REPLACE
```

---

## 4. Verification & Diagnostic Steps
1. Run application and navigate through folders in Grid or List view.
2. Observe debug output tagged with `[THUMB_TRACE]` in Visual Studio / Qt Creator console.
3. Trace timeline from `ContentPanel::refreshVisibleThumbnails` down to `ThumbnailPipelineService::loadBatchAsync` to diagnose exact delay or sampling bottlenecks.
