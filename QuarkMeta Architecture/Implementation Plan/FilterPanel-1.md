# Implementation Plan: In-place FilterPanel Refresh & ColumnView Sub-column Anti-redundancy Reload

## Overview
This implementation plan addresses the issue where clicking or selecting an already open folder in ColumnView causes the FilterPanel to undergo a complete widget destruction and recreation (large visual flicker/refresh), and causes ColumnView to destroy and rebuild sub-columns unnecessarily.

### Key Fixes:
1. **ColumnView Anti-Redundancy Reload (`ColumnViewWidget.cpp`)**: When clicking a folder in column `paneIdx`, check if column `paneIdx + 1` already exists with the same directory path. If so, preserve column `paneIdx + 1` and only dismiss columns deeper than `paneIdx + 1`, skipping redundant column rebuilding and statistics calculation.
2. **ScanStats Equality Comparison (`ScanStats.h`)**: Add `operator==` to `ScanStats` to enable idempotent snapshot comparison.
3. **FilterPanel In-place Refresh & Idempotency (`FilterPanel.cpp`)**: Modify `FilterPanel::populateStats` to compare incoming `ScanStats` with `m_currentStats` (returning early if identical) and delegate to `populate(...)` instead of unconditionally invoking `rebuildGroups()`. When group structures remain unchanged, `populate(...)` performs in-place label count updates (`cntLabel->setText(...)`), completely eliminating widget destruction/re-creation flickers.

## Modified Files List
- `src/ui/ScanStats.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/FilterPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ScanStats.h`
Add `operator==` and `operator!=` to `QuarkMeta::ScanStats`.

```
<<<<<<< SEARCH
    QSet<QString> duplicatePaths;
};
=======
    QSet<QString> duplicatePaths;

    bool operator==(const ScanStats& o) const {
        return ratingCounts == o.ratingCounts &&
               colorCounts == o.colorCounts &&
               typeCounts == o.typeCounts &&
               createDateCounts == o.createDateCounts &&
               modifyDateCounts == o.modifyDateCounts &&
               emptyFolderCount == o.emptyFolderCount &&
               hasLinkCount == o.hasLinkCount &&
               noLinkCount == o.noLinkCount &&
               hasNoteCount == o.hasNoteCount &&
               noNoteCount == o.noNoteCount &&
               hasTagCount == o.hasTagCount &&
               noTagCount == o.noTagCount &&
               ratioHorizontalCount == o.ratioHorizontalCount &&
               ratioVerticalCount == o.ratioVerticalCount &&
               ratioSquareCount == o.ratioSquareCount &&
               ratio169Count == o.ratio169Count &&
               duplicateCount == o.duplicateCount &&
               uniqueCount == o.uniqueCount &&
               noThumbnailCount == o.noThumbnailCount &&
               hasThumbnailCount == o.hasThumbnailCount &&
               duplicatePaths == o.duplicatePaths;
    }

    bool operator!=(const ScanStats& o) const { return !(*this == o); }
};
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
In `ColumnViewWidget::appendColumn`, check if sub-column `paneIdx + 1` already displays `folderPath` before dismissing and appending.

```
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
        if (m_contentPanel) {
            m_contentPanel->recalculateAndEmitStats();
        }
    });
=======
    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        if (paneIdx + 1 < m_panes.size() &&
            QDir::cleanPath(m_panes[paneIdx + 1]->currentPath()) == QDir::cleanPath(folderPath)) {
            dismissSubColumns(paneIdx + 1);
            m_activePaneIndex = paneIdx + 1;
            emit selectionChanged();
            return;
        }

        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
        if (m_contentPanel) {
            m_contentPanel->recalculateAndEmitStats();
        }
    });
>>>>>>> REPLACE
```

### 3. `src/ui/FilterPanel.cpp`
Delegate `populateStats` to `populate(...)` and skip when stats are identical.

```
<<<<<<< SEARCH
void FilterPanel::populateStats(const QuarkMeta::ScanStats& stats) {
    if (m_statsEngine) {
        m_statsEngine->updateStats(stats);
    }
    m_currentStats = stats;
    m_ratingCounts = stats.ratingCounts;
    m_colorCounts = stats.colorCounts;
    m_typeCounts = stats.typeCounts;
    m_createDateCounts = stats.createDateCounts;
    m_modifyDateCounts = stats.modifyDateCounts;
    m_emptyFolderCount = stats.emptyFolderCount;

    rebuildGroups();
}
=======
void FilterPanel::populateStats(const QuarkMeta::ScanStats& stats) {
    if (m_statsEngine) {
        m_statsEngine->updateStats(stats);
    }
    if (m_currentStats == stats) {
        return;
    }
    m_currentStats = stats;
    populate(stats.ratingCounts, stats.colorCounts, stats.typeCounts,
             stats.createDateCounts, stats.modifyDateCounts, stats.emptyFolderCount);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Perform C++ compilation check:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ```
2. Run test verification (if available) or verify code structure.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reused `populate(...)` in `FilterPanel.cpp` which contains existing in-place label update logic (`cntLabel->setText(...)`) instead of reinventing widget updates or calling `rebuildGroups()`.
- **Anti-Redundancy**: No duplicate code introduced; eliminated redundant `rebuildGroups()` calls and avoided unnecessary sub-column destruction in `ColumnViewWidget.cpp`.
