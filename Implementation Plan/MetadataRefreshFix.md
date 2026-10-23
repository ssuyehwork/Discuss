# Implementation Plan - Real-time MetadataPanel & FilterPanel Refresh Fix

## 1. Overview
When a user tags an item with a color from the right-click context menu (`ContentContextMenu`), `CoreEngine::handleSetColor` publishes a `CentralEventHub` event (`AppEventType::MetadataUpdated`) with `targetPath` set to the native path string (e.g. `C:/foo/bar.txt`).
However:
1. `PanelMediator.cpp`'s `CentralEventHub` event listener calls `contentPanel->updateItemMetadata(event.targetPath)`.
2. Inside `PanelMediator.cpp`, the `dataChanged` slot on `contentPanel->model()` compares `selPath` and `changedPath` using direct `QString::compare(selPath, changedPath)`. If path separators (slash vs backslash) or case differ slightly, the `MetaPanel` update for `selPath` was skipped.
3. In `FilterPanel.cpp`, `populate()` checked color count updates using hardcoded Chinese string checks (`name == "红色"`) while `ScanStatsWorker` calculates counts using normalized uppercase Hex strings (e.g. `#E24B4A`), causing `FilterPanel` count labels not to refresh dynamically.

This implementation plan fixes:
1. Path string normalization in `PanelMediator.cpp` for `MetaPanel` synchronization.
2. Direct dynamic Hex lookup in `FilterPanel.cpp` for `m_colorCounts`.
3. Dispatching `MetaPanel` refresh directly when `CentralEventHub` receives `MetadataUpdated`.

---

## 2. Modified Files List
- `src/ui/PanelMediator.cpp`
- `src/ui/FilterPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/PanelMediator.cpp`
Normalize paths when updating `MetaPanel` in `dataChanged` and `CentralEventHub` events:

<<<<<<< SEARCH
            QModelIndex currentSel = selected.first();
            QString selPath = currentSel.data(PathRole).toString();
            QString changedPath = topLeft.data(PathRole).toString();

            if (!selPath.isEmpty() && QString::compare(selPath, changedPath, Qt::CaseInsensitive) == 0) {
=======
            QModelIndex currentSel = selected.first();
            QString selPath = QDir::cleanPath(currentSel.data(PathRole).toString());
            QString changedPath = QDir::cleanPath(topLeft.data(PathRole).toString());

            if (!selPath.isEmpty() && QString::compare(selPath, changedPath, Qt::CaseInsensitive) == 0) {
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (event.type == QuarkMeta::AppEventType::MetadataUpdated) {
            if (!event.targetPath.isEmpty()) {
                contentPanel->updateItemMetadata(event.targetPath);
            } else if (!event.paths.isEmpty()) {
                for (const QString& p : event.paths) {
                    contentPanel->updateItemMetadata(p);
                }
            } else {
                contentPanel->refreshAll();
            }
            contentPanel->recalculateAndEmitStats();
        }
=======
        if (event.type == QuarkMeta::AppEventType::MetadataUpdated) {
            if (!event.targetPath.isEmpty()) {
                contentPanel->updateItemMetadata(event.targetPath);
                if (metaPanel) {
                    QString targetClean = QDir::cleanPath(event.targetPath);
                    for (const QString& p : metaPanel->selectedPaths()) {
                        if (QString::compare(QDir::cleanPath(p), targetClean, Qt::CaseInsensitive) == 0) {
                            QString newColor = event.payload.value("value").toString();
                            metaPanel->setColor(newColor, false);
                            break;
                        }
                    }
                }
            } else if (!event.paths.isEmpty()) {
                for (const QString& p : event.paths) {
                    contentPanel->updateItemMetadata(p);
                }
            } else {
                contentPanel->refreshAll();
            }
            contentPanel->recalculateAndEmitStats();
        }
>>>>>>> REPLACE

---

### 3.2 `src/ui/FilterPanel.cpp`
Use normalized Hex lookup in `FilterPanel::populate`:

<<<<<<< SEARCH
                 else if (name == "红色") count = m_colorCounts.value("#E24B4A", m_colorCounts.value("红色", 0));
                 else if (name == "橙色") count = m_colorCounts.value("#EF9F27", m_colorCounts.value("橙色", 0));
                 else if (name == "黄色") count = m_colorCounts.value("#FECF0E", m_colorCounts.value("黄色", 0));
                 else if (name == "绿色") count = m_colorCounts.value("#639922", m_colorCounts.value("绿色", 0));
                 else if (name == "青色") count = m_colorCounts.value("#1D9E75", m_colorCounts.value("青色", 0));
                 else if (name == "蓝色") count = m_colorCounts.value("#378ADD", m_colorCounts.value("蓝色", 0));
                 else if (name == "紫色") count = m_colorCounts.value("#7F77DD", m_colorCounts.value("紫色", 0));
                 else if (name == "灰色") count = m_colorCounts.value("#5F5E5A", m_colorCounts.value("灰色", 0));
                 else if (name == "无色标") count = m_colorCounts.value("", m_colorCounts.value("无色标", 0));
=======
                 else if (name == "无色标") count = m_colorCounts.value("", m_colorCounts.value("无色标", 0));
                 else {
                     QString hex = Style::getColorHexByName(name);
                     if (!hex.isEmpty()) {
                         count = m_colorCounts.value(hex, m_colorCounts.value(name, 0));
                     }
                 }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Select an item in any view mode and set color via right-click context menu.
4. Confirm both `MetaPanel` (right panel) and `FilterPanel` (left panel) update instantly.
