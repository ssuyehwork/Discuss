# Implementation Plan - PanelMediator-5.md

## Overview
This implementation plan provides the architectural decoupling of `FilterPanel` in multi-pane (split view) scenarios.
Currently, `FilterPanel` (filtering controls & directory scanning statistics) is statically connected only to the root/primary `ContentPanel`.
When a user switches focus to a secondary sub-pane, this decoupled mediator binding ensures that `FilterPanel` dynamically syncs filter criteria and scanning statistics (`ScanStats`) with whichever `ContentPanel` currently holds focus (`m_activeContentPanel`).

## Modified Files List
- `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### `src/ui/PanelMediator.cpp`
```
<<<<<<< SEARCH
    // 4. 统计与过滤联动
    if (contentPanel && filterPanel) {
        connect(contentPanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
            filterPanel->populateStats(stats);
            AppEvent ev;
            ev.type = AppEventType::FilterStateChanged;
            CentralEventHub::instance().publishEvent(ev);
        });

        connect(filterPanel, &FilterPanel::filterChanged, contentPanel, [contentPanel](const FilterState& state) {
            contentPanel->applyFilters(state);
        });
    }
=======
    // 4. 统计与过滤联动 (动态支持多分栏焦点切换)
    if (filterPanel) {
        auto bindFilterToActivePanel = [this, filterPanel](ContentPanel* activePanel) {
            if (!activePanel) return;

            // 收到分栏统计准备就绪信号时，刷出统计
            connect(activePanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
                filterPanel->populateStats(stats);
                AppEvent ev;
                ev.type = AppEventType::FilterStateChanged;
                CentralEventHub::instance().publishEvent(ev);
            }, Qt::UniqueConnection);
        };

        // 绑定初始主面板
        if (contentPanel) {
            bindFilterToActivePanel(contentPanel);
        }

        // 焦点分栏切换时动态绑定并应用当前筛选条件
        connect(this, &PanelMediator::activeContentPanelChanged, this, [this, filterPanel, bindFilterToActivePanel](ContentPanel* newActivePanel) {
            if (newActivePanel) {
                bindFilterToActivePanel(newActivePanel);
            }
        });

        // 筛选条件变动时应用至当前激活分栏
        connect(filterPanel, &FilterPanel::filterChanged, this, [this](const FilterState& state) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : m_contentPanel.data();
            if (target) {
                target->applyFilters(state);
            }
        });
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. **Compilation**:
   ```bash
   cd build
   cmake ..
   make -j8
   ```
2. **Verification Method**:
   - Open dual-pane / multi-pane split view (`ContentPanel`).
   - Focus on the secondary pane on the right side.
   - Adjust filter controls in `FilterPanel` (e.g. filter by 5-star rating or green color tag).
   - Confirm that the filter conditions take immediate effect on the active secondary pane without disturbing non-focused panes.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused SSOT Entry Points**:
  - Reused `PanelMediator::activeContentPanelChanged` signal for dynamic panel tracking.
  - Reused `ContentPanel::applyFilters(FilterState)` and `ContentPanel::directoryStatsReady(ScanStats)`.
- **Anti-Redundancy**: No duplicate filtering logic created; re-used existing signal-slot mediator pipeline.

## Header API Signature Verification
- `PanelMediator::activeContentPanelChanged(ContentPanel* panel)` -> Exact signature in `src/ui/PanelMediator.h`
- `ContentPanel::directoryStatsReady(const ScanStats& stats)` -> Exact signal in `src/ui/ContentPanel.h`
- `ContentPanel::applyFilters(const FilterState& state)` -> Exact slot/method in `src/ui/ContentPanel.h`
- `FilterPanel::populateStats(const ScanStats& stats)` -> Exact method in `src/ui/FilterPanel.h`
