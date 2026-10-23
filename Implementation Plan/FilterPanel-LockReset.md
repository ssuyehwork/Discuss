# Implementation Plan - FilterPanel-LockReset.md

## 1. Overview
In QuarkMeta, when navigating directories, the Filter Panel (`FilterPanel`) resets filter options back to their default state by calling `clearAllFilters(false)`. If the filter lock button (Button ③) is **unlocked** (`m_isFilterPinned == false`), filters are automatically cleared on directory change. If **locked** (`m_isFilterPinned == true`), active filter settings persist across directory changes.

Currently, global navigation via `NavigationService::currentUrlChanged` calls `filterPanel->clearAllFilters()` in `PanelMediator.cpp`. However, expanding or navigating subfolders within Column View mode (`ColumnViewWidget`) emits `pathNavigated` or expands sub-columns without notifying `FilterPanel` to reset unpinned filters. Consequently, if a user set a filter option (e.g. unchecking "文件夹") while in Column View, expanding a subfolder would retain the hidden-folder filter even when Button ③ was **unlocked**, causing newly expanded subfolders to show empty content ("0个项目，7个已隐藏").

This plan wires Column View path navigation (`pathNavigated`) to `FilterPanel::clearAllFilters(false)` via `PanelMediator`, ensuring that when Button ③ is unlocked, expanding subfolders in Column View resets the filter panel as intended.

## 2. Modified Files List
- `src/ui/PanelMediator.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/PanelMediator.cpp`
1. Connect `m_contentPanel`'s Column View path navigation signal to reset unpinned filters in `filterPanel`:

```
<<<<<<< SEARCH
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        if (favoritePanel) {
=======
    if (contentPanel) {
        connect(contentPanel, &ContentPanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        if (filterPanel && contentPanel->columnView()) {
            connect(contentPanel->columnView(), &ColumnViewWidget::pathNavigated, filterPanel, [filterPanel](const QString&) {
                filterPanel->clearAllFilters(false);
            });
        }

        if (favoritePanel) {
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run `cmake --build build` to verify clean compilation.
2. **Behavioral Verification**:
   - Unlock Button ③ (Filter Lock Button) so it shows the tilted unpinned icon `pin_tilted`.
   - In Column View mode, set a filter condition (e.g. uncheck "文件夹").
   - Click a subfolder in Column View to expand it.
   - Verify that `filterPanel->clearAllFilters(false)` triggers and resets all filter checkboxes to default because Button ③ is unlocked.
   - Now lock Button ③ (so it shows the straight pinned icon `pin`).
   - Set a filter condition and expand a subfolder in Column View.
   - Verify that `clearAllFilters(false)` returns early (`if (!force && m_isFilterPinned) return;`), allowing filter conditions to persist across subfolders when locked.
