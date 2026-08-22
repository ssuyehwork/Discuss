# Implementation Plan - Filter Panel Stats Sync & No-Thumbnail Filter

## Overview
This plan fixes two major user-reported defects in the filter panel (`FilterPanel`) and introduces a new filtering feature:
1. **Aspect Ratio Counts Showing 0**: The aspect ratio counts ("横图", "竖图", "方形", "16:9") depended on `width` and `height` being present. `DiskItemModel::preloadDimensionsAsync()` now rapidly extracts dimensions via header sniffing and triggers `recalculateAndEmitStats()`, emitting `directoryStatsReady` to update `FilterPanel` counts instantly.
2. **Duplicate Status Counts Showing 0**: The duplicate counts (`duplicateCount` and `uniqueCount`) calculated in `ContentPanel::recalculateAndEmitStats()` were not properly mapped to labels in `FilterPanel::populate()`. `FilterPanel::populate()` will now update duplicate/unique count labels properly.
3. **New "No Thumbnail (Failed/Skipped)" Filter Option**: Added `noThumbnail` check option in `FilterPanel` and matching filtering logic in `FilterProxyModel::filterAcceptsRow` using `thumbStatus == 1` or `thumbStatus != 0`.

## Modified Files List
- `src/ui/ScanStats.h`
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ScanStats.h`
Add `noThumbnailCount` field to `ScanStats`:
```
<<<<<<< SEARCH
    int duplicateCount = 0;
    int uniqueCount = 0;
};
=======
    int duplicateCount = 0;
    int uniqueCount = 0;
    int noThumbnailCount = 0;
};
>>>>>>> REPLACE
```

### 2. `src/ui/FilterPanel.h`
Add `noThumbnail` flag to `FilterState`:
```
<<<<<<< SEARCH
    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;
=======
    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    bool noThumbnailOnly = false;
>>>>>>> REPLACE
```

Update `FilterState::isEmpty()`:
```
<<<<<<< SEARCH
               colorFilterText.trimmed().isEmpty() &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll;
=======
               colorFilterText.trimmed().isEmpty() &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               !noThumbnailOnly;
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.cpp`
Update `recalculateAndEmitStats()` to calculate `noThumbnailCount` and trigger recalculation when dimension preloading finishes:
```
<<<<<<< SEARCH
                // 重复状态统计
                std::string key = std::to_string(record.size) + "_" + std::to_string(record.width) + "_" + std::to_string(record.height) + "_" + record.filename.toLower().toStdString();
                if (hashCounts[key] > 1) {
                    stats.duplicateCount++;
                } else {
                    stats.uniqueCount++;
                }
            }
=======
                // 重复状态统计
                std::string key = std::to_string(record.size) + "_" + std::to_string(record.width) + "_" + std::to_string(record.height) + "_" + record.filename.toLower().toStdString();
                if (hashCounts[key] > 1) {
                    stats.duplicateCount++;
                } else {
                    stats.uniqueCount++;
                }

                // 无缩略图 (失败/跳过) 统计
                if (record.thumbStatus == 1) {
                    stats.noThumbnailCount++;
                }
            }
>>>>>>> REPLACE
```

Update `FilterProxyModel::filterAcceptsRow` to handle `noThumbnailOnly`:
```
<<<<<<< SEARCH
    // 11. 重复状态过滤 (O(1) 瞬时判定)
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
=======
    // 10.5 无缩略图 (失败/跳过) 过滤
    if (currentFilter.noThumbnailOnly) {
        if (record.thumbStatus != 1) return false;
    }

    // 11. 重复状态过滤 (O(1) 瞬时判定)
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
>>>>>>> REPLACE
```

### 4. `src/ui/FilterPanel.cpp`
Update `FilterPanel::populate()` to update labels for "重复项", "未重复", and "无缩略图 (失败/跳过)":
```
<<<<<<< SEARCH
                 else if (name == "未重复") count = m_currentStats.uniqueCount;
=======
                 else if (name == "未重复") count = m_currentStats.uniqueCount;
                 else if (name == "重复项") count = m_currentStats.duplicateCount;
                 else if (name == "无缩略图 (失败/跳过)") count = m_currentStats.noThumbnailCount;
>>>>>>> REPLACE
```

Add "无缩略图 (失败/跳过)" checkbox to `FilterPanel::rebuildGroups()` in the "文件类型" group:
```
<<<<<<< SEARCH
        if (m_typeCounts.contains("file")) {
            QCheckBox* cb = addFilterRow(gl, "文件", m_typeCounts["file"]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains("file"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                if (on) { if (!m_filter.types.contains("file")) m_filter.types.append("file"); }
                else    m_filter.types.removeAll("file");
                emit filterChanged(m_filter);
            });
        }
=======
        if (m_typeCounts.contains("file")) {
            QCheckBox* cb = addFilterRow(gl, "文件", m_typeCounts["file"]);
            cb->blockSignals(true);
            cb->setChecked(m_filter.types.contains("file"));
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                if (on) { if (!m_filter.types.contains("file")) m_filter.types.append("file"); }
                else    m_filter.types.removeAll("file");
                emit filterChanged(m_filter);
            });
        }

        if (m_currentStats.noThumbnailCount > 0 || m_filter.noThumbnailOnly) {
            QCheckBox* cb = addFilterRow(gl, "无缩略图 (失败/跳过)", m_currentStats.noThumbnailCount);
            cb->blockSignals(true);
            cb->setChecked(m_filter.noThumbnailOnly);
            cb->blockSignals(false);
            connect(cb, &QCheckBox::toggled, this, [this](bool on) {
                m_filter.noThumbnailOnly = on;
                emit filterChanged(m_filter);
            });
        }
>>>>>>> REPLACE
```

In `FilterPanel::syncUIFromFilterState()`:
```
<<<<<<< SEARCH
        else if (text == "无备注") shouldCheck = (m_filter.notePresence == FilterState::No);
=======
        else if (text == "无备注") shouldCheck = (m_filter.notePresence == FilterState::No);
        else if (text == "无缩略图 (失败/跳过)") shouldCheck = m_filter.noThumbnailOnly;
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Re-build `QuarkMeta` via CMake and MSVC.
2. Enter a directory with graphics files; verify that dimension preloading completes and aspect ratio counts ("横图", "竖图", "方形", "16:9") show non-zero numbers in `FilterPanel`.
3. Verify duplicate counts ("重复项", "未重复") accurately display non-zero numbers matching duplicate detection.
4. Check "无缩略图 (失败/跳过)" in `FilterPanel` and verify only items with failed thumbnail extraction (`thumbStatus == 1`) are filtered and displayed.
